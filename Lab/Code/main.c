#include "lexical_syntax.h"
#include "semantic.h"
#include "stdio.h"

extern int yyrestart(FILE*);
extern int yyparse();

int main(int argc, char** argv) {
  if (argc <= 1) yyparse();
  FILE* fr = fopen(argv[1], "r");

  if (!fr) {
    perror(argv[1]);
    return 1;
  }

  yyrestart(fr);
  yyparse();
  /*
  if (!error_type) {
    eval_syntax_tree(root, 0);
  }
  */

  if (argc > 2) freopen(argv[2], "w", stdout);

  if (!error_type) {
    eval_semantic(root);
  }
  return 0;
}
