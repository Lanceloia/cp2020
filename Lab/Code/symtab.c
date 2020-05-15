#include "symtab.h"

#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define SYMTAB_SIZE 0x3FFF

Symtab* global;  // 全局符号表
Symtab* local;   // 局部符号表

// 域类型
int fieldlist_equal(const FieldList* fl1, const FieldList* fl2) {
  while (fl1 && fl2) {
    assert(fl1->sym->skind == S_VARIABLE && fl2->sym->skind == S_VARIABLE);
    const Type *t1 = fl1->sym->pvar->vtype, *t2 = fl2->sym->pvar->vtype;
    if (!type_equal(t1, t2)) return 0;
    fl1 = fl1->next, fl2 = fl2->next;
  }
  if (fl1 != NULL || fl2 != NULL) return 0;
  return 1;
}

// 变量类型比较
int type_equal(const Type* t1, const Type* t2) {
  if (t1->tkind != t2->tkind) return 0;
  // 对于数组类型, 比较它们的基类型
  if (t1->tkind == T_ARRAY) return type_equal(t1->array.type, t2->array.type);
  // 对于结构体类型, 比较它们的域
  if (t1->tkind == T_STRUCTURE) return fieldlist_equal(t1->field, t2->field);
  // 对于INT和FLOAT(等基本)类型, 由上文知相等
  return 1;
}

// 函数比较
static int function_equal(const FuncName* f1, const FuncName* f2) {
  if (!type_equal(f1->rtype, f2->rtype)) return 0;
  return fieldlist_equal(f1->params, f2->params);
}

static Symtab* Symtab_Create(int h, int v, Symtab* hor, Symtab* ver) {
  Symtab* ret = malloc(sizeof(Symtab));
  ret->hor = h;
  ret->vert = v;
  ret->symcnt = 0;
  ret->hor_last_symtab = hor;
  ret->vert_last_symtab = ver;
}

static void* Symtab_Drop(Symtab* st) {}

int ver = 0;

static void Symtab_Push() {
  assert(local->hor == 0);
  Symtab* st = Symtab_Create(local->hor, local->vert + 1, NULL, local);
  local = st;
}

static void Symtab_Pop() {
  assert(local->hor == 0);
  // assert(local->vert_last_symtab);
  Symtab* st = local;
  local = local->vert_last_symtab;
  Symtab_Drop(st);
}

void Symtab_Init() {
  global = Symtab_Create(0, 0, NULL, NULL);
  local = global;
}

void Symtab_Uninit() { Symtab_Pop(global); }

static Symbol* Query_At_Symtab(char* sbname, Symtab* st) {
  for (int i = 0; i < st->symcnt; i++)
    if (!strcmp(sbname, st->syms[i]->sbname)) return st->syms[i];
  return NULL;
}

int Insert_Symtab(Symbol* sb) {
#ifdef DEBUG
  // printf("insert: %s at [%d, %d]\n", sb->sbname, local->vert, local->hor);
#endif
  assert(sb->skind);
  Symbol* other;

  if (sb->skind == S_VARIABLE) {
    // 普通变量重定义: 3, 域变量重定义: 15
    if (other = Query_At_Symtab(sb->sbname, local))
      // 局部作用域, 任意符号都算重名
      return local->hor ? 15 : 3;
    else if (other = Query_Symtab(sb->sbname)) {
      // 全局作用域, 同名变量不算重名
      if (other->skind != S_VARIABLE) return local->hor ? 15 : 3;
    }
    // 之前所有的重名检测都通过, 插入局部表
    local->syms[local->symcnt++] = sb;
    return 0;

  } else if (sb->skind == S_FUNCTIONNAME) {
    if (other = Query_Symtab(sb->sbname)) {
      // 全局作用域, 查找冲突声明或定义
      assert(other->skind == S_FUNCTIONNAME);
      if (sb->pfunc->fdec_kind == F_DEFINITION &&
          other->pfunc->fdec_kind == F_DEFINITION)
        // 函数重定义: 4
        return 4;
      else {
        // 双声明或者定义和声明
        int eq = function_equal(sb->pfunc, other->pfunc);
        if (sb->pfunc->fdec_kind == F_DEFINITION)
          // 新定义覆盖旧声明
          other->pfunc = sb->pfunc;
        // 冲突: 19
        if (!eq)
          return 19;
        else
          return 0;
      }
    }
    // 之前所有的重名检测都通过, 插入局部表
    local->syms[local->symcnt++] = sb;
    return 0;

  } else if (sb->skind == S_STRUCTNAME) {
    if (other = Query_Symtab(sb->sbname))
      // 结构体重定义: 16
      return 16;

    // 之前所有的重名检测都通过, 插入函数栈帧上的表
    Symtab* st = local;
    while (st->hor_last_symtab) st = st->hor_last_symtab;
    st->syms[st->symcnt++] = sb;
    /*
    // 之前所有的重名检测都通过, 插入局部表
    // local->syms[local->symcnt++] = sb;
    */
    return 0;
  }
  assert(0);
}

