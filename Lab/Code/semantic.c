#include "semantic.h"

#include "lexical_syntax.h"
#include "stdio.h"
#include "string.h"
#include "symboltable.h"

struct ast {
  int line, num;
  char* name;
  struct ast* children[8];
  union {
    char id_name[32];
    int int_value;
    float float_value;
  };
};

#define MAX_NAME_LEN 32

#define DEFINITION 1
#define DECLARATION 2

#define child(x) node->children[x]

int type_equal(Type* type1, Type* type2) { return 1; }

void eval_semantic(struct ast* root) {
  eval_syntax_tree(root, 0);
  Program_s(root);
}

void Program_s(struct ast* node) {
  initSymtab();
  createSymtab();
  ExtDefList_s(child(0));
  dropSymtab();
}

void ExtDefList_s(struct ast* node) {
  if (node->num == -1) return;
  ExtDef_s(child(0));
  ExtDefList_s(child(1));
}

void ExtDef_s(struct ast* node) {
  Type* type;                                   // INT, FLOAT, ARRAY...
  char name[MAX_NAME_LEN];                      // struct a, struct t...
  Specifier_s(child(0), type, name);            // type, name = Specifier.val
  if (!strcmp(child(1)->name, "ExtDecList")) {  // 定义：int a,b,c
    ExtDecList_s(child(1), type);
  }
  if (!strcmp(child(1)->name, "SEMI")) {  // 声明：struct a;
    Type* ans_type;
    int ans_how;
    querySymtab(name, ans_type, &ans_how);
    if (!type_equal(type, ans_type))
      printf("冲突的声明\n");
    else
      printf("正确的声明\n");
  }
  if (!strcmp(child(1)->name, "FunDec")) {  // 定义：int f(...){...}
    if (!strcmp(child(2)->name, "CompSt")) {
      // FunDec
      FunDec_s(child(1), type, DEFINITION);
      // CompSt
      CompSt_s(child(2));
    } else if (!strcmp(child(2)->name, "SEMI")) {
      // FunDec
      FunDec_s(child(1), type, DECLARATION);
      // SEMI
    }
  }
}

void ExtDecList_s(struct ast* node, Type* type) {
  if (node->num == 1) {
    // VarDec
    VarDec_s(child(0), type);
  } else if (node->num == 3) {
    // VarDec
    VarDec_s(child(0), type);
    // COMMA
    // ExtDecList
    ExtDecList_s(child(2), type);
  }
}

void FunDec_s(struct ast* node, Type* ret_type, int how) {
  // ID
  struct ast* id = child(0);
  insertSymtab(id->id_name, ret_type, how);
  // LP
  // VarList
  // RP
}

void CompSt_s(struct ast* node) {
  // LC
  createSymtab();
  // DefList
  DefList_s(child(1));
  // StmtList
  StmtList_s(child(2));
  // RC
  dropSymtab();
}

void DefList_s(struct ast* node) {
  printf("FCK, %s, %d\n", node->name, node->num);
  if (node->num == -1) return;
  Def_s(child(0));
  DefList_s(child(1));
}

void Def_s(struct ast* node) {
  // Specifier
  Type* type;
  char name[32];
  Specifier_s(child(0), type, name);
  // DecList
  DecList_s(child(1), type);
  // SEMI
}

void StmtList_s(struct ast* node) {
  // printf("StmtList\n");
  if (node->num == -1) return;
  // Stmt
  Stmt_s(child(0));
  // StmtList
  StmtList_s(child(1));
}

