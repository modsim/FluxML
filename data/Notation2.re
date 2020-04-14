// Scanner zur Identifizierung / Prüfung einer Permutationsnotation.
// Rueckgabewerte: empty=0, short=1, long=2
// und -1 falls ein Fehler aufgetreten ist oder die Notation nicht erkannt
// wurde.
int identify_perm_spec(char const * YYCURSOR)
{
	char const * YYMARKER;

	// keine Notation
	if (YYCURSOR == 0 || *YYCURSOR == '\0')
		return 0;

/*!re2c
re2c:define:YYCTYPE = char;
re2c:yyfill:enable = 0;
spc	= [ \t\n\r\v\f];
atom	= [CHNOS];
letter	= [A-Za-z];
digit	= [0-9];
alnum	= (letter|digit);
num	= [1-9]digit*;

shortn	= alnum+;
longn	= spc*atom[#]num[@]alnum+(spc+atom[#]num[@]alnum+)*spc*;

shortn	{ if (YYCURSOR!=0 and *(YYCURSOR)=='\0') return 1; return -1; }
longn	{ if (YYCURSOR!=0 and *(YYCURSOR)=='\0') return 2; return -1; }
*       {}
*/
	return -1;
}

