/* 接口 */
typedef struct Type Type;
typedef struct Function Function;
typedef struct FieldList FieldList;
typedef struct SymbolStkNode SymbolStkNode;
typedef struct SymbolBucket SymbolBucket;
typedef struct SymtabStk SymtabStk;

extern void initSymtab();
extern void createSymtab();
extern void dropSymtab();
extern void insertSymtab(char* name, Type* type, int how);
extern void querySymtab(char* name, Type* ans_type, int* ans_how);
extern void showSymtab();
