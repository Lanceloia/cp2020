#include "lexical_syntax.h"
#include "symboltable.h"

#ifndef SEMANTIC_H
#define SEMANTIC_H

void Program(struct ast* node);
void ExtDefList(struct ast* node);
void ExtDef(struct ast* node);
void ExtDecList(struct ast* node, const Type* type);

void Specifier(struct ast* node, Type* type);
void StructSpecifier(struct ast* node, Type* type);
void OptTag(struct ast* node, Symbol* sb);
void Tag(struct ast* node, Symbol* sb);

void FunDec(struct ast* node, const Type* ret_type, const int dec);
void CompSt(struct ast* node);

void DefList(struct ast* node);
void Def(struct ast* node);
void StmtList(struct ast* node);
void Stmt(struct ast* node);

void DecList(struct ast* node, const Type* type, const int dec);
void Dec(struct ast* node, const Type* type, const int dec);
void VarDec(struct ast* node, const Type* type, const int dec);
int Exp(struct ast* node, Type* val_type);
void Args(struct ast* node, FieldList* parameter);

void ID(struct ast* node, Symbol* sb);

/* 接口 */
void eval_semantic(struct ast* root);

#define RET_INSERTSYMTAB_CHECK(symbol)                     \
  switch (ret) {                                           \
    case 0:                                                \
      break;                                               \
    case 1:                                                \
      if (symbol.kind == _VARIABLE)                        \
        semantic_error(3, node->line, symbol.symbolname);  \
      else if (symbol.kind == _FUNCTION_NAME)              \
        semantic_error(4, node->line, symbol.symbolname);  \
      else if (symbol.kind == _STRUCT_NAME)                \
        semantic_error(16, node->line, symbol.symbolname); \
      else                                                 \
        assert(0);                                         \
      break;                                               \
    default:                                               \
      assert(0);                                           \
      break;                                               \
  }

#endif