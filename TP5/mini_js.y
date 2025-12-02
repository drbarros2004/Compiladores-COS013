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
  int n_args = 0;   // contador de argumentos
  int i = 0;        // contador de parâmetros ?

  vector<string> valor_default; // Para argumentos default

  // Metadados para LVALUE_PROP
  bool is_prop = false;
  string tb; // temporário para base
  string tk; // temporário para key

  void clear() {
    c.clear();
    linha = 0;
    coluna = 0;
    n_args = 0;
    i = 0;
    valor_default.clear(); 
    is_prop = false;
    tb.clear();
    tk.clear();
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
vector< vector<string> > capturas_escopo;
vector<string> funcoes; 
vector <int> alinhamento_blocos; 

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

// Função trim (copia da que está no lexer)
string trim_str(const string& s) {
    size_t start = s.find_first_not_of(" \n\r\t\f\v");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \n\r\t\f\v");
    return s.substr(start, end - start + 1);
}

void verifica_e_captura(string nome) {
    // DEBUG: Mostra o que está chegando
    // cerr << "DEBUG verifica_e_captura: nome=[" << nome << "] tamanho=" << nome.size() << endl;
    
    nome = trim_str(nome);
    
    // cerr << "DEBUG após trim: nome=[" << nome << "] tamanho=" << nome.size() << endl;
    
    if (ts.back().count(nome) > 0) return;

    for (int i = ts.size() - 2; i >= 0; i--) {
        if (ts[i].count(nome) > 0) {
            if (capturas_escopo.empty()) return;

            bool ja_capturou = false;
            for(const string& s : capturas_escopo.back()) {
                if(s == nome) ja_capturou = true;
            }
            
            if(!ja_capturou) {
                // cerr << "DEBUG adicionando captura: [" << nome << "]" << endl;
                capturas_escopo.back().push_back(nome);
            }
            return;
        }
    }
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

// Helpers de temporários de propriedade
string novo_temp() {
  static int n = 0;
  return "__t" + to_string(++n);
}

vector<string> SET_TEMP(string nome, vector<string> expr) {
  return declara_var( Var, nome, 0, 0 )
       + nome + expr + "=" + "^";
}

vector<string> GET_NOME(string nome) {
  return vector<string>{ nome } + "@";
}

vector<string> GET_PROP(Atributos const& lval) {
  return GET_NOME(lval.tb) + GET_NOME(lval.tk) + "[@]";
}

vector<string> SET_PROP(Atributos const& lval, vector<string> valor) {
  return GET_NOME(lval.tb) + GET_NOME(lval.tk) + valor + "[=]";
}

vector<string> GET_LVALUE_VAL( Atributos lval ) {
  if (lval.is_prop) {
    // abre um RA temporário para as temps (tb, tk)
    return vector<string>{"<{"} + lval.c + GET_PROP(lval) + vector<string>{"}>"};
  } else {
    return GET(lval.c);
  }
}

%}

// Declaração de tokens
%token ID IF ELSE LET CONST VAR FOR WHILE 
%token CDOUBLE CSTRING CINT
%token AND OR ME_IG MA_IG DIF IGUAL
%token MAIS_IGUAL MAIS_MAIS MENOS_MENOS MENOS_IGUAL
%token RETURN FUNCTION ASM // FUNÇÕES
%token TRUE FALSE  // VALORES BOLEANOS
%token SETA FPL 

%nonassoc ID '}'       // Precedência baixa (Shift)
%nonassoc FORCE_BLOCK  // Precedência alta (Reduce da ação)



// Definição de precedência e associatividade dos operadores
%left ':'
%right '=' MAIS_IGUAL MENOS_IGUAL SETA
%left OR
%left AND
%nonassoc '<' '>' IGUAL MA_IG ME_IG DIF
%left '+' '-'
%left '*' '/' '%'
%left '['
%left '.'
%left '(' ')' // Precedência para chamada de função
%right MAIS_MAIS MENOS_MENOS

%%


// Regra inicial da gramática
S : CMDs { print( resolve_enderecos( $1.c + "." + funcoes ) ); } 
  ;

BLVAZIO : '{' '}' ;


// Regra para uma lista de comandos
CMDs : CMDs CMD  { $$.c = $1.c + $2.c; }
     | CMD       { $$.c = $1.c; }      // Isso faz com que não possa ter bloco totalmente vazio; resolve sr
     ;

// ... CMDs : CMDs CMD | CMD ; (Mantenha o CMDs original)

OPT_CMDs : CMDs { $$.c = $1.c; }
         |      { $$.clear(); }
         ;

INICIA_BLOCO : { if (!alinhamento_blocos.empty()) alinhamento_blocos.back()++; } %prec FORCE_BLOCK ;

// Regra que define um comando
CMD : DECL ';'
    | CMD_IF
    | CMD_FOR
    | CMD_WHILE
    | E ';'
      { $$.c = $1.c + "^"; }
    | '{' INICIA_BLOCO EMPILHA_TS OPT_CMDs '}'
      { 
        ts.pop_back(); 
        if (!alinhamento_blocos.empty()) {
          alinhamento_blocos.back()--;
        }
        $$.c = "<{" + $4.c + "}>"; 
      }
    | ';' { $$.clear(); }
    | CMD_FUNC
    | CMD_RETURN
    | E ASM ';'  // ASM é uma forma de gerar código DIRETO para a máquina de pilha
      { 
        $$.c = $1.c + $2.c + "^"; 
      }
    /* | BLVAZIO 
      { 
        $$.clear(); // Bloco vazio não gera código nenhum!
      } */
    ;

EMPILHA_TS : { ts.push_back( map< string, Simbolo >{} ); } 
           ;

EMPILHA_ALINHAMENTO : { alinhamento_blocos.push_back(1); } // Empilha 1 (o RA da função)
                    ;

CMD_FUNC : FUNCTION ID { declara_var( Var, $2.c[0], $2.linha, $2.coluna ); } 
           '(' EMPILHA_TS LISTA_PARAMS ')' 
           EMPILHA_ALINHAMENTO  
           '{' OPT_CMDs '}'
           { 
             string lbl_endereco_funcao = gera_label( "func_" + $2.c[0] );
             string definicao_lbl_endereco_funcao = ":" + lbl_endereco_funcao;
             
             $$.c = $2.c + "&" + $2.c + "{}"  + "=" + "'&funcao'" +
                    lbl_endereco_funcao + "[=]" + "^";
                    
             funcoes = funcoes + definicao_lbl_endereco_funcao
                     + $6.c   // LISTA_PARAMS
                     + $10.c  // CMDs
                     + "undefined" + "@" + "'&retorno'" + "@"+ "~";
                     
             ts.pop_back();
             alinhamento_blocos.pop_back(); 
           }
         ;
         
// Substitua as regras antigas por estas:

LISTA_PARAMS : PARAMS OPT_TRAIL_COMMA { $$.c = $1.c; $$.i = $1.i; }
             | { $$.clear(); $$.i = 0; }
             ;

PARAMS : PARAMS ',' PARAM
       {
          // Recupera o índice do argumento atual
          int idx = $1.i;

          // 1. Declara na TS (tempo de compilação) para evitar captura indevida
          declara_var(Var, $3.c[0], $3.linha, $3.coluna);

          // 2. Gera código: y & y arguments @ idx [@] = ^
          // Nota: $1.c vem primeiro (código dos parâmetros anteriores)
          $$.c = $1.c + 
                 $3.c + "&" + $3.c + "arguments" + "@" + to_string(idx) + "[@]" + "=" + "^";
          
          // 3. Suporte a Valor Default (Opcional, mas recomendado)
          if( $3.valor_default.size() > 0 ) {
             string lbl = gera_label("fim_def");
             string def_lbl = ":" + lbl;
             $$.c += $3.c + "@" + "undefined" + "@" + "==" + 
                     "!" + lbl + "?" + 
                     $3.c + $3.valor_default + "=" + "^" + 
                     def_lbl;
          }
          
          // Incrementa contador
          $$.i = idx + 1;
       }
       | PARAM
       {
          // Primeiro parâmetro (Índice 0)
          declara_var(Var, $1.c[0], $1.linha, $1.coluna);
          
          // Gera código: y & y arguments @ 0 [@] = ^
          $$.c = $1.c + "&" + $1.c + "arguments" + "@" + "0" + "[@]" + "=" + "^";
          
          // Default
          if( $1.valor_default.size() > 0 ) {
             string lbl = gera_label("fim_def");
             string def_lbl = ":" + lbl;
             $$.c += $1.c + "@" + "undefined" + "@" + "==" + 
                     "!" + lbl + "?" + 
                     $1.c + $1.valor_default + "=" + "^" + 
                     def_lbl;
          }
          
          $$.i = 1;
       }
       ;

PARAM : ID 
      { 
        $$.c = $1.c; 
        $$.valor_default.clear();
        $$.linha = $1.linha; $$.coluna = $1.coluna;
      }
      | ID '=' E
      {
        $$.c = $1.c;
        $$.valor_default = $3.c; 
        $$.linha = $1.linha; $$.coluna = $1.coluna;
      }
      ;


CMD_RETURN : RETURN E ';' // return com expressão
              { 
                if (alinhamento_blocos.empty()) {
                  yyerror("Erro: 'return' encontrado fora de uma função.");
                  YYABORT;
                }
                
                int num_pops_blocos = alinhamento_blocos.back() - 1;
                $$.c.clear(); // Limpa o "return" de $1.c
                
                $$.c += $2.c; 
                
                // Gera N-1 '}>' para fechar os blocos
                for(int i = 0; i < num_pops_blocos; i++) {
                  $$.c += "}>";
                }
                
                $$.c = $$.c + "'&retorno'" + "@" + "~";
              }
            | RETURN ';' // return vazio
              {
                if (alinhamento_blocos.empty()) {
                  yyerror("Erro: 'return' encontrado fora de uma função.");
                  YYABORT;
                }
                
                int num_pops_blocos = alinhamento_blocos.back() - 1;
                $$.c.clear(); // Limpa o "return" de $1.c

                // Adiciona o valor 'undefined' antes
                $$.c += vector<string>{"undefined"} + "@";

                // Gera N-1 '}>' para fechar os blocos
                for(int i = 0; i < num_pops_blocos; i++) {
                  $$.c += "}>";
                }
                
                $$.c = $$.c + "'&retorno'" + "@" + "~";
              }
            ;

L_ARGS : ARGS OPT_TRAIL_COMMA { $$.c = $1.c; $$.n_args = $1.n_args; }
       |                      { $$.clear(); $$.n_args = 0; } 
       ;

ARGS : ARGS ',' E { $$.c = $1.c + $3.c; $$.n_args = $1.n_args + 1; } // aqui também deve ser EOBJ
     | E          { $$.c = $1.c; $$.n_args = 1; }
     ;

// Melhor solução que encontrei para minha gramática recursiva à esquerda.
// A do professor é recursiva à direita, por isso que isso fica mais
// fácil de reproduzir lá
OPT_TRAIL_COMMA : /* Vazio */
                | ','
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

LVALUE_PROP : F '[' E ']' // segundo a GramProf, deve ser F [ EOBJ ]
            {
              // Avalia base e índice uma única vez em temporários
              $$.tb = novo_temp();
              $$.tk = novo_temp();
              $$.c  = SET_TEMP($$.tb, $1.c)
                    + SET_TEMP($$.tk, $3.c);
              $$.is_prop = true;
            }
            | F '.' ID    
            {
              // Chave por nome deve ser string literal
              $$.tb = novo_temp();
              $$.tk = novo_temp();
              vector<string> key = vector<string>{ "'" + $3.c[0] + "'" };
              $$.c  = SET_TEMP($$.tb, $1.c)
                    + SET_TEMP($$.tk, key);
              $$.is_prop = true;
            }
            // adicionar: (baseado na GramProf)
            // LVP [ EOBJ ]
            // LVP . EOBJ
            ;

// LVALUE_VAR: ID simples
LVALUE_VAR : ID 
             {
               // Verifica se "x" vem de fora e marca para captura se necessário
               verifica_e_captura($1.c[0]); 
             }
           ;

// ATRIB: Regras de atribuição
ATRIB : LVALUE_VAR '=' E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + $3.c + "=";
        }
      | LVALUE_PROP '=' E
        {
          // usa RA temporário para esconder __t*
          $$.c = "<{" + $1.c + SET_PROP($1, $3.c) + "}>";
        }
      | LVALUE_VAR MAIS_IGUAL E
        {
          checa_simbolo( $1.c[0], true );
          $$.c = $1.c + GET($1.c) + $3.c + "+" + "=";
        }
      | LVALUE_PROP MAIS_IGUAL E
        {
          $$.c = "<{" 
               + $1.c
               + GET_NOME($1.tb) + GET_NOME($1.tk)
               + GET_PROP($1)
               + $3.c + "+"
               + "[=]"
               + "}>";
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
      | LVALUE_VAR MAIS_MAIS // ver se vale a pena botar em atrib toda essa tropa
        {
          checa_simbolo($1.c[0], true);
          $$.c = GET($1.c) + $1.c + GET($1.c) + "1" + "+" + "=" + "^";
        }
      | LVALUE_VAR MENOS_MENOS
        {
          checa_simbolo($1.c[0], true);
          $$.c = GET($1.c) + $1.c + GET($1.c) + "1" + "-" + "=" + "^";
        }
      | MAIS_MAIS LVALUE_VAR
        {
          checa_simbolo($2.c[0], true);
          $$.c = $2.c + GET($2.c) + "1" + "+" + "=";
        }
      | MENOS_MENOS LVALUE_VAR
        {
          checa_simbolo($2.c[0], true);
          $$.c = $2.c + GET($2.c) + "1" + "-" + "=";
        }
      | LVALUE_PROP MAIS_MAIS
        {
          // pós-incremento com RA temporário
          $$.c = "<{"
               + $1.c
               + GET_PROP($1)
               + GET_NOME($1.tb) + GET_NOME($1.tk)
               + GET_PROP($1) + "1" + "+" + "[=]" + "^"
               + "}>";
        }
      | LVALUE_PROP MENOS_MENOS
        {
          // pós-decremento com RA temporário
          $$.c = "<{"
               + $1.c
               + GET_PROP($1)
               + GET_NOME($1.tb) + GET_NOME($1.tk)
               + GET_PROP($1) + "1" + "-" + "[=]" + "^"
               + "}>";
        }
      | MAIS_MAIS LVALUE_PROP
        {
          // pré-incremento
          $$.c = "<{"
               + $2.c
               + GET_NOME($2.tb) + GET_NOME($2.tk)
               + GET_PROP($2) + "1" + "+" + "[=]"
               + "}>";
        }
      | MENOS_MENOS LVALUE_PROP
        {
          // pré-decremento
          $$.c = "<{"
               + $2.c
               + GET_NOME($2.tb) + GET_NOME($2.tk)
               + GET_PROP($2) + "1" + "-" + "[=]"
               + "}>";
        }
      | F
      ;

// F: Fatores. A base de uma expressão. 
F : LVALUE      { $$.c = GET_LVALUE_VAL($1); }
  | F '(' L_ARGS ')' 
    {
      $$.c = $3.c + to_string($3.n_args) + $1.c + "$";
    }
  | TRUE    { $$.c = $1.c; } 
  | FALSE   { $$.c = $1.c; } 
  | CDOUBLE
  | CINT
  | CSTRING
  | '(' E ')'   { $$.c = $2.c; }
  | '-' F       { $$.c = vector<string>{"0"} + $2.c + "-"; }
  | '+' F       { $$.c = vector<string>{"0"} + $2.c + "+"; } 
  | BLVAZIO
    { 
      $$.c = vector<string>{"{}"}; 
    }
  | '{' O_CAMPOS '}' 
    { 
       $$.c = vector<string>{"{}"} + $2.c; 
    }
  | '[' A_ELEMS ']'  
    {
       $$.c = vector<string>{"[]"} + $2.c; 
    }
  | FUNCTION 
    { capturas_escopo.push_back(vector<string>{}); }
    '(' 
    { ts.push_back( map< string, Simbolo >{} ); } 
    LISTA_PARAMS 
    ')' 
    { alinhamento_blocos.push_back(1); }
    '{' CMDs '}'
    {
       string lbl_func = gera_label("func_anon");
       string def_lbl = ":" + lbl_func;

       vector<string> caps = capturas_escopo.back();
       capturas_escopo.pop_back();
       
       // DEBUG: Mostra todas as capturas
      //  cerr << "DEBUG FUNCTION: " << caps.size() << " capturas" << endl;
      //  for(size_t i = 0; i < caps.size(); i++) {
      //      cerr << "  captura[" << i << "]=[" << caps[i] << "] tamanho=" << caps[i].size() << endl;
      //  }
       
       vector<string> codigo_captura = vector<string>{"{}"};
       for(string var : caps) {
           string var_limpa = trim_str(var);
           
           // CORREÇÃO AQUI: Criar a chave completa antes de somar ao vetor
           string chave = "'" + var_limpa + "'";
           
           // Agora somamos a 'chave' inteira como um único elemento do vetor
           codigo_captura = codigo_captura + 
                            chave + 
                            (var_limpa + "@") + 
                            "[<=]";
       }

       $$.c = vector<string>{"{}"} + 
              "'&funcao'" + lbl_func + "[<=]" +
              "'captura'" + codigo_captura + "[<=]";

       funcoes = funcoes + def_lbl + $5.c + $9.c + 
                 "undefined" + "@" + "'&retorno'" + "@" + "~";

       ts.pop_back();
       alinhamento_blocos.pop_back();
    };


// O_CAMPOS agora é estritamente NÃO-VAZIO. 
// Isso ajuda o parser: se ele vê "ID :", ele sabe que É um objeto, e não um bloco.
O_CAMPOS : O_CAMPOS ',' O_CAMPO { $$.c = $1.c + $3.c; }
         | O_CAMPO              { $$.c = $1.c; }
         ;

O_CAMPO : ID ':' E 
          {
            $$.c = vector<string>{ "'" + $1.c[0] + "'" } + $3.c + "[<=]";
          }
        ;

A_ELEMS : A_ELEMS ',' E 
          { 
            // $1.c já tem o código dos elementos anteriores (já com [<=]).
            // $1.n_args é o número de elementos anteriores.
            int idx = $1.n_args;
            
            // Gera: indice valor [<=]
            $$.c = $1.c + to_string(idx) + $3.c + "[<=]";
            $$.n_args = idx + 1;
          }
        | E 
          { 
            // Primeiro elemento (índice 0)
            $$.c = vector<string>{"0"} + $1.c + "[<=]";
            $$.n_args = 1;
          }
        | /* vazio */ 
          { 
            $$.clear(); 
            $$.n_args = 0; 
          }
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