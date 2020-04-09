#include "symboltable.h"

#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define DEBUG

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

对于符号：
（1）struct a和a不是同名
  （1.1）因为ID不可以包含空格，将"struct
a"作为前者的名字（结构体类型名），"a"作为后者的名字（变量名） （1.2）此时struct
a不会覆盖a的作用域

*/

void semantic_error(int error_type, int lineno, char* msg);

// 用于全局符号表
/* SymbolBucket symtab_g[SYMTAB_SIZE]
            ||
    SymbolBucket[0]: name == x,  name ==  y, hash(x)==hash(y)==0
    SymbolBucket[1]: name == w，其中hash(w)==1
    ...
*/
struct SymbolBucket {
  // char name[MAX_NAME_LEN];  // 当前符号的名字，其必定与head->symbolname相同
  Symbol* head;              //指向该符号的类型链表的首元素
  SymbolBucket* nextbucket;  // 符号表中的下一个桶
};

struct SymtabStk {
  Symbol* head;  // 指向当前符号表内的第一个元素
  SymtabStk* lastsymtab;  // 栈上的上一个符号表（即外层代码块的符号表）
};

/*
void build_FieldList(FieldList* fl, char* name, Type* type) {
  // 新申请一个FieldList，插在fl的末尾
  if (fl->type == NULL) {
    strcpy(fl->name, name);
    fl->type = type;
    fl->next = NULL;
  } else {
    FieldList* temp = malloc(sizeof(FieldList));
    strcpy(temp->name, name);
    temp->type = type;
    temp->next = NULL;
    FieldList* fl2 = fl;
    while (fl2->next != NULL) {
      printf("%s, ", fl2->name);
      fl2 = fl2->next;
    }
    fl2->next = temp;
  }
  printf("目前的域：\n");
  while (fl != NULL) {
    printf("%s:%s\n", fl->type->typename, fl->name);
    fl = fl->next;
  }
}

void build_Type_INT(Type* type, int val) {
  strcpy(type->typename, "int ");
  type->kind = _INT;
  type->int_val = val;
}

void build_Type_FLOAT(Type* type, float val) {
  strcpy(type->typename, "float ");
  type->kind = _FLOAT;
  type->float_val = val;
}

void build_Type_ARRAY(Type* type, Type* array_type, int array_size) {
  char newname[MAX_NAME_LEN];
  sprintf(newname, "%.16s[]", array_type->typename, array_size);
  strcpy(type->typename, newname);
  type->kind = _ARRAY;
  type->array.type = array_type;
  type->array.size = array_size;
}

void build_Type_STRUCTURE(Type* type, FieldList* structure) {
  type->kind = _STRUCTURE;
  type->structure = structure;
}
*/

#define SYMTAB_SIZE 0x3FFF

SymbolBucket* symtab_g[SYMTAB_SIZE];  // 全局符号表，使用散列表存储
SymtabStk* symtab_l;  // 局部符号表，（的链表中的）当前符号表指针
int symtab_l_cnt;  // 表的嵌套层数，debug用

/* 域类型比较 */
int fieldlist_equal(const FieldList* fl1, const FieldList* fl2) {
  while (fl1 && fl2) {
    const Type *t1 = &fl1->vartype, *t2 = &fl2->vartype;
    if (t1->kind != t2->kind) return 0;
    if (!type_equal(t1, t2)) return 0;
    fl1 = fl1->next, fl2 = fl2->next;
  }
  if (fl1 != NULL || fl2 != NULL) return 0;
  return 1;
}

/* 变量类型比较 */
int type_equal(const Type* t1, const Type* t2) {
  if (t1->kind != t2->kind) return 0;
  if (t1->kind == _ARRAY)
    // 对于数组类型，比较它们的基类型
    return type_equal(t1->array.type, t2->array.type);
  else if (t1->kind == _STRUCTURE)
    // 对于结构体类型，比较它们的域
    return fieldlist_equal(t1->structure.field, t2->structure.field);
  else
    // 对于INT和FLOAT（等基本）类型，由上文知相等
    return 1;
}

