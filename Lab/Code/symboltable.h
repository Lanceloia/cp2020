/* 接口 */
typedef struct Type Type;
typedef struct FieldList FieldList;
typedef struct TypeList TypeList;
typedef struct SymbolStk SymbolStk;
typedef struct SymtabStk SymtabStk;

extern void initSymtab();
extern void createSymtab();
extern void dropSymtab();
extern void insertSymtab(char* name, Type* type, int how);
extern void querySymtab(char* name, Type* ans_type, int* ans_how);
extern void showSymtab();
