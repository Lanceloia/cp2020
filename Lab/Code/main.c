#include "lex.yy.c"
#include "stdarg.h"
#include "stdio.h"

int main(int argc, char** argv) {
  if (argc <= 1) yyparse();
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 1;
  }

  yyrestart(f);
  yyparse();
  return 0;
}

void yyerror(char* msg) { printf("At Line %d: %s\n", yylineno, msg); }

extern int yylineno;

struct ast {
  int line;
  char* name;
  int num;
  struct ast* children[8];
} * root;

struct ast* newnode(char* name, int num, ...) {
  // printf("%s\n", name);
  struct ast* node = malloc(sizeof(struct ast));

  node->name = name;
  node->num = num;

  if (num > 0) {
    // Nonterminal
    va_list v;
    va_start(v, num);
    for (int i = 0; i < num; i++) {
      struct ast* temp = va_arg(v, struct ast*);
      node->children[i] = temp;
    }
    node->line = node->children[0]->line;
    va_end(v);
  } else {
    // Terminal or Empty
    node->line = yylineno;
  }
  return node;
}

void eval(struct ast* node, int level) {
  if (node->num >= 0)  // Nonempty
    for (int i = 0; i < level; i++) printf("---");
  if (node->num > 0) {
    // Nonterminal
    printf("%s(%d)\n", node->name, node->line);
    for (int i = 0; i < node->num; i++) {
      eval(node->children[i], level + 1);
    }
  } else if (node->num == 0) {
    // Terminal
    printf("%s\n", node->name);
  } else {
    // Empty
  }
}