#include "semantic.h"

#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int semantic_error(int error_type, int lineno, char* msg);

const Type INT = {.tkind = T_INT, .left_val = 0};
const Type FLOAT = {.tkind = T_FLOAT, .left_val = 0};
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
  if (node->num == -1)
    // ExtDefList -> epsilon
    return;
  // ExtDefList -> ExtDef ExtDefList
  ExtDef(node->children[0]);      // ExtDef
  ExtDefList(node->children[1]);  // ExtDefList
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

      FunDecDotCompSt(func);
      CompSt(node->children[2], type);
      FunDecCompStDot(NULL);
    }
  }
}

void ExtDecList(struct ast* node, const Type* type) {
  if (node->num == 1) {
    // ExtDecList -> VarDec
    VarDec(node->children[0], type);
  } else {
    // ExtDecList -> VarDec COMMA ExtDecList
    VarDec(node->children[0], type);
    ExtDecList(node->children[2], type);
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

    // 插入
    Insert_Symtab(st);
    StructSpecifierLC(st);
    DefList(node->children[3]);
    StructSpecifierRC();

    return BuildStructure(st->pstruct->this_symtab, stname);
  } else {
    // StructSpecifier -> STRUCT Tag
    Tag(node->children[1], stname);

    Symbol* st = Query_Symtab(stname);
    if (!st) {
      semantic_error(17, node->lineno, stname);
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
  if (node->num == 1)
    // VarList -> ParamDec
    ParamDec(node->children[0]);
  else {
    // VarList -> ParamDec COMMA VarList
    ParamDec(node->children[0]);
    VarList(node->children[2]);
  }
}

void ParamDec(struct ast* node) {
  // ParamDec -> Specifier VarDec
  const Type* type = Specifier(node->children[0]);
  VarDec(node->children[1], type);
}

void CompSt(struct ast* node, const Type* ret_type) {
  // CompSt -> LC DefList StmtList RC
  CompStLC();
  DefList(node->children[1]);
  StmtList(node->children[2], ret_type);
  CompStRC();
}

void DefList(struct ast* node) {
  if (node->num != -1) {
    // DefList -> Def DefList
    Def(node->children[0]);
    DefList(node->children[1]);
  } else {
    // DefList -> empty
  }
}

void Def(struct ast* node) {
  // Def -> Specifier DecList SEMI
  const Type* type = Specifier(node->children[0]);
  DecList(node->children[1], type);
}

void DecList(struct ast* node, const Type* type) {
  if (node->num == 1) {
    // DecList -> Dec
    Dec(node->children[0], type);
  } else {
    // DecList -> Dec COMMA DecList
    Dec(node->children[0], type);
    DecList(node->children[2], type);
  }
}

void Dec(struct ast* node, const Type* type) {
  assert(type->tkind);
  if (node->num == 1)
    //  Dec -> VarDec
    VarDec(node->children[0], type);
  else {
    // Dec -> VarDec ASSIGNOP Exp
    if (Symtab_mode() != 0) semantic_error(15, node->lineno, NULL);
    VarDec(node->children[0], type);
    if (!type_equal(type, Exp(node->children[2])))
      semantic_error(5, node->lineno, NULL);
  }
}

void VarDec(struct ast* node, const Type* type) {
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
}

void StmtList(struct ast* node, const Type* ret_type) {
  if (node->num != -1) {
    // StmtList -> Stmt StmtList
    Stmt(node->children[0], ret_type);
    StmtList(node->children[1], ret_type);
  } else {
    // StmtList -> empty
    return;
  }
}

void Stmt(struct ast* node, const Type* ret_type) {
  if (!strcmp(node->children[0]->name, "Exp")) {
    // Stmt -> Exp SEMI
    Exp(node->children[0]);
  } else if (!strcmp(node->children[0]->name, "CompSt")) {
    CompStLC();
    CompSt(node->children[0], ret_type);
    CompStRC();
  } else if (!strcmp(node->children[0]->name, "RETURN")) {
    // Stmt -> RETURN Exp SEMI
    // 判断RETURN的类型和函数是否相容
    const Type* type = Exp(node->children[1]);
    if (type && !type_equal(ret_type, Exp(node->children[1])))
      semantic_error(8, node->lineno, NULL);
  } else if (!strcmp(node->children[0]->name, "IF")) {
    if (node->num == 5) {
      // Stmt -> IF LP Exp RP Stmt
      // 检查Exp的类型是否为int型
      const Type* type = Exp(node->children[2]);
      if (type && type->tkind != T_INT) semantic_error(7, node->lineno, NULL);
      Stmt(node->children[4], ret_type);
    } else if (node->num == 7) {
      // Stmt -> IF LP Exp RP Stmt ELSE Stmt
      // 检查Exp的类型是否为int型
      const Type* type = Exp(node->children[2]);
      if (type && type->tkind != T_INT) semantic_error(7, node->lineno, NULL);
      Stmt(node->children[4], ret_type);
      Stmt(node->children[6], ret_type);
    }
  } else if (!strcmp(node->children[0]->name, "WHILE")) {
    // Stmt -> WHILE LP Exp RP Stmt
    // 检查Exp的类型是否为int型
    const Type* type = Exp(node->children[2]);
    assert(type);
    if (type && type->tkind != T_INT) semantic_error(7, node->lineno, NULL);
    Stmt(node->children[4], ret_type);
  } else {
    assert(0);
  }
}

const Type* Exp(struct ast* node) {
  if (node->num == 3 && !strcmp(node->children[0]->name, "Exp") &&
      !strcmp(node->children[2]->name, "Exp")) {
    // 双目运算符
    const Type* t1 = Exp(node->children[0]);
    const Type* t2 = Exp(node->children[2]);

    if (!strcmp(node->children[1]->name, "ASSIGNOP")) {
      // 赋值运算
      if (!t1 || !t2) return NULL;
      if (!type_equal(t1, t2)) semantic_error(5, node->lineno, NULL);
      if (!t1->left_val) semantic_error(6, node->lineno, NULL);
      // (禁用)即使左边不是左值，将赋值表达式视作左值继续分析
      return t1;
    } else if (!strcmp(node->children[1]->name, "AND") ||
               !strcmp(node->children[1]->name, "OR")) {
      // 与运算和或运算
      if (!t1 || !t2) return &INT;
      // 操作数要求都为INT类型
      if (t1->tkind != T_INT || t2->tkind != T_INT)
        semantic_error(7, node->lineno, NULL);
      // 将与或表达式视为INT类型继续分析
      return &INT;
    } else if (!strcmp(node->children[1]->name, "RELOP")) {
      // 比较运算
      if (!t1 || !t2) return &INT;
      if (!type_equal(t1, t2) || (t1->tkind != T_INT && t1->tkind != T_FLOAT))
        semantic_error(7, node->lineno, NULL);
      // 整条表达式的类型等于INT
      return &INT;
    } else {  // 加减乘除
      if (!t1 || !t2) return NULL;
      if (!type_equal(t1, t2) || (t1->tkind != T_INT && t1->tkind != T_FLOAT))
        semantic_error(7, node->lineno, NULL);
      Type* type = malloc(sizeof(Type));
      memcpy(type, t1, sizeof(Type));
      type->left_val = 0;
      return type;
    }
  } else if (node->num == 3 && !strcmp(node->children[0]->name, "LP")) {
    // Exp -> LP Exp RP
    return Exp(node->children[1]);
  } else if (!strcmp(node->children[0]->name, "MINUS")) {
    // Exp -> MINUS
    const Type* type = Exp(node->children[1]);
    if (!type || (type->tkind != T_INT && type->tkind != T_FLOAT)) return NULL;
    return type;
  } else if (!strcmp(node->children[0]->name, "NOT")) {
    // Exp -> NOT
    const Type* type = Exp(node->children[1]);
    if (!type || type->tkind != T_INT) semantic_error(7, node->lineno, NULL);
    return &INT;
  } else if (node->num > 2 && !strcmp(node->children[0]->name, "ID") &&
             !strcmp(node->children[1]->name, "LP")) {
    // Exp -> ID LP RP
    // Exp -> ID LP Args RP
    char fname[MAX_NAME_LEN];
    ID(node->children[0], fname);
    Symbol* func = Query_Symtab(fname);
    if (func == NULL) {
      semantic_error(2, node->lineno, fname);
    } else if (func->skind != S_FUNCTIONNAME) {
      semantic_error(11, node->lineno, fname);
    } else if (node->num == 4) {
      // 有参数的函数，判断参数是否类型匹配
      if (Args(node->children[2], func->pfunc->params))
        semantic_error(9, node->lineno, fname);
      return func->pfunc->rtype;
    } else if (node->num == 3) {
      // 无参数的函数
      if (func->pfunc->params != NULL) semantic_error(9, node->lineno, fname);
      return func->pfunc->rtype;
    }
  } else if (node->num == 4 && !strcmp(node->children[1]->name, "LB") &&
             !strcmp(node->children[3]->name, "RB")) {
    // Exp -> Exp LB Exp RB
    const Type* t1 = Exp(node->children[0]);
    const Type* t2 = Exp(node->children[2]);

    if (!t1 || !t2) return NULL;

    if (t2->tkind != T_INT) semantic_error(12, node->lineno, NULL);
    if (t1->tkind != T_ARRAY)
      semantic_error(10, node->lineno, NULL);
    else
      return t1->array.type;
    return NULL;
  } else if (node->num == 3 && !strcmp(node->children[1]->name, "DOT")) {
    // 域变量访问
    char varname[MAX_NAME_LEN];
    const Type* type = Exp(node->children[0]);
    ID(node->children[2], varname);

    if (!type || type->tkind != T_STRUCTURE) {
      semantic_error(13, node->lineno, NULL);
      return NULL;
    } else {
      FieldList* fl = type->field;
      while (fl) {
        assert(fl->sym);
        if (!strcmp(fl->sym->sbname, varname)) break;
        fl = fl->next;
      }
      if (fl == NULL)
        semantic_error(14, node->lineno, varname);
      else {
        assert(fl->sym->skind == S_VARIABLE);
        assert(fl->sym->pvar);
        assert(fl->sym->pvar->vtype);
        return fl->sym->pvar->vtype;
      }
      return NULL;
    }
  } else if (!strcmp(node->children[0]->name, "ID") && node->num == 1) {
    // 标识符
    char sbname[MAX_NAME_LEN];
    ID(node->children[0], sbname);
    Symbol* sb = Query_Symtab(sbname);
    if (!sb || sb->skind != S_VARIABLE)
      semantic_error(1, node->lineno, node->children[0]->id_name);
    else {
      assert(sb->skind == S_VARIABLE);
      assert(sb->pvar->vtype);
      assert(sb->pvar->vtype->left_val);
      return sb->pvar->vtype;
    }
  } else if (!strcmp(node->children[0]->name, "INT")) {
    return &INT;
  } else if (!strcmp(node->children[0]->name, "FLOAT")) {
    return &FLOAT;
  } else {
    assert(0);
  }
  return NULL;
}

int Args(struct ast* node, FieldList* parameter) {
  assert(parameter->sym->skind == S_VARIABLE);
  if (node->num == 1) {
    // Args -> Exp
    // 剩余的参数多于一个
    if (parameter->next != NULL) return 1;
    // 检查最后一个参数的类型与表达式的类型是否相容
    Var* var = parameter->sym->pvar;
    const Type* type = Exp(node->children[0]);
    if (!type || !type_equal(type, var->vtype)) return 1;
    return 0;
  } else {
    // Args -> Exp COMMA Args
    // 没有剩余的参数
    if (parameter->next == NULL) return 1;
    // 检查当前参数的类型与表达式的类型是否相容
    Var* var = parameter->sym->pvar;
    const Type* type = Exp(node->children[0]);
    if (!type || !type_equal(type, var->vtype)) return 1;
    // 转移到下一个参数
    return Args(node->children[2], parameter->next);
  }
}

void ID(struct ast* node, char* ans_name) { strcpy(ans_name, node->id_name); }

void eval_semantic(struct ast* root) {
  // eval_syntax_tree(root, 0);
  Program(root);
}

int semantic_error(int error_type, int lineno, char* msg) {
  printf("Error type %d at Line %d: ", error_type, lineno);
  switch (error_type) {
    case 1:
      printf("Undefined variable: \"%s\". \n", msg);
      break;
    case 2:
      printf("Undefined function: \"%s\". \n", msg);
      break;
    case 3:
      printf("Redefined variable: \"%s\". \n", msg);
      break;
    case 4:
      printf("Redefined function: \"%s\". \n", msg);
      break;
    case 5:
      printf("Type mismatched for assignment. \n");
      break;
    case 6:
      printf("The left-hand side of an assignment must be a var. \n");
      break;
    case 7:
      printf("Type mismatched for operands. \n");
      break;
    case 8:
      printf("Type mismatched for return. \n");
      break;
    case 9:
      printf("Function \"%s\"'params are not applicable. \n", msg);
      break;
    case 10:
      printf("The variable before \"[\" and \"]\" is not an array. \n");
      break;
    case 11:
      printf("\"%s\" is not a function. \n", msg);
      break;
    case 12:
      printf("The expression between \"[\" and \"]\" is not an integer. \n");
      break;
    case 13:
      printf("Illegal use of \".\". \n");
      break;
    case 14:
      printf("There is no dominant name \"%s\". \n", msg);
      break;
    case 15:
      if (msg)
        printf("Redefined  field: \"%s\". \n", msg);
      else
        printf("Initiated field. \n");
      break;
    case 16:
      printf("Duplicated name \"%s\". \n", msg);
      break;
    case 17:
      printf("Undefined structure \"%s\". \n", msg);
      break;
    case 18:
      printf("Undefined function \"%s\". \n", msg);
      break;
    case 19:
      printf("Inconsistent declaration of function \"%s\". \n", msg);
      break;
    default:
      printf("default msg: %s. \n", msg);
      break;
  }
  return error_type;
}

static Type* LType_INT() {
  Type* type = malloc(sizeof(Type));
  type->tkind = T_INT;
  type->left_val = 1;
  // strcpy(type->tname, "int");
  return type;
}

static Type* LType_FLOAT() {
  Type* type = malloc(sizeof(Type));
  type->tkind = T_FLOAT;
  type->left_val = 1;
  // strcpy(type->tname, "float");
  return type;
}

static Type* LType_UKST() {
  Type* type = malloc(sizeof(Type));
  type->tkind = T_STRUCTURE;
  type->left_val = 1;
  // strcpy(type->tname, "uk-st");
  return type;
}