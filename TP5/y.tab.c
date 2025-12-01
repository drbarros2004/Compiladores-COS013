/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "mini_js.y"

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


#line 267 "y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ID = 258,
    IF = 259,
    ELSE = 260,
    LET = 261,
    CONST = 262,
    VAR = 263,
    FOR = 264,
    WHILE = 265,
    CDOUBLE = 266,
    CSTRING = 267,
    CINT = 268,
    AND = 269,
    OR = 270,
    ME_IG = 271,
    MA_IG = 272,
    DIF = 273,
    IGUAL = 274,
    MAIS_IGUAL = 275,
    MAIS_MAIS = 276,
    MENOS_MENOS = 277,
    MENOS_IGUAL = 278,
    RETURN = 279,
    FUNCTION = 280,
    ASM = 281,
    TRUE = 282,
    FALSE = 283,
    SETA = 284,
    FPL = 285
  };
#endif
/* Tokens.  */
#define ID 258
#define IF 259
#define ELSE 260
#define LET 261
#define CONST 262
#define VAR 263
#define FOR 264
#define WHILE 265
#define CDOUBLE 266
#define CSTRING 267
#define CINT 268
#define AND 269
#define OR 270
#define ME_IG 271
#define MA_IG 272
#define DIF 273
#define IGUAL 274
#define MAIS_IGUAL 275
#define MAIS_MAIS 276
#define MENOS_MENOS 277
#define MENOS_IGUAL 278
#define RETURN 279
#define FUNCTION 280
#define ASM 281
#define TRUE 282
#define FALSE 283
#define SETA 284
#define FPL 285

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);





#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  79
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   480

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  49
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  45
/* YYNRULES -- Number of rules.  */
#define YYNRULES  116
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  206

#define YYUNDEFTOK  2
#define YYMAXUTOK   285


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    39,     2,     2,
      42,    43,    37,    35,    47,    36,    41,    38,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    31,    46,
      33,    32,    34,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    40,     2,    48,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    44,     2,    45,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   224,   224,   227,   231,   232,   236,   237,   238,   239,
     240,   243,   242,   259,   260,   261,   262,   266,   272,   275,
     278,   278,   301,   302,   305,   331,   353,   359,   368,   387,
     409,   410,   413,   414,   420,   421,   424,   436,   452,   453,
     454,   458,   459,   463,   464,   467,   470,   471,   474,   476,
     482,   485,   486,   489,   491,   496,   499,   500,   503,   508,
     514,   529,   530,   533,   542,   558,   566,   571,   576,   581,
     594,   595,   599,   600,   601,   602,   603,   604,   605,   606,
     607,   608,   609,   610,   611,   612,   617,   622,   627,   632,
     642,   652,   661,   670,   674,   675,   679,   680,   681,   682,
     683,   684,   685,   686,   687,   691,   695,   700,   702,   705,
     699,   748,   749,   752,   758,   768,   775
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ID", "IF", "ELSE", "LET", "CONST",
  "VAR", "FOR", "WHILE", "CDOUBLE", "CSTRING", "CINT", "AND", "OR",
  "ME_IG", "MA_IG", "DIF", "IGUAL", "MAIS_IGUAL", "MAIS_MAIS",
  "MENOS_MENOS", "MENOS_IGUAL", "RETURN", "FUNCTION", "ASM", "TRUE",
  "FALSE", "SETA", "FPL", "':'", "'='", "'<'", "'>'", "'+'", "'-'", "'*'",
  "'/'", "'%'", "'['", "'.'", "'('", "')'", "'{'", "'}'", "';'", "','",
  "']'", "$accept", "S", "BLVAZIO", "CMDs", "CMD", "$@1", "EMPILHA_TS",
  "EMPILHA_ALINHAMENTO", "CMD_FUNC", "$@2", "LISTA_PARAMS", "PARAMS",
  "PARAM", "CMD_RETURN", "L_ARGS", "ARGS", "OPT_TRAIL_COMMA", "CMD_FOR",
  "CMD_WHILE", "DECL", "SF", "EF", "CMD_LET", "LET_VARs", "LET_VAR",
  "CMD_VAR", "VAR_VARs", "VAR_VAR", "CMD_CONST", "CONST_VARs", "CONST_VAR",
  "CMD_IF", "LVALUE", "LVALUE_PROP", "LVALUE_VAR", "ATRIB", "E", "E_BIN",
  "F", "$@3", "$@4", "$@5", "O_CAMPOS", "O_CAMPO", "A_ELEMS", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,    58,    61,    60,    62,    43,    45,    42,    47,    37,
      91,    46,    40,    41,   123,   125,    59,    44,    93
};
# endif

