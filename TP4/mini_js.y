%{
#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

// Variáveis globais para rastrear a posição no arquivo fonte
int linha = 1,
    coluna = 0;

// Estrutura para armazenar os atributos de cada símbolo da gramática
struct Atributos {
  vector<string> c; // Vetor que acumula o código intermediário gerado
  int linha = 0;
  int coluna = 0;

  // Atributos relacionados à funções
  int n_args = 0;
  int i = 0;

  vector<string> valor_default; // Para argumentos default

  void clear() {
    c.clear();
    linha = 0;
    coluna = 0;
    n_args = 0;
    i = 0;
    valor_default.clear(); 
  }
};

// Enum para os tipos de declaração de variável.
enum TipoDecl { Let = 1, Const, Var };

// Estrutura para armazenar informações sobre cada símbolo na Tabela de Símbolos
struct Simbolo {
  TipoDecl tipo;
  int linha;
  int coluna;
};

// Tabela de símbolos - agora é uma pilha (cada escopo tem sua própria tabela de símbolos)
vector < map < string, Simbolo > > ts = { map< string, Simbolo >{} }; 
vector<string> funcoes; 

// Protótipos de funções
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

// Gera um label para uso em jumps
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

vector<string> JUMP_FALSE (string lbl) {
  return vector<string>{"!", lbl, "?"};
}

vector<string> GET (vector<string> var) {
  return var + "@";
}

vector<string> GET_LVALUE_VAL( Atributos lval ) {
  // O tamanho do vetor de código indica se é um ID simples ou um acesso composto (propriedade de objeto).
  if (lval.c.size() > 1) {
    return lval.c + "[@]";
  } else {
    return GET(lval.c); 
  }
}

%}

// Declaração de tokens
%token ID IF ELSE LET CONST VAR FOR WHILE 
%token CDOUBLE CSTRING CINT
%token AND OR ME_IG MA_IG DIF IGUAL
%token MAIS_IGUAL MAIS_MAIS
%token RETURN FUNCTION ASM // FUNÇÕES
%token TRUE FALSE  // VALORES BOLEANOS

// Definição de precedência e associatividade dos operadores
%right '=' MAIS_IGUAL 
%left OR
%left AND
%nonassoc '<' '>' IGUAL MA_IG ME_IG DIF
%left '+' '-'
%left '*' '/' '%'
%left '['
%left '.'
%left '(' ')' // Precedência para chamada de função
%right MAIS_MAIS 

// a(2)(3) é possível, logo é associativo à esquerda
// ponto tem que ter precedência menor que o parênteses; ex.: a.nome(2)

%%


// Regra inicial da gramática
S : CMDs { print( resolve_enderecos( $1.c + "." + funcoes ) ); } 
  ;

// Regra para uma lista de comandos
CMDs : CMDs CMD  { $$.c = $1.c + $2.c; }
     | CMD       { $$.c = $1.c; }      // Isso faz com que não possa ter bloco totalmente vazio; resolve sr
     ;

// Regra que define um comando
CMD : DECL ';'
    | CMD_IF
    /* | PRINT E ';'
      { $$.c = $2.c + "println" + "#"; } */
    | CMD_FOR
    | CMD_WHILE
    | E ';'
      { $$.c = $1.c + "^"; }
    /* | '{' CMDs '}'            // Bloco de comandos (conflito com a regra logo abaixo; conferir)
      { $$.c = $2.c; } */
    | '{' EMPILHA_TS CMDs '}'  // (para usar escopo)
      { 
        ts.pop_back(); 
        $$.c = "<{" + $3.c + "}>"; // Gera instruções de escopo 
      }
    | ';' { $$.clear(); }
    | CMD_FUNC
    | CMD_RETURN
    | E ASM ';'  // ASM é uma forma de gerar código DIRETO para a máquina de pilha
      { 
        $$.c = $1.c + $2.c + "^"; 
      }
    ;

// F -> E ( ARG )

EMPILHA_TS : { ts.push_back( map< string, Simbolo >{} ); } 
           ;
    
