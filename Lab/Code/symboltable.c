#include "symboltable.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/*
符号表使用散列表来存储
[散列表]存储[同名符号的栈]，使用链表来解决散列表中符号的冲突问题
遇到一个符号时，先查散列表找到对应的栈，然后返回栈顶的符号
遇到一个新的Compst的左括号时，开辟一个数组记录新的符号的集合
遇到该Compst的右括号时，将数组中记录的符号从散列表中弹出
*/

#define SYMTAB_SIZE 0x3FFF
#define MAX_NAME_LEN 32

#define WRONG 0
#define DEFINITION 1
#define DECLARATION 2

struct Type {
  enum { _INT, _FLOAT, _ARRAY, _STRUCTURE } kind;
  union {
    struct {
      Type* type;
      int size;
    } array;
    FieldList* structure;
  };
};

struct FieldList {
  char name[MAX_NAME_LEN];
  Type* type;
  FieldList* next;
};

struct TypeList {
  Type* type;      // 当前符号的类型
  int how;         // 声明或是定义
  TypeList* next;  // 与该符号同名的下一种类型
};

struct SymbolStk {
  char name[MAX_NAME_LEN];  //当前符号的名字
  TypeList* head;           //指向该符号的类型链表的首元素
  SymbolStk* next;          //元素表中的下一个符号
};

struct SymtabStk {
  SymbolStk* head;  //指向当前符号表内的第一个元素
  SymtabStk* next;  //栈上的下一个符号表（即外层代码块的符号表）
};

SymbolStk* symtab_g[SYMTAB_SIZE];  //全局符号表，使用散列表存储
SymtabStk* symtab_l;  //局部符号表，（的链表中的）当前符号表指针
int symtab_l_cnt;  //表的嵌套层数，debug用

/* 栈 */

void stack_push(SymbolStk* addr, char* name, Type* type, int how) {
  if (strcmp(name, addr->name) != 0) {
    printf("上层逻辑错误，表中符号为%s，期望插入符号为%s\n", addr->name, name);
    return;
  }
  TypeList* old_head = addr->head;
  TypeList* new_head = malloc(sizeof(TypeList));
  // type赋值
  new_head->type = type;
  // how赋值
  new_head->how = how;
  new_head->next = old_head;
  addr->head = new_head;
}

void stack_pop(SymbolStk* addr, char* name) {
  if (strcmp(name, addr->name) != 0) {
    printf("上层逻辑错误，表中符号为%s，期望插入符号为%s\n", addr->name, name);
    return;
  }
  if (addr->head == NULL) {
    printf("对空栈(%s)进行弹出\n", addr->name);
    return;
  }
  TypeList* old_head = addr->head;
  addr->head = addr->head->next;
  free(old_head);
}

/* 全局符号表*/

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

  int val = hash(name);             // 计算name对应的hash值
  SymbolStk* temp = symtab_g[val];  // temp指向name对应的桶中的首元素
  while (temp != NULL)              // 通过while循环strcmp找到hash桶
    if (strcmp(name, temp->name) == 0) break;
  if (temp == NULL) {  //没找到name这个符号，name是新符号
    // 创建一个新的栈
    temp = malloc(sizeof(SymbolStk));
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
  printf("global_query(%s)\n", name);
  int val = hash(name);
  SymbolStk* temp = symtab_g[val];
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
  SymbolStk* temp = symtab_g[val];
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
      SymbolStk* temp = symtab_g[eachval];
      while (temp) {
        int cnt = 0;
        TypeList* temp2 = temp->head;
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
  SymbolStk* temp = symtab_l->head;
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
  temp = malloc(sizeof(SymbolStk));
  strcpy(temp->name, name);
  temp->head = malloc(sizeof(TypeList));
  temp->head->type = type;
  temp->head->how = how;
  // 将该符号加入局部符号表
  temp->next = symtab_l->head;
  symtab_l->head = temp;

  return 0;
}

void local_query(char* name, Type* ans_type, int* ans_how) {
  printf("local_query(%s)\n", name);
  SymbolStk* temp = symtab_l->head;
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
  printf("createSymtab: %d\n", symtab_l_cnt);
  global_debug();
}

void insertSymtab(char* name, Type* type, int how) {
  printf("insert %s\n", name);
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
  SymbolStk* temp2 = temp->head;
  while (temp2 != NULL) {
    if (temp2->head->how == DEFINITION) {
      global_remove(temp2->name);
    } else if (temp2->head->how == DECLARATION) {
      printf("只有声明没有定义的符号: %s", temp2->name);
    }
    SymbolStk* next = temp2->next;
    free(temp2->head);
    free(temp2);
    temp2 = next;
  }
  symtab_l = temp->next;
  free(temp);
  symtab_l_cnt -= 1;
  printf("dropSymtab: %d\n", symtab_l_cnt + 1);
  global_debug();
}

void showSymtab() {
  SymbolStk* temp = symtab_l->head;
  printf("%p\n", temp);
  printf("current Symtab: ");
  while (temp != NULL) {
    printf("%s, ", temp->name);
    temp = temp->next;
  }
  printf("\n\n");
}