#include "semantic.h"

#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

typedef struct Type Type;
typedef struct Symbol Symbol;

// 抽象语法树
struct ast {
  int line, num;
  char* name;
  struct ast* children[8];
  union {
    char id_name[MAX_NAME_LEN];
    int int_value;
    float float_value;
  };
};

void semantic_error(int error_type, int lineno, char* msg);

void Program(struct ast* node) {
  // 初始化全局符号表
  initSymtab();
  // 创建最底层的局部符号表（作用域也为全局）
  // 遇到结构体/函数的声明则将其记录
  // 遇到结构体/函数的定义则将原有声明覆盖
  // 变量只能定义不能声明，重复的全局变量均视为错误
  createSymtab();
  // 开始进行文法分析
  ExtDefList(node->children[0]);
  // 删除最底层的局部符号表
  // 依次检查各个符号，如果某个符号只有声明没有定义则视为错误
  dropSymtab();
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
  Type type;
  // Specifier识别出type的内容
  Specifier(node->children[0], &type);
  if (!strcmp(node->children[1]->name, "ExtDecList")) {
    // ExtDef -> Specifier ExtDecList SEMI
    // 全局变量的定义
    ExtDecList(node->children[1], &type);
  }
  // ExtDef -> Specifier SEMI
  // 结构体类型的的定义/声明
  else if (!strcmp(node->children[1]->name, "SEMI")) {
    // 检查结构体是否存在冲突定义/声明
    // 这个specifier可能会变成int？我不知道哦
    assert(type.kind == _STRUCTURE);
    // puts("Ext -> Specifier SEMI\n");
  } else {
    // ExtDef -> Specifier FunDec SEMI
    if (!strcmp(node->children[2]->name, "SEMI")) {
      // type为函数的返回值，此处为函数的声明
      // 函数的声明要求不与定义和原有声明冲突
      FunDec(node->children[1], &type, DECLARATION);
    }
    // ExtDef -> Specifier FunDec CompSt
    else {
      // 此处为函数的定义
      // 函数的只能有一个定义，不能和原有声明冲突
      FunDec(node->children[1], &type, DEFINITION);
      CompSt(node->children[2]);
    }
    // 删除局部符号表（见FunDec处）
    dropSymtab();
  }
}

void ExtDecList(struct ast* node, const Type* type) {
  if (node->num == 1) {
    // ExtDecList -> VarDec
    // 通过继承属性type来形成变量的定义
    VarDec(node->children[0], type, DEFINITION);
  } else {
    // ExtDecList -> VarDec COMMA ExtDecList
    VarDec(node->children[0], type, DEFINITION);
    ExtDecList(node->children[2], type);
  }
}

void Specifier(struct ast* node, Type* type) {
  if (!strcmp(node->children[0]->name, "TYPE")) {
    // Specifier -> TYPE
    if (!strcmp(node->children[0]->id_name, "int")) {
      type->kind = _INT;
    } else {
      type->kind = _FLOAT;
    }
  } else if (!strcmp(node->children[0]->name, "StructSpecifier")) {
    // Specifier -> StructSpecifier
    // 转交给StructSpecifier来填充type
    type->kind = _STRUCTURE;
    StructSpecifier(node->children[0], type);
  }
  // 完成了对type的填写
}

void StructSpecifier(struct ast* node, Type* type) {
  // sb的kind是variable，对应的type.kind是structure
  assert(type->kind == _STRUCTURE);
  Symbol ss;  // StructSpecifier
  if (!strcmp(node->children[1]->name, "OptTag")) {
    // StructSpecifier -> STRUCT OptTag LC DefList RC
    // OptTag过程确定structure A{int var;}中的A，A可以是匿名
    // 注意要在OptTag处记录这个structure的name
    // 建立起structure.name->fieldlist的映射关系
    OptTag(node->children[1], &ss);
    //此处类型为结构体类型
    // 将structure的kind设置为struct_name
    ss.kind = _STRUCT_NAME;
    // 有LC和RC的StructSpecifier是definition
    ss.dec = DEFINITION;
    // LC: 创建新的局部符号表，用于定义域变量
    createSymtab();
    // DefList将括号中的内容提取到最新的局部符号表
    DefList(node->children[3]);
    // 将局部符号表的符号填入sb中
    // 填充sb->structure.fieldlist
    ss.structure.field = NULL;
    buildFieldListFromSymtab(&ss.structure);
    // RC: 删除刚才创建的局部符号表
    dropSymtab();
    // 记录下新定义的struct A
    int ret = insertSymtab(&ss);
    RET_INSERTSYMTAB_CHECK(ss);
    type->structure.field = ss.structure.field;
  } else {
    // StructSpecifier -> STRUCT Tag
    // 使用之前在OptTag中创建的非匿名结构体
    // 曾经建立了struct.name->fieldlist的映射关系
    // 使用Tag过程找到OptTag定义的fieldlist，将其载入type
    Tag(node->children[1], &ss);
    // 检查是否有ss.symbolname对应的fieldlist
    // 如果有，将其填入type中
    // 如果没有，报错：结构体struct.name未定义
    Symbol ans_sb;
    querySymtab(ss.symbolname, &ans_sb);
    if (ans_sb.dec == UNDEFINE) {
      semantic_error(17, node->line, ss.symbolname);
      type->structure.field = NULL;
    } else
      type->structure.field = ans_sb.structure.field;
  }
}

