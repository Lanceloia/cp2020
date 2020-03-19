%{
    #include <stdio.h>
    extern struct ast* newnode(char* name, int num, ...);
%}

/* declared types */
%union {
    struct ast* a;
}

/* declared tokens*/
%token <a> INT
%token <a> FLOAT
%token <a> ID
%token <a> SEMI COMMA ASSIGNOP RELOP
%token <a> PLUS MINUS STAR DIV 
%token <a> AND OR DOT NOT
%token <a> TYPE LP RP LB RB LC RC
%token <a> STRUCT RETURN IF ELSE WHILE

%type <a> Program ExtDefList ExtDef ExtDecList Specifier StructSpecifier 
OptTag Tag VarDec FunDec VarList ParamDec CompSt StmtList Stmt 
DefList Def DecList Dec Exp Args

/* priority */
%right ASSIGNOP
%left OR AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT 
%left LP RP LB RB DOT

/* association */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

/* declared non-terminals*/
%%

/* high-level definitions*/

Program : ExtDefList { $$=newnode("Program", 1, $1); eval($$,0); printf("fuckme\n"); }
    ;
ExtDefList : ExtDef ExtDefList      { $$=newnode("ExtDefList", 2, $1, $2); }
    | /* empty */                   { $$=newnode("ExtDefList", -1); }
    ;
ExtDef : Specifier ExtDecList SEMI  { $$=newnode("ExtDef", 3, $1, $2, $3); }
    | Specifier SEMI                { $$=newnode("ExtDef", 2, $1, $2); }
    | Specifier FunDec CompSt       { $$=newnode("ExtDef", 3, $1, $2, $3); }
    ;
ExtDecList : VarDec             { $$=newnode("ExtDecList", 1, $1); }
    | VarDec COMMA ExtDecList   { $$=newnode("ExtDecList", 3, $1, $2, $3); }
    ;

/* specifiers*/
Specifier : TYPE        { $$=newnode("Specifier", 1, $1); }
    | StructSpecifier   { $$=newnode("Specifier", 1, $1); }
    ;
StructSpecifier : STRUCT OptTag LC DefList RC       { $$=newnode("StructSpecifier", 5, $1, $2, $3, $4, $5); }
    | STRUCT Tag        { $$=newnode("StructSpecifier", 2, $1, $2); }
    ;
OptTag : ID             { $$=newnode("OptTag", 1, $1); }
    | /* empty */       { $$=newnode("OptTag", -1); }
    ;
Tag : ID                { $$=newnode("Tag", 1, $1); }
    ;

/* declarators */
VarDec : ID                         { $$=newnode("VarDec", 1, $1); }
    | VarDec LB INT RB              { $$=newnode("VarDec", 4, $1, $2, $3, $4); }
    ;
FunDec : ID LP VarList RP           { $$=newnode("FunDec", 4, $1, $2, $3, $4); }
    | ID LP RP                      { $$=newnode("FunDec", 3, $1, $2, $3); }
    ;
VarList : ParamDec COMMA VarList    { $$=newnode("VarList", 3, $1, $2, $3); }
    | ParamDec                      { $$=newnode("VarList", 1, $1); }
    ;
ParamDec : Specifier VarDec         { $$=newnode("ParamDec", 2, $1, $2); }
    ;

/* statements */
CompSt : LC DefList StmtList RC     { $$=newnode("CompSt", 4, $1, $2, $3, $4); }
    | error RC
    ;
StmtList : Stmt StmtList            { $$=newnode("StmtList", 2, $1, $2); }
    | /* empty */                   { $$=newnode("StmtList", -1); }
    ;
Stmt : Exp SEMI                                 { $$=newnode("Stmt", 2, $1, $2); }
    | CompSt                                    { $$=newnode("Stmt", 1, $1); }
    | RETURN Exp SEMI                           { $$=newnode("Stmt", 3, $1, $2, $3); }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE   { $$=newnode("Stmt", 5, $1, $2, $3, $4, $5); }
    | IF LP Exp RP Stmt ELSE Stmt               { $$=newnode("Stmt", 7, $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt                      { $$=newnode("Stmt", 5, $1, $2, $3, $4, $5); }
    | error SEMI
    ;

/* local definitions */
DefList : Def DefList       { $$=newnode("DefList", 2, $1, $2); }
    | /* empty */           { $$=newnode("DefList", -1); }
    ;
Def : Specifier DecList SEMI    { $$=newnode("Def", 3, $1, $2, $3); } 
    ;
DecList : Dec               { $$=newnode("DecList", 1, $1); }
    | Dec COMMA DecList     { $$=newnode("DecList", 3, $1, $2, $3); }
    ;
Dec : VarDec                { $$=newnode("Dec", 1, $1); }
    | VarDec ASSIGNOP Exp   { $$=newnode("Dec", 3, $1, $2, $3); }
    ;

/* expressions */

Exp : Exp ASSIGNOP Exp  { $$=newnode("Exp", 3, $1, $2, $3); }
    | Exp AND Exp       { $$=newnode("Exp", 3, $1, $2, $3); }
    | Exp OR Exp        { $$=newnode("Exp", 3, $1, $2, $3); }
    | Exp RELOP Exp     { $$=newnode("Exp", 3, $1, $2, $3); }
    | Exp PLUS Exp      { $$=newnode("Exp", 3, $1, $2, $3); }
    | Exp MINUS Exp     { $$=newnode("Exp", 3, $1, $2, $3); }
    | Exp STAR Exp      { $$=newnode("Exp", 3, $1, $2, $3); }
    | Exp DIV Exp       { $$=newnode("Exp", 3, $1, $2, $3); }
    | LP Exp RP         { $$=newnode("Exp", 3, $1, $2, $3); }
    | MINUS Exp         { $$=newnode("Exp", 2, $1, $2); }
    | NOT Exp           { $$=newnode("Exp", 2, $1, $2); }
    | ID LP Args RP     { $$=newnode("Exp", 4, $1, $2, $3, $4); }
    | ID LP RP          { $$=newnode("Exp", 3, $1, $2, $3); }
    | Exp LB Exp RB     { $$=newnode("Exp", 4, $1, $2, $3, $4); }
    | Exp DOT ID        { $$=newnode("Exp", 3, $1, $2, $3); }
    | ID                { $$=newnode("Exp", 1, $1); }
    | INT               { $$=newnode("Exp", 1, $1); }
    | FLOAT             { $$=newnode("Exp", 1, $1); }
    ;

Args : Exp COMMA Args   { $$=newnode("Args", 3, $1, $2, $3); }
    | Exp               { $$=newnode("Args", 1, $1); }
    ;

%%
