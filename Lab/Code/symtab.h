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
（一）符号表概述：

符号表为多态的表
-- 根据skind确定具体的符号类型
有两个坐标
-- 局部作用域的变化引起竖直方向(v)变化
-- 结构体的定义过程引起水平方向(h)变化

遇到FunDec的LP时：FunDecLP()
-- v + 1
遇到FunDec的RP时：FunDecRP()
-- v - 1, 将表中符号填入FunDec的参数列表，销毁表中符号

遇到FunDec的CompSt的LC时：FunDecDotCompSt()
-- v + 1, VarList插入新符号表
遇到FunDec的CompSt的RC时：FunDecCompStDot()
-- v - 1, 销毁表中符号

遇到StructSpecifier的LC时：StructSpecifierLC()
-- h + 1
遇到StructSpecifier的RC时：StructSpecifierRC()
-- h - 1, 不能销毁表中的符号

遇到普通CompSt的LC时：DotComSt()
-- v + 1, 销毁表中符号
遇到普通CompSt的RC时：CompStDot()
-- v - 1, 销毁表中符号

（二）定义新符号时：

--- 新符号是变量
------ 查看局部作用域（同坐标的表）中是否有重定义的符号
--------- 有：重定义错误3
--------- 无： 查看全局作用域中是否有重定义的符号
------------ 有且不是变量：重定义错误3
--- 新符号是结构体名
------ 查看全局作用域中是否有重定义的符号
--------- 有：重定义错误16
--- 新符号是函数名
------ 查看全局作用域中是否有重定义的符号
--------- 有：assert(该符号也是函数名)
------------ 二者都是函数名定义：重定义错误4
------------ 二者至少有一是声明：检查冲突
--------------- 冲突：声明冲突错误19
--------------- 不冲突：若新符号是定义，则覆盖旧符号

（三）使用某符号时：

--- 查看全局作用域中是否有该符号
------ 有：检查类型是否与用途一致
------ 无：未定义错误
*/

// 外部接口
extern void Symtab_Init();    // 初始化全局表
extern void Symtab_Uninit();  // 反初始化

extern void FunDecLP();
extern void FunDecRP(Symbol* sb);
extern void FunDecDotCompSt(Symbol* sb);
extern void FunDecCompStDot();
extern void DotCompSt();
extern void CompStDot();
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