void OptTag(struct ast* node, Symbol* sb) {
  // 功能在上文中提及了
  // OptTag只能变为终结符ID或者空串
  /* 这是为了支持struct a和a不重名的代码
  char name[MAX_NAME_LEN] = "struct ";
  if (node->num != -1)
    // 变为ID的情形下
    ID(node->children[0], sb);
  else
    // 变为空串的情形下
    strcpy(sb->symbolname, "unname struct ");
  strcat(name, sb->symbolname);
  strcpy(sb->symbolname, name);
  */
  static int unname_cnt = 0;
  if (node->num != -1)
    // 变为ID的情形下
    ID(node->children[0], sb);
  else
    // 变为空串的情形下
    sprintf(sb->symbolname, "unname(%d)", unname_cnt++);
}

void Tag(struct ast* node, Symbol* sb) {
  // 功能在上文中提及了
  // Tag只能变为终结符ID
  ID(node->children[0], sb);
}

void FunDec(struct ast* node, const Type* ret_type, const int dec) {
  // ret_type是由上文的Specifier填写的返回值类型
  Symbol sb;
  sb.kind = _FUNCTION_NAME;
  sb.dec = dec;
  sb.function.ret_type = *ret_type;
  // sb->symbolname由ID填写
  ID(node->children[0], &sb);
  //（SEMI结束为声明，CompSt结束为定义
  //函数的参数的作用域为局部，创建新的符号表

  /* 因为没能解决funcname和parameter的作用域问题 先吧insert放前面 */
  int ret = insertSymtab(&sb);
  RET_INSERTSYMTAB_CHECK(sb);
  createSymtab();
  if (node->num == 4) {
    // FunDec -> ID LP VarList RP
    // sb->function.parameter由VarList填写
    // VarList(node->children[2], sb);
    assert(0);
  } else {
    // FunDec -> ID LP RP
    sb.function.parameter = NULL;
  }

  /*
  // 构造完sb，将其作为函数名记录在符号表
  printf("funcname: %s\n", sb.symbolname);
  insertSymtab(&sb);
  */

  // Q群中提及不考虑函数内部局部变量与函数重名的情况
  // 即不考虑“int fun(){ int fun; }”的情况
}

void CompSt(struct ast* node) {
  // CompSt -> LC DefList StmtList RC
  // LC: 局部作用域开始F
  createSymtab();
  // DefList: 局部变量定义语句
  DefList(node->children[1]);
  // StmtList: 陈述句
  StmtList(node->children[2]);
  // RC: 局部作用域结束
  dropSymtab();
}

void DefList(struct ast* node) {
  if (node->num == -1)
    // DefList -> empty
    return;
  // DefList -> Def DefList
  // 递归地调用自身，逐句处理局部变量的定义语句
  Def(node->children[0]);
  DefList(node->children[1]);
}

void Def(struct ast* node) {
  // Def -> Specifier DecList SEMI
  Type type;
  // Specifier再次出场，填写type
  Specifier(node->children[0], &type);
  // 强声明（definition）
  // DecList使用sb作为继承属性，完成局部变量的定义
  DecList(node->children[1], &type, DEFINITION);
  // SEMI
}

void DecList(struct ast* node, const Type* type, const int dec) {
  if (node->num == 1) {
    // DecList -> Dec
    // 一次只定义一个类型为type的变量
    Dec(node->children[0], type, dec);
  } else {
    // DecList -> Dec COMMA DecList
    // 递归调用自己，完成处理
    Dec(node->children[0], type, dec);
    DecList(node->children[2], type, dec);
  }
}

