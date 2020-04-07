#include "symboltable.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

// #define DEBUG

/*
全局符号表使用散列表来存储，用桶来处理hash值冲突的问题

局部符号表使用链表来存储，利用函数栈的方式实现

遇到一个新的Compst的左括号时，创建一个新的局部符号表

该Compst中定义的符号会加入全局符号表，声明的符号不会加入全局符号表

使用某个符号时
（1）如果在局部符号表中查询到，有该符号的定义/声明，绑定
（2）否则，去到全局符号表中查询，找到该符号在桶中的最新的定义/声明，绑定
（3）如果找不到，那么该符号未被定义/声明

（尝试）新增某个符号时
（1）如果是该符号的声明，查询局部符号表是否有冲突的定义/声明
    （1.1）没有定义/声明：将声明存入局部符号表
    （1.2）有定义/声明但不冲突：pass
    （1.3）有定义/声明且冲突：根据该符号的类型（变量/函数）选择不同的报错方式
（2）如果是该符号的定义，查询局部符号表中是否有冲突的定义/声明
    （2.1）有定义：重定义错误
    （2.2）有声明且冲突：定义声明冲突错误
    （2.3）有声明但不冲突：将局部符号表中的声明改为定义，然后压入全局符号表中的符号桶
    （2.4）没有定义/声明：将定义存入局部符号表，然后压入全局符号表中的符号桶

遇到该Compst的右括号时，删除该局部符号表，并将定义的符号从全局符号表中删除，如果还有声明的符号，说明有未定义但声明了的符号
*/

#define SYMTAB_SIZE 0x3FFF
#define MAX_NAME_LEN 32

#define WRONG 0
#define DEFINITION 1
#define DECLARATION 2

struct Type {
  enum { _INT, _FLOAT, _ARRAY, _STRUCTURE } kind;
  union {
    int int_val;      // 暂时没用
    float float_val;  // 暂时没用
    struct {
      Type* type;
      int size;
    } array;
    FieldList* structure;
  };
};

struct Function {
  Type* ret_type;
  FieldList* parameter;
};
// 新增

struct FieldList {
  char name[MAX_NAME_LEN];
  Type* type;
  FieldList* next;
};

struct SymbolStkNode {
  enum { _VARABLE, _FUNCTION } kind;  // 该符号是变量还是函数
  union {
    Type* type;          // 当前符号（变量）的类型
    Function* function;  // 当前符号（函数）的返回值和参数列表
  };
  int how;              // 声明或是定义
  SymbolStkNode* next;  // 与该符号同名的下一种类型
};

struct SymbolBucket {
  char name[MAX_NAME_LEN];  // 当前符号的名字
  SymbolStkNode* head;      //指向该符号的类型链表的首元素
  SymbolBucket* next;       // 符号表中的下一个桶
};

struct SymtabStk {
  SymbolBucket* head;  // 指向当前符号表内的第一个桶
  SymtabStk* next;  // 栈上的下一个符号表（即外层代码块的符号表）
};

SymbolBucket* symtab_g[SYMTAB_SIZE];  // 全局符号表，使用散列表存储
SymtabStk* symtab_l;  // 局部符号表，（的链表中的）当前符号表指针
int symtab_l_cnt;  // 表的嵌套层数，debug用

/* 栈 */

void stack_push(SymbolBucket* addr, char* name, Type* type, int how) {
  if (strcmp(name, addr->name) != 0) {
    printf("上层逻辑错误，表中符号为%s，期望插入符号为%s\n", addr->name, name);
    return;
  }
  SymbolStkNode* old_head = addr->head;
  SymbolStkNode* new_head = malloc(sizeof(SymbolStkNode));
  // type赋值
  new_head->type = type;
  // how赋值
  new_head->how = how;
  new_head->next = old_head;
  addr->head = new_head;
}

void stack_pop(SymbolBucket* addr, char* name) {
  if (strcmp(name, addr->name) != 0) {
    printf("上层逻辑错误，表中符号为%s，期望插入符号为%s\n", addr->name, name);
    return;
  }
  if (addr->head == NULL) {
    printf("对空栈(%s)进行弹出\n", addr->name);
    return;
  }
  SymbolStkNode* old_head = addr->head;
  addr->head = addr->head->next;
  free(old_head);
}

/* 全局符号表 */

void global_init() { memset(symtab_g, 0x00, sizeof(symtab_g)); }

unsigned hash(char* name) {
  unsigned val = 0, i;
  for (; *name; ++name) {
    val = (val << 2) + *name;
    if (i = val & ~SYMTAB_SIZE) val = (val ^ (i >> 12)) & SYMTAB_SIZE;
  }
  return val;
}

void global_insert(char* name, Type* type, int how) {
  /*
    向全局符号表中插入一个符号
    名字为name，值为type
   */

  int val = hash(name);                // 计算name对应的hash值
  SymbolBucket* temp = symtab_g[val];  // temp指向name对应的桶中的首元素
  while (temp != NULL)                 // 通过while循环strcmp找到hash桶
    if (strcmp(name, temp->name) == 0) break;
  if (temp == NULL) {  //没找到name这个符号，name是新符号
    // 创建一个新的桶
    temp = malloc(sizeof(SymbolBucket));
    strcpy(temp->name, name);
    temp->next = NULL;
    symtab_g[val] = temp;
  }
  // 全局表只放定义，不放声明
  if (how == DEFINITION) stack_push(temp, name, type, how);
}