CMD_FUNC : FUNCTION ID { declara_var( Var, $2.c[0], $2.linha, $2.coluna ); } // Declara nome da função no escopo *atual*
           '(' EMPILHA_TS LISTA_PARAMS ')' '{' CMDs '}'
           { 
             string lbl_endereco_funcao = gera_label( "func_" + $2.c[0] );
             string definicao_lbl_endereco_funcao = ":" + lbl_endereco_funcao;
             
             $$.c = $2.c + "&" + $2.c + "{}"  + "=" + "'&funcao'" +
                    lbl_endereco_funcao + "[=]" + "^";
                    
             funcoes = funcoes + definicao_lbl_endereco_funcao
                     + $6.c   
                     + $9.c 
                     + "undefined" + "@" + "'&retorno'" + "@"+ "~";
                     
             ts.pop_back();
           }
         ;
         
LISTA_PARAMS : PARAMS { $$.c = $1.c; }
             |       { $$.clear(); } // Lista vazia
             ;
// aqui deveria ser PARAMs, e não ARGs (definição)
           
PARAMS : PARAMS ',' PARAM  
         { 
           // $1.i é o índice do novo parâmetro
           declara_var( Var, $3.c[0], $3.linha, $3.coluna );
           
           // Gera código para copiar de arguments[i]
           $$.c = $1.c + $3.c + "&" + $3.c + "arguments" + "@" + to_string($1.i)
                  + "[@]" + "=" + "^"; 
                  
           // Gera código para testar valor default (se existir)
           if( $3.valor_default.size() > 0 ) {
             string lbl_fim_if = gera_label( "fim_default_if" );
             string def_lbl_fim_if = ":" + lbl_fim_if;

             // Gera: if (param == undefined) { param = default_value; }
             $$.c += $3.c + "@" + "undefined" + "@" + "==" + // if (param == undefined)
                     JUMP_FALSE(lbl_fim_if) +          //   (pula se não for)
                     $3.c + $3.valor_default + "=" + "^" +   //   param = default_value;
                     def_lbl_fim_if;                         // :fim_if
           }
           
           // O novo "próximo índice" é o contador anterior + 1
           $$.i = $1.i + 1; 
         }
       | PARAM 
         { 
           declara_var( Var, $1.c[0], $1.linha, $1.coluna );
           
           // Gera código para copiar de arguments[0]
           $$.c = $1.c + "&" + $1.c + "arguments" + "@" + "0" + "[@]" + "=" + "^"; 
                  
           // Gera código para testar valor default (se existir)
           if( $1.valor_default.size() > 0 ) {
             string lbl_fim_if = gera_label( "fim_default_if" );
             string def_lbl_fim_if = ":" + lbl_fim_if;
             
             $$.c += $1.c + "@" + "undefined" + "@" + "==" + 
                     JUMP_FALSE(lbl_fim_if) +
                     $1.c + $1.valor_default + "=" + "^" +
                     def_lbl_fim_if;
           }
           
           // O "próximo índice" é 1
           $$.i = 1; 
         }
       ;

PARAM : ID 
        { 
          $$.c = $1.c;      
          $$.valor_default.clear(); // Sem valor default
          // -> info para declara)var
          $$.linha = $1.linha;
          $$.coluna = $1.coluna;
        }
      | ID '=' E
        { 
          $$.c = $1.c;    
          $$.valor_default = $3.c; // Passa o código do valor default 
          // -> info para declara_var
          $$.linha = $1.linha;
          $$.coluna = $1.coluna;
        }
      ;


CMD_RETURN : RETURN E ';' // return com expressão
           { 
             $$.c = $2.c + "'&retorno'" + "@" + "~"; 
           }
         | RETURN ';' // return vazio -> tem que retornar undefined de qualquer forma!
           {
             $$.c = vector<string>{"undefined"} + "@" + "'&retorno'" + "@" + "~";
           }
         ;

L_ARGS : ARGS { $$.c = $1.c; $$.n_args = $1.n_args; }
       |     { $$.clear(); $$.n_args = 0; } 
       ;

