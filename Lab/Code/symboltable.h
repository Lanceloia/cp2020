#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

typedef struct Type Type;
typedef struct FieldList FieldList;
typedef struct Structure Structure;
typedef struct Function Function;
typedef struct Symbol Symbol;

typedef struct SymbolBucket SymbolBucket;
typedef struct SymtabStk SymtabStk;

extern void initSymtab();
extern void createSymtab();
extern void dropSymtab();
extern int insertSymtab(Symbol* sb);
extern void querySymtab(char* name, Symbol* ans_sb);
extern void buildFieldListFromSymtab(Structure* structure);
extern void dropFieldListAfterPush(FieldList* fl);
extern void showSymtab();

/* 接口 */
int type_equal(const Type* t1, const Type* t2);
int symbol_kind_equal(const Symbol* sb1, const Symbol* sb2);

/*
void build_FieldList(FieldList* fl, char* name, Type* type);
void build_Type_INT(Type* type, int val);
void build_Type_FLOAT(Type* type, float val);
void build_Type_ARRAY(Type* type, Type* array_type, int array_size);
void build_Type_STRUCTURE(Type* type, FieldList* structure);
*/

#define MAX_NAME_LEN 32

#define FUCK(n) printf("Debug[%d]\n", n);

// 变量的类型
struct Type {
  enum { _UNKNOWN, _INT, _FLOAT, _ARRAY, _STRUCTURE } kind;
  union {
    struct {
      const Type* type;
      // int size;
    } array;
    struct {
      FieldList* field;
    } structure;
  };
};

// 结构体名的类型
struct Structure {
  FieldList* field;
};

// 函数名的类型
struct Function {
  Type ret_type;
  FieldList* parameter;
};

//  符号的类型
struct Symbol {
  enum { _VARIABLE, _STRUCT_NAME, _FUNCTION_NAME } kind;
  union {
    struct {
      Type type;
    } variable;
    Structure structure;
    Function function;
  };
  char symbolname[MAX_NAME_LEN];
  enum { UNDEFINE, DECLARATION, DEFINITION } dec;
  // 在symtab_g中，nextsymbol指上一个同名的符号，其作用域被该符号覆盖
  // 在symtab_l中，nextsymbol指下一个不同名的符号，其作用域与该符号相同
  Symbol* nextsymbol;
};

// 域
struct FieldList {
  char varname[MAX_NAME_LEN];
  Type vartype;
  FieldList* next;
};

#endif