# CP2020：C--编译器前端与中间代码生成

南京大学编译原理课程项目，使用**C、Flex和Bison**，把C--子集源码转成语法树、语义诊断和三地址式中间代码。
Lanceloia在2020年3月至5月完成课程实现，仓库保留从前端到Lab3的迭代；2026年补充中文阅读导航。

## 编译路径

```text
C--源码
  → Flex词法分析
  → Bison语法分析／AST
  → 符号表、作用域、类型与语义检查
  → 三地址式IR
```

| 看什么 | 入口 | 核心问题 |
| --- | --- | --- |
| 词法 | [lexical.l](Lab/Code/lexical.l) | 标识符、数值与符号怎样成为token？ |
| 语法／AST | [syntax.y](Lab/Code/syntax.y)、[lexical_syntax.c](Lab/Code/lexical_syntax.c) | 文法规则怎样构造树与报告错误？ |
| 符号与类型 | [symtab.c](Lab/Code/symtab.c)、[symtab.h](Lab/Code/symtab.h) | 数组、结构体、函数与作用域怎样表示？ |
| 语义／IR | [semantic.c](Lab/Code/semantic.c) | 类型诊断、临时变量、标号和地址计算怎样结合？ |
| 驱动 | [main.c](Lab/Code/main.c) | 输入、输出和各阶段如何串接？ |
| 样例与报告 | [Lab/Test](Lab/Test)、[Lab/report.pdf](Lab/report.pdf) | 用具体输入理解课程要求与实现过程 |

继续阅读：[代码与历史导览](docs/CODE_TOUR.md)。

## 历史增量

- [b5856ce](https://github.com/Lanceloia/cp2020/commit/b5856ce)：符号表与语义遍历的早期增量。
- [9a062ff](https://github.com/Lanceloia/cp2020/commit/9a062ff)：未完成结构体诊断与嵌套结构体作用域处理。
- [619db3a](https://github.com/Lanceloia/cp2020/commit/619db3ac4a260292004fb900d9897ba95834ce2e)：Lab3中间代码阶段。

课程提供C--要求、提交格式与测试组织，个人工作集中在前端、符号／类型处理和IR生成。
当前保存的目标产物是三地址式IR；目标机器码和自举不在这一项目的实现范围。

## 构建入口

原课程[Makefile](Lab/Code/Makefile)使用GCC、Flex、Bison及对应运行库：

```bash
cd Lab/Code
make parser
./parser ../Test/test1.cmm
```

驱动还支持第二个文件参数作为输出路径；省略输入参数的路径需要另行检查，展示时使用显式输入。
仓库保留的旧`Lab/parser`是历史二进制，运行应优先从源码重新构建。

2026年本次整理完成了代码／历史与链接核对，尚未在现代工具链重建。
原[Lab/README](Lab/README)继续保留课程提交要求；本次未修改C代码、测试或Makefile。
