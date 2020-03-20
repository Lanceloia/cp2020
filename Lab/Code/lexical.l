%{
    #include "syntax.tab.h"

    extern struct ast* newnode(char* name, int num, ...);
    extern void eval(struct ast* node, int level);
    extern void yyerror(char*);
    extern int fileno(FILE *);

    extern int error_type;
%}

%option yylineno

LINECOMMENT  \/\/.*
BLOCKCOMMENT \/\*[^*]*\*+([^\/*][^*]*\*+)*\/
dec         [1-9][0-9]*|0
oct         0[0-7]*
hex         0[xX][0-9a-fA-F]*
flt         [0-9]+\.[0-9]*|\.[0-9]+
INT         {dec}|{oct}|{hex}
FLOAT       {flt}|{flt}[eE][-\+]?[0-9]+|{dec}[eE][-\+]?[0-9]+
ID          [a-z_A-Z][a-zA-Z_0-9]*
ERRORID     [0-9]{ID}
SEMI        ;
COMMA       ,
ASSIGNOP    =
RELOP       >|<|>=|<=|==|!=
PLUS        \+
MINUS       -
STAR        \*
DIV         \/
AND         &&
OR          \|\|
DOT         \.
NOT         !
TYPE        int|float
LP          \(
RP          \)
LB          \[
RB          \]
LC          \{
RC          \}
SPACE       [ \t\r]+
EOL         \n
STRUCT      struct
RETURN      return
IF          if
ELSE        else
WHILE       while
ANERROR     .

%%

{INT}       { yylval.a = newnode("INT", 0); return INT;}
{FLOAT}     { yylval.a = newnode("FLOAT", 0); return FLOAT; }
{TYPE}      { yylval.a = newnode("TYPE", 0); return TYPE; }
{STRUCT}    { yylval.a = newnode("STRUCT", 0); return STRUCT; }
{RETURN}    { yylval.a = newnode("RETURN", 0); return RETURN; }
{IF}        { yylval.a = newnode("IF", 0); return IF; }
{ELSE}      { yylval.a = newnode("ELSE", 0); return ELSE; }
{WHILE}     { yylval.a = newnode("WHILE", 0); return WHILE; }
{ID}        { yylval.a = newnode("ID", 0); return ID; }
{ERRORID}   { error_type = 2; yyerror(yytext); return ID; }
{SEMI}      { yylval.a = newnode("SEMI", 0); return SEMI; }
{COMMA}     { yylval.a = newnode("COMMA", 0); return COMMA; }
{ASSIGNOP}  { yylval.a = newnode("ASSIGNOP", 0); return ASSIGNOP; }
{RELOP}     { yylval.a = newnode("RELOP", 0); return RELOP; }
{PLUS}      { yylval.a = newnode("PLUS", 0); return PLUS; }
{MINUS}     { yylval.a = newnode("MINUS", 0); return MINUS; }
{STAR}      { yylval.a = newnode("STAR", 0); return STAR; }
{DIV}       { yylval.a = newnode("DIV", 0); return DIV; }
{AND}       { yylval.a = newnode("AND", 0); return AND; }
{OR}        { yylval.a = newnode("OR", 0); return OR; }
{DOT}       { yylval.a = newnode("DOT", 0); return DOT; }
{NOT}       { yylval.a = newnode("NOT", 0); return NOT; }
{LINECOMMENT}   { }
{BLOCKCOMMENT}  { }
{EOL}           { }
{SPACE}         { }
{LP}        { yylval.a = newnode("LP", 0); return LP; }
{RP}        { yylval.a = newnode("RP", 0); return RP; }
{LB}        { yylval.a = newnode("LB", 0); return LB; }
{RB}        { yylval.a = newnode("RB", 0); return RB; }
{LC}        { yylval.a = newnode("LC", 0); return LC; }
{RC}        { yylval.a = newnode("RC", 0); return RC; }
{ANERROR}   { error_type = 1; yyerror(yytext); }

%%