void global_query(char* name, Type* ans_type, int* ans_how) {
/*
  查询全局符号表中的名字为name的符号的值type和定义方式how
  若存在多个值对应，返回第一个值
 */
#ifdef DEBUG
  printf("global_query(%s)\n", name);
#endif
  int val = hash(name);
  SymbolBucket* temp = symtab_g[val];
  //哈希表中横向查询
  while (temp != NULL)
    if (strcmp(name, temp->name) == 0) break;
  if (temp == NULL) {  //没有找到name这个符号，name未定义
    printf("未定义的符号");
    ans_type = NULL;
    ans_how = WRONG;
  } else {  //找到了name这个符号，对应栈顶的对象
    ans_type = temp->head->type;
    *ans_how = temp->head->how;
  }
}

void global_remove(char* name) {
  /*
    移除全局符号表中的名字为name的符号的第一个值type
    若不存在这样的type，报错
  */
  int val = hash(name);
  SymbolBucket* temp = symtab_g[val];
  while (temp != NULL)
    if (strcmp(name, temp->name) == 0) break;
  if (temp == NULL)  //没有找到name这个符号，name未定义
    printf("未定义的符号: %s", name);
  else {  //找到了name这个符号，弹出栈顶的对象
    stack_pop(temp, name);
    if (temp->head == NULL)  //如果弹出了栈中的所有对象
      symtab_g[val] = NULL;  //抹去散列表中的对应项
  }
}

int global_debug() {
  return 0;
  for (int eachval = 0; eachval < SYMTAB_SIZE; eachval++) {
    if (symtab_g[eachval]) {
      SymbolBucket* temp = symtab_g[eachval];
      while (temp) {
        int cnt = 0;
        SymbolStkNode* temp2 = temp->head;
        while (temp2) {
          cnt++;
          temp2 = temp2->next;
        }
        printf("name: %s, cnt: %d\n", temp->name, cnt);
        temp = temp->next;
      }
    }
  }
  printf("\n");
  return 0;
}

/* 局部符号表 */

int local_insert(char* name, Type* type, int how) {
  SymbolBucket* temp = symtab_l->head;
  while (temp != NULL) {
    if (strcmp(temp->name, name) == 0) {
      // 符号已存在
      printf("已存在\n");
      if (how == DEFINITION && temp->head->how == DEFINITION) {
        // 且两个都是定义，则冲突
        printf("重定义的符号: %s\n", name);
        return 1;
      } else if (how == DEFINITION) {
        // 原有的是声明，新的是定义，则覆盖
        temp->head->how = how;
        return 0;
      } else {
        // 先定义后声明，或者两个声明，则空过
        return 0;
      }
    }
    temp = temp->next;
  }
  // 该符号不存在，则创建
  temp = malloc(sizeof(SymbolBucket));
  strcpy(temp->name, name);
  temp->head = malloc(sizeof(SymbolStkNode));
  temp->head->type = type;
  temp->head->how = how;
  // 将该符号加入局部符号表
  temp->next = symtab_l->head;
  symtab_l->head = temp;

  return 0;
}

void local_query(char* name, Type* ans_type, int* ans_how) {
#ifdef DEBUG
  printf("local_query(%s)\n", name);
#endif
  SymbolBucket* temp = symtab_l->head;
  while (temp != NULL) {
    if (strcmp(temp->name, name) == 0) {
      ans_type = temp->head->type;
      *ans_how = temp->head->how;
      return;
    }
    temp = temp->next;
  }
  ans_type = NULL;
  *ans_how = WRONG;
}

/* 符号表 */

void initSymtab() { global_init(); }

void createSymtab() {
  if (symtab_l == NULL) {
    symtab_l = malloc(sizeof(SymtabStk));
    symtab_l->next = NULL;
  } else {
    SymtabStk* temp = malloc(sizeof(SymtabStk));
    temp->next = symtab_l;
    symtab_l = temp;
  }
  symtab_l_cnt += 1;
#ifdef DEBUG
  printf("createSymtab: %d\n", symtab_l_cnt);
#endif
  global_debug();
}

void insertSymtab(char* name, Type* type, int how) {
#ifdef DEBUG
  printf("insert %s\n", name);
#endif
  if (local_insert(name, type, how)) {
    printf("一些错误发生了\n");
  } else {
    global_insert(name, type, how);
  }
  global_debug();
}

void querySymtab(char* name, Type* ans_type, int* ans_how) {
  local_query(name, ans_type, ans_how);
  // 局部表找到声明或者定义都算成功，结果存储在ans_how上
  if (*ans_how == WRONG) {
    // 局部表中找不到，去全局表找
    global_query(name, ans_type, ans_how);
  }
}

void dropSymtab() {
  SymtabStk* temp = symtab_l;
  SymbolBucket* temp2 = temp->head;
  while (temp2 != NULL) {
    if (temp2->head->how == DEFINITION) {
      global_remove(temp2->name);
    } else if (temp2->head->how == DECLARATION) {
      printf("只有声明没有定义的符号: %s", temp2->name);
    }
    SymbolBucket* next = temp2->next;
    free(temp2->head);
    free(temp2);
    temp2 = next;
  }
  symtab_l = temp->next;
  free(temp);
  symtab_l_cnt -= 1;
#ifdef DEBUG
  printf("dropSymtab: %d\n", symtab_l_cnt + 1);
#endif
  global_debug();
}

void showSymtab() {
  SymbolBucket* temp = symtab_l->head;
  printf("%p\n", temp);
  printf("current Symtab: ");
  while (temp != NULL) {
    printf("%s, ", temp->name);
    temp = temp->next;
  }
  printf("\n\n");
}