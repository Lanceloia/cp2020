#ifndef SYMTAB_H
#define SYMTAB_H

#include "lexical_syntax.h"

typedef struct Type Type;
typedef struct Var Var;
typedef struct FieldList FieldList;
typedef struct StructName StructName;
typedef struct FuncName FuncName;
typedef struct Symbol Symbol;
typedef struct Symtab Symtab;

#define DEBUG

/*
根据假设，函数、
符号表是多态符号的表，使用存储链表的函数栈的形式
符号表有两种形态，0和k（k>0)，k表示目前位于嵌套中的第k层
    （0）普通定义：作用域为栈帧结束
    （k）特殊定义：作用域与上一个栈帧相同

遇到FunDec的LP时：push符号表，模式不变
遇到FunDec的RP时：pop符号表，将表中符号填入FunDec的参数列表，销毁表中符号

遇到FunDec的LC时：push符号表，将FunDec的参数列表插入新符号表，模式不变
遇到CompSt的LC时：push符号表，模式不变
遇到StructSpecifier的LC时：记录上一个表的位置，create另一个符号表，模式+1

遇到FunDec的RC时：pop符号表，销毁表中符号
遇到CompSt的RC时：pop符号表，销毁表中符号
遇到StructSpecifier的RC时：回到上一个表的位置，模式-1

                        //Symtab_Init(), mode == 0
struct A{       //StructLC("A"), mode == 1, LC包含了insert功能
    int a;          //insert("a", var, int)
    struct B{   //StructLC("B"), mode == 2
        int b;      //insert("b", var, int)
    }E;               //StructRC(), mode == 1
                        //insert("B", structname)
                        //insert("E", var, "B");
    int c;          //insert("c", var, int)
} ;                     //StructRC(), mode == 0

int main(){   //pushSymtab();
    struct A s; //query("A"), ans == structname, OK
                        //buildField("s", "A"->this_symtab)
                        //insert("s", var, struct)
    s.a;              //在s的Field中查询a
    s.E.b;          //左结合，先在s的Field中查询E
                        //然后在s.E的Field中查询b
    struct B{   //StructLC("B")
        int b;      //...
        int c;      //...
    } t;               //...
}                       //popSymtab();
*/

// 外部接口
extern void Symtab_Init();    // 初始化全局表
extern void Symtab_Uninit();  // 反初始化

extern void FunDecLP();
extern void FunDecRP(Symbol* sb);
extern void FunDecDotCompSt(Symbol* sb);
extern void FunDecCompStDot();
extern void CompStLC();
extern void CompStRC();
extern void StructSpecifierLC(Symbol* sb);
extern void StructSpecifierRC();
extern Type* BuildStructure(Symtab* st, char* stname);

extern int Symtab_mode();

extern int Insert_Symtab(Symbol* sb);
extern Symbol* Query_Symtab(char* sbname);

/* 接口 */
int type_equal(const Type* t1, const Type* t2);
int fieldlist_equal(const FieldList* fl1, const FieldList* fl2);

// 变量的类型
struct Type {
  enum { T_WRONG, T_INT, T_FLOAT, T_ARRAY, T_STRUCTURE } tkind;
  union {
    struct {
      const Type* type;
      int size;  // 数组的大小
    } array;
    FieldList* field;
  };
  int left_val;
  // char tname[MAX_NAME_LEN];
};

//变量
struct Var {
  const Type* vtype;
};

// 域
struct FieldList {
  Symbol* sym;
  FieldList* next;
};

// 结构体名
struct StructName {
  // 结构体有自己的符号表
  Symtab* this_symtab;
};

// 函数名
struct FuncName {
  const Type* rtype;
  FieldList* params;
  enum { F_DECLARATION, F_DEFINITION } fdec_kind;
};

// 符号
struct Symbol {
  enum { S_UNDEFINED, S_VARIABLE, S_STRUCTNAME, S_FUNCTIONNAME } skind;
  union {
    Var* pvar;
    StructName* pstruct;
    FuncName* pfunc;
  };
  char sbname[MAX_NAME_LEN];
  int dec_lineno;
};

// 符号表
struct Symtab {
  // 这里放变量，结构体名，函数名
  // 结构体名可以延伸出横向符号表
  Symbol* syms[1024];
  // 纵向移动
  Symtab* vert_last_symtab;
  // 横向移动
  Symtab* hor_last_symtab;
  // 当前符号表的的模式
  int vert, hor;
  // 当前符号表的符号数量
  int symcnt;
};

#endif