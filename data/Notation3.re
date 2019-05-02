//
// Prüft auf Gültigkeit eines Variablennamens
//
bool is_varname(char const * s)
{
	char const *p = s;
#define YYCTYPE		char
#define YYCURSOR	p
#define YYLIMIT		p
#define YYMARKER	q
#define YYFILL(n)

/*!re2c
varname	= [A-Za-z_]([A-Za-z0-9_])*;

varname	{ if (YYCURSOR!=0 and *(YYCURSOR)=='\0') return true; return false; }
*/
	return false;
}

