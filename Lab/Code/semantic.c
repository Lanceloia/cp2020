#include "semantic.h"

#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define translate_printf printf

int new_vtemp() {
  static int vtemp = 0;
  return ++vtemp;
}

int new_temp() {
  static int temp = 0;
  return ++temp;
}

int new_label() {
  static int label = 0;
  return ++label;
}

const Type INT = {.tkind = T_INT, .left_val = 0, .type_size = 4};
const Type FLOAT = {.tkind = T_FLOAT, .left_val = 0, .type_size = 4};
static Type* LType_INT();
static Type* LType_FLOAT();
static Type* LType_UKST();

void Program(struct ast* node) {
  // 初始化全局符号表
  Symtab_Init();
  ExtDefList(node->children[0]);
  Symtab_Uninit();
}

void ExtDefList(struct ast* node) {
  // 右递归改迭代
  while (node->num != -1) {
    // ExtDefList -> ExtDef ExtDefList
    ExtDef(node->children[0]);
    node = node->children[1];
  }
  // ExtDefList -> empty
  return;
}

void ExtDef(struct ast* node) {
  // Specifier识别出type的内容
  const Type* type = Specifier(node->children[0]);

  // type == NULL: 结构体的定义冲突
  if (!type) return;

  if (!strcmp(node->children[1]->name, "ExtDecList")) {
    // ExtDef -> Specifier . ExtDecList SEMI
    ExtDecList(node->children[1], type);
  } else if (!strcmp(node->children[1]->name, "SEMI")) {
    // ExtDef -> Specifier . SEMI
  } else {
    // ExtDef -> Specifier . FunDec SEMI
    // ExtDef -> Specifier . FunDec CompSt
    Symbol* func = FunDec(node->children[1]);

    func->pfunc->rtype = type;

    if (!strcmp(node->children[2]->name, "SEMI")) {
      // ExtDef -> Specifier FunDec . SEMI
      func->pfunc->fdec_kind = F_DECLARATION;
      Insert_Symtab(func);
    } else {
      // ExtDef -> Specifier FunDec . CompSt
      func->pfunc->fdec_kind = F_DEFINITION;
      Insert_Symtab(func);

      translate_printf("FUNCTION %s :\n", func->sbname);
      FieldList* fl = func->pfunc->params;
      while (fl) {
        fl->sym->pvar->vname = new_vtemp();
        fl->sym->pvar->isParam = 1;
        translate_printf("PARAM v%d\n", fl->sym->pvar->vname);
        fl = fl->next;
      }

      FunDecDotCompSt(func);
      CompSt(node->children[2], type);
      FunDecCompStDot(NULL);

      translate_printf("\n");
    }
  }
}

void ExtDecList(struct ast* node, const Type* type) {
  // 右递归改迭代
  while (TRUE) {
    VarDec(node->children[0], type);

    if (node->num == 1) {
      // ExtDecList -> VarDec .
      break;
    } else {
      // ExtDecList -> VarDec . COMMA ExtDecList
      node = node->children[2];
    }
  }
}

const Type* Specifier(struct ast* node) {
  if (!strcmp(node->children[0]->name, "TYPE")) {
    // Specifier -> TYPE
    if (!strcmp(node->children[0]->id_name, "int"))
      return &INT;
    else
      return &FLOAT;
  } else
    // Specifier -> StructSpecifier
    return StructSpecifier(node->children[0]);
}

const Type* StructSpecifier(struct ast* node) {
  char stname[MAX_NAME_LEN];
  if (!strcmp(node->children[1]->name, "OptTag")) {
    // StructSpecifier -> STRUCT OptTag LC DefList RC
    OptTag(node->children[1], stname);

    // 符号定义素质五连
    Symbol* st = malloc(sizeof(Symbol));
    strcpy(st->sbname, stname);
    st->skind = S_STRUCTNAME;
    st->dec_lineno = node->lineno;
    st->pstruct = malloc(sizeof(StructName));

    st->pstruct->stdec_kind = ST_UNDEFINED;

    // 插入
    Insert_Symtab(st);
    StructSpecifierLC(st);
    DefList(node->children[3]);
    StructSpecifierRC();

    st->pstruct->stdec_kind = ST_DEFINED;
    return BuildStructure(st->pstruct->this_symtab, stname);
  } else {
    // StructSpecifier -> STRUCT Tag
    Tag(node->children[1], stname);

    Symbol* st = Query_Symtab(stname);
    if (!st || st->skind != S_STRUCTNAME ||
        st->pstruct->stdec_kind != ST_DEFINED) {
      return LType_UKST();
    }
    return BuildStructure(st->pstruct->this_symtab, stname);
  }
}