Symbol* Query_Symtab(char* sbname) {
#ifdef DEBUG
  // printf("query: %s\n", sbname);
#endif
  Symtab* st = local;
  Symbol* ans;
  while (st->hor) {
    if (ans = Query_At_Symtab(sbname, st)) return ans;
    st = st->hor_last_symtab;
  }
  while (st) {
    if (ans = Query_At_Symtab(sbname, st)) return ans;
    st = st->vert_last_symtab;
  }
  return NULL;
}

static FieldList* BuildFieldListFromSymtab(Symtab* st) {
  FieldList* ret = NULL;
  if (st->symcnt) {
    FieldList* fl = ret = malloc(sizeof(FieldList));
    for (int i = 0; i < st->symcnt; i++) {
      if (st->syms[i]->skind == S_VARIABLE) {
        fl->sym = st->syms[i];
        if (i != st->symcnt - 1)
          fl->next = malloc(sizeof(FieldList));
        else
          fl->next = NULL;
        fl = fl->next;
      }
    }
  }
  return ret;
}

void FunDecLP() {
  assert(local->hor == 0);
  Symtab_Push();
}

void FunDecRP(Symbol* sb) {
  assert(local->hor == 0);
  assert(sb->skind == S_FUNCTIONNAME);
  sb->pfunc->params = BuildFieldListFromSymtab(local);
  Symtab_Pop();
}

void FunDecDotCompSt(Symbol* sb) {
  assert(local->hor == 0);
  assert(sb->skind == S_FUNCTIONNAME);
  Symtab_Push();
  FieldList* fl = sb->pfunc->params;

  while (fl != NULL) {
    Insert_Symtab(fl->sym);
    fl = fl->next;
  }
}

void FunDecCompStDot() { Symtab_Pop(); }
void DotCompSt() { Symtab_Push(); }
void CompStDot() { Symtab_Pop(); }

void StructSpecifierLC(Symbol* sb) {
  sb->pstruct->this_symtab =
      Symtab_Create(local->hor + 1, local->vert, local, NULL);
  local = sb->pstruct->this_symtab;
}

void StructSpecifierRC() {
  assert(local->hor > 0);
  assert(local->hor_last_symtab);
  local = local->hor_last_symtab;
}

int BuildBiasFromFieldList(FieldList* fl) {
  int bias = 0;
  while (fl) {
    fl->bias = bias;
    bias += fl->sym->pvar->vtype->type_size;
    fl = fl->next;
  }
  return bias;
}

Type* BuildStructure(Symtab* st, char* stname) {
  assert(st->hor);
  Type* type = malloc(sizeof(Type));

  type->tkind = T_STRUCTURE;
  // strcpy(type->tname, stname);
  type->field = BuildFieldListFromSymtab(st);
  type->left_val = 1;
  type->type_size = BuildBiasFromFieldList(type->field);
  return type;
}

int Symtab_mode() {
  // 根据当前符号表的水平坐标确定模式
  return local->hor;
}
