%{
int token( int tk );
string lexema;
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

"&&"	   { return token( AND ); }
"||"	   { return token( OR ); }
"<="	   { return token( ME_IG ); }
">="	   { return token( MA_IG ); }
"!="	   { return token( DIF ); }
"=="	   { return token( IGUAL ); }

"+="       { return token( MAIS_IGUAL ); }
"++"       { return token( MAIS_MAIS ); }

"const"    { return token( CONST ); }
"var"      { return token( VAR ); }
"let"      { return token( LET ); }
"if"       { return token( IF ); }
"else"     { return token( ELSE ); }
"print"    { return token( PRINT ); }
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