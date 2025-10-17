%{
#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

// Variáveis globais para rastrear a posição no arquivo fonte.
int linha = 1,
    coluna = 0;

// Estrutura para armazenar os atributos de cada símbolo da gramática.
struct Atributos {
  vector<string> c; // Vetor que acumula o código intermediário gerado.
  int linha = 0;
  int coluna = 0;

  void clear() {
    c.clear();
    linha = 0;
    coluna = 0;
  }
};

// Enum para os tipos de declaração de variável.
enum TipoDecl { Let = 1, Const, Var };

// Estrutura para armazenar informações sobre cada símbolo na Tabela de Símbolos.
struct Simbolo {
  TipoDecl tipo;
  int linha;
  int coluna;
};

// Tabela de Símbolos global.
map< string, Simbolo > ts;

// Protótipos de funções.
vector<string> declara_var( TipoDecl tipo, string nome, int linha, int coluna );
void checa_simbolo( string nome, bool modificavel );

#define YYSTYPE Atributos

extern "C" int yylex();
int yyparse();
void yyerror(const char *);

// --- Funções Auxiliares --- //

// Resolve os endereços simbólicos (labels) para endereços numéricos no final da compilação.
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

// Gera um rótulo (label) único para uso em desvios (jumps).
string gera_label( string prefixo ) {
  static int n = 0;
  return prefixo + "_" + to_string( ++n ) + ":";
}

// Sobrecarga de operadores para facilitar a concatenação de código.
vector<string> concatena( vector<string> a, vector<string> b ) {
  a.insert( a.end(), b.begin(), b.end() );
  return a;
}

vector<string> operator+( vector<string> a, vector<string> b ) { return concatena( a, b ); }
vector<string>& operator+=( vector<string>& a, vector<string> b ) { a.insert( a.end(), b.begin(), b.end() ); return a; }
vector<string> operator+( vector<string> a, string b ) { a.push_back( b ); return a; }
vector<string>& operator+=( vector<string>& a, string b ) { a.push_back( b ); return a; }
vector<string> operator+( string a, vector<string> b ) { return vector<string>{ a } + b; }

// --- Funções de Saída e Erro --- //

void print( vector<string> codigo ) {
  for( string s : codigo )
    cout << s << " ";
  cout << endl;
}

// --- Atalhos para geração de código --- //

const string JUMP = "#";
const string JUMP_TRUE = "?";
const string POP = "^";

// string JUMP_FALSE (string lbl) {
//   return "!" + lbl + JUMP_TRUE;
// }

vector<string> JUMP_FALSE (string lbl) {
  // Retorna um vetor com os 3 tokens que a máquina de pilha espera
  return vector<string>{"!", lbl, "?"};
}

vector<string> GET (vector<string> var) {
  return var + "@";
}

%}

// Declaração de tokens
%token ID IF ELSE LET CONST VAR PRINT FOR WHILE
%token CDOUBLE CSTRING CINT
%token AND OR ME_IG MA_IG DIF IGUAL
%token MAIS_IGUAL MAIS_MAIS

// Definição de precedência e associatividade dos operadores
%right '=' MAIS_IGUAL // Atribuições são associativas à direita
%left OR
%left AND
%nonassoc '<' '>' IGUAL MA_IG ME_IG DIF
%left '+' '-'
%left '*' '/' '%'
%left '['
%left '.'
%right MAIS_MAIS // Operadores unários como ++ costumam ter alta precedência

%%

// Regra inicial da gramática
S : CMDs { print( resolve_enderecos( $1.c + "." ) ); }
  ;

// Regra para uma lista de comandos
CMDs : CMDs CMD  { $$.c = $1.c + $2.c; }
     |           { $$.clear(); }
     ;

// Regra que define um comando
CMD : DECL ';'
    | CMD_IF
    | PRINT E ';'
      { $$.c = $2.c + "println" + "#"; }
    | CMD_FOR
    | CMD_WHILE
    | E ';'
      { $$.c = $1.c + "^"; }
    | '{' CMDs '}'            // Bloco de comandos
      { $$.c = $2.c; }
    ;

CMD_FOR : FOR '(' SF ';' E ';' EF ')' CMD
        {
          string lbl_fim_for = gera_label( "fim_for" );
          string lbl_condicao_for = gera_label( "condicao_for" );
          string def_lbl_condicao_for = ":" + lbl_condicao_for;
          string def_lbl_fim_for = ":" + lbl_fim_for;

          $$.c = $3.c + def_lbl_condicao_for + $5.c + JUMP_FALSE(lbl_fim_for) +
                 $9.c + $7.c + lbl_condicao_for + JUMP + def_lbl_fim_for;
        }

        ;