void OptTag(struct ast* node, char* ans_name) {
  static int unname_cnt = 0;
  if (node->num != -1)
    // OptTag -> ID
    ID(node->children[0], ans_name);
  else {
    // OptTag -> empty
    sprintf(ans_name, "unname(%d)", unname_cnt++);
  }
}

void Tag(struct ast* node, char* ans_name) {
  // Tag -> ID
  ID(node->children[0], ans_name);
}

Symbol* FunDec(struct ast* node) {
  // 符号定义素质五连
  Symbol* func = malloc(sizeof(Symbol));
  ID(node->children[0], func->sbname);
  func->skind = S_FUNCTIONNAME;
  func->dec_lineno = node->lineno;
  func->pfunc = malloc(sizeof(FuncName));

  FunDecLP();
  if (node->num == 4) {
    // FunDec -> ID LP . VarList RP
    VarList(node->children[2]);
  } else {
    // FunDec -> ID LP . RP
  }
  FunDecRP(func);
  return func;
}

void VarList(struct ast* node) {
  // 右递归改迭代
  while (TRUE) {
    ParamDec(node->children[0]);
    if (node->num == 1) {
      // VarList -> ParamDec .
      break;
    } else {
      // VarList -> ParamDec .  COMMA VarList
      node = node->children[2];
    }
  }
}

void ParamDec(struct ast* node) {
  // ParamDec -> Specifier VarDec
  const Type* type = Specifier(node->children[0]);
  VarDec(node->children[1], type);
}

void CompSt(struct ast* node, const Type* ret_type) {
  // CompSt -> LC DefList StmtList RC
  DefList(node->children[1]);
  StmtList(node->children[2], ret_type);
}

void DefList(struct ast* node) {
  // 右递归改迭代
  while (node->num != -1) {
    // DefList -> Def DefList
    Def(node->children[0]);
    node = node->children[1];
  }
  // DefList -> empty
}

void Def(struct ast* node) {
  // Def -> Specifier DecList SEMI
  const Type* type = Specifier(node->children[0]);
  DecList(node->children[1], type);
}

void DecList(struct ast* node, const Type* type) {
  // 右递归改迭代
  while (TRUE) {
    Dec(node->children[0], type);
    if (node->num != 1) {
      // DecList -> Dec . DecList
      node = node->children[2];
    } else
      // DecList -> Dec .
      break;
  }
}

void Dec(struct ast* node, const Type* type) {
  assert(type->tkind);
  if (node->num == 1) {
    //  Dec -> VarDec
    const Symbol* sb = VarDec(node->children[0], type);
    // translate
    if (!Symtab_mode()) {
      sb->pvar->vname = new_vtemp();
      sb->pvar->isParam = 0;
      translate_printf("DEC v%d %d\n", sb->pvar->vname,
                       sb->pvar->vtype->type_size);
    }
  } else {
    // Dec -> VarDec ASSIGNOP Exp
    const Symbol* sb = VarDec(node->children[0], type);
    // translate
    if (!Symtab_mode()) {
      sb->pvar->vname = new_vtemp();
      sb->pvar->isParam = 0;
      translate_printf("DEC v%d %d\n", sb->pvar->vname,
                       sb->pvar->vtype->type_size);
      int t1 = new_temp();
      int t2 = new_temp();
      Exp(node->children[0], t1, LEFT);
      Exp(node->children[2], t2, RIGHT);
      translate_printf("*t%d := t%d\n", t1, t2);
    }
  }
}