#define YYPACT_NINF (-123)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-105)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     315,  -123,   -32,     8,    11,    21,   -13,    -7,  -123,  -123,
    -123,   122,   122,    45,    44,  -123,  -123,   122,   122,   367,
     367,     1,  -123,    76,   382,   315,  -123,  -123,  -123,  -123,
    -123,    40,  -123,  -123,  -123,  -123,  -123,     6,    84,  -123,
     -23,   410,    -1,   367,    56,  -123,    48,    58,  -123,    50,
      67,  -123,    54,   341,   367,  -123,     1,  -123,    19,    42,
      -1,    19,    42,  -123,    57,  -123,    68,  -123,  -123,    -1,
      -1,  -123,   -41,    74,    80,  -123,  -123,     5,  -123,  -123,
    -123,  -123,   367,  -123,  -123,   367,   367,  -123,  -123,   367,
      72,  -123,   367,   367,   367,   367,   367,   367,   367,   367,
     367,   367,   367,   367,   367,   367,   110,   367,    77,   367,
       8,   367,    11,   367,    21,  -123,    73,  -123,  -123,    78,
    -123,    86,  -123,   367,  -123,  -123,   367,   315,  -123,   111,
    -123,  -123,  -123,  -123,  -123,    43,    71,   417,   153,   441,
     441,   441,   441,   441,   441,    70,    70,  -123,  -123,  -123,
      90,  -123,    79,    85,  -123,   315,  -123,  -123,  -123,  -123,
    -123,  -123,   367,   315,  -123,   123,  -123,  -123,     9,  -123,
    -123,  -123,   367,  -123,   135,    95,  -123,   123,   113,   103,
     101,  -123,  -123,  -123,   315,   367,   109,   367,  -123,   123,
    -123,  -123,   116,  -123,  -123,   121,  -123,   315,   131,   315,
    -123,   315,   227,   271,  -123,  -123
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    65,     0,     0,     0,     0,     0,     0,    98,   100,
      99,     0,     0,     0,   107,    96,    97,     0,     0,   116,
       0,    11,    13,     0,    17,     2,     5,    14,    15,     8,
       9,     0,    38,    39,    40,     7,    94,    62,    61,    70,
       0,    71,    93,     0,    48,    45,    47,     0,    55,    57,
      53,    50,    52,    44,     0,   107,     0,   104,    91,    87,
       0,    92,    88,    29,     0,    20,     0,    62,    61,   103,
     102,   115,     0,     0,     0,     3,    18,     0,   112,     1,
       4,     6,     0,    89,    90,     0,     0,    85,    86,     0,
       0,    10,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    31,     0,     0,
       0,     0,     0,     0,     0,    42,     0,    41,    43,     0,
      28,     0,   108,     0,   106,   101,     0,     0,   105,     0,
      69,    67,    68,    66,    16,    62,    61,    79,    78,    76,
      75,    77,    74,    72,    73,    80,    81,    82,    83,    84,
       0,    64,     0,    34,    33,     0,    49,    46,    58,    56,
      54,    51,     0,     0,    18,    23,   114,   113,     0,   111,
      63,    95,    35,    30,    59,     0,    37,    23,    26,     0,
      34,    25,    12,    32,     0,    44,     0,     0,   109,    35,
      22,    60,     0,    19,    27,     0,    24,     0,     0,     0,
      36,     0,     0,     0,   110,    21
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -123,  -123,     0,  -122,   -24,  -123,   -10,  -123,  -123,  -123,
     -21,  -123,   -36,  -123,  -123,  -123,    -3,  -123,  -123,   127,
    -123,    -4,  -123,    75,  -123,  -123,    69,  -123,  -123,    81,
    -123,  -123,  -123,   112,   125,  -123,   -11,   369,    51,  -123,
    -123,  -123,  -123,    53,  -123
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    23,    57,    25,    26,    76,   127,   198,    27,   121,
     179,   180,   181,    28,   152,   153,   173,    29,    30,    31,
     116,   117,    32,    45,    46,    33,    51,    52,    34,    48,
      49,    35,    36,    37,    38,    39,    40,    41,    42,    66,
     165,   195,    77,    78,    72
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      24,    80,    64,    90,    74,   168,   123,   124,    71,    73,
      43,    44,     1,     2,    47,     3,     4,     5,     6,     7,
       8,     9,    10,    91,    50,    24,    82,    83,    84,    53,
      11,    12,   108,    13,    14,    54,    15,    16,    85,   105,
     106,   107,   118,   119,    17,    18,    75,    65,     1,    19,
     128,    20,   129,    21,   182,    22,     8,     9,    10,   -62,
     -62,   -62,    60,    60,    83,    84,    11,    12,    69,    70,
      55,   130,    15,    16,   131,   132,    79,   202,   133,   203,
      17,    18,   -61,   -61,   -61,    19,    81,    20,   109,    56,
     111,    63,    87,    88,   150,   110,   154,   112,   156,   113,
     158,   114,   160,   120,    86,    87,    88,   102,   103,   104,
     122,   126,   166,   151,    74,   167,    89,   125,   134,   162,
     155,   163,   171,    58,    61,     1,   178,    24,   164,    67,
      67,   174,   172,     8,     9,    10,    59,    62,   170,   176,
     184,   185,    68,    68,    80,   187,   188,    55,   189,    15,
      16,   175,   193,   196,   177,    24,   186,    17,    18,   197,
     191,   183,    19,    24,    20,   199,    56,    92,    24,    94,
      95,    96,    97,   200,   118,   201,   194,   190,    80,    80,
     115,   192,   169,   161,    24,   157,    98,    99,   100,   101,
     102,   103,   104,   159,     0,     0,     0,    24,     0,    24,
       0,    24,    24,    24,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   136,
       1,     2,     0,     3,     4,     5,     6,     7,     8,     9,
      10,     0,     0,     0,     0,     0,     0,     0,    11,    12,
       0,    13,    14,     0,    15,    16,     0,     0,     0,     0,
       0,     0,    17,    18,     0,     0,     0,    19,     0,    20,
       0,    21,   204,    22,     1,     2,     0,     3,     4,     5,
       6,     7,     8,     9,    10,     0,     0,     0,     0,     0,
       0,     0,    11,    12,     0,    13,    14,     0,    15,    16,
       0,     0,     0,     0,     0,     0,    17,    18,     0,     0,
       0,    19,     0,    20,     0,    21,   205,    22,     1,     2,
       0,     3,     4,     5,     6,     7,     8,     9,    10,     0,
       0,     0,     0,     0,     0,     0,    11,    12,     0,    13,
      14,     0,    15,    16,     1,     0,     0,     3,     4,     5,
      17,    18,     8,     9,    10,    19,     0,    20,     0,    21,
       0,    22,    11,    12,     0,     0,    55,     0,    15,    16,
       1,     0,     0,     0,     0,     0,    17,    18,     8,     9,
      10,    19,     0,    20,     0,    56,     0,     0,    11,    12,
       0,     0,    55,     0,    15,    16,  -104,  -104,  -104,  -104,
    -104,  -104,    17,    18,     0,     0,     0,    19,  -104,    20,
       0,    56,     0,     0,     0,  -104,  -104,     0,     0,  -104,
    -104,  -104,     0,  -104,    92,    93,    94,    95,    96,    97,
       0,     0,     0,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,    98,    99,   100,   101,   102,   103,   104,
      98,    99,   100,   101,   102,   103,   104,  -105,  -105,  -105,
    -105,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,  -105,  -105,   100,   101,   102,   103,
     104
};

