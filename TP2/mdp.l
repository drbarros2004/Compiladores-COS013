%{ 
// Máquia=na de pilha do Moodle
#include <stdlib.h>
#include <math.h>
#include <map>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

struct Valor {
    bool isNumber;
    double valNum;
    string valStr;
    
    Valor(): isNumber(true), valNum(0), valStr("") {}
    Valor(double valNum): isNumber(true), valNum(valNum), valStr("") {}
    Valor(string valStr): isNumber(false), valNum(0), valStr(valStr) {}
};

enum TOKEN { CDOUBLE = 256, CSTR, ID };

int linha = 1;
int coluna = 1;
string lexema;

typedef void (*Funcao)();

vector<Valor> pilha;
map<string,Valor> var;

int token(int);
void erro(string msg);
void remove_escape( string& );
%}

DIGITO  [0-9]
LETRA   [A-Za-z_]
DOUBLE  {DIGITO}+("."{DIGITO}+)?
ID      {LETRA}({LETRA}|{DIGITO})*
STR   \"([^\"\n\\]|(\\\")|\"\"|"\\\\")+\"

%%

"\t"       { coluna += 4; }
" "        { coluna++; }
"\n"     { linha++; coluna = 1; }

{DOUBLE}   { return token( CDOUBLE ); }
{STR}     { return token( CSTR ); }


{ID}       { return token( ID ); }

.          { return token( *yytext ); }

%%

int token( int tk ) {  
  lexema = yytext; 
  if( tk == CSTR )
    remove_escape( lexema );

  coluna += strlen( yytext ); 

  return tk;
}

void replace_all( string& subject, const  string& search, const  string& replace ) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

void remove_escape( string& st ) {
    st = st.substr( 1, lexema.size() - 2 );
    replace_all( st, "\"\"", "\"" );
    replace_all( st, "\\\"", "\"" );
    replace_all( st, "\\\\", "\\" );
}

inline void push( Valor valor ) {
  pilha.push_back( valor );
}

inline void push( double valor ) {
  push( Valor( valor ) );
}

inline void push( string valor ) {
  push( Valor( valor ) );
}

inline Valor pop() {
  if( pilha.size() <= 0 )
    erro( "Tentou desempilhar mas a pilha está vazia" );
  
  Valor temp = pilha.back();
  pilha.pop_back();
  
  return temp;
}

inline double pop_number() {
  Valor temp = pop();
  
  if( !temp.isNumber )
    erro( "Esperado um operando numérico, encontrado \"" + temp.valStr + "\"" );
    
  return temp.valNum;
}

inline string pop_string() {
  Valor temp = pop();
  
  if( temp.isNumber )
    erro( "Esperado um operando string, encontrado " + to_string( temp.valNum ) );
    
  return temp.valStr;
}

inline ostream& operator << ( ostream& o, const Valor& valor ) {
  if( valor.isNumber )
      o << "Num: " << valor.valNum;
    else
      o << "Str: " << valor.valStr;
      
  return o;
}

inline ostream& operator << ( ostream& o, const map<string,Valor>& v ) {
  for( auto x : v ) 
    o << "|" << x.first << " ==> " << x.second << "|" << endl;
  return o;
}

inline ostream& operator << ( ostream& o, const vector<Valor>& v ) {
  for( unsigned int i = 0; i < 10 && i < v.size(); i++ ) 
    o << "|" << v[i] << "|" << endl;
  return o;
}

inline void erro( string msg ) {
  cout << "=== Erro: " << msg << " ===" <<endl;
  cout << "=== Vars ===" << endl << var;
  cout << "=== Pilha ===" << endl << pilha;
  exit( 0 ); 
}

double fatorial( int n ) {
  if( n < 0 ) 
    erro( "Fatorial somente está definido para n >= 0" );
  else if( n == 0 )
    return 1;
  else
    return n * fatorial( n - 1 );
}

map<string, Funcao> func = {
  { "max", []() { double b = pop_number();
                  double a = pop_number();
                  push( a > b ? a : b ); } },
  { "power", []() { double b = pop_number();
                    double a = pop_number();
                    push( pow( a, b ) ); } },
  { "print", []() { cout << pop() << endl; } }, 
  { "fat", []() { push( fatorial( pop_number() ) ); } }, 
  { "dtos", []() { push( to_string( pop_number() ) ); } }
};

int main() {
    int token = yylex();
    auto p = func.begin();
    string nome;
    double op1, op2;
    Valor val;
    
    cout << "=== Console ===" << endl;
    
    while( token != 0 ) {
        switch( token ) {
            case CDOUBLE:
                push( stod( lexema ) ); 
                break;
                
            case ID:
                push( lexema );
                break;
                
            case CSTR:
                push( lexema );
                break;
                
            case '#': 
                if( (p = func.find( nome = pop_string() )) == func.end() ) {
                erro( "Funcao não definida: " + nome );
                }
                
                (p->second)();
                break;
                
            case '@':
                push( var[pop_string()] );
                break;
                
            case '=':
                val = pop();
                var[pop_string()] = val;
                push( val );
                break;
                
            case '^' :
              pop();
            break;    
                
            case '+':
                val = pop();
                
                if( val.isNumber ) 
                push( pop_number() + val.valNum );
                else  
                push( pop_string() + val.valStr );
                
                break;
                
            case '-':
                op2 = pop_number();
                op1 = pop_number();
                
                push( op1 - op2 ); 
                break;
                
            case '/':
                op2 = pop_number();
                op1 = pop_number();
                
                push( op1 / op2 ); 
                break;
                
            case '*':
                op2 = pop_number();
                op1 = pop_number();
                
                push( op1 * op2 ); 
                break;  
              
            default:
                erro( "Instrução ou operador inválido: " + lexema );
        }
        
        token = yylex();
    }
    
    cout << "=== Vars ===" << endl << var;
    cout << "=== Pilha ===" << endl << pilha;
    
    return 0;
}