const Symbol* VarDec(struct ast* node, const Type* type) {
  // 变量为左值
  if (type->tkind == T_INT) type = LType_INT();
  if (type->tkind == T_FLOAT) type = LType_FLOAT();
  assert(type->left_val);
  // 左递归改为迭代
  // VarDec -> VarDec LB INT RB
  Type* arr = NULL;
  while (node->num == 4) {
    int size = node->children[2]->int_value;
    arr = malloc(sizeof(Type));
    arr->tkind = T_ARRAY;
    arr->left_val = 1;
    arr->array.type = type;
    arr->array.size = size;
    arr->type_size = type->type_size * size;

    node = node->children[0];
    type = arr;
  }

  // VarDec -> ID
  char vname[MAX_NAME_LEN];
  ID(node->children[0], vname);

  // 符号定义素质五连
  Symbol* sb = malloc(sizeof(Symbol));
  strcpy(sb->sbname, vname);
  sb->skind = S_VARIABLE;
  sb->dec_lineno = node->lineno;
  sb->pvar = malloc(sizeof(Var));

  sb->pvar->vtype = type;
  Insert_Symtab(sb);
  // 返回最终定义变量用的类型
  return sb;
}

void StmtList(struct ast* node, const Type* ret_type) {
  // 右递归改迭代
  while (node->num != -1) {
    // StmtList -> Stmt StmtList
    Stmt(node->children[0], ret_type);
    node = node->children[1];
  }
  // StmtList -> empty
}

void Stmt(struct ast* node, const Type* ret_type) {
  if (!strcmp(node->children[0]->name, "Exp")) {
    // Stmt -> Exp SEMI
    int t1 = new_temp();
    Exp(node->children[0], t1, RIGHT);
  } else if (!strcmp(node->children[0]->name, "CompSt")) {
    DotCompSt();
    CompSt(node->children[0], ret_type);
    CompStDot();
  } else if (!strcmp(node->children[0]->name, "RETURN")) {
    // Stmt -> RETURN Exp SEMI
    // 判断RETURN的类型和函数是否相容
    int t1 = new_temp();
    Exp(node->children[1], t1, RIGHT);
    translate_printf("RETURN t%d\n", t1);
  } else if (!strcmp(node->children[0]->name, "IF")) {
    if (node->num == 5) {
      // Stmt -> IF LP Exp RP Stmt
      int l1 = new_label();
      int l2 = new_label();
      Cond(node->children[2], l1, l2);
      translate_printf("LABEL label%d :\n", l1);
      Stmt(node->children[4], ret_type);
      translate_printf("LABEL label%d :\n", l2);
    } else if (node->num == 7) {
      // Stmt -> IF LP Exp RP Stmt ELSE Stmt
      int l1 = new_label();
      int l2 = new_label();
      int l3 = new_label();
      Cond(node->children[2], l1, l2);
      translate_printf("LABEL label%d :\n", l1);
      Stmt(node->children[4], ret_type);
      translate_printf("GOTO label%d\n", l3);
      translate_printf("LABEL label%d :\n", l2);
      Stmt(node->children[6], ret_type);
      translate_printf("LABEL label%d :\n", l3);
    }
  } else if (!strcmp(node->children[0]->name, "WHILE")) {
    // Stmt -> WHILE LP Exp RP Stmt
    int l1 = new_label();
    int l2 = new_label();
    int l3 = new_label();
    translate_printf("LABEL label%d :\n", l1);
    Cond(node->children[2], l2, l3);
    translate_printf("LABEL label%d :\n", l2);
    Stmt(node->children[4], ret_type);
    translate_printf("GOTO label%d\n", l1);
    translate_printf("LABEL label%d :\n", l3);
  } else {
    assert(0);
  }
}

void Cond(struct ast* node, int ltrue, int lfalse) {
  if (node->num == 2) {
    if (!strcmp(node->children[0]->name, "NOT")) {
      Cond(node->children[1], lfalse, ltrue);
      return;
    }
  } else if (node->num == 3) {
    if (!strcmp(node->children[1]->name, "RELOP")) {
      int t1 = new_temp();
      int t2 = new_temp();
      Exp(node->children[0], t1, RIGHT);
      Exp(node->children[2], t2, RIGHT);
      translate_printf("IF t%d %s t%d GOTO label%d\n", t1,
                       node->children[1]->id_name, t2, ltrue);
      translate_printf("GOTO label%d\n", lfalse);
      return;
    } else if (!strcmp(node->children[1]->name, "AND")) {
      int l1 = new_label();
      Cond(node->children[0], l1, lfalse);
      translate_printf("LABEL label%d :\n", l1);
      Cond(node->children[2], ltrue, lfalse);
      return;
    } else if (!strcmp(node->children[1]->name, "OR")) {
      int l1 = new_label();
      Cond(node->children[0], ltrue, l1);
      translate_printf("LABEL label%d :\n", l1);
      Cond(node->children[2], ltrue, lfalse);
      return;
    }
  }

  // default
  int t1 = new_temp();
  Exp(node, t1, RIGHT);
  translate_printf("IF t%d != #0 GOTO label%d\n", t1, ltrue);
  translate_printf("GOTO label%d\n", lfalse);
}