void Stmt_s(struct ast* node) {
  // printf("Stmt\n");
  if (!strcmp(child(0)->name, "Exp")) {
    // Exp
    Exp_s(child(0));
    // SEMI
  } else if (!strcmp(child(0)->name, "RETURN")) {
    // RETURN
    // Exp
    Exp_s(child(1));
    // SEMI
  } else if (!strcmp(child(0)->name, "IF")) {
    if (node->num == 5) {
      // IF
      // LP
      // Exp
      Exp_s(child(2));
      // RP
      // Stmt
      Stmt_s(child(4));
    } else if (node->num == 7) {
      // IF
      // LP
      // Exp
      Exp_s(child(2));
      // RP
      // Stmt
      Stmt_s(child(4));
      // ELSE
      // Stmt
      Stmt_s(child(6));
    }
  } else if (!strcmp(child(0)->name, "WHILE")) {
    // WHILE
    // LP
    // Exp
    Exp_s(child(2));
    // RP
    // Stmt
    Stmt_s(child(4));
  }
}

void Specifier_s(struct ast* node, Type* type, char* name) { type = NULL; }

void DecList_s(struct ast* node, Type* type) {
  if (node->num == 1) {
    // Dec
    Dec_s(child(0), type);
  } else if (node->num == 3) {
    // Dec
    Dec_s(child(0), type);
    // COMMA
    // DecList
    DecList_s(child(2), type);
  }
}

void Dec_s(struct ast* node, Type* type) {
  // VarDec
  VarDec_s(child(0), type);
}

void VarDec_s(struct ast* node, Type* type) {
  // ID
  // printf("VarDec\n");
  if (!strcmp(child(0)->name, "ID")) {
    struct ast* id = child(0);
    insertSymtab(id->id_name, type, DEFINITION);
  }
}

void Exp_s(struct ast* node) {
  // printf("Exp\n");
  if (node->num == 3 && !strcmp(child(0)->name, "Exp") &&
      !strcmp(child(2)->name, "Exp")) {
    // Exp
    Exp_s(child(0));
    // op
    // Exp
    Exp_s(child(2));
  } else if (!strcmp(child(0)->name, "LP")) {
    // LP
    // Exp
    Exp_s(child(1));
    // RP
  } else if (!strcmp(child(0)->name, "MINUS")) {
    // UMINUS
    // Exp
    Exp_s(child(1));
  } else if (!strcmp(child(0)->name, "NOT")) {
    // NOT
    // Exp
    Exp_s(child(1));
  } else if (node->num > 2 && !strcmp(child(0)->name, "ID") &&
             !strcmp(child(1)->name, "LP")) {
    // ID (function)
    Type* ans_type;
    int ans_how;
    querySymtab(child(0)->id_name, ans_type, &ans_how);
    if (ans_how == 0)  // how == WRONG 或 type != function(void)
      printf("标识符\"%s\"并非函数\n", child(0)->id_name);
    if (node->num == 4) {
      // LP
      // Args
      Args_s(child(2));
      // RP
    } else if (node->num == 3) {
      // LP
      // RP
    }
  } else if (node->num == 3 && !strcmp(child(1)->name, "LB") &&
             !strcmp(child(3)->name, "RB")) {
    // Exp
    Exp_s(child(0));
    // LB
    Exp_s(child(2));
    // RB
  } else if (node->num == 3 && !strcmp(child(1)->name, "DOT")) {
    // Exp
    Exp_s(child(0));
    // DOT
    // ID
    // 此处ID为结构体内域名
  } else if (!strcmp(child(0)->name, "ID") && node->num == 1) {
    Type* ans_type;
    int ans_how;
    // printf("ID\n");
    querySymtab(child(0)->id_name, ans_type, &ans_how);
    if (ans_how == 0)  // how == WRONG
      printf("未定义的标识符: %s\n", child(0)->id_name);
  } else if (!strcmp(child(0)->name, "INT")) {
    // INT
  } else if (!strcmp(child(0)->name, "FLOAT")) {
    // FLOAT
  }
}

void Args_s(struct ast* node) {
  if (node->num == 1) {
    // Exp
    Exp_s(child(0));
  } else if (node->num == 3) {
    // Exp
    Exp_s(child(0));
    // COMMA
    // Args
    Args_s(child(2));
  }
}