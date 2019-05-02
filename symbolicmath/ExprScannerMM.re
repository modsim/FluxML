//
// Ein spezieller Scanner fuer die Kurznotationen im Messmodell.
// Der Scanner erfordert eine Uebersetzung der Datei mit re2c.
// 
// @return Typ des eingelesenen Tokens
//
int et_lex_mm()
{
	char *q = 0;
	char *p = et_inputstring;
#define YYCTYPE         char
#define YYCURSOR        p
#define YYLIMIT         p
#define YYMARKER        q
#define YYFILL(n)
start:
/*!re2c
ws	= [ \t\n\r\v\f]+;
let	= [A-Za-z_];
dig	= [0-9];
id	= let(let|dig)*;
num	= dig+;
pair	= "("num","num")";
tuple	= "("num","num(","num)*")";
subr	= (num|num"-"num);
range	= "["subr(","subr)*"]";
range2	= "["subr(","subr)*":"subr(","subr)*"]";
ngen	= id"#"[01x]+;
nnmr1h  = id"#P"num((","|",P")num)*;
nnmr13c	= id"#"(("S"|"DL"|"DR"|"DD"|"T")num(","num)*)(","(("S"|"DL"|"DR"|"DD"|"T")num(","num)*))*;
nms	= id range?"#M"num(","num)*;
ms2	= id range2?"#M"pair(","pair)*;
tms	= id range?"#M"tuple(","tuple)*;
mid	= (ngen|nnmr1h|nnmr13c|nms|ms2|tms);
dbln	= dig*"."?num([eE][-+]?num)?;
ld	= "log2("|"ld(";
lg	= "log10("|"lg(";
ln	= "log("|"ln(";

mid	{
	if (YYCURSOR == 0 or YYCURSOR-et_inputstring==0)
		return 0; // Scanner-Fehler
	strncpy(et_text_token,et_inputstring,YYCURSOR-et_inputstring);
	et_text_token[YYCURSOR-et_inputstring] = '\0';
	et_inputstring = YYCURSOR;
	return T_ID;
	}
dbln	{
	char * end_ptr;
	if (YYCURSOR == 0 or YYCURSOR-et_inputstring==0)
		return 0; // Scanner-Fehler
	strncpy(et_text_token,et_inputstring,YYCURSOR-et_inputstring);
	et_text_token[YYCURSOR-et_inputstring] = '\0';
	et_text_dblval = strtod(et_text_token, &end_ptr);
	et_inputstring = YYCURSOR;
	return T_NUM;
	}
"^"	{ et_inputstring = YYCURSOR; return T_POW; }
"+"	{ et_inputstring = YYCURSOR; return T_ADD; }
"-"	{ et_inputstring = YYCURSOR; return T_SUB; }
"*"	{ et_inputstring = YYCURSOR; return T_MUL; }
"/"	{ et_inputstring = YYCURSOR; return T_DIV; }
"("	{ et_inputstring = YYCURSOR; return T_BRL; }
")"	{ et_inputstring = YYCURSOR; return T_BRR; }
"="	{ et_inputstring = YYCURSOR; return T_EQ;  }
"<="	{ et_inputstring = YYCURSOR; return T_LEQ; }
"<"	{ et_inputstring = YYCURSOR; return T_LT;  }
">="	{ et_inputstring = YYCURSOR; return T_GEQ; }
">"	{ et_inputstring = YYCURSOR; return T_GT;  }
"!="	{ et_inputstring = YYCURSOR; return T_NEQ; }
","	{ et_inputstring = YYCURSOR; return T_COMMA; }
"abs("	{ et_inputstring = YYCURSOR-1; return T_ABS; }
"exp("	{ et_inputstring = YYCURSOR-1; return T_EXP; }
"max("	{ et_inputstring = YYCURSOR-1; return T_MAX; }
"min("	{ et_inputstring = YYCURSOR-1; return T_MIN; }
"sqrt("	{ et_inputstring = YYCURSOR-1; return T_SQRT; }
ln	{ et_inputstring = YYCURSOR-1; return T_LOG; }
ld	{ et_inputstring = YYCURSOR-1; return T_LOG2; }
lg	{ et_inputstring = YYCURSOR-1; return T_LOG10; }
"sqr("	{ et_inputstring = YYCURSOR-1; return T_SQR; }
"diff("	{ et_inputstring = YYCURSOR-1; return T_DIFF; }
"sin("	{ et_inputstring = YYCURSOR-1; return T_SIN; }
"cos("	{ et_inputstring = YYCURSOR-1; return T_COS; }
ws	{
	et_inputstring = YYCURSOR;
	goto start;
	}
"\000"	{ return 0; }
*/
	fTHROW(ExprParserException,"scanner error");
	return -1;
}

