//
// Prüft auf Gültigkeit eines Variablennamens
//
bool is_varname(char const * YYCURSOR)
{

/*!re2c
re2c:define:YYCTYPE = char;
re2c:yyfill:enable = 0;
varname	= [A-Za-z_]([A-Za-z0-9_])*;

varname	{ if (YYCURSOR!=0 and *(YYCURSOR)=='\0') return true; return false; }
*/
	return false;
}