/* 结构体名比较 */
int structure_equal(const Structure* st1, const Structure* st2) {
  // 使用结构等价
  return fieldlist_equal(st1->field, st2->field);
}

/*  函数名比较 */
int function_equal(const Function* func1, const Function* func2) {
  if (!type_equal(&func1->ret_type, &func2->ret_type)) return 0;
  if (!fieldlist_equal(func1->parameter, func2->parameter)) return 0;
  return 1;
}

/* 符号比较 */
int symbol_kind_equal(const Symbol* sb1, const Symbol* sb2) {
  if (sb1->kind == _STRUCT_NAME || sb2->kind == _STRUCT_NAME) {
    printf("symbol_kind_equal(): should not be here. \n");
    // 不应该到这里
    return 0;
  }
  if (sb1->kind != sb2->kind) return 0;
  if (sb1->kind == _VARIABLE)
    return type_equal(&sb1->variable.type, &sb2->variable.type);
  else
    // if (sb1->kind == _FUNCTION_NAME)
    return function_equal(&sb1->function, &sb2->function);
}

/* 栈 */
void stack_push(SymbolBucket* addr, Symbol* sb) {
  Symbol* new_head = malloc(sizeof(Symbol));
  *new_head = *sb;
  // printf("sb(%d): %s, %p\n", sb->kind, sb->symbolname, sb->structure.field);
  assert(addr->head == NULL);
  new_head->nextsymbol = addr->head;
  addr->head = new_head;
  /*
  Symbol* sbb = addr->head;
  while (sbb) {
    printf("name: %s\n", sbb->symbolname);
    sbb = sbb->nextsymbol;
  }
  */
  // printf("newhead: %p,", addr->head);
}

void stack_pop(SymbolBucket* addr) {
  assert(addr->head != NULL);
  Symbol* head = addr->head;
  addr->head = head->nextsymbol;
  // printf("bucket of %s, addr->next: %p\n", addr->name, addr->head);
  free(head);
}

/* 域 */
void fieldlist_pushall(FieldList* fl) {
  // 获取当前的局部符号表
  // FieldList* debugfl = fl;

  Symbol* sb = symtab_l->head;
  do {
    if (sb->kind == _VARIABLE) {
      strcpy(fl->varname, sb->symbolname);
      fl->vartype = sb->variable.type;
      if (sb->nextsymbol != NULL) fl->next = malloc(sizeof(FieldList));
      fl = fl->next;
    } else if (sb->kind == _STRUCT_NAME) {
      // 这个是在结构体内定义的struct类型，并不是fieldlist的内容
      // 什么都不做，pass
    } else {
      assert(0);
    }
    // printf("name: %s, kind: %d\n", sb->symbolname, sb->kind);
    sb = sb->nextsymbol;
  } while (sb != NULL);
  /*
    while (debugfl) {
      printf("name: %s\n", debugfl->varname);
      debugfl = debugfl->next;
    }
    */
}