void Dec(struct ast* node, const Type* type, const int dec) {
  if (node->num == 1)
    //  Dec -> VarDec
    VarDec(node->children[0], type, dec);
  else {
    // Dec -> VarDec ASSIGNOP Exp
    VarDec(node->children[0], type, dec);
    Type exp_type;
    // Exp过程计算出表达式的类型，填写在exp_type中
    Exp(node->children[2], &exp_type);
    // 判定二者类型是否相容
    if (!type_equal(type, &exp_type))
      // 报错
      assert(0);
  }
}

void VarDec(struct ast* node, const Type* type, const int dec) {
  if (node->num == 1) {
    Symbol symbol;
    // VarDec -> ID
    symbol.kind = _VARIABLE;
    symbol.variable.type = *type;
    symbol.dec = dec;
    ID(node->children[0], &symbol);
    assert(symbol.dec != UNDEFINE);
    int ret = insertSymtab(&symbol);
    RET_INSERTSYMTAB_CHECK(symbol);
  } else {
    // VarDec -> VarDec LB INT RB
    int size = node->children[2]->int_value;
    // 将symbol改写成数组的形式
    Type array_type;
    array_type.kind = _ARRAY;
    array_type.array.type = type;
    /// array_type.array.size = size;
    // 递归调用自己以创建高维数组
    VarDec(node->children[0], &array_type, dec);
  }
}

void StmtList(struct ast* node) {
  if (node->num == -1)
    // StmtList -> empty
    return;
  // StmtList -> Stmt StmtList
  Stmt(node->children[0]);
  StmtList(node->children[1]);
}

void Stmt(struct ast* node) {
  // puts("Stmt");
  Type exp_type;
  if (!strcmp(node->children[0]->name, "Exp")) {
    // Stmt -> Exp SEMI
    Exp(node->children[0], &exp_type);
  } else if (!strcmp(node->children[0]->name, "RETURN")) {
    // Stmt -> RETURN Exp SEMI
    // 这里需要判断RETURN的类型和函数是否相容
    Exp(node->children[1], &exp_type);
  } else if (!strcmp(node->children[0]->name, "IF")) {
    if (node->num == 5) {
      // Stmt -> IF LP Exp RP Stmt
      // 这里需要检查Exp的类型是否为int型
      Exp(node->children[2], &exp_type);
      Stmt(node->children[4]);
    } else if (node->num == 7) {
      // Stmt -> IF LP Exp RP Stmt ELSE Stmt
      // 这里需要检查Exp的类型是否为int型
      Exp(node->children[2], &exp_type);
      Stmt(node->children[4]);
      Stmt(node->children[6]);
    }
  } else if (!strcmp(node->children[0]->name, "WHILE")) {
    // Stmt -> WHILE LP Exp RP Stmt
    // 这里需要检查Exp的类型是否为int型
    Exp(node->children[2], &exp_type);
    Stmt(node->children[4]);
  }
}