CMD_WHILE : WHILE '(' E ')' CMD 
          {
            string lbl_fim_while = gera_label( "fim_while" );
            string lbl_condicao_while = gera_label( "condicao_while" );
            string def_lbl_condicao_while = ":" + lbl_condicao_while;
            string def_lbl_fim_while = ":" + lbl_fim_while;

            $$.c = def_lbl_condicao_while 
            + $3.c + JUMP_FALSE(lbl_fim_while) // verificar a condição e pula para o fim se for falsa
            + $5.c                             // executa os comandos dentro do WHILE
            + lbl_condicao_while + JUMP        // pula sempre para a condição
            + def_lbl_fim_while;
          }
          ;

// Regra para os diferentes tipos de declaração
DECL : CMD_LET
     | CMD_VAR
     | CMD_CONST
     ;

// Inicialização do FOR: pode ser uma declaração ou uma expressão
SF : EF
   | DECL
   ;

// Seção de expressão do FOR (ou expressão vazia)
EF : E { $$.c = $1.c + "^"; }
   |   { $$.clear(); }
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
         {
           string lbl_fim_if = gera_label( "fim_if" );
           string def_lbl_fim_if = ":" + lbl_fim_if;

            $$.c = $3.c + "!" + lbl_fim_if  + "?" + $5.c + def_lbl_fim_if;
         }
       | IF '(' E ')' CMD ELSE CMD
          {
            string lbl_fim_if = gera_label( "fim_if" );
            string lbl_else_if = gera_label( "else_if" );
            string def_lbl_fim_if = ":" + lbl_fim_if;
            string def_lbl_else_if = ":" + lbl_else_if;

            $$.c = $3.c + "!" + lbl_else_if + "?" +
                   $5.c + lbl_fim_if + "#" +
                   def_lbl_else_if + $7.c +
                   def_lbl_fim_if ;
          }
        ;

        
/* // LVALUE: Define o que pode estar à esquerda de uma atribuição.
// OBS.: juntei LVALUE e LVALUE PROP apenas nisso aqui. Acontece um tratamento em ATRIB
// para garantir que isso não aconteça. */
LVALUE : ID
      | LVALUE '[' E ']' { $$.c = GET($1.c) + $3.c; } // Usa GET
      | LVALUE '.' ID    { $$.c = GET($1.c) + $3.c; } // Usa GET 





//// --- teste

// LVALUE agora é a união de um LVALUE de variável ou de propriedade.
/* LVALUE : LVALUE_VAR
       | LVALUE_PROP
       ; */

// VAR_LVALUE é apenas um ID simples.
/* LVALUE_VAR : ID ; */
/* 
// PROP_LVALUE é um acesso a uma propriedade.
// Esta é a regra que gera o código para buscar o objeto/array na pilha.
LVALUE_PROP : LVALUE '[' E ']' { $$.c = GET($1.c) + $3.c; }
            | LVALUE '.' ID    { $$.c = GET($1.c) + $3.c; }
            ;
            
// LVALUE: Define o que pode estar à esquerda de uma atribuição.
LVALUE : ID
      | LVALUE '[' E ']' { $$.c = GET($1.c) + $3.c; }
      | LVALUE '.' ID    { $$.c = GET($1.c) + $3.c; }
      ;

// ATRIB: Agora temos regras separadas e sem ambiguidade.
ATRIB : LVALUE_VAR '=' E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + $3.c + "="; // Sempre usa "=" para variáveis
        }
      | LVALUE_PROP '=' E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + $3.c + "[=]"; // Sempre usa "[=]" para propriedades
        }
      | LVALUE_VAR MAIS_IGUAL E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + GET($1.c) + $3.c + "+" + "=";
        }
      | LVALUEPROP MAIS_IGUAL E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + $1.c + "[@]" + $3.c + "+" + "[=]"; // Note que aqui usamos $1.c duas vezes
        }
      ; */

