%{
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <string>
#include "Error.h"
#include "ExprTree.h"
#include "ExprParser.h"
#include "cstringtools.h" // strtoutf8

using namespace flux::symb;

/** Prototyp für den Error-Handler */
static void et_error(char const *);
/** Text-Wert des Tokens */
static char   et_text_token[256];
/** double-Wert des Tokens */
static double et_text_dblval;

/** Eingabe-String des Parsers (globale Variable) */
char * et_inputstring;
/** Position im Eingabe-String des Parsers (globale Variable) */
int et_inputstring_pos;

/** Wurzelknoten des ge-parse-ten Ausdrucks (globale Variable) */
ExprTree * et_root;
/** Funktionspointer auf die Scanner-Funktion */
int (*et_lex)();

%}

/*
 * Bison-spezifisch: Namenspräfix kann per Direktive gegeben werden. Es ist
 * besser, wenn das Präfix per Parameter "-p" angegeben wird, um Kompatibilität
 * zu (b)yacc zu gewährleisten.
 * %name-prefix="et_"
 */

%union
{
	ExprTree * expr_val;
	char * str_val;
	double dbl_val;
}

%token T_POW T_ADD T_SUB T_MUL T_DIV T_BRL T_BRR
%token T_EQ  T_LEQ T_LT T_GEQ T_GT T_NEQ
%token T_ABS T_EXP T_MAX T_MIN T_SQRT T_COMMA
%token T_LOG T_LOG2 T_LOG10 T_SQR T_DIFF T_SIN T_COS
%token <str_val> T_ID
%token <dbl_val> T_NUM
%type <expr_val> reln expr term fact prim id num

%%

myexpr :
	reln			{ et_root = $1; }

/* Vergleichsoperatoren haben die niedrigste Priorität */
reln :
	expr			{ $$ = $1; }
	| reln T_EQ  expr	{ $$ = new ExprTree(et_op_eq , $1,$3); }
	| reln T_LEQ expr	{ $$ = new ExprTree(et_op_leq, $1,$3); }
	| reln T_LT  expr	{ $$ = new ExprTree(et_op_lt , $1,$3); }
	| reln T_GEQ expr	{ $$ = new ExprTree(et_op_geq, $1,$3); }
	| reln T_GT  expr	{ $$ = new ExprTree(et_op_gt , $1,$3); }
	| reln T_NEQ expr	{ $$ = new ExprTree(et_op_neq, $1,$3); }

/* Addition und Subtraktion binden stärker als Vergleich */
expr :	term			{ $$ = $1; }
	| expr T_ADD term	{ $$ = new ExprTree(et_op_add, $1,$3); }
	| expr T_SUB term	{ $$ = new ExprTree(et_op_sub, $1,$3); }
	;

/* Unäre Operatoren und Mul./Div. binden stärker als Add./Sub. */
term :	fact			{ $$ = $1; }
	| T_SUB fact		{ $$ = new ExprTree(et_op_uminus, $2,0); }
	| T_ADD fact		{ $$ = $2; /* unäres Plus */ }
	| term T_MUL fact	{ $$ = new ExprTree(et_op_mul, $1,$3); }
	| term T_DIV fact	{ $$ = new ExprTree(et_op_div, $1,$3); }
	;

/* Potenz bindet stärker als Mul./Div. */
fact :	prim			{ $$ = $1; }
	| fact T_POW prim	{ $$ = new ExprTree(et_op_pow, $1,$3); }
	;