ARGS : ARGS ',' E { $$.c = $1.c + $3.c; $$.n_args = $1.n_args + 1; }
     | E          { $$.c = $1.c; $$.n_args = 1; }
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

// Inicialização do FOR: declaração ou uma expressão
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

// LVALUE: L-value de variável ou de propriedade.
LVALUE : LVALUE_VAR
       | LVALUE_PROP
       ;

// LVALUE_PROP: L-value de propriedade. Gera código para buscar o objeto/array.
LVALUE_PROP : F '[' E ']' { $$.c = $1.c + $3.c; }
            | F '.' ID    { $$.c = $1.c + $3.c; }
            ;

// LVALUE_VAR: ID simples
LVALUE_VAR : ID ;



// ATRIB: Regras de atribuição
ATRIB : LVALUE_VAR '=' E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + $3.c + "=";     // "=" para variáveis
        }
      | LVALUE_PROP '=' E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + $3.c + "[=]";   // "[=]" para propriedades
        }
      | LVALUE_VAR MAIS_IGUAL E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + GET($1.c) + $3.c + "+" + "=";
        }
      | LVALUE_PROP MAIS_IGUAL E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + $1.c + "[@]" + $3.c + "+" + "[=]";
        }
      ;

// E: Expressões. Pode ser uma atribuição ou uma operação binária/fator.
E : ATRIB
  | E_BIN
  ;

// E_BIN: Regras para todas as operações binárias.
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

// F: Fatores. A base de uma expressão. 
F : LVALUE      { $$.c = GET_LVALUE_VAL($1); }
  | F '(' L_ARGS ')' // Chamada de função
    {
      // $1 -> F (qualquer fator, ex: 'log @' ou '(log @)')
      // $3 -> L_ARGS (ex: 36 60, n_args=2)
      // $1.c já contém o código para obter o objeto função
      $$.c = $3.c + to_string($3.n_args) + $1.c + "$";
    }
  | TRUE    { $$.c = $1.c; } 
  | FALSE   { $$.c = $1.c; } 
  | '{' '}'     { $$.c = vector<string>{"{}"}; }
  | '[' ']'     { $$.c = vector<string>{"[]"}; }
  | CDOUBLE
  | CINT
  | CSTRING
  | LVALUE MAIS_MAIS
    {
      checa_simbolo($1.c[0], true);
      $$.c = GET($1.c) + $1.c + GET($1.c) + "1" + "+" + "=" + "^" ;
    }
  | '(' E ')'   { $$.c = $2.c; }
  | '-' F       { $$.c = vector<string>{"0"} + $2.c + "-"; }
  | '+' F       { $$.c = vector<string>{"0"} + $2.c + "-"; }
  ;
  
%%

#include "lex.yy.c"

vector<string> declara_var( TipoDecl tipo, string nome, int linha, int coluna ) {
  // Pega o escopo ATUAL (o topo da pilha)
  auto& topo = ts.back();    
       
  if( topo.count( nome ) == 0 ) {
    topo[nome] = Simbolo{ tipo, linha, coluna };
    return vector<string>{ nome, "&" };
  }
  else if( tipo == Var && topo[nome].tipo == Var ) {
    topo[nome] = Simbolo{ tipo, linha, coluna };
    return vector<string>{};
  } 
  else {
    cerr << "Erro: a variável '" << nome << "' já foi declarada na linha " << topo[nome].linha << "." << endl;
    exit( 1 );     
  }
}

void checa_simbolo( string nome, bool modificavel ) {
  // Procura do escopo mais interno (topo) para o mais externo (base)
  for( int i = ts.size() - 1; i >= 0; i-- ) {  
    auto& atual = ts[i];
    
    if( atual.count( nome ) > 0 ) {
      if( modificavel && atual[nome].tipo == Const ) {
        cerr << "Variavel '" << nome << "' não pode ser modificada." << endl;
        exit( 1 );     
      }
      else 
        return;
    }
  }

  cerr << "Variavel '" << nome << "' não declarada." << endl;
  exit( 1 );     
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