// F: Fatores.
F : LVALUE
    {
      checa_simbolo( $1.c[0], false );
      // Se for um acesso a propriedade, precisa do getProp. Se for só ID, precisa do get.
      // O LVALUE já gera 'obj @ prop', então para ler o valor, adicionamos '[@]'
      if ($1.c.size() > 1)
        $$.c = $1.c + "[@]";
      else // Se for só um ID
        $$.c = GET($1.c);
    }
  | '{' '}'     { $$.c = vector<string>{"{}"}; }
  | '[' ']'     { $$.c = vector<string>{"[]"}; }
  | CDOUBLE
  | CINT
  | CSTRING
  | LVALUE MAIS_MAIS
    {
       // Esta regra pode precisar de uma lógica similar à de ATRIB para ++
       // mas vamos focar no problema principal primeiro.
       checa_simbolo($1.c[0], true);
       $$.c = GET($1.c) + $1.c + GET($1.c) + "1" + "+" + "=" + "^" ;
    }
  | '(' E ')'   { $$.c = $2.c; }
  | '-' F       { $$.c = vector<string>{"0"} + $2.c + "-"; }
  ;



// ATRIB: Operações de atribuição.
ATRIB : LVALUE '=' E
        {
          checa_simbolo( $1.c[0], true );
          // Se o código do LVALUE tem mais de 1 parte (ex: 'a', '.','b'), é uma atribuição de propriedade.
          if ($1.c.size() > 1)
             $$.c = $1.c + $3.c + "[=]";
          else
             $$.c = $1.c + $3.c + "=";
        }
      | LVALUE MAIS_IGUAL E
        {
          checa_simbolo( $1.c[0], true );
          if ($1.c.size() > 1) 
            $$.c = $1.c + $1.c + "[@]" + $3.c + "+" + "[=]";
          else 
            $$.c = $1.c + GET($1.c) + $3.c + "+" + "=";
        }
      ;



// E: Expressões. Uma expressão pode ser uma atribuição ou uma operação binária.
E : ATRIB
  | E_BIN
  ;

// E_BIN: Regras para todas as operações binárias (aritméticas, lógicas, etc.).
E_BIN : E_BIN '<' E_BIN     { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN '>' E_BIN     { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN IGUAL E_BIN   { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN MA_IG E_BIN   { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN ME_IG E_BIN   { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN DIF E_BIN     { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN OR E_BIN      { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN AND E_BIN     { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN '+' E_BIN     { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN '-' E_BIN     { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN '*' E_BIN     { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN '/' E_BIN     { $$.c = $1.c + $3.c + $2.c; }
      | E_BIN '%' E_BIN     { $$.c = $1.c + $3.c + $2.c; }
      | F
      ;

// F: Fatores. Elementos de maior precedência em uma expressão.
F : ID
    {
      checa_simbolo( $1.c[0], false ); // Checa se a variável existe para leitura.
      $$.c = GET($1.c);
    }
  | '{' '}'     { $$.c = vector<string>{"{}"}; }
  | '[' ']'     { $$.c = vector<string>{"[]"}; }
  | CDOUBLE
  | CINT
  | CSTRING
  | LVALUE MAIS_MAIS
    {
      checa_simbolo($1.c[0], true); // Checa se pode modificar.
      // Gera código para post-incremento (retorna valor antigo, depois incrementa)
      $$.c = GET($1.c) + $1.c + GET($1.c) + "1" + "+" + "=" + "^" ;
    }
  | '(' E ')'   { $$.c = $2.c; }
  | '-' F       { $$.c = vector<string>{"0"} + $2.c + "-"; }
  ;

%%

#include "lex.yy.c"

// Insere variáveis na tabela de símbolos, checando as regras da linguagem.
vector<string> declara_var( TipoDecl tipo, string nome, int linha, int coluna ) {
  if( ts.count( nome ) == 0 ) {
    ts[nome] = Simbolo{ tipo, linha, coluna };
    return vector<string>{ nome, "&" };
  }
  else if( tipo == Var && ts[nome].tipo == Var ) {
    ts[nome] = Simbolo{ tipo, linha, coluna };
    return vector<string>{};
  }
  else {
    cerr << "Erro: a variável '" << nome << "' ja foi declarada na linha " << ts[nome].linha << "." << endl;
    exit( 1 );
  }
}

// Consulta a tabela de símbolos para checar se uma variável existe e se pode ser modificada.
void checa_simbolo( string nome, bool modificavel ) {
  if( ts.count( nome ) > 0 ) {
    if( modificavel && ts[nome].tipo == Const ) {
      cerr << "Erro: tentativa de modificar uma variavel constante ('" << nome << "')." << endl;
      exit( 1 );
    }
  }
  else {
    cerr << "Erro: a variável '" << nome << "' não foi declarada." << endl;
    exit( 1 );
  }
}

// Função de erro padrão do Bison.
void yyerror( const char* st ) {
   cerr << "Erro de sintaxe proximo a '" << yytext << "' na linha " << linha << "." << endl;
   exit( 1 );
}

// Função principal.
int main( int argc, char* argv[] ) {
  yyparse();
  return 0;
}