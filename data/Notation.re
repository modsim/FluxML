// Scanner zur Identifizierung von Kurznotationen.
// Rueckgabewerte: MS=1,MSMS=2,NMR1H=3,NMR13C=4,generic=5,MI_MS=6
// und -1 falls ein Fehler aufgetreten ist oder die Notation nicht erkannt
// wurde
static int identify_notation(const char *YYCURSOR)
{
	const char *YYMARKER;
/*!re2c
re2c:define:YYCTYPE = char;
re2c:yyfill:enable = 0;
ws	= [ \t\n]+;
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
nms2	= id range2"#M"pair(","pair)*;
tms	= id range?"#M"tuple(","tuple)*;


nms	{
	if (YYCURSOR-et_inputstring==0)
		return -1; // scanner error
	if (YYCURSOR!=0 and *(YYCURSOR)=='\0')
		return 1;
	return -1;
	}
nms2	{
	if (YYCURSOR-et_inputstring==0)
		return -1; // scanner error
	if (YYCURSOR!=0 and *(YYCURSOR)=='\0')
		return 2;
	return -1;
	}
nnmr1h	{
	if (YYCURSOR-et_inputstring==0)
		return -1; // scanner error
	if (YYCURSOR!=0 and *(YYCURSOR)=='\0')
		return 3;
	return -1;
	}
nnmr13c	{
	if (YYCURSOR-et_inputstring==0)
		return -1; // scanner error
	if (YYCURSOR!=0 and *(YYCURSOR)=='\0')
		return 4;
	return -1;
	}
ngen	{
	if (YYCURSOR-et_inputstring==0)
		return -1; // scanner error
	if (YYCURSOR!=0 and *(YYCURSOR)=='\0')
		return 5;
	return -1;
	}
tms	{
	if (YYCURSOR-et_inputstring==0)
		return -1; // scanner error
	if (YYCURSOR!=0 and *(YYCURSOR)=='\0')
		return 6;
	return -1;
	}
*       {}
*/
	return -1;
}
