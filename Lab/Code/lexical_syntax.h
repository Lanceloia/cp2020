extern int yylineno;

extern struct ast* root;

extern int error_type;
extern struct ast* newnode(char* name, int num, ...);
extern void eval_syntax_tree();