void fieldlist_popall(FieldList* fl) {
  while (fl != NULL) {
    FieldList* fl2 = fl;
    fl = fl->next;
    free(fl2);
  }
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

/* 向全局符号表中插入一个符号
  名字为name，值为type
 */

void global_insert(Symbol* sb) {
#ifdef DEBUG
  // printf("  global_insert(%s)\n", sb->symbolname);
#endif
  // 全局表只放定义，不放声明
  // printf("%s: defmethond%d\n", sb->symbolname, sb->dec);
  assert(sb->dec == DEFINITION);
  int val = hash(sb->symbolname);  // 计算name对应的hash值
  // printf("hash(%s) = %d\n", sb->symbolname, val);
  SymbolBucket* bk = symtab_g[val];  // temp指向name对应的桶中的首元素
  while (bk != NULL) {               // 通过while循环strcmp找到hash桶
    if (!strcmp(sb->symbolname, bk->head->symbolname)) break;
    bk = bk->nextbucket;
  }
  if (bk == NULL) {  //没找到name这个符号，name是新符号
    // 创建一个新的桶
    bk = malloc(sizeof(SymbolBucket));
    // strcpy(bk->name, sb->symbolname);
    bk->head = NULL;
    bk->nextbucket = symtab_g[val];
    symtab_g[val] = bk;
  }

  stack_push(bk, sb);
  // 桶中元素的符号名相同
  assert(!strcmp(bk->head->symbolname, sb->symbolname));
}

/* 查询全局符号表中的名字为name的符号的值type和定义方式how
  如果存在多个值对应，返回栈上的第一个值
  如果查询的是结构体类型，即name的前缀为"struct "，type返回的是该结构体的域
 */
void global_query(char* symbolname, Symbol* ans_sb) {
#ifdef DEBUG
  printf("global_query(%s)\n", symbolname);
#endif
  int val = hash(symbolname);
  SymbolBucket* bk = symtab_g[val];
  while (bk != NULL) {
    // 空桶需要删除，此处assert保证桶均非空
    assert(bk->head != NULL);
    if (!strcmp(bk->head->symbolname, symbolname)) break;
  }
  // if (bk == NULL || bk->head == NULL)
  if (bk == NULL)
    // 没有找到这个符号，未定义
    ans_sb->dec = UNDEFINE;
  else
    // 找到这个符号
    *ans_sb = *bk->head;
}

/*  移除全局符号表中的名字为name的符号的第一个值type
  若不存在这样的type，报错
*/
void global_remove(char* symbolname) {
#ifdef DEBUG
  // printf("global_remove(%s)\n", symbolname);
#endif
  int val = hash(symbolname);
  SymbolBucket* bk = symtab_g[val];
  while (bk != NULL)
    if (!strcmp(bk->head->symbolname, symbolname)) break;
  // printf("name: %s\n", symbolname);
  assert(bk != NULL);
  // 桶中元素的符号名相同
  // assert(!strcmp(bk->name, symbolname));
  stack_pop(bk);
  // 在桶为空时删除桶
  if (bk->head == NULL) {
    SymbolBucket* pre_bk = symtab_g[val];
    if (pre_bk == bk) {
      symtab_g[val] = pre_bk->nextbucket;
    } else {
      while (pre_bk->nextbucket != bk) pre_bk = pre_bk->nextbucket;
      assert(0);
      assert(pre_bk->nextbucket == bk);
      pre_bk->nextbucket = bk->nextbucket;
    }
    free(bk);
  }
}

int global_debug() {
  for (int val = 0; val < SYMTAB_SIZE; val++) {
    if (symtab_g[val]) {
      SymbolBucket* bk = symtab_g[val];
      while (bk) {
        int cnt = 0;
        Symbol* sb = bk->head;
        while (sb) {
          // printf("%s,%p\n", bk->head->symbolname, bk->head);
          cnt++;
          sb = sb->nextsymbol;
        }
        printf("name: %s, cnt: %d\n", bk->head->symbolname, cnt);
        bk = bk->nextbucket;
      }
    }
  }
  printf("\n");
  return 0;
}

/* 局部符号表 */
void local_init() { symtab_l = NULL; }

int local_insert(Symbol* sb) {
#ifdef DEBUG
  // printf("  local_insert(%s)\n", sb->symbolname);
#endif
  Symbol* symbol = symtab_l->head;
  while (symbol != NULL) {
    if (!strcmp(symbol->symbolname, sb->symbolname)) {
      // 符号已存在
      if (sb->dec == DEFINITION &&
          symbol->dec == DEFINITION) {     // 两个都是定义，则冲突
        return 1;                          // 重定义的符号: return 1;
      } else if (sb->dec == DEFINITION) {  // 新的是定义
        if (symbol_kind_equal(sb, symbol)) {
          symbol->dec = DEFINITION;
          return 0;
        } else {
          return 2;  // 定义与声明kind冲突: return 2
        }
      } else {  // 先定义后声明，或者两个声明，则检查冲突
        if (!symbol_kind_equal(symbol, sb))
          return 0;
        else
          return 3;  // 声明与定义冲突、声明与声明冲突 return 3
      }
    }
    // 在局部符号表中，nextsymbol是不同名的符号
    symbol = symbol->nextsymbol;
  }

  // 该符号不存在，则创建
  assert(symbol == NULL);
  symbol = malloc(sizeof(Symbol));
  *symbol = *sb;

  // 将该符号加入局部符号表
  symbol->nextsymbol = symtab_l->head;
  symtab_l->head = symbol;
  return 0;
}

void local_query(char* symbolname, Symbol* ans_sb) {
#ifdef DEBUG
  printf("local_query(%s)\n", symbolname);
#endif
  Symbol* sb = symtab_l->head;
  while (sb != NULL) {
    if (!strcmp(sb->symbolname, symbolname)) {
      *ans_sb = *sb;
      return;
    }
    sb = sb->nextsymbol;
  }
  ans_sb->dec = UNDEFINE;
}

/* 符号表 */
void initSymtab() {
  global_init();
  local_init();
}

void createSymtab() {
  if (symtab_l == NULL) {
    symtab_l = malloc(sizeof(SymtabStk));
    symtab_l->lastsymtab = NULL;
  } else {
    SymtabStk* st = malloc(sizeof(SymtabStk));
    st->lastsymtab = symtab_l;
    symtab_l = st;
  }
  symtab_l->head = NULL;
  symtab_l_cnt += 1;
#ifdef DEBUG
  printf("createSymtab: %d\n", symtab_l_cnt);
#endif
}

int insertSymtab(Symbol* sb) {
#ifdef DEBUG
  printf("insert symbolname: %s, dec: %d\n", sb->symbolname, sb->dec);
#endif
  int ret = local_insert(sb);
  if (ret)
    return ret;
  else
    global_insert(sb);
  return 0;
}

void querySymtab(char* symbolname, Symbol* ans_sb) {
  local_query(symbolname, ans_sb);
  // 局部表找到声明或者定义都算成功，结果存储在ans_how上
  if (ans_sb->dec == UNDEFINE) {
    // 局部表中找不到，去全局表找
    global_query(symbolname, ans_sb);
  }
}

void buildFieldListFromSymtab(Structure* structure) {
  // FuncDec和StructSpecifier会调用此函数
  assert(structure->field == NULL);
  structure->field = malloc(sizeof(FieldList));
  fieldlist_pushall(structure->field);
}

void dropFieldListAfterPush(FieldList* fl) { fieldlist_popall(fl); }

void dropSymtab() {
  // 对于FunDec和Compst的Symtab，其作用域结束时所有符号都不能再使用，故从global中移除
  // 对于StructSpecifier的Symtab，其作用域结束时符号还可能被其他Stmt使用
  // 当StructSpecifier外部的Symtab需要销毁时，这些符号才不能再被访问
  // 对于嵌套问题，可以在变量名前加前缀解决
  /*
  struct A{   // type: s(A), symbol: (none)
    int a;        //A.a
    struct{     // type: s(A).us(b), symbol: A.b
      int a;      //A.b.a
      struct{   // type: s(A).us(b).us(c), symbol: A.b.c
        int a;    //A.b.c.a
      }c;
    }b;
    struct C{  // type: s(A).s(C), symbol: A.e
      int a;       // A.e.a
    }e;
  };
  */

#ifdef DEBUG
  printf("dropSymtab: %d\n", symtab_l_cnt);
#endif
  Symbol* sb = symtab_l->head;
  while (sb != NULL) {
    if (sb->dec == DEFINITION) {
      global_remove(sb->symbolname);
    } else {
      assert(sb->dec == DECLARATION);
      // if (bk->head->dec == DECLARATION)
      printf("没有定义的已声明符号: (%s)\n", sb->symbolname);
    }
    Symbol* nextsb = sb->nextsymbol;
    free(sb);
    sb = nextsb;
  }
  SymtabStk* st = symtab_l;
  symtab_l = st->lastsymtab;
  free(st);
  symtab_l_cnt -= 1;
}

void showSymtab() {
  int cnt = 0;
  Symbol* sb = symtab_l->head;
  printf("current Symtab: \n");
  while (sb != NULL) {
    printf("(%d)%s, ", cnt++, sb->symbolname);
    sb = sb->nextsymbol;
  }
  printf("\n");
}