/* "Atome" und Klammerung bindet am stärksten */
prim :	T_BRL reln T_BRR	{ $$ = $2; }
	| T_ABS T_BRL reln T_BRR		{ $$ = new ExprTree(et_op_abs,  $3,  0); }
	| T_EXP T_BRL reln T_BRR		{ $$ = new ExprTree(et_op_exp,  $3,  0); }
	| T_MAX T_BRL reln T_COMMA reln T_BRR	{ $$ = new ExprTree(et_op_max,  $3, $5); }
	| T_MIN T_BRL reln T_COMMA reln T_BRR	{ $$ = new ExprTree(et_op_min,  $3, $5); }
	| T_SQRT T_BRL reln T_BRR		{ $$ = new ExprTree(et_op_sqrt, $3,  0); }
	| T_LOG2 T_BRL reln T_BRR		{ $$ = new ExprTree(et_op_log2, $3,  0); }
	| T_LOG T_BRL reln T_BRR		{ $$ = new ExprTree(et_op_log,  $3,  0); }
	| T_LOG10 T_BRL reln T_BRR		{ $$ = new ExprTree(et_op_log10,$3,  0); }
	| T_SQR T_BRL reln T_BRR		{ $$ = new ExprTree(et_op_sqr,  $3,  0); }
	| T_DIFF T_BRL reln T_COMMA id T_BRR	{ $$ = new ExprTree(et_op_diff, $3, $5); }
        | T_SIN T_BRL reln T_BRR		{ $$ = new ExprTree(et_op_sin,  $3,  0); }	
        | T_COS T_BRL reln T_BRR		{ $$ = new ExprTree(et_op_cos,  $3,  0); }	
	| id
	| num        
	;

id : T_ID			{ $$ = new ExprTree(et_text_token); }

num : T_NUM			{ $$ = new ExprTree(et_text_dblval); }

%%

/**
 * Error-Handler. Wirft eine Exception und bricht damit das Parsen ab.
 *
 * @param err Fehlermeldung
 */
static void et_error(char const * err)
{
	fTHROW(ExprParserException,err);
}

static bool is_non_op_ascii(int c)
{
	switch (c)
	{
	case '"':
	case '#':
	case '$':
	case '%':
	case '&':
	case '\'':
	case '.':
	case ':':
	case '@':
	case '[':
	case '\\':
	case ']':
	case '_':
	case '{':
	case '}':
	case '|':
		return true;
	}
	return false;
}

/**
 * Der Scanner. Zerlegt et_inputstring in Tokens und gibt deren Wert und Typ
 * zurück.
 *
 * @return Typ des eingelesenen Tokens
 */
