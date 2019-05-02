#ifndef CSTRINGTOOLS_H
#define CSTRINGTOOLS_H

#include <cstddef>
#include <cstdio>
#include <cstring>

/**
 * Verkürzt einen C-String inplace auf eine Maximallänge len
 * indem der Mittelteil durch ".." ersetzt wird, d.h.
 * Präfix und Suffix bleiben erhalten. Der Parameter muss
 *
 * @param str zur beschneidender String
 * @param len Länge des beschnittenen Strings, minimal 3
 */
void strtrunc_inplace(char * str, size_t len);

/**
 * In-Place links-rechts-Trim eines C-Strings.
 * Schneidet Leerzeichen / Tabulatoren am linken und rechten Ende des
 * Strings ab.
 *
 * @param str zu beschneidender String
 */
void strtrim_inplace(char * str);

/**
 * In-Place links-Trim eines C-Strings.
 * Schneidet Leerzeichen / Tabulatoren am linken Ende des Strings ab.
 *
 * @param str zu beschneidender String
 */
void strLtrim_inplace(char * str);

/**
 * In-Place rechts-Trim eines C-Strings.
 * Schneidet Leerzeichen / Tabulatoren am rechten Ende des Strings ab.
 *
 * @param str zu beschneidender String
 */
void strRtrim_inplace(char * str);

/**
 * Allokierende strdup-Funktion.
 * Die Funktion erzeugt eine Kopie vom C-String todup und legt sie in
 * einem neu angeforderten Speicherbereich ab.
 *
 * @param todup zu duplizierender C-String
 * @retval Zeiger auf den duplizierten C-String
 */
char * strdup_alloc(char const * todup);

/**
 * Ersatz für die Funktion strcasecmp.
 *
 * @param s1 erster C-String
 * @param s2 zu vergleichender, zweiter C-String
 */
int strcmp_ignorecase(char const * s1, char const * s2);

/**
 * Gibt true zurück, falls ein String mit einer angegebenen Zeichenkette
 * endet.
 *
 * @param str String
 * @param s Suffix-String
 * @return true, falls str mit Suffix-String s endet
 */
inline bool strEndsWith(char const * str, char const * s)
{
	return strcmp(str+strlen(str)-strlen(s),s) == 0;
}

/**
 * Gibt true zurück, falls ein String mit einer angegebenen Zeichenkette
 * beginnt.
 *
 * @param str String
 * @param s Präfix-String
 * @return true, falls str mit Präfix-String s beginnt
 */
inline bool strStartsWith(char const * str, char const * s)
{
	return strncmp(str,s,strlen(s)) == 0;
}

/**
 * Liest ein UTF-8 Zeichen (bis zu vier Bytes) aus einem Standard-C-String.
 * Der Aufbau der Funktion orientiert sich an strtol: Bei einem Fehler in der
 * UTF-8 Codierung wird -1 zurückgegeben und *endptr hat den Wert nptr.
 * Bei erfolgreichen Aufruf zeigt *endptr auf das nächste UTF-8 Zeichen.
 *
 * Die Funktion verarbeitet ASCII-NUL '\0' (Terminator) wie ein normales
 * UTF-8-Zeichen und gibt entsprechend 0 zurück.
 *
 * Folgender Einsatz der Funktion macht Sinn, um einen String zu verarbeiten
 * und den Terminator '\0' nicht zu überlesen:
 *
 *   char * str = (char*)input_utf8_str;
 *   while ((c = strtoutf8(str,&str)) > 0) { ... }
 *
 * @param nptr Zeiger auf das UTF-8-Zeichen / den UTF-8 String
 * @param endptr Ende-Zeiger des Zeichens (Zeiger auf nächstes UTF-8 Zeichen)
 * @param compress falls true, redundanzfreie Darstellung ohne leading bits
 * 	erstellen
 * @return Integer mit UTF-8-Code in den least significant bytes
 */
int strtoutf8(char const * nptr, char ** endptr, bool compress = false);

/**
 * Wrapper für g_fmt.
 *
 * @param s Puffer (sollte 32 Zeichen lang sein).
 * 			Wird 0-terminiert
 * @param f ein double precision float
 * @param l Puffergroesse
 * @return Anzahl der geschriebenen Bytes ohne 0 (oder -1 bei Fehler)
 */
int dbl2str(char * s, double f, size_t len);

/**
 * Formatiert ein Zeichen als einen druckbaren String
 *
 * @param ch Zeichencode
 * @return Zeiger auf static-Buffer
 */
char const * escape_chr(int ch);

/**
 * Formatiert einen double-Wert à la Matlab/Octave.
 *
 * "s"  <=> format short
 * "l"  <=> format long
 * "se" <=> format short e
 * "sl" <=> format long e
 * "sg" <=> format short g
 * "sl" <=> format long g
 * "a"  <=> automatik (wie dbl2str)
 *
 * @param buf Puffer für String (>21 Bytes)
 * @param blen Puffergröße
 * @param d double-Wert
 * @param fmt Formatierungskürzel
 */
void formatdbl(char * buf, size_t blen, double d, char const * fmt);

/**
 * Liest eine Zeile von einem Stream speichert sie inkl LF in einem
 * allokierten, mit NUL terminierten Puffer. Die Funktionalität entspricht
 * also fgets (eignet sich aber für Zeilen unbekannter Länge).
 *
 * @param stream Eingabe-Stream
 * @param max_len maximale String-Länge (0=unlimitiert)
 * @param lenp Zeiger auf String-Länge, falls lenp != 0
 * @return Zeiger auf allokierten String oder 0-Zeiger bei Fehler / Länge 0
 */
char * fgetline(FILE * stream, size_t max_len = 0, size_t * lenp = 0);

/**
 * String-Suche
 *
 * @param y Zeichenkette (Heuhaufen)
 * @param n Länge von y
 * @param x Zeichenkette (Nadel)
 * @param m Länge von x
 * @return Zeiger auf gefundene Nadel, oder 0
 */
char const * strsearch(
	char const * y,
	int n,
	char const * x,
	int m
	);

#endif
