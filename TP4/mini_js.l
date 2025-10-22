%{
#include <string>  
#include <vector>  
#include <sstream>  
#include <iterator> 
#include <algorithm> 

int token( int tk );
string lexema;

vector<string> tokeniza(const string& str) {
    stringstream ss(str);
    istream_iterator<string> begin(ss), end;
    return vector<string>(begin, end);
}

string ltrim(const string& s) {
    size_t start = s.find_first_not_of(" \n\r\t\f\v");
    return (start == string::npos) ? "" : s.substr(start);
}

string rtrim(const string& s) {
    size_t end = s.find_last_not_of(" \n\r\t\f\v");
    return (end == string::npos) ? "" : s.substr(0, end + 1);
}

string trim(const string& s) {
    return rtrim(ltrim(s));
}

string trim(const string& s, const string& chars) {
     string res = s;
     res.erase(0, res.find_first_not_of(chars));
     res.erase(res.find_last_not_of(chars) + 1);
     return trim(res);
}
%}

D  	        [0-9]
L   	    [A-Za-z_]
INT	        {D}+
DOUBLE      {D}+"."{D}+([Ee][+\-]?{D}+)?
UNDERLINE   "_"
ID          ({L}|{UNDERLINE}|\$)({L}|{D}|{UNDERLINE})*
STRING      (\"([^\n\r\"\\]*|\\.|\"\")*\")|(\'([^\n\r\'\\]*|\\.|\'\')*\')

%%

"\t"       { coluna += 4; }
" "        { coluna++; }
"\n"	   { linha++; coluna = 1; }

"asm{"[^}\n]*"}"  {  
                   // Pega o conteúdo de dentro das chaves (+4 para pular "asm{")
                   lexema = trim( yytext + 4, "{}" ); 
                   // Tokeniza o conteúdo por espaços
                   yylval.c = tokeniza( lexema ); 
                   coluna += strlen( yytext ); 
                   yylval.linha = linha;
                   yylval.coluna = coluna;
                   return ASM; 
                 }


"&&"	   { return token( AND ); }
"||"	   { return token( OR ); }
"<="	   { return token( ME_IG ); }
">="	   { return token( MA_IG ); }
"!="	   { return token( DIF ); }
"=="	   { return token( IGUAL ); }

"+="       { return token( MAIS_IGUAL ); }
"++"       { return token( MAIS_MAIS ); }

"return"   { return token( RETURN ); }
"function" { return token( FUNCTION ); }
"const"    { return token( CONST ); }
"var"      { return token( VAR ); }
"let"      { return token( LET ); }
"if"       { return token( IF ); }
"else"     { return token( ELSE ); }
"true"     { return token( TRUE ); }   
"false"    { return token( FALSE ); } 
    /* "print"    { return token( PRINT ); } */
"for"      { return token( FOR ); }
"while"    { return token( WHILE ); }

{STRING}   {return token ( CSTRING );}

{DOUBLE}   { return token( CDOUBLE ); }
{INT}	   { return token( CINT ); } 

{ID}       { return token( ID ); }

.          { return token( *yytext ); }

%%

int token( int tk ) {  
    yylval.c = vector<string>{ yytext };
    coluna += strlen( yytext ); 
    
    yylval.linha = linha;
    yylval.coluna = coluna;

    return tk;
}