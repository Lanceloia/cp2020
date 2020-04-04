#include "lexical_syntax.h"
#include "semantic.h"
#include "stdio.h"

extern int yyrestart(FILE*);
extern int yyparse();

int main(int argc, char** argv) {
  if (argc <= 1) yyparse();
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 1;
  }

  yyrestart(f);
  yyparse();
  /*
  if (!error_type) {
    eval_syntax_tree(root, 0);
  }
  */
  if (!error_type) {
    eval_semantic(root);
  }
  return 0;
}
