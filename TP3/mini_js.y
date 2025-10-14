%{
#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

int linha = 1, 
    coluna = 0; 

struct Atributos {
  vector<string> c; // Código

  int linha = 0, coluna = 0;

  void clear() {
    c.clear();
    linha = 0;
    coluna = 0;
  }
};

enum TipoDecl { Let = 1, Const, Var };

struct Simbolo {
  TipoDecl tipo;
  int linha;
  int coluna;
};

map< string, Simbolo > ts; // Tabela de símbolos

vector<string> declara_var( TipoDecl tipo, string nome, int linha, int coluna );
void checa_simbolo( string nome, bool modificavel );

#define YYSTYPE Atributos

extern "C" int yylex();
int yyparse();
void yyerror(const char *);

vector<string> concatena( vector<string> a, vector<string> b ) {
  a.insert( a.end(), b.begin(), b.end() );
  return a;
}

vector<string> operator+( vector<string> a, vector<string> b ) {
  return concatena( a, b );
}

vector<string>& operator+=( vector<string>& a, vector<string> b ) {
  a.insert( a.end(), b.begin(), b.end() );
  return a;
}

vector<string> operator+( vector<string> a, string b ) {
  a.push_back( b );
  return a;
}

vector<string>& operator+=( vector<string>& a, string b ) {
  a.push_back( b );
  return a;
} // ver isso aqui!!!

vector<string> operator+( string a, vector<string> b ) {
  return vector<string>{ a } + b;
}

vector<string> resolve_enderecos( vector<string> entrada ) {
  map<string,int> label;
  vector<string> saida;
  for( int i = 0; i < entrada.size(); i++ ) 
    if( entrada[i][0] == ':' ) 
        label[entrada[i].substr(1)] = saida.size();
    else
      saida.push_back( entrada[i] );
  
  for( int i = 0; i < saida.size(); i++ ) 
    if( label.count( saida[i] ) > 0 )
        saida[i] = to_string(label[saida[i]]);
    
  return saida;
}

string gera_label( string prefixo ) {
  static int n = 0;
  return prefixo + "_" + to_string( ++n ) + ":";
}

void print( vector<string> codigo ) {
  for( string s : codigo )
    cout << s << " ";
    
  cout << endl;  
}

const string JUMP = "#";
const string JUMP_TRUE = "?";
const string POP = "^";

string JUMP_FALSE (string lbl) {
  return "!" + lbl + JUMP_TRUE;
}

vector<string> GET (vector<string> var) {
  return var + "@";
}

%}

%token ID IF ELSE LET CONST VAR PRINT FOR WHILE
%token CDOUBLE CSTRING CINT
%token AND OR ME_IG MA_IG DIF IGUAL
%token MAIS_IGUAL MAIS_MAIS

%right '='
%nonassoc '<' '>'
%left '+' '-'
%left '*' '/' '%'

%left '['
%left '.'


%%

S : CMDs { print( resolve_enderecos( $1.c + "." ) ); }
  ;

CMDs : CMDs CMD  { $$.c = $1.c + $2.c; };
     |           { $$.clear(); }
     ;
     
CMD : DECL ';'
    | CMD_IF
    | PRINT E ';' 
      { $$.c = $2.c + "println" + "#"; }
    | CMD_FOR
    | E ';'
      { $$.c = $1.c + "^"; };
    | '{' CMDs '}'            // BLOCO (próx. trabalho)
      { $$.c = $2.c; }
    ;
 
CMD_FOR : FOR '(' SF ';' E ';' EF ')' CMD 
        {
          string lbl_fim_for = gera_label( "fim_for" );
          string lbl_condicao_for = gera_label( "condicao_for" );
          string def_lbl_condicao_for = ":" + lbl_condicao_for;
          string def_lbl_fim_for = ":" + lbl_fim_for;

          $$.c = $3.c + def_lbl_condicao_for + $5.c + JUMP_FALSE(lbl_fim_for) + 
                 $9.c + $7.c + lbl_condicao_for + JUMP + def_lbl_fim_for; // não tem que desempilhar nada?

        }
        ;

DECL : CMD_LET 
     | CMD_VAR 
     | CMD_CONST
     ;

SF : EF 
   | DECL
   ;

EF : E { $$.c = $1.c + "^"; }
   | { $$.clear(); }        // sempre que tivermos algo indo para vazio, tem que ter um clear
   ;  

CMD_LET : LET LET_VARs { $$.c = $2.c; }
        ;

LET_VARs : LET_VAR ',' LET_VARs { $$.c = $1.c + $3.c; } 
         | LET_VAR
         ;

LET_VAR : ID  
          { $$.c = declara_var( Let, $1.c[0], $1.linha, $1.coluna ); }
        | ID '=' E
          { 
            $$.c = declara_var( Let, $1.c[0], $1.linha, $1.coluna ) + 
                   $1.c + $3.c + "=" + "^"; }
        ;
  
CMD_VAR : VAR VAR_VARs { $$.c = $2.c; }
        ;
        
VAR_VARs : VAR_VAR ',' VAR_VARs { $$.c = $1.c + $3.c; } 
         | VAR_VAR
         ;