static const yytype_int16 yycheck[] =
{
       0,    25,    13,    26,     3,   127,    47,    48,    19,    20,
      42,     3,     3,     4,     3,     6,     7,     8,     9,    10,
      11,    12,    13,    46,     3,    25,    20,    21,    22,    42,
      21,    22,    43,    24,    25,    42,    27,    28,    32,    40,
      41,    42,    53,    54,    35,    36,    45,     3,     3,    40,
      45,    42,    47,    44,    45,    46,    11,    12,    13,    40,
      41,    42,    11,    12,    21,    22,    21,    22,    17,    18,
      25,    82,    27,    28,    85,    86,     0,   199,    89,   201,
      35,    36,    40,    41,    42,    40,    46,    42,    32,    44,
      32,    46,    21,    22,   105,    47,   107,    47,   109,    32,
     111,    47,   113,    46,    20,    21,    22,    37,    38,    39,
      42,    31,   123,     3,     3,   126,    32,    43,    46,    46,
      43,    43,    43,    11,    12,     3,     3,   127,    42,    17,
      18,   155,    47,    11,    12,    13,    11,    12,    48,   163,
       5,    46,    17,    18,   168,    32,    43,    25,    47,    27,
      28,   162,    43,   189,   164,   155,   177,    35,    36,    43,
     184,   172,    40,   163,    42,    44,    44,    14,   168,    16,
      17,    18,    19,   197,   185,    44,   187,   180,   202,   203,
      53,   185,   129,   114,   184,   110,    33,    34,    35,    36,
      37,    38,    39,   112,    -1,    -1,    -1,   197,    -1,   199,
      -1,   201,   202,   203,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
       3,     4,    -1,     6,     7,     8,     9,    10,    11,    12,
      13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,
      -1,    24,    25,    -1,    27,    28,    -1,    -1,    -1,    -1,
      -1,    -1,    35,    36,    -1,    -1,    -1,    40,    -1,    42,
      -1,    44,    45,    46,     3,     4,    -1,     6,     7,     8,
       9,    10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    21,    22,    -1,    24,    25,    -1,    27,    28,
      -1,    -1,    -1,    -1,    -1,    -1,    35,    36,    -1,    -1,
      -1,    40,    -1,    42,    -1,    44,    45,    46,     3,     4,
      -1,     6,     7,     8,     9,    10,    11,    12,    13,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    -1,    24,
      25,    -1,    27,    28,     3,    -1,    -1,     6,     7,     8,
      35,    36,    11,    12,    13,    40,    -1,    42,    -1,    44,
      -1,    46,    21,    22,    -1,    -1,    25,    -1,    27,    28,
       3,    -1,    -1,    -1,    -1,    -1,    35,    36,    11,    12,
      13,    40,    -1,    42,    -1,    44,    -1,    -1,    21,    22,
      -1,    -1,    25,    -1,    27,    28,    14,    15,    16,    17,
      18,    19,    35,    36,    -1,    -1,    -1,    40,    26,    42,
      -1,    44,    -1,    -1,    -1,    33,    34,    -1,    -1,    37,
      38,    39,    -1,    41,    14,    15,    16,    17,    18,    19,
      -1,    -1,    -1,    16,    17,    18,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    33,    34,    35,    36,    37,    38,    39,
      33,    34,    35,    36,    37,    38,    39,    16,    17,    18,
      19,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,    33,    34,    35,    36,    37,    38,
      39
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     6,     7,     8,     9,    10,    11,    12,
      13,    21,    22,    24,    25,    27,    28,    35,    36,    40,
      42,    44,    46,    50,    51,    52,    53,    57,    62,    66,
      67,    68,    71,    74,    77,    80,    81,    82,    83,    84,
      85,    86,    87,    42,     3,    72,    73,     3,    78,    79,
       3,    75,    76,    42,    42,    25,    44,    51,    82,    83,
      87,    82,    83,    46,    85,     3,    88,    82,    83,    87,
      87,    85,    93,    85,     3,    45,    54,    91,    92,     0,
      53,    46,    20,    21,    22,    32,    20,    21,    22,    32,
      26,    46,    14,    15,    16,    17,    18,    19,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    85,    32,
      47,    32,    47,    32,    47,    68,    69,    70,    85,    85,
      46,    58,    42,    47,    48,    43,    31,    55,    45,    47,
      85,    85,    85,    85,    46,    82,    83,    86,    86,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      85,     3,    63,    64,    85,    43,    85,    72,    85,    78,
      85,    75,    46,    43,    42,    89,    85,    85,    52,    92,
      48,    43,    47,    65,    53,    85,    53,    55,     3,    59,
      60,    61,    45,    85,     5,    46,    59,    32,    43,    47,
      65,    53,    70,    43,    85,    90,    61,    43,    56,    44,
      53,    44,    52,    52,    45,    45
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    49,    50,    51,    52,    52,    53,    53,    53,    53,
      53,    54,    53,    53,    53,    53,    53,    53,    55,    56,
      58,    57,    59,    59,    60,    60,    61,    61,    62,    62,
      63,    63,    64,    64,    65,    65,    66,    67,    68,    68,
      68,    69,    69,    70,    70,    71,    72,    72,    73,    73,
      74,    75,    75,    76,    76,    77,    78,    78,    79,    80,
      80,    81,    81,    82,    82,    83,    84,    84,    84,    84,
      85,    85,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    86,    86,    86,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    87,    87,    87,    88,    89,    90,
      87,    91,    91,    92,    93,    93,    93
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     2,     1,     2,     1,     1,     1,
       2,     0,     5,     1,     1,     1,     3,     1,     0,     0,
       0,    11,     2,     0,     3,     1,     1,     3,     3,     2,
       2,     0,     3,     1,     0,     1,     9,     5,     1,     1,
       1,     1,     1,     1,     0,     2,     3,     1,     1,     3,
       2,     3,     1,     1,     3,     2,     3,     1,     3,     5,
       7,     1,     1,     4,     3,     1,     3,     3,     3,     3,
       1,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     2,     2,     2,     2,
       2,     2,     2,     1,     1,     4,     1,     1,     1,     1,
       1,     3,     2,     2,     1,     3,     3,     0,     0,     0,
      10,     3,     1,     3,     3,     1,     0
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2:
#line 224 "mini_js.y"
         { print( resolve_enderecos( yyvsp[0].c + "." + funcoes ) ); }
#line 1748 "y.tab.c"
    break;

  case 4:
#line 231 "mini_js.y"
                 { yyval.c = yyvsp[-1].c + yyvsp[0].c; }
#line 1754 "y.tab.c"
    break;

  case 5:
#line 232 "mini_js.y"
                 { yyval.c = yyvsp[0].c; }
#line 1760 "y.tab.c"
    break;

  case 10:
#line 241 "mini_js.y"
      { yyval.c = yyvsp[-1].c + "^"; }
#line 1766 "y.tab.c"
    break;

  case 11:
#line 243 "mini_js.y"
      { 
        // Ação para incrementar alinhamento_blocos
        if (!alinhamento_blocos.empty()) {
          alinhamento_blocos.back()++;
        }
      }
#line 1777 "y.tab.c"
    break;

  case 12:
#line 250 "mini_js.y"
      { 
        ts.pop_back(); 
        
        // Ação para decrementar alinhamento_blocos
        if (!alinhamento_blocos.empty()) {
          alinhamento_blocos.back()--;
        }
        yyval.c = "<{" + yyvsp[-1].c + "}>"; 
      }
#line 1791 "y.tab.c"
    break;

  case 13:
#line 259 "mini_js.y"
          { yyval.clear(); }
#line 1797 "y.tab.c"
    break;

  case 16:
#line 263 "mini_js.y"
      { 
        yyval.c = yyvsp[-2].c + yyvsp[-1].c + "^"; 
      }
#line 1805 "y.tab.c"
    break;

  case 17:
#line 267 "mini_js.y"
      { 
        yyval.clear(); // Bloco vazio não gera código nenhum!
      }
#line 1813 "y.tab.c"
    break;

  case 18:
#line 272 "mini_js.y"
             { ts.push_back( map< string, Simbolo >{} ); }
#line 1819 "y.tab.c"
    break;

  case 19:
#line 275 "mini_js.y"
                      { alinhamento_blocos.push_back(1); }
#line 1825 "y.tab.c"
    break;

  case 20:
#line 278 "mini_js.y"
                       { declara_var( Var, yyvsp[0].c[0], yyvsp[0].linha, yyvsp[0].coluna ); }
#line 1831 "y.tab.c"
    break;

  case 21:
#line 282 "mini_js.y"
           { 
             string lbl_endereco_funcao = gera_label( "func_" + yyvsp[-9].c[0] );
             string definicao_lbl_endereco_funcao = ":" + lbl_endereco_funcao;
             
             yyval.c = yyvsp[-9].c + "&" + yyvsp[-9].c + "{}"  + "=" + "'&funcao'" +
                    lbl_endereco_funcao + "[=]" + "^";
                    
             funcoes = funcoes + definicao_lbl_endereco_funcao
                     + yyvsp[-5].c   // LISTA_PARAMS
                     + yyvsp[-1].c  // CMDs
                     + "undefined" + "@" + "'&retorno'" + "@"+ "~";
                     
             ts.pop_back();
             alinhamento_blocos.pop_back(); 
           }
#line 1851 "y.tab.c"
    break;

  case 22:
#line 301 "mini_js.y"
                                      { yyval.c = yyvsp[-1].c; yyval.i = yyvsp[-1].i; }
#line 1857 "y.tab.c"
    break;

  case 23:
#line 302 "mini_js.y"
               { yyval.clear(); yyval.i = 0; }
#line 1863 "y.tab.c"
    break;

  case 24:
#line 306 "mini_js.y"
       {
          // Recupera o índice do argumento atual
          int idx = yyvsp[-2].i;

          // 1. Declara na TS (tempo de compilação) para evitar captura indevida
          declara_var(Var, yyvsp[0].c[0], yyvsp[0].linha, yyvsp[0].coluna);

          // 2. Gera código: y & y arguments @ idx [@] = ^
          // Nota: $1.c vem primeiro (código dos parâmetros anteriores)
          yyval.c = yyvsp[-2].c + 
                 yyvsp[0].c + "&" + yyvsp[0].c + "arguments" + "@" + to_string(idx) + "[@]" + "=" + "^";
          
          // 3. Suporte a Valor Default (Opcional, mas recomendado)
          if( yyvsp[0].valor_default.size() > 0 ) {
             string lbl = gera_label("fim_def");
             string def_lbl = ":" + lbl;
             yyval.c += yyvsp[0].c + "@" + "undefined" + "@" + "==" + 
                     "!" + lbl + "?" + 
                     yyvsp[0].c + yyvsp[0].valor_default + "=" + "^" + 
                     def_lbl;
          }
          
          // Incrementa contador
          yyval.i = idx + 1;
       }
#line 1893 "y.tab.c"
    break;

  case 25:
#line 332 "mini_js.y"
       {
          // Primeiro parâmetro (Índice 0)
          declara_var(Var, yyvsp[0].c[0], yyvsp[0].linha, yyvsp[0].coluna);
          
          // Gera código: y & y arguments @ 0 [@] = ^
          yyval.c = yyvsp[0].c + "&" + yyvsp[0].c + "arguments" + "@" + "0" + "[@]" + "=" + "^";
          
          // Default
          if( yyvsp[0].valor_default.size() > 0 ) {
             string lbl = gera_label("fim_def");
             string def_lbl = ":" + lbl;
             yyval.c += yyvsp[0].c + "@" + "undefined" + "@" + "==" + 
                     "!" + lbl + "?" + 
                     yyvsp[0].c + yyvsp[0].valor_default + "=" + "^" + 
                     def_lbl;
          }
          
          yyval.i = 1;
       }
#line 1917 "y.tab.c"
    break;

  case 26:
#line 354 "mini_js.y"
      { 
        yyval.c = yyvsp[0].c; 
        yyval.valor_default.clear();
        yyval.linha = yyvsp[0].linha; yyval.coluna = yyvsp[0].coluna;
      }
#line 1927 "y.tab.c"
    break;

  case 27:
#line 360 "mini_js.y"
      {
        yyval.c = yyvsp[-2].c;
        yyval.valor_default = yyvsp[0].c; 
        yyval.linha = yyvsp[-2].linha; yyval.coluna = yyvsp[-2].coluna;
      }
#line 1937 "y.tab.c"
    break;

  case 28:
#line 369 "mini_js.y"
              { 
                if (alinhamento_blocos.empty()) {
                  yyerror("Erro: 'return' encontrado fora de uma função.");
                  YYABORT;
                }
                
                int num_pops_blocos = alinhamento_blocos.back() - 1;
                yyval.c.clear(); // Limpa o "return" de $1.c
                
                yyval.c += yyvsp[-1].c; 
                
                // Gera N-1 '}>' para fechar os blocos
                for(int i = 0; i < num_pops_blocos; i++) {
                  yyval.c += "}>";
                }
                
                yyval.c = yyval.c + "'&retorno'" + "@" + "~";
              }
#line 1960 "y.tab.c"
    break;

  case 29:
#line 388 "mini_js.y"
              {
                if (alinhamento_blocos.empty()) {
                  yyerror("Erro: 'return' encontrado fora de uma função.");
                  YYABORT;
                }
                
                int num_pops_blocos = alinhamento_blocos.back() - 1;
                yyval.c.clear(); // Limpa o "return" de $1.c

                // Adiciona o valor 'undefined' antes
                yyval.c += vector<string>{"undefined"} + "@";

                // Gera N-1 '}>' para fechar os blocos
                for(int i = 0; i < num_pops_blocos; i++) {
                  yyval.c += "}>";
                }
                
                yyval.c = yyval.c + "'&retorno'" + "@" + "~";
              }
#line 1984 "y.tab.c"
    break;

  case 30:
#line 409 "mini_js.y"
                              { yyval.c = yyvsp[-1].c; yyval.n_args = yyvsp[-1].n_args; }
#line 1990 "y.tab.c"
    break;

  case 31:
#line 410 "mini_js.y"
                              { yyval.clear(); yyval.n_args = 0; }
#line 1996 "y.tab.c"
    break;

  case 32:
#line 413 "mini_js.y"
                  { yyval.c = yyvsp[-2].c + yyvsp[0].c; yyval.n_args = yyvsp[-2].n_args + 1; }
#line 2002 "y.tab.c"
    break;

  case 33:
#line 414 "mini_js.y"
                  { yyval.c = yyvsp[0].c; yyval.n_args = 1; }
#line 2008 "y.tab.c"
    break;

  case 36:
#line 425 "mini_js.y"
        {
          string lbl_fim_for = gera_label( "fim_for" );
          string lbl_condicao_for = gera_label( "condicao_for" );
          string def_lbl_condicao_for = ":" + lbl_condicao_for;
          string def_lbl_fim_for = ":" + lbl_fim_for;

          yyval.c = yyvsp[-6].c + def_lbl_condicao_for + yyvsp[-4].c + JUMP_FALSE(lbl_fim_for) +
                 yyvsp[0].c + yyvsp[-2].c + lbl_condicao_for + JUMP + def_lbl_fim_for;
        }
#line 2022 "y.tab.c"
    break;

  case 37:
#line 437 "mini_js.y"
          {
            string lbl_fim_while = gera_label( "fim_while" );
            string lbl_condicao_while = gera_label( "condicao_while" );
            string def_lbl_condicao_while = ":" + lbl_condicao_while;
            string def_lbl_fim_while = ":" + lbl_fim_while;

            yyval.c = def_lbl_condicao_while 
            + yyvsp[-2].c + JUMP_FALSE(lbl_fim_while) // verificar a condição e pula para o fim se for falsa
            + yyvsp[0].c                             // executa os comandos dentro do WHILE
            + lbl_condicao_while + JUMP        // pula sempre para a condição
            + def_lbl_fim_while;
          }
#line 2039 "y.tab.c"
    break;

  case 43:
#line 463 "mini_js.y"
       { yyval.c = yyvsp[0].c + "^"; }
#line 2045 "y.tab.c"
    break;

  case 44:
#line 464 "mini_js.y"
       { yyval.clear(); }
#line 2051 "y.tab.c"
    break;

  case 45:
#line 467 "mini_js.y"
                       { yyval.c = yyvsp[0].c; }
#line 2057 "y.tab.c"
    break;

  case 46:
#line 470 "mini_js.y"
                                { yyval.c = yyvsp[-2].c + yyvsp[0].c; }
#line 2063 "y.tab.c"
    break;

  case 48:
#line 475 "mini_js.y"
          { yyval.c = declara_var( Let, yyvsp[0].c[0], yyvsp[0].linha, yyvsp[0].coluna ); }
#line 2069 "y.tab.c"
    break;

  case 49:
#line 477 "mini_js.y"
          {
            yyval.c = declara_var( Let, yyvsp[-2].c[0], yyvsp[-2].linha, yyvsp[-2].coluna ) +
                   yyvsp[-2].c + yyvsp[0].c + "=" + "^"; }
#line 2077 "y.tab.c"
    break;

  case 50:
#line 482 "mini_js.y"
                       { yyval.c = yyvsp[0].c; }
#line 2083 "y.tab.c"
    break;

  case 51:
#line 485 "mini_js.y"
                                { yyval.c = yyvsp[-2].c + yyvsp[0].c; }
#line 2089 "y.tab.c"
    break;

  case 53:
#line 490 "mini_js.y"
          { yyval.c = declara_var( Var, yyvsp[0].c[0], yyvsp[0].linha, yyvsp[0].coluna ); }
#line 2095 "y.tab.c"
    break;

  case 54:
#line 492 "mini_js.y"
          {  yyval.c = declara_var( Var, yyvsp[-2].c[0], yyvsp[-2].linha, yyvsp[-2].coluna ) +
                    yyvsp[-2].c + yyvsp[0].c + "=" + "^"; }
#line 2102 "y.tab.c"
    break;

  case 55:
#line 496 "mini_js.y"
                            { yyval.c = yyvsp[0].c; }
#line 2108 "y.tab.c"
    break;

  case 56:
#line 499 "mini_js.y"
                                      { yyval.c = yyvsp[-2].c + yyvsp[0].c; }
#line 2114 "y.tab.c"
    break;

  case 58:
#line 504 "mini_js.y"
            { yyval.c = declara_var( Const, yyvsp[-2].c[0], yyvsp[-2].linha, yyvsp[-2].coluna ) +
                     yyvsp[-2].c + yyvsp[0].c + "=" + "^"; }
#line 2121 "y.tab.c"
    break;

  case 59:
#line 509 "mini_js.y"
         {
           string lbl_fim_if = gera_label( "fim_if" );
           string def_lbl_fim_if = ":" + lbl_fim_if;
           yyval.c = yyvsp[-2].c + "!" + lbl_fim_if  + "?" + yyvsp[0].c + def_lbl_fim_if;
         }
#line 2131 "y.tab.c"
    break;

  case 60:
#line 515 "mini_js.y"
          {
            string lbl_fim_if = gera_label( "fim_if" );
            string lbl_else_if = gera_label( "else_if" );
            string def_lbl_fim_if = ":" + lbl_fim_if;
            string def_lbl_else_if = ":" + lbl_else_if;

            yyval.c = yyvsp[-4].c + "!" + lbl_else_if + "?" +
                   yyvsp[-2].c + lbl_fim_if + "#" +
                   def_lbl_else_if + yyvsp[0].c +
                   def_lbl_fim_if ;
          }
#line 2147 "y.tab.c"
    break;

  case 63:
#line 534 "mini_js.y"
            {
              // Avalia base e índice uma única vez em temporários
              yyval.tb = novo_temp();
              yyval.tk = novo_temp();
              yyval.c  = SET_TEMP(yyval.tb, yyvsp[-3].c)
                    + SET_TEMP(yyval.tk, yyvsp[-1].c);
              yyval.is_prop = true;
            }
#line 2160 "y.tab.c"
    break;

  case 64:
#line 543 "mini_js.y"
            {
              // Chave por nome deve ser string literal
              yyval.tb = novo_temp();
              yyval.tk = novo_temp();
              vector<string> key = vector<string>{ "'" + yyvsp[0].c[0] + "'" };
              yyval.c  = SET_TEMP(yyval.tb, yyvsp[-2].c)
                    + SET_TEMP(yyval.tk, key);
              yyval.is_prop = true;
            }
#line 2174 "y.tab.c"
    break;

  case 65:
#line 559 "mini_js.y"
             {
               // Verifica se "x" vem de fora e marca para captura se necessário
               verifica_e_captura(yyvsp[0].c[0]); 
             }
#line 2183 "y.tab.c"
    break;

  case 66:
#line 567 "mini_js.y"
        {
          checa_simbolo( yyvsp[-2].c[0], true );
          yyval.c = yyvsp[-2].c + yyvsp[0].c + "=";
        }
#line 2192 "y.tab.c"
    break;

  case 67:
#line 572 "mini_js.y"
        {
          // usa RA temporário para esconder __t*
          yyval.c = "<{" + yyvsp[-2].c + SET_PROP(yyvsp[-2], yyvsp[0].c) + "}>";
        }
#line 2201 "y.tab.c"
    break;

  case 68:
#line 577 "mini_js.y"
        {
          checa_simbolo( yyvsp[-2].c[0], true );
          yyval.c = yyvsp[-2].c + GET(yyvsp[-2].c) + yyvsp[0].c + "+" + "=";
        }
#line 2210 "y.tab.c"
    break;

  case 69:
#line 582 "mini_js.y"
        {
          yyval.c = "<{" 
               + yyvsp[-2].c
               + GET_NOME(yyvsp[-2].tb) + GET_NOME(yyvsp[-2].tk)
               + GET_PROP(yyvsp[-2])
               + yyvsp[0].c + "+"
               + "[=]"
               + "}>";
        }
#line 2224 "y.tab.c"
    break;

  case 72:
#line 599 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2230 "y.tab.c"
    break;

  case 73:
#line 600 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2236 "y.tab.c"
    break;

  case 74:
#line 601 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2242 "y.tab.c"
    break;

  case 75:
#line 602 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2248 "y.tab.c"
    break;

  case 76:
#line 603 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2254 "y.tab.c"
    break;

  case 77:
#line 604 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2260 "y.tab.c"
    break;

  case 78:
#line 605 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2266 "y.tab.c"
    break;

  case 79:
#line 606 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2272 "y.tab.c"
    break;

  case 80:
#line 607 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2278 "y.tab.c"
    break;

  case 81:
#line 608 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2284 "y.tab.c"
    break;

  case 82:
#line 609 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2290 "y.tab.c"
    break;

  case 83:
#line 610 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2296 "y.tab.c"
    break;

  case 84:
#line 611 "mini_js.y"
                            { yyval.c = yyvsp[-2].c + yyvsp[0].c + yyvsp[-1].c; }
#line 2302 "y.tab.c"
    break;

  case 85:
#line 613 "mini_js.y"
        {
          checa_simbolo(yyvsp[-1].c[0], true);
          yyval.c = GET(yyvsp[-1].c) + yyvsp[-1].c + GET(yyvsp[-1].c) + "1" + "+" + "=" + "^";
        }
#line 2311 "y.tab.c"
    break;

  case 86:
#line 618 "mini_js.y"
        {
          checa_simbolo(yyvsp[-1].c[0], true);
          yyval.c = GET(yyvsp[-1].c) + yyvsp[-1].c + GET(yyvsp[-1].c) + "1" + "-" + "=" + "^";
        }
#line 2320 "y.tab.c"
    break;

  case 87:
#line 623 "mini_js.y"
        {
          checa_simbolo(yyvsp[0].c[0], true);
          yyval.c = yyvsp[0].c + GET(yyvsp[0].c) + "1" + "+" + "=";
        }
#line 2329 "y.tab.c"
    break;

  case 88:
#line 628 "mini_js.y"
        {
          checa_simbolo(yyvsp[0].c[0], true);
          yyval.c = yyvsp[0].c + GET(yyvsp[0].c) + "1" + "-" + "=";
        }
#line 2338 "y.tab.c"
    break;

  case 89:
#line 633 "mini_js.y"
        {
          // pós-incremento com RA temporário
          yyval.c = "<{"
               + yyvsp[-1].c
               + GET_PROP(yyvsp[-1])
               + GET_NOME(yyvsp[-1].tb) + GET_NOME(yyvsp[-1].tk)
               + GET_PROP(yyvsp[-1]) + "1" + "+" + "[=]" + "^"
               + "}>";
        }
#line 2352 "y.tab.c"
    break;

  case 90:
#line 643 "mini_js.y"
        {
          // pós-decremento com RA temporário
          yyval.c = "<{"
               + yyvsp[-1].c
               + GET_PROP(yyvsp[-1])
               + GET_NOME(yyvsp[-1].tb) + GET_NOME(yyvsp[-1].tk)
               + GET_PROP(yyvsp[-1]) + "1" + "-" + "[=]" + "^"
               + "}>";
        }
#line 2366 "y.tab.c"
    break;

  case 91:
#line 653 "mini_js.y"
        {
          // pré-incremento
          yyval.c = "<{"
               + yyvsp[0].c
               + GET_NOME(yyvsp[0].tb) + GET_NOME(yyvsp[0].tk)
               + GET_PROP(yyvsp[0]) + "1" + "+" + "[=]"
               + "}>";
        }
#line 2379 "y.tab.c"
    break;

  case 92:
#line 662 "mini_js.y"
        {
          // pré-decremento
          yyval.c = "<{"
               + yyvsp[0].c
               + GET_NOME(yyvsp[0].tb) + GET_NOME(yyvsp[0].tk)
               + GET_PROP(yyvsp[0]) + "1" + "-" + "[=]"
               + "}>";
        }
#line 2392 "y.tab.c"
    break;

  case 94:
#line 674 "mini_js.y"
                { yyval.c = GET_LVALUE_VAL(yyvsp[0]); }
#line 2398 "y.tab.c"
    break;

  case 95:
#line 676 "mini_js.y"
    {
      yyval.c = yyvsp[-1].c + to_string(yyvsp[-1].n_args) + yyvsp[-3].c + "$";
    }
#line 2406 "y.tab.c"
    break;

  case 96:
#line 679 "mini_js.y"
            { yyval.c = yyvsp[0].c; }
#line 2412 "y.tab.c"
    break;

  case 97:
#line 680 "mini_js.y"
            { yyval.c = yyvsp[0].c; }
#line 2418 "y.tab.c"
    break;

  case 101:
#line 684 "mini_js.y"
                { yyval.c = yyvsp[-1].c; }
#line 2424 "y.tab.c"
    break;

  case 102:
#line 685 "mini_js.y"
                { yyval.c = vector<string>{"0"} + yyvsp[0].c + "-"; }
#line 2430 "y.tab.c"
    break;

  case 103:
#line 686 "mini_js.y"
                { yyval.c = vector<string>{"0"} + yyvsp[0].c + "+"; }
#line 2436 "y.tab.c"
    break;

  case 104:
#line 688 "mini_js.y"
    { 
      yyval.c = vector<string>{"{}"}; 
    }
#line 2444 "y.tab.c"
    break;

  case 105:
#line 692 "mini_js.y"
    { 
       yyval.c = vector<string>{"{}"} + yyvsp[-1].c; 
    }
#line 2452 "y.tab.c"
    break;

  case 106:
#line 696 "mini_js.y"
    {
       yyval.c = vector<string>{"[]"} + yyvsp[-1].c; 
    }
#line 2460 "y.tab.c"
    break;

  case 107:
#line 700 "mini_js.y"
    { capturas_escopo.push_back(vector<string>{}); }
#line 2466 "y.tab.c"
    break;

  case 108:
#line 702 "mini_js.y"
    { ts.push_back( map< string, Simbolo >{} ); }
#line 2472 "y.tab.c"
    break;

  case 109:
#line 705 "mini_js.y"
    { alinhamento_blocos.push_back(1); }
#line 2478 "y.tab.c"
    break;

  case 110:
#line 707 "mini_js.y"
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

       yyval.c = vector<string>{"{}"} + 
              "'&funcao'" + lbl_func + "[<=]" +
              "'captura'" + codigo_captura + "[<=]";

       funcoes = funcoes + def_lbl + yyvsp[-5].c + yyvsp[-1].c + 
                 "undefined" + "@" + "'&retorno'" + "@" + "~";

       ts.pop_back();
       alinhamento_blocos.pop_back();
    }