const Type* Exp(struct ast* node, int place, int addr) {
  if (node->num == 3 && !strcmp(node->children[0]->name, "Exp") &&
      !strcmp(node->children[2]->name, "Exp")) {
    // 双目运算符

    if (!strcmp(node->children[1]->name, "ASSIGNOP")) {
      // 赋值运算

      // translate
      int t1 = new_temp();
      int t2 = new_temp();
      Exp(node->children[0], t1, LEFT);
      Exp(node->children[2], t2, RIGHT);
      translate_printf("*t%d := t%d\n", t1, t2);
      translate_printf("t%d := t%d\n", place, t2);

    } else if (!strcmp(node->children[1]->name, "AND") ||
               !strcmp(node->children[1]->name, "OR") ||
               !strcmp(node->children[1]->name, "RELOP")) {
      // 与运算和或运算以及比较运算

      // translate
      int l1 = new_label();
      int l2 = new_label();
      translate_printf("t%d := #0\n", place);
      Cond(node, l1, l2);
      translate_printf("LABEL label%d :\n", l1);
      translate_printf("t%d := #1\n", place);
      translate_printf("LABEL label%d :\n", l2);
    } else {  // 加减乘除
      int t1 = new_temp();
      int t2 = new_temp();
      Exp(node->children[0], t1, RIGHT);
      Exp(node->children[2], t2, RIGHT);
      if (!strcmp(node->children[1]->name, "PLUS"))
        translate_printf("t%d := t%d + t%d\n", place, t1, t2);
      else if (!strcmp(node->children[1]->name, "MINUS"))
        translate_printf("t%d := t%d - t%d\n", place, t1, t2);
      else if (!strcmp(node->children[1]->name, "STAR"))
        translate_printf("t%d := t%d * t%d\n", place, t1, t2);
      else if (!strcmp(node->children[1]->name, "DIV"))
        translate_printf("t%d := t%d / t%d\n", place, t1, t2);
    }
  } else if (node->num == 3 && !strcmp(node->children[0]->name, "LP")) {
    // Exp -> LP Exp RP
    return Exp(node->children[1], place, addr);
  } else if (!strcmp(node->children[0]->name, "MINUS")) {
    // Exp -> MINUS

    // translate
    int t1 = new_temp();

    Exp(node->children[1], t1, addr);
    translate_printf("t%d := #0 - t%d\n", place, t1);

  } else if (!strcmp(node->children[0]->name, "NOT")) {
    // Exp -> NOT

    // translate
    int l1 = new_label();
    int l2 = new_label();
    translate_printf("t%d := #0\n", place);
    Cond(node, l1, l2);
    translate_printf("LABEL label%d :\n", l1);
    translate_printf("t%d := #1\n", place);
    translate_printf("LABEL label%d :\n", l2);

  } else if (node->num > 2 && !strcmp(node->children[0]->name, "ID") &&
             !strcmp(node->children[1]->name, "LP")) {
    // Exp -> ID LP RP
    // Exp -> ID LP Args RP
    char fname[MAX_NAME_LEN];
    ID(node->children[0], fname);
    Symbol* func = Query_Symtab(fname);

    if (node->num == 4) {
      // translate
      if (!strcmp(node->children[0]->id_name, "write")) {
        int t1 = new_temp();
        Exp(node->children[2]->children[0], t1, RIGHT);
        translate_printf("WRITE t%d\n", t1);
      } else {
        struct ArgList* arglist = Args(node->children[2], func->pfunc->params);
        while (arglist) {
          translate_printf("ARG t%d\n", arglist->place);
          arglist = arglist->next;
        }
        translate_printf("t%d := CALL %s\n", place, fname);
      }
    } else if (node->num == 3) {
      // 无参数的函数

      // translate
      if (!strcmp(node->children[0]->id_name, "read"))
        translate_printf("READ t%d\n", place);
      else
        translate_printf("t%d := CALL %s\n", place, fname);
    }
  } else if (node->num == 4 && !strcmp(node->children[1]->name, "LB") &&
             !strcmp(node->children[3]->name, "RB")) {
    // Exp -> Exp LB Exp RB
    int t1 = new_temp();
    int t2 = new_temp();
    const Type* arr = Exp(node->children[0], t1, LEFT);
    Exp(node->children[2], t2, RIGHT);
    assert(arr);
    assert(arr->tkind == T_ARRAY);
    int width = arr->array.type->type_size;
    translate_printf("t%d := t%d * #%d\n", t2, t2, width);
    translate_printf("t%d := t%d + t%d\n", t1, t1, t2);
    if (addr == LEFT)
      translate_printf("t%d := t%d\n", place, t1);
    else
      translate_printf("t%d := *t%d\n", place, t1);

    return arr->array.type;
  } else if (node->num == 3 && !strcmp(node->children[1]->name, "DOT")) {
    // 域变量访问
    char varname[MAX_NAME_LEN];
    int t1 = new_temp();
    const Type* type = Exp(node->children[0], t1, LEFT);
    ID(node->children[2], varname);
    FieldList* fl = type->field;
    while (fl) {
      assert(fl->sym);
      if (!strcmp(fl->sym->sbname, varname)) break;
      fl = fl->next;
    }
    assert(fl);
    translate_printf("t%d := t%d + #%d\n", t1, t1, fl->bias);
    if (addr == LEFT)
      translate_printf("t%d := t%d\n", place, t1);
    else
      translate_printf("t%d := *t%d\n", place, t1);

    return fl->sym->pvar->vtype;

  } else if (!strcmp(node->children[0]->name, "ID") && node->num == 1) {
    // 标识符
    char sbname[MAX_NAME_LEN];
    ID(node->children[0], sbname);
    Symbol* sb = Query_Symtab(sbname);

    // translate
    // 要求左值时，将变量的地址返回，若其本身就是地址则忽略
    if (addr == LEFT &&
        (sb->pvar->vtype->tkind == T_INT || sb->pvar->vtype->tkind == T_FLOAT ||
         sb->pvar->isParam == 0))
      translate_printf("t%d := &v%d\n", place, sb->pvar->vname);
    else
      translate_printf("t%d := v%d\n", place, sb->pvar->vname);

    return sb->pvar->vtype;
  } else if (!strcmp(node->children[0]->name, "INT")) {
    // translate
    int value = node->children[0]->int_value;
    translate_printf("t%d := #%d\n", place, value);
  } else if (!strcmp(node->children[0]->name, "FLOAT")) {
    // translate
    float value = node->children[0]->float_value;
    translate_printf("t%d := #%f\n", place, value);
  } else {
    assert(0);
  }

  return NULL;
}