int Exp(struct ast* node, Type* left_type) {
  // val_type是需要填写的
  // 一堆算式，检查类型是否为可以运算的类型
  // 是否同为int或者同为float相等，数组中间的表达式需要为int
  // 函数调用要求ID是函数名
  // 函数调用要求参数数量与等等
  // 返回值表示Exp能否为左值
  // 表达式的类型等于最左子表达式的类型
  // puts("Exp");

  if (node->num == 3 && !strcmp(node->children[0]->name, "Exp") &&
      !strcmp(node->children[2]->name, "Exp")) {
    Type right_type;
    int left_ret;
    left_ret = Exp(node->children[0], left_type);
    Exp(node->children[2], &right_type);
    // 当左值的类型出错时，（可以）认为左值应该与右值类型相同
    // if (left_type->kind == _UNKNOWN) left_type->kind = right_type.kind;
    // 当左右的类型不相等时，报错
    if (!type_equal(left_type, &right_type))
      semantic_error(5, node->line, NULL);

    // 赋值运算符的左边是不是左值，报错
    if (strcmp(node->children[1]->name, "ASSIGNOP"))
      if (!left_ret) semantic_error(6, node->line, NULL);

    // 整条语句并非左值
    return 0;
  } else if (node->num == 2 && (!strcmp(node->children[0]->name, "LP") ||
                                !strcmp(node->children[0]->name, "MINUS") ||
                                !strcmp(node->children[0]->name, "NOT"))) {
    Exp(node->children[1], left_type);
    // 并非左值
    return 0;
  } else if (node->num > 2 && !strcmp(node->children[0]->name, "ID") &&
             !strcmp(node->children[1]->name, "LP")) {
    // I函数调用
    Symbol ans_sb;
    // 以后可以改为ID
    querySymtab(node->children[0]->id_name, &ans_sb);
    if (ans_sb.dec == UNDEFINE)
      semantic_error(2, node->line, node->children[0]->id_name);
    else if (ans_sb.kind != _FUNCTION_NAME)
      semantic_error(11, node->line, node->children[0]->id_name);
    else {
      if (node->num == 4) {
        // 有参数的函数，判断参数是否类型匹配
        Args(node->children[2], ans_sb.function.parameter);
      } else if (node->num == 3) {
        // 无参数的函数
        assert(ans_sb.function.parameter == NULL);
      }
      // 并非左值
      return 0;
    }
  } else if (node->num == 3 && !strcmp(node->children[1]->name, "LB") &&
             !strcmp(node->children[3]->name, "RB")) {
    // Exp -> Exp LB Exp RB
    assert(0);
    Type *base_type = malloc(sizeof(Type)), integer_type;
    Exp(node->children[0], base_type);
    Exp(node->children[2], &integer_type);
    if (integer_type.kind != _INT) semantic_error(10, node->line, NULL);
    left_type->kind = _ARRAY;
    left_type->array.type = base_type;

    // 左值
    return 1;
  } else if (node->num == 3 && !strcmp(node->children[1]->name, "DOT")) {
    assert(0);
    return 1;
  } else if (!strcmp(node->children[0]->name, "ID") && node->num == 1) {
    Symbol ans_sb;
    querySymtab(node->children[0]->id_name, &ans_sb);
    if (ans_sb.dec == UNDEFINE)
      semantic_error(1, node->line, node->children[0]->id_name);
    else {
      if (ans_sb.kind == _VARIABLE) {
        // printf("%s", ans_sb.symbolname);
        *left_type = ans_sb.variable.type;
      } else
        assert(0);
    }
    // 未定义的标示符也当作标示符，视为左值
    return 1;
  } else if (!strcmp(node->children[0]->name, "INT")) {
    left_type->kind = _INT;
    return 0;
  } else if (!strcmp(node->children[0]->name, "FLOAT")) {
    left_type->kind = _FLOAT;
    return 0;
  } else {
    assert(0);
  }
}

void Args(struct ast* node, FieldList* parameter) {
  if (node->num == 1) {
    // Args -> Exp
    Type exp_type;
    Exp(node->children[0], &exp_type);
    // 检查最后一个参数的类型与表达式的类型是否相容
    if (!type_equal(&exp_type, &parameter->vartype)) assert(0);
  } else {
    // Args -> Exp COMMA Args
    Type exp_type;
    Exp(node->children[0], &exp_type);
    // 检查当前参数的类型与表达式的类型是否相容
    if (!type_equal(&exp_type, &parameter->vartype)) assert(0);
    // 转移到下一个参数
    Args(node->children[2], parameter->next);
  }
}

void ID(struct ast* node, Symbol* sb) { strcpy(sb->symbolname, node->id_name); }

void eval_semantic(struct ast* root) {
  eval_syntax_tree(root, 0);
  Program(root);
}

void semantic_error(int error_type, int lineno, char* msg) {
  printf("Error type %d at Line %d: ", error_type, lineno);
  switch (error_type) {
    case 1:
      printf("Undefinition variable: %s. \n", msg);
      break;
    case 2:
      printf("Undefinition function: %s. \n", msg);
      break;
    case 3:
      printf("Redefinition variable: %s \n", msg);
      break;
    case 4:
      printf("Redefinition function: %s. \n", msg);
      break;
    case 5:
      printf("Type error near the '='.  \n");
      break;
    case 6:
      printf("The left side of '=' is a RIGHT exp. \n");
      break;
    case 7:
      printf("Type error in this exp. \n");
      break;
    case 8:
      printf("Return type error. \n");
      break;
    case 9:
      printf("The amount of parameter, or paramter's type is wrong. \n");
      break;
    case 10:
      printf("Use [] with non-array variable. \n");
      break;
    case 11:
      printf("Use () with non-function variable. \n");
      break;
    case 12:
      printf("FLOAT between '[' and ']' . \n");
      break;
    case 13:
      printf("Use . with non-structure variable. \n");
      break;
    case 14:
      printf("There is no dominant name %s. \n", msg);
      break;
    case 15:
      printf("Redefinition dominant name, or initiate in the structure: %s. \n",
             msg);
      break;
    case 16:
      printf("Structure name %s is already exists. \n", msg);
      break;
    case 17:
      printf("Undefinition structure %s. \n", msg);
      break;
    case 18:
      printf("There is no definition of function %s. \n", msg);
      break;
    case 19:
      printf("Conflict definition of function %s. \n", msg);
      break;
    default:
      printf("default msg: %s. \n", msg);
      break;
  }
}
