%{
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <map>

using namespace std;

string lexema;
int token;

void P();
void A();
void PRINT();
void E();
void E_linha();
void T();
void T_linha();
void POW();
void FAT();
void FAT_AUX();
void U();
void F();
void ID_AUX();
void ARGS();
void ARGF();
void V();
void print(string);
void casa( int esperado );
int next_token();
string nome_token( int token );
void erro (string s);

enum { _ID = 256, _NUM, _STR, _PRINT };

map<int,string> nome_tokens = {
  { _NUM, "num" },
  { _ID, "nome de identificador" },
  { _STR, "string" },
  { _PRINT, "print"}
};

%}

/* Coloque aqui definições regulares */

WS	    [ \t\n\r]
DIGITO  [0-9]
LETRA   [A-Za-z_]
NUM     {DIGITO}+("."{DIGITO}+)?
ID      {LETRA}({LETRA}|{DIGITO})*
STR     \"([^\"\n\\]|(\\\")|\"\"|"\\\\")+\"
PRINT   [pP][rR][iI][nN][tT]



%%

{WS}	                  { /* ignora espaços, tabs e '\n' */ } 
{PRINT}                { lexema = yytext; return _PRINT; }
{NUM}                  { lexema = yytext; return _NUM; }
{STR}                  { lexema = yytext; return _STR; }
{ID}                   { lexema = yytext; return _ID; }




.     { lexema = yytext; return *yytext; }


%%

// Adicione esta função junto com as outras
void erro(string s) {
    cerr << s << endl;
    exit(1);
}

int next_token() {
    return yylex();
}

void print(string s) {
    cout << s << " ";
}

string nome_token( int token ) {
  if( nome_tokens.find( token ) != nome_tokens.end() )
    return nome_tokens[token];
  else {
    string r;
    
    r = token;
    return r;
  }
}


void casa( int esperado ) {
  if( token == esperado )
    token = next_token();
  else {
      cout << "Esperado " << nome_token( esperado ) 
	   << " , encontrado: " << nome_token( token ) << endl;
    exit( 1 );
  }
}

void V() {

    switch (token) {
        case (_ID): {
            A();
            casa(';');
            V();
            break;
        }
        

        case (_PRINT): {
            PRINT();
            casa(';');
            V();
            break;
        }
        
        
        default:
            break; 
    }
}


void A() { 
    // Gram: A -> id { p( lexema ); } = E { print ( “= ^” ); } 

    string temp = lexema; // Guardamos o lexema pois a função 'casa' altera o seu valor.
    casa( _ID );
    print( temp );
    casa( '=' );
    E();
    print( "= ^" );

}

void PRINT() {
    //Gram: PRINT -> print E { p ( “print #” ) ; } 

    casa (_PRINT);
    E();
    print ("print #");

}

void E() {
    // Gram: E -> T E’

    T();
    E_linha();

}

void E_linha() {
    // Gram:
    // E’ -> + T { p (“+”) ; } E’ |
    //       - T { p (“-”) ; } E’ |
    //       ε

    switch( token ) { // depende se token for + ou - 
        case '+' :
            casa( '+' ); 
            T();
            print( "+" ); 
            E_linha(); 
            break;

        case '-' : 
            casa( '-' ); 
            T(); 
            print( "-"); 
            E_linha(); 
            break;
    }
}

void T() { 
    // Gram: T -> U T’

    U();
    T_linha();
}

void T_linha() {
    // Gram: T’ -> * U { p ( “*” ) } T’| 
    //             / U { p ( “/” ) } T’| 
	//             ε

    switch( token ) {
        case '*' : 
            casa( '*' ); 
            U(); 
            print( "*" ); 
            T_linha(); 
            break;

        case '/' : 
            casa( '/' ); 
            U(); 
            print( "/" ); 
            T_linha(); 
            break;
  }
}

void U() {

    // Gram: U -> - U | 
    //            + U | 
    //            POW 

    if (token == '-') {
        casa('-');
        print("0");
        U(); 
        print("-");
    }
    else if (token == '+') {
        casa('+');
        U(); // Chamada recursiva
    }
    else {
        POW(); 
    }
}


void POW() {
    // Gram: POW -> FAT ^ POW | FAT
    FAT();
    if (token == '^') {
        casa( '^' );
        POW();
        print("power #");
    }
}

void FAT() {
    // Gram: FAT -> F FAT_AUX
    F();
    FAT_AUX();
}

void FAT_AUX() {
    // Gram: FAT_AUX -> ! { p ( “fat #” ) ; } 
    //                  | ε

    if (token == '!') {
        casa ('!');
        print ("fat #");
    }
}



void F() {
    // Gram: F -> num | 
    //            id ID_AUX | 
    //            string | 
    //            ( E ) 

    switch( token ) {
        case _NUM : {
            string temp = lexema;
            casa( _NUM ); 
            print( temp ); }
            break;
        case _ID : {
            ID_AUX(); 
            } 
            break;
        case _STR: {
            string temp = lexema;
            casa (_STR);
            print ( temp ); }
            break;
        case '(': {
            casa( '(' );
            E(); 
            casa( ')' ); }
            break;
        default:
            erro( "Operando esperado, encontrado " + lexema );
    }
}

void ID_AUX() {

    // Gram: ID_AUX -> ( ARGS ) | 
    //                 ( ) | 
    //                 ε 


    string temp = lexema;
    casa(_ID);
    if (token == '(') {
        casa( '(' );
        if (token != ')') { 
            ARGS();
        }
        print (temp);
        print (" #");
        casa ( ')' );
    }
    else { // se não for função, vem pra cá 
        print(temp);
        print(" @");
    }
}

void ARGS() {
    // Gram: ARGS -> E ARGF
    E();
    ARGF();
}

void ARGF() {
    // Gram:  ARGF -> ,E ARGF |
    //                ε
    if (token == ',') {
        casa(',');
        E();
        ARGF();
    }
}

int main() {
    token = next_token();
    V();

    return 0;
}
