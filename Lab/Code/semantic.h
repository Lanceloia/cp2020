#include "lexical_syntax.h"
#include "symboltable.h"

void Program_s(struct ast* node);
void ExtDefList_s(struct ast* node);
void ExtDef_s(struct ast* node);
void Specifier_s(struct ast* node, Type* type, char* name);
void FunDec_s(struct ast* node, Type* ret_type, int how);
void CompSt_s(struct ast* node);
void DefList_s(struct ast* node);
void Def_s(struct ast* node);
void StmtList_s(struct ast* node);
void Stmt_s(struct ast* node);
void Specifier_s(struct ast* node, Type* type, char* name);
void ExtDecList_s(struct ast* node, Type* type);
void DecList_s(struct ast* node, Type* type);
void Dec_s(struct ast* node, Type* type);
void VarDec_s(struct ast* node, Type* type);
void Exp_s(struct ast* node);
void Args_s(struct ast* node);

/* 接口 */
void eval_semantic(struct ast* root);
