/*
#include "symboltable.c"

int main() {
  initSymtab();
  createSymtab();
  char op[32], name[32];

  while (1) {
    switch (op[0]) {
      case 'c':  // create
        createSymtab();
        break;
      case 's':  // show
        showSymtab();
        break;
      case 'd':  // drop
        dropSymtab();
        break;
      case 'i':  // insert
        scanf("%s", name);
        insertSymtab(name, NULL);
        break;
      case 'a':  // all symtab
        global_debug();
        break;
      default:
        break;
    }
    scanf("%s", op);
  }
  return 0;
}
*/