#include "lexical_syntax.h"
#include "symtab.h"

#ifndef SEMANTIC_H
#define SEMANTIC_H

void Program(struct ast* node);
void ExtDefList(struct ast* node);
void ExtDef(struct ast* node);
void ExtDecList(struct ast* node, const Type* type);

const Type* Specifier(struct ast* node);
const Type* StructSpecifier(struct ast* node);
void OptTag(struct ast* node, char* ans_name);
void Tag(struct ast* node, char* ans_name);

Symbol* FunDec(struct ast* node);
void CompSt(struct ast* node, const Type* rtype);
void VarList(struct ast* node);
void ParamDec(struct ast* node);

void DefList(struct ast* node);
void Def(struct ast* node);
void StmtList(struct ast* node, const Type* ret_type);
void Stmt(struct ast* node, const Type* ret_type);

void DecList(struct ast* node, const Type* type);
void Dec(struct ast* node, const Type* type);
void VarDec(struct ast* node, const Type* type);
const Type* Exp(struct ast* node);
int Args(struct ast* node, FieldList* parameter);

void ID(struct ast* node, char* ans_name);

/* 接口 */
void eval_semantic(struct ast* root);

#endif