#line 2520 "y.tab.c"
    break;

  case 111:
#line 748 "mini_js.y"
                                { yyval.c = yyvsp[-2].c + yyvsp[0].c; }
#line 2526 "y.tab.c"
    break;

  case 112:
#line 749 "mini_js.y"
                                { yyval.c = yyvsp[0].c; }
#line 2532 "y.tab.c"
    break;

  case 113:
#line 753 "mini_js.y"
          {
            yyval.c = vector<string>{ "'" + yyvsp[-2].c[0] + "'" } + yyvsp[0].c + "[<=]";
          }
#line 2540 "y.tab.c"
    break;

  case 114:
#line 759 "mini_js.y"
          { 
            // $1.c já tem o código dos elementos anteriores (já com [<=]).
            // $1.n_args é o número de elementos anteriores.
            int idx = yyvsp[-2].n_args;
            
            // Gera: indice valor [<=]
            yyval.c = yyvsp[-2].c + to_string(idx) + yyvsp[0].c + "[<=]";
            yyval.n_args = idx + 1;
          }
#line 2554 "y.tab.c"
    break;

  case 115:
#line 769 "mini_js.y"
          { 
            // Primeiro elemento (índice 0)
            yyval.c = vector<string>{"0"} + yyvsp[0].c + "[<=]";
            yyval.n_args = 1;
          }
#line 2564 "y.tab.c"
    break;

  case 116:
#line 775 "mini_js.y"
          { 
            yyval.clear(); 
            yyval.n_args = 0; 
          }
#line 2573 "y.tab.c"
    break;


#line 2577 "y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[+*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 782 "mini_js.y"


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
