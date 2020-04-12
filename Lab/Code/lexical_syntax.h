#ifndef LEXICAL_SYNTAX_H
#define LEXICAL_SYNTAX_H

#define MAX_NAME_LEN 32
#define d(n) printf("Debug[%d]\n", n);

extern int yylineno;

extern struct ast* root;

extern int error_type;
extern struct ast* newnode(char* name, int num, ...);
extern void eval_syntax_tree();

// 抽象语法树
struct ast {
  int lineno, num;
  char* name;
  struct ast* children[8];
  union {
    char id_name[MAX_NAME_LEN];
    int int_value;
    float float_value;
  };
};

#endif