VAR_VAR : ID  
          { $$.c = declara_var( Var, $1.c[0], $1.linha, $1.coluna ); }
        | ID '=' E
          {  $$.c = declara_var( Var, $1.c[0], $1.linha, $1.coluna ) + 
                    $1.c + $3.c + "=" + "^"; }
        ;
  
CMD_CONST: CONST CONST_VARs { $$.c = $2.c; }
         ;
  
CONST_VARs : CONST_VAR ',' CONST_VARs { $$.c = $1.c + $3.c; } 
           | CONST_VAR
           ;

CONST_VAR : ID '=' E
            { $$.c = declara_var( Const, $1.c[0], $1.linha, $1.coluna ) + 
                     $1.c + $3.c + "=" + "^"; }
          ;
  
CMD_IF : IF '(' E ')' CMD 
         { string lbl_true = gera_label( "lbl_true" );
           string lbl_fim_if = gera_label( "lbl_fim_if" );
           string def_lbl_true = ":" + lbl_true;
           string def_lbl_fim_if = ":" + lbl_fim_if;

            $$.c = $3.c + "!" + lbl_fim_if  + "?" + $5.c + def_lbl_fim_if; // feito em aula

        }
        
        | IF '(' E ')' CMD ELSE CMD
          { string lbl_fim_if = gera_label( "lbl_fim_if" );
            string lbl_else_if = gera_label( "lbl_fim_if" ); 
            string def_lbl_fim_if = ":" + lbl_fim_if;
            string def_lbl_else_if = ":" + lbl_else_if;

            $$.c = $3.c + "!" + lbl_else_if + "?" +       // feito em aula 
                   $5.c + lbl_fim_if + "#" +
                   def_lbl_else_if + $7.c + 
                   def_lbl_fim_if ; 
          }
        ;
        
LVALUE : ID 
       ;
       
LVALUEPROP : E '[' E ']' { $$.c = $1.c + $3.c; } 
           | E '.' ID    { $$.c = $1.c + $3.c; }


E : LVALUE '=' '{' '}'
    { checa_simbolo( $1.c[0], true ); $$.c = $1.c + "{}" + "="; }
  | LVALUE '=' E 
    { checa_simbolo( $1.c[0], true ); $$.c = $1.c + $3.c + "="; }
  | LVALUEPROP '=' E { $$.c = $1.c + $3.c + "[=]"; }
  | E '<' E     { $$.c = $1.c + $3.c + $2.c; }
  | E '>' E     { $$.c = $1.c + $3.c + $2.c; }
    
  | E '+' E     { $$.c = $1.c + $3.c + $2.c; }
    
  | E '-' E     { $$.c = $1.c + $3.c + $2.c; }
  | E '*' E     { $$.c = $1.c + $3.c + $2.c; }
  | E '/' E     { $$.c = $1.c + $3.c + $2.c; }
  | E '%' E     { $$.c = $1.c + $3.c + $2.c; } 
  | LVALUE MAIS_IGUAL E { checa_simbolo( $1.c[0], true );
                          $$.c = $1.c + GET($1.c) + $3.c + "+" + "="; }
  | LVALUEPROP { $$.c = $1.c + "[@]"; }
  | '(' E ')'
    { $$.c = $2.c; }
   
  | '(' '{' '}' ')'
    { $$.c = vector<string>{"{}"}; }
  | F
  ;

/* fazer um E vai para F, onde F teria operadores unários (maior prioridade) */

F : ID { $$.c = GET($1.c); }
  | '{' '}'   { $$.c = vector<string>{"{}"}; }
  | '[' ']'   { $$.c = vector<string>{"[]"}; }
  | CDOUBLE
  | CINT 
  | CSTRING
  | LVALUE MAIS_MAIS
    /* { $$.c = $1.c + GET($1.c) + $1.c + GET($1.c) + "1" + "+" + "=" ; } // fazer uma temp? */
    { $$.c = GET($1.c) + $1.c + GET($1.c) + "1" + "+" + "=" + "^" ; }
  ;
  

%%

#include "lex.yy.c"

vector<string> declara_var( TipoDecl tipo, string nome, int linha, int coluna ) {
  cerr << "insere_simbolo( " << tipo << ", " << nome 
       << ", " << linha << ", " << coluna << ")" << endl;
       
  if( ts.count( nome ) == 0 ) {
    ts[nome] = Simbolo{ tipo, linha, coluna };
    return vector<string>{ nome, "&" };
  }
  else if( tipo == Var && ts[nome].tipo == Var ) {
    ts[nome] = Simbolo{ tipo, linha, coluna };
    return vector<string>{};
  } 
  else {
    cerr << "Variavel '" << nome << "' já declarada na linha: " << ts[nome].linha 
         << ", coluna: " << ts[nome].coluna << endl;
    exit( 1 );     
  }
}

void checa_simbolo( string nome, bool modificavel ) {
  if( ts.count( nome ) > 0 ) {
    if( modificavel && ts[nome].tipo == Const ) {
      cerr << "Variavel '" << nome << "' não pode ser modificada." << endl;
      exit( 1 );     
    }
  }
  else {
    cerr << "Variavel '" << nome << "' não declarada." << endl;
    exit( 1 );     
  }
}

void yyerror( const char* st ) {
   cerr << st << endl; 
   cerr << "Proximo a: " << yytext << endl;
   exit( 1 );
}

int main( int argc, char* argv[] ) {
  yyparse();
  
  return 0;
}