int et_lex_default()
{
	int i;
	int c;
	char * end_ptr;
	int last_pos;

	// UTF-8 / whitespace überlesen
	do
	{
		last_pos = et_inputstring_pos;
		c = strtoutf8(et_inputstring + et_inputstring_pos, &end_ptr);
		if (c == -1)
			fTHROW(ExprParserException,"scanner error: invalid Unicode character");
		et_inputstring_pos = end_ptr - et_inputstring;
	}
	while (c < 128 && (isblank(c) || c=='\r' || c=='\n'));

	if (c == '\0')
	{
		et_text_token[0] = '\0';
		return 0; // Token ist ""; Token-Typ ist T_EOF
	}
	
	i = -1;
	switch (c)
	{
	case '+': i=T_ADD; break;
	case '-': i=T_SUB; break;
	case '*': i=T_MUL; break;
	case '/': i=T_DIV; break;
	case '^': i=T_POW; break;
	case '(': i=T_BRL; break;
	case ')': i=T_BRR; break;
	case '=': i=T_EQ;  break;
	case '<': i=(*(et_inputstring+et_inputstring_pos)=='=') ? T_LEQ : T_LT; break;
	case '>': i=(*(et_inputstring+et_inputstring_pos)=='=') ? T_GEQ : T_GT; break;
	case '!': i=(*(et_inputstring+et_inputstring_pos)=='=') ? T_NEQ :   -1; break; // es gibt kein T_NOT
	case '~': i=(*(et_inputstring+et_inputstring_pos)=='=') ? T_NEQ :   -1; break; // es gibt kein T_NOT
	case ',': i=T_COMMA; break;
	case 'a':
		if (strncmp(et_inputstring+et_inputstring_pos,"bs(",3)==0)
		{
			i = T_ABS;
			et_inputstring_pos+=2;
		}
		break;
	case 'c':
		if (strncmp(et_inputstring+et_inputstring_pos,"os(",3)==0)
		{
			i = T_COS;
			et_inputstring_pos+=2;
		}
		break;
	case 'd':
		if (strncmp(et_inputstring+et_inputstring_pos,"iff(",4)==0)
		{
			i = T_DIFF;
			et_inputstring_pos+=3;
		}
		break;
	case 'e':
		if (strncmp(et_inputstring+et_inputstring_pos,"xp(",3)==0)
		{
			i = T_EXP;
			et_inputstring_pos+=2;
		}
		break;
	case 'm':
		if (strncmp(et_inputstring+et_inputstring_pos,"ax(",3)==0)
		{
			i = T_MAX;
			et_inputstring_pos+=2;
		}
		else if (strncmp(et_inputstring+et_inputstring_pos,"in(",3)==0)
		{
			i = T_MIN;
			et_inputstring_pos+=2;
		}
		break;
	case 'l':
		// Logarithmen inkl. ihrer Alias-Namen
		if (strncmp(et_inputstring+et_inputstring_pos,"og2(",4)==0) // log2(x):=log[2](x)
		{
			i = T_LOG2;
			et_inputstring_pos+=3;
		}
		else if (strncmp(et_inputstring+et_inputstring_pos,"d(",2)==0) // ld(x):=log[2](x)
		{
			i = T_LOG2;
			et_inputstring_pos++;
		}
		else if (strncmp(et_inputstring+et_inputstring_pos,"og10(",5)==0) // log10(x):=log[10](x)
		{
			i = T_LOG10;
			et_inputstring_pos+=4;
		}
		else if (strncmp(et_inputstring+et_inputstring_pos,"g(",2)==0) // lg(x):=log[10](x)
		{
			i = T_LOG10;
			et_inputstring_pos++;
		}
		else if (strncmp(et_inputstring+et_inputstring_pos,"og(",3)==0) // log(x):=log[e](x)
		{
			i = T_LOG;
			et_inputstring_pos+=2;
		}
		else if (strncmp(et_inputstring+et_inputstring_pos,"n(",2)==0) // ln(x):=log[e](x)
		{
			i = T_LOG;
			et_inputstring_pos++;
		}
		break;
	case 's':
		if (strncmp(et_inputstring+et_inputstring_pos,"qr(",3)==0)
		{
			i = T_SQR;
			et_inputstring_pos+=2;
		}
		else if (strncmp(et_inputstring+et_inputstring_pos,"qrt(",4)==0)
		{
			i = T_SQRT;
			et_inputstring_pos+=3;
		}
                else if (strncmp(et_inputstring+et_inputstring_pos,"in(",3)==0)
		{
			i = T_SIN;
			et_inputstring_pos+=2;
		}
		break;
	}
	
	if (i != -1)
	{
		if (i == T_LEQ || i == T_GEQ || i == T_NEQ)
			et_inputstring_pos++;
		return i;
	}
	
	if (isdigit(c) || (c == '.' && isdigit(*(et_inputstring+et_inputstring_pos))))
	{
		// für double-Werte ist strtod hervorragend geeignet ...
		et_text_token[0] = '\0';

		et_inputstring_pos = last_pos; // = et_inputstring_pos-1
		et_text_dblval = strtod(et_inputstring+et_inputstring_pos, &end_ptr);
		if (end_ptr == et_inputstring+et_inputstring_pos)
		{
			et_text_dblval = 0.;
			fTHROW(ExprParserException,"scanner error: error scanning double value");
		}
		strncpy(et_text_token,
			et_inputstring + et_inputstring_pos,
			end_ptr - (et_inputstring + et_inputstring_pos)
			);
		et_text_token[end_ptr - (et_inputstring + et_inputstring_pos)] = '\0';
		et_inputstring_pos = end_ptr - et_inputstring;
		return T_NUM;
	}

	if (isalpha(c) || c > 127 || is_non_op_ascii(c))
	{
		char * prev_end_ptr;
		prev_end_ptr = et_inputstring + et_inputstring_pos;

		while ((c = strtoutf8(prev_end_ptr,&end_ptr)) > 0
				&& (isalnum(c) || c>127 || is_non_op_ascii(c)))
		{
			prev_end_ptr = end_ptr;
		}
		// end_ptr steht jetzt ggfs hinter dem Terminator; korrigieren:
		end_ptr = prev_end_ptr;
		strncpy(et_text_token,
			et_inputstring + last_pos,
			end_ptr - (et_inputstring + last_pos)
			);
		et_text_token[end_ptr - (et_inputstring + last_pos)] = '\0';
		et_inputstring_pos = end_ptr - et_inputstring;
		return T_ID;
	}
	fTHROW(ExprParserException,"scanner error");
	return 0;
}

/*
 * Scanner für das Messmodell einbinden
 * (benötigt die Token-Definitionen).
 */
#include "ExprScannerMM.inc"


