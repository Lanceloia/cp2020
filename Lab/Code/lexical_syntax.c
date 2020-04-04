#include "lexical_syntax.h"

#include "lex.yy.c"
#include "stdarg.h"

int error_type = 0;

struct ast {
  int line, num;
  char* name;
  struct ast* children[8];
  union {
    char id_name[32];
    int int_value;
    float float_value;
  };
} * root;

void yyerror(char* msg) {
  switch (error_type) {
    case 1:
      printf("Error type A at Line %d: Mysterious character \'%s\'.\n",
             yylineno, msg);
      break;
    case 2:
      printf("Error type A at Line %d: Invalid ID \'%s\'.\n", yylineno, msg);
      break;
    default:
      printf("Error type B at Line %d: %s.\n", yylineno, msg);
      break;
  }

  // default
  error_type = -1;
}

struct ast* newnode(char* name, int num, ...) {
  // printf("%s\n", name);
  struct ast* node = malloc(sizeof(struct ast));

  node->name = name;
  node->num = num;

  if (strcmp(name, "ID") == 0 || strcmp(name, "TYPE") == 0)
    strcpy(node->id_name, yytext);
  else if (strcmp(name, "INT") == 0) {
    node->int_value = strtol(yytext, NULL, 0);
  } else if (strcmp(name, "FLOAT") == 0)
    node->float_value = atof(yytext);

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

void eval_syntax_tree(struct ast* node, int level) {
  if (node->num >= 0)  // Nonempty
    for (int i = 0; i < level; i++) printf("  ");
  if (node->num > 0) {
    // Nonterminal
    printf("%s (%d)\n", node->name, node->line);
    for (int i = 0; i < node->num; i++) {
      eval_syntax_tree(node->children[i], level + 1);
    }
  } else if (node->num == 0) {
    // Terminal
    if (strcmp(node->name, "ID") == 0 || strcmp(node->name, "TYPE") == 0)
      printf("%s: %s\n", node->name, node->id_name);
    else if (strcmp(node->name, "INT") == 0)
      printf("%s: %d\n", node->name, node->int_value);
    else if (strcmp(node->name, "FLOAT") == 0)
      printf("%s: %f\n", node->name, node->float_value);
    else
      printf("%s\n", node->name);
  } else {
    // Empty node->num == -1
  }
}