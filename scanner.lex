%{
    /* Declarations section */

#include <stdio.h>
#include "Stypes.hpp"
#include "parser.tab.hpp"
#include "hw3_output.hpp"
using namespace output;

%}

%option yylineno
%option noyywrap
void "void"
int "int"
byte "byte"
bool "bool"
and "and"
or "or"
not "not"
true "true"
false "false"
return "return"
if "if"
else "else"
while "while"
break "break"
continue "continue"
sc (\;)
comma (\,)
lparen (\()
rparen(\))
lbrace (\{)
rbrace (\})
assign (\=)
relop (<|>|<=|>=)
equal_relop (==|!=)
plus (\+)
minus (\-)
mult_div ([\*\/])
comment (\/\/[^\r\n]*[\r|\n|\r\n]?)
digit ([0-9])
letter ([a-zA-Z])
whitespace ([ \t\n\r])
id ([a-zA-Z]+[a-zA-Z0-9]*)
num (0|([1-9]{digit}*))
space ([ ])
string (\"([^\n\r\"\\]|\\[rnt\"\\])+\")


%%
{void} return VOID;
{int} return INT;
{byte} return BYTE;
b return  B;
{bool} return BOOL;
{and} return AND;
{or} return OR;
{not} return NOT;
{true} return TRUE;
{false} return FALSE;
{return} return RETURN;
{if} return IF;
{else} return ELSE;
{while} return WHILE;
{break} return BREAK;
{continue} return CONTINUE;
{sc} return SC;
{comma} return COMMA;
{lparen} return LPAREN;
{rparen} return RPAREN;
{lbrace} return LBRACE;
{rbrace} return RBRACE;
{assign} return ASSIGN;
{relop} return RELOP;
{equal_relop} return EQUAL_RELOP;
{plus} return PLUS;
{minus} return MINUS;
{mult_div} return MULT_DIV;
{num} {Num_c* num = new Num_c(yytext); yylval = (Node*)num; return NUM;}
{id} {ID_c* id = new ID_c(yytext); yylval = (Node*)id; return ID;}
{string} return STRING;
<<EOF>> return END;
{comment}
{whitespace}

. {errorLex(yylineno);}
%%