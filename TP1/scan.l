%{
#include <stdio.h>
#include <string>

using namespace std;
string lexema;
%}

/* Coloque aqui definições regulares */

COMENTARIO    ("//"[^\n\r\t]*)|("/*"([^*]|\*+[^*/])*\*+"/")
WS	          [ \t\n\r]
D             [0-9]
L             [A-Za-z]
INT           {D}+
FLOAT         {D}+(\.{D}+)?([Ee][+\-]?{D}+)?
UNDERLINE     "_"
FOR           [fF][oO][rR]
IF            [iI][fF]
ID            ({L}|{UNDERLINE}|\$)({L}|{D}|{UNDERLINE})*
STRING1       (\"([^\n\r\"\\]*|\\.|\"\")*\")|(\'([^\n\r\'\\]*|\\.|\'\')*\')
STRING2       \`[^\`]*\`
ERRO_VAR      ({D}+({L}|{UNDERLINE}|\$))({L}|{D}|{UNDERLINE})*|({L}|{UNDERLINE}|\$)+(\$)+({L}|{D}|{UNDERLINE})*

/* Erro em variável: "$" em qualquer posição exceto a primeira; dígito como primeiro caractere */

%%
    /* Padrões e ações. Nesta seção, comentários devem ter um tab antes */


{WS}	                  { /* ignora espaços, tabs e '\n' */ } 
{COMENTARIO}            { lexema = yytext; return _COMENTARIO; }


">="    { lexema = yytext; return _MAIG; }
"<="    { lexema = yytext; return _MEIG; }
"=="    { lexema = yytext; return _IG; }
"!="    { lexema = yytext; return _DIF; }

  /* Todas as palavras reservadas devem aparecer antes do padrão do ID */
{FOR}               { lexema = yytext; return _FOR; }
{IF}                { lexema = yytext; return _IF; }

{STRING1}           { 
                    lexema = yytext; string resultado = "";
                      if (lexema[0] == '"') {
                        for (int i = 1; i < lexema.length() - 1; i++) {
                            if (((lexema[i] == '"') && (lexema[i + 1] == '"')) || ((lexema[i] == '\\') && (lexema[i + 1] == '"'))) {
                                resultado += '"';
                                i++;
                              }
                            else {
                              resultado += lexema[i];
                            }
                        }}
                      else {
                        for (int i = 1; i < lexema.length() - 1; i++) {
                            if (((lexema[i] == '\'') && (lexema[i + 1] == '\'')) || ((lexema[i] == '\\') && (lexema[i + 1] == '\''))) {
                                resultado += '\'';
                                i++;
                              }
                            else {
                              resultado += lexema[i];
                            }
                      }}
                      lexema = resultado;
                      return _STRING;
                    }
      
{STRING2}           { lexema = yytext; string str1, expr, str2 = "";
                      int mode = 0; 
                      for (int i = 1; i < lexema.length() - 1; i++) {
                          if (lexema[i] == '$' && i + 1 < lexema.length() - 1 && lexema[i+1] == '{') {
                              mode = 1;
                              i++; 
                              continue; 
                          } 
                          else if ((lexema[i] == '}') && (mode == 1)) {
                              mode = 2;
                              continue; 
                          }

                          if (mode == 0) {
                              str1 += lexema[i];
                          }
                          if (mode == 1) {
                              expr += lexema[i];
                          }
                          if (mode == 2) {
                              str2 += lexema[i];
                          }
                      }

                      if (expr.length() != 0) { // Se existir expressão, printamos str1 e expr e retornamos str2
                        printf( "%d %s\n", _STRING2, str1.c_str() );
                        printf( "%d %s\n", _EXPR, expr.c_str() );
                        lexema = str2;
                        return _STRING2;
                      }
                      else{ // não existe expressão
                        lexema = str1;
                        return _STRING2;
                      }
                    }

{ERRO_VAR}          { lexema = yytext; 
                      printf( "%s %s\n", "Erro: Identificador invalido:", lexema.c_str()); }


{ID}                   { lexema = yytext; return _ID; }
{INT}                 { lexema = yytext; return _INT; }
{FLOAT}                 { lexema = yytext; return _FLOAT; }

.       { lexema = yytext; return *yytext; 

/* Essa deve ser a última regra. Dessa forma qualquer caractere isolado será retornado pelo seu código ascii. */ 
}


%%

/* Não coloque nada aqui - a função main é automaticamente incluída na hora de avaliar e dar a nota. */