struct ArgList* Args(struct ast* node, FieldList* fl) {
  struct ArgList* ret = NULL;
  while (fl) {
    int t1 = new_temp();
    if (fl->sym->pvar->vtype->tkind != T_INT &&
        fl->sym->pvar->vtype->tkind != T_FLOAT)
      Exp(node->children[0], t1, LEFT);
    else
      Exp(node->children[0], t1, RIGHT);
    struct ArgList* arglist = malloc(sizeof(struct ArgList));
    arglist->place = t1;
    arglist->next = ret;
    ret = arglist;

    node = node->children[2];
    fl = fl->next;
  }
  return ret;
}

void ID(struct ast* node, char* ans_name) { strcpy(ans_name, node->id_name); }

void eval_semantic(struct ast* root) {
  // eval_syntax_tree(root, 0);
  Program(root);
}

static Type* LType_INT() {
  Type* type = malloc(sizeof(Type));
  type->tkind = T_INT;
  type->left_val = 1;
  type->type_size = 4;
  // strcpy(type->tname, "int");
  return type;
}

static Type* LType_FLOAT() {
  Type* type = malloc(sizeof(Type));
  type->tkind = T_FLOAT;
  type->left_val = 1;
  type->type_size = 4;
  // strcpy(type->tname, "float");
  return type;
}

static Type* LType_UKST() {
  Type* type = malloc(sizeof(Type));
  type->tkind = T_STRUCTURE;
  type->left_val = 1;
  // nouse
  type->type_size = 996;
  // strcpy(type->tname, "uk-st");
  return type;
}