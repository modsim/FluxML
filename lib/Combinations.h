#ifndef COMBINATIONS_H
#define COMBINATIONS_H

#include <cstddef>
extern "C"
{
#include <stdint.h>
#include <limits.h>
}

class charptr_array;

/** Testen eines Bits i im Integer n */
#define TSTBIT(n,i)     (((n)>>(i))&(1>0))
/** Setzen eines Bits i im Integer n */
#define SETBIT(n,i)	((n)|(1<<(i)))
/** Löschen eines Bits i im Integer n */
#define CLRBIT(n,i)	((n)&(~(1<<(i))))

/** eine Maske mit Einsen von bit i bis Bit j (jeweils inklusive) */
#define ONEMSK(i,j)	( ~( ((1<<(i))-1)|~((1<<((j)+1))-1) ) )
/** Zählen der gesetzten Bits in einem Integer */
#define ONES(n)		count_ones(n)

/** Binäre Max-Funktion */
#define MAX2(a,b)	((a)>=(b)?(a):(b))
/** Ternäre Max-Funktion */
#define MAX3(a,b,c)	MAX2(a,MAX2(c,d))
/** Quartäre Max-Funktion */
#define MAX4(a,b,c,d)	MAX2(MAX2(a,b),MAX2(c,d))

/** Binäre Min-Funktion */
#define MIN2(a,b)	((a)<=(b)?(a):(b))
/** Ternäre Min-Funktion */
#define MIN3(a,b,c)	MIN2(a,MIN2(c,d))
/** Quartäre Min-Funktion */
#define MIN4(a,b,c,d)	MIN2(MIN2(a,b),MIN2(c,d))

/** Wert einer Binärzahl in ASCII-Darstellung */
#define BC(s)		str2bin(s)
/** Index einer Cumomer-Konfiguration */
#define IDX(s)		str2bin_rev(s)

/** Start-Konfiguration des Kolex-Algorithmus */
#define	COLEX_START(n,k)	(uint32_t)((1<<(k))-1)
/** Stopp-Konfiguration des Kolex-Algorithmus */
#define COLEX_STOP(n,k)		(uint32_t)ONEMSK((n)-(k),(n)-1)

/** Start-Konfiguration des Kolex-Algorithmus mit Maske */
#define COLEX_MASK_START(mask,k)	next_comb_colex_mask(mask,k,0)
/** Stopp-Konfiguration des Kolex-Algorithmus mit Maske */
#define COLEX_MASK_STOP(mask,k)		colex_mask_stop(mask,k)

/** Scope der Prüfsummenberechnung */
enum CheckSumScopeType {
	// reactionnetwork (Metabolite, Reaktionen, Vernetzung)
	CRC_REACTIONNETWORK = 1,
	// constraints (global+cfg)
	CRC_CONSTRAINTS = 2,
	// configuration: input
	CRC_CFG_SUBSTRATES = 4,
	// configuration / measurements: model
	CRC_CFG_MEAS_MODEL = 8,
	// configuration / measurements: data
	CRC_CFG_MEAS_DATA = 16,
	// configuration / simulation: attribute
	CRC_CFG_SIM_SETTINGS = 32,
	// configuration / simulation: model
	CRC_CFG_SIM_MODEL = 64,
	// configuration / simulation: variables
	CRC_CFG_SIM_VARIABLES = 128,
	// configuration / simulation: variables (Flusswerte)
	CRC_CFG_SIM_VARIABLES_DATA = 256,
	// alle Annotationen (info, annotation, comment, mlabel, dlabel)
	CRC_ALL_ANNOTATIONS = 512,
	// ALLES
	CRC_EVERYTHING = 1024-1
	};

/**
 * Logarithmus dualis.
 *
 * @param n Argument
 * @return Logarithmus von n zur Basis 2
 */
unsigned int log_base_2(unsigned int n);

/**
 * Berechnung der nächsten Zweierpotenz.
 *
 * @param n eine Zahl
 * @return nächste Zweierpotenz >=n
 */
unsigned int next_pwr_2(unsigned int n);

/**
 * Zweier-Potenz-Prädikat
 *
 * @param n eine Zahl
 * @return true, falls n einen Zweierpotenz ist, false sonst
 */
bool is_pwr_2(unsigned int n);


/**
 * Bestimmung der Permutation einer Isotopomer-Reaktion.
 * Beispiel: abcdef -> cdafeb => P=[2,5,0,1,4,3]
 *
 * @param L konkatenierte Edukt-Seite
 * @param R konkatenierte Produkt-Seite
 */
bool get_perm(char const * L, char const * R, int ** P);

/**
 * Bestimmung der Permutation einer Isotopomer-Reaktion.
 * Beispiel: a,b,c,d,e,f -> c,d,a,f,e,b => P=[2,5,0,1,4,3]
 *
 * @param L konkatenierte Edukt-Seite (Liste)
 * @param R konkatenierte Produkt-Seite (Liste)
 */
bool get_perm(charptr_array const & L, charptr_array const & R, int ** P);

/**
 * Bestimmung der Permutation einer Bondomer-Reaktion.
 * Weil in Bondomer-Reaktionen Bonds verschwinden können, wird eine Vorwärts-
 * und eine Rückwärts-Permutation bestimmt (für Vor- und Rück-Reaktion):
 *
 * Beispiel: #ABC + #abc -> #A?c?C?a
 *
 *  L = "ABCabc", R = "A?c?C?a" => P = (0,-1,5,-1,2,-1,3) (vorwärts)
 *  L = "A?c?C?a", R = "ABCabc" => P = (0,-1,4,6,-1,2)   (rückwärts)
 *
 * @param L konkatenierte Edukt-Seite
 * @param R konkatenierte Produkt-Seite
 * @param P Permutation für Reaktion von L nach R (genausolang wie R)
 */
bool get_bond_perm(
	char const * L,
	char const * R,
	int ** P
	) __attribute__ ((deprecated));

/**
 * Binomialkoeffizient "(n über k)".
 * Der Algorithmus verwendet den binaryGCD-Algorithmus und kommt komplett
 * mit Integer-Zahlen / Integer-Arithmetik aus:
 *
 * @param n Gesamtanzahl der Auswählbaren
 * @param k Anzahl der Ausgewählten
 * @retval Anzahl der Möglichkeiten k aus n auszuwählen
 */
uint64_t bin_coeff(uint64_t n, uint64_t k);

/**
 * Wandelt die niederen nbits Bits eines Integers n in einen
 * String um, in dem die Bits mit geringerer Signifikanz weiter
 * vorne stehen (LSB-first). Diese Funktion wird zur Generierung
 * der Cumomer-Config-Strings verwendet -- statt der 0-Bits enthält
 * der generierte String ein 'x' an der jeweiligen Position.
 *
 * @param n Integer
 * @param nbits Anzahl der zu konvertierenden Bits
 * @return allokierter String
 */
char * bin2str(uint32_t n, uint16_t nbits);

/**
 * Wie bin2str, jedoch beginnt der generierte String mit dem Bit welches
 * die höchste Signifikanz hat (MSB-first). Diese Funktion eignet sich
 * (per Konvention) NICHT für die Generierung von Cumomer-Config-Strings.
 *
 * @see bin2str
 * @param n Integer
 * @param nbits Anzahl der zu konvertierenden Bits
 * @return allokierter String
 */
char * bin2str_rev(uint32_t n, uint16_t nbits);

/**
 * Konvertiert einen String, bestehend aus den Symbolen '1', '0' und 'x'/'X'
 * in einen vorzeichenlosen Integer. Diese Funktion geht davon aus, dass der
 * String mit dem MSB beginnt (Bit mit der höchsten Signifikanz zuerst) und
 * eigenet sich deshalb (per Konvention) NICHT für die Verarbeitung von
 * Cumomer/Isotopomer-Config-Strings. Stattdessen sollte die Funktion
 * str2bin_rev verwendet werden.
 *
 * @param s String mit einer ASCII-Binärdarstellung
 * @return vorzeichenloser Integer
 */
uint32_t str2bin(char const * s);

/**
 * Wie str2bin, die Funktion geht jedoch davon aus, dass der String mit dem
 * LSB beginnt (Bit mit der niedrigsten Signifikanz). Diese Funktion eignet
 * sich (per Konvention) für die Verarbeitung von Cumomer/Isotopomer-Config-
 * Strings: Das Zeichen 'x' oder 'X' wird als '0' gewertet.
 *
 * @see str2bin
 * @param s String mit einer ASCII-Binärdarstellung
 * @return vorzeichenloser Integer
 */
uint32_t str2bin_rev(char const * s); // LSB zuerst

/**
 * Zählt die gesetzten Bits in einem vorzeichenlosen Integer.
 *
 * @param n Integer
 * @return Anzahl gesetzter Bits
 */
uint32_t count_ones(uint32_t n);

/**
 * Generiert eine Cumomer-Darstellung poolname#cfg.
 *
 * @param poolname Pool-/Metabolit-Bezeichnung
 * @param cumo Cumomer-Konfiguration (vorzeichenloser Integer)
 * @param atoms Anzahl der Atome des Pools
 * @return allokierter String mit Cumomer-Darstellung
 */
char * makeCumomerName(char const * poolname, uint32_t cumo, uint16_t atoms);

/**
 * Generiert eine EMU-Darstellung poolname_atome
 *
 * @param poolname Pool-/Metabolit-Bezeichnung
 * @param emu EMU-Konfiguration (vorzeichenloser Integer)
 * @param atoms Anzahl der Atome des Pools (optional)
 * @return allokierter String mit EMU-Darstellung
 */
char * makeEMUName(char const * poolname, uint32_t emu, uint16_t atoms = 0);

/**
 * Generiert eine Isotopomer-Darstellung poolname#cfg.
 *
 * @param poolname Pool-/Metabolit-Bezeichnung
 * @param iso Isotopomer-Konfiguration (vorzeichenloser Integer)
 * @param atoms Anzahl der Atome des Pools
 * @return allokierter String mit Isotopomer-Darstellung
 */
char * makeIsotopomerName(char const * poolname, uint32_t iso, uint16_t atoms);

//uint64_t next_comb_gray(unsigned int n, unsigned int k, bool init=false);
//uint32_t next_comb_lex(uint8_t n, uint8_t k);

/**
 * Erzeugung der "n über k" Kombinationen als gesetzte Bits in einem
 * vorzeichenlosen 32 Bit Integer. Diese Variante verwendet static-
 * Variablen, um sich den Zustand zu merken und sollte deshalb
 * nicht verwendet werden (nicht "thread-safe"; stattdessen
 * next_comb_colex_stateless verwenden!).
 *
 * @param n Größe der Gesamtheit
 * @param k Größe der Auswahl aus der Gesamtheit (0 < k <= n)
 * @param init true, falls Reinitialisierung erwünscht
 * @return nächste Kombination
 */
uint32_t next_comb_colex(uint8_t n, uint8_t k, bool init=false) __attribute__ ((deprecated));

/**
 * Erzeugung der "n über k" Kombinationen als gesetzte Bits in einem
 * vorzeichenlosen 32 Bit Integer. Jeder Aufruf gibt die, relativ zur
 * übergebenen, letzten Kombination "comb", nächste Kombination zurück.
 * Die Funktion ist zustandslos. Die lexikographisch erste Kombination
 * wird zurückgegeben, wenn die letzte Kombination die lex. größte
 * Kombination war -- oder, falls comb=0 übergeben wurde.
 *
 * @param n Größe der Gesamtheit
 * @param k Größe der Auswahl aus der Gesamtheit (0 < k <= n)
 * @param comb letzte Kombination
 * @return nächste Kombination relativ zu comb
 */
uint32_t next_comb_colex_stateless(uint8_t n, uint8_t k, uint32_t comb);

/**
 * Wie next_comb_colex_stateles, nur rückwärts.
 *
 * @param n Größe der Gesamtheit
 * @param k Größe der Auswahl aus der Gesamtheit (0 < k <= n)
 * @param comb letzte Kombination
 * @return vorherige Kombination relativ zu comb
 */
uint32_t prev_comb_colex_stateless(uint8_t n, uint8_t k, uint32_t comb);

/**
 * Wie next_comb_colex_stateless, es wird jedoch eine Bit-Maske verwendet.
 * Die Kombinationen werden nur in den gesetzten Bits der Bit-Maske erzeugt.
 * Der Parameter "n" von next_comb_colex_stateless entspricht der Anzahl
 * der in mask gesetzten Bits und wird automatisch bestimmt.
 * Beispiel:
 *
 * Maske: 1x1x11x1
 * Kombinationen bei k=2:
 * 1x1xxxxx, 1xxx1xxx, xx1x1xxx, 1xxxx1xx, xx1xx1xx,
 * xxxx11xx, 1xxxxxx1, xx1xxxx1, xxxx1xx1, xxxxx1x1
 *
 * @param mask Bitmaske (n = Anzahl der gesetzten Bits)
 * @param k Anzahl der in den Kombinationen gesetzten Bits
 * @param comb letzte Kombination
 * @return nächste Kombination relativ zu comb
 */
uint32_t next_comb_colex_mask(uint32_t mask, uint8_t k, uint32_t comb);

/**
 * Wie next_comb_colex_mask, nur rückwärts.
 *
 * @param mask Bitmaske (n = Anzahl der gesetzten Bits)
 * @param k Anzahl der in den Kombinationen gesetzten Bits
 * @param comb letzte Kombination
 * @return vorherige Kombination relativ zu comb
 */
uint32_t prev_comb_colex_mask(uint32_t mask, uint8_t k, uint32_t mcomb);

/**
 * Gibt die lexikographisch größte Kombination (=End-Kombination)
 * für next_comb_colex_mask zurück.
 *
 * @param mask Bitmaske
 * @param k Anzahl der in den Kombinationen gesetzten Bits
 * @return lexikographisch größte Kombination entsprechend mask
 */
uint32_t colex_mask_stop(uint32_t mask, uint8_t k);

/**
 * Erzeugung der "n über k" Kombinationen in lexikographischer Ordnung.
 * Die erste Kombination wird zurückgegeben, falls C[0]>=n-k oder falls
 * init = true übergeben wird. Zurückgegeben wird das Array C, dessen
 * erste k Einträge die Kombination darstellen (für Anwendungsbeispiel
 * siehe Combinations.cc)
 *
 * @param n Größe der Gesamtheit
 * @param k Größe der Auswahl aus der Gesamtheit (0 < k <= n)
 * @param C Array der Länge n
 * @param init Initialisierung erfolgt bei init=true
 * @return Array der Länge k (im Speicherplatz von C)
 * @see Combinations.cc
 */
#define COLEX_FINISHED(C,n,k)	((C)[0]==(n)-(k))
uint32_t * next_comb_colex(uint32_t n, uint32_t k, uint32_t *C, bool init=false);

/* Verschiedene Funktionen zum Verarbeiten von Bondomer-Permutationen */

/**
 * "Gewicht" einer Bondomer-Permutation
 * 
 * Zählt die Einträge einer Permutation, die != -1 sind.
 * (als "Gewicht" der Permutation)
 *
 * @param P Permutationsvektor
 * @param P_len Länge des Permutationsvektors
 * @return Gewicht der Permutation
 */
size_t bond_perm_weight(
	int * P,
	size_t P_len
	) __attribute__ ((deprecated));

/**
 * Setzt die Bits in einem uint32_t-Integer entsprechend der
 * gesetzten Bits in einer Kombination k_comb und entsprechend
 * einer Permutation P.
 *
 * @param comb Kombinations-Bit-Vektor (Integer)
 * @param P Permutationsvektor
 * @param P_len Länge des Permutationsvektors
 * @param P_weight der Wert bond_perm_weight(P,P_len)
 * @return Edukt-Seite der Vorwärtsreaktion bzw. Produkt-Seite der Rückreaktion
 */
uint32_t bond_comb_shuffle(
	uint32_t comb,
	int * P,
	size_t P_len,
	size_t P_weight
	) __attribute__ ((deprecated));

/**
 * Permutiert mit Hilfe einer Bondomer-Permutation
 *
 * @param in Konkatenierte Bits des Edukt-Seite
 * @param P Permutationsvektor zum Zusammenbau der Produkt-Seite
 * @param P_len Länge des Permutationsvektors (=Länge der Produkt-Seite)
 * @return konkatenierte Produktseite (=permutierte Eduktseite)
 */
uint32_t bond_reaction_shuffle(
	uint32_t in,
	int * P,
	size_t P_len
	) __attribute__ ((deprecated));

/**
 * Fast-Isotopomer/Cumomer-Transformation (in-place)
 *
 * Konvertiert zwischen Isotopomer- (=Vektor mit Summe 1) und kumulierter
 * Isotopomer(=Cumomer)-Darstellung. Der zweite Parameter ist optional und
 * gibt die Richtung der Transformation an.
 *
 * Isotopomer -> Cumomer:
 *    c = cumulate(v,v_len)
 *
 * Cumomer -> Isotopomer:
 *    v = cumulate(c,c_len,false);
 *
 * @param v Array
 * @param v_len Länge des Arrays v (2er-Potenz)
 * @param fwd true, falls vorwärts-Transformation (Cumulieren)
 * @return Zeiger auf Array v
 */
template< typename Stype > Stype * cumulate(
	Stype * v, size_t v_len,
	bool fwd = true
	)
{
	size_t n = v_len;
	size_t ldm, ldn = log_base_2(n);
	size_t j, m, mh, r, s, t;

	// fASSERT( IS_PWR2(v_len) );
	for (ldm = ldn; ldm >= 1; ldm--)
	{
		m = (1<<ldm);
		mh = (m>>1);
		for (r=0; r<n; r+=m)
			for (j=0; j<mh; j++)
			{
				s = n-r-j-1;
				t = s-mh;
				// wird hier das +/- durch ein XOR ersetzt,
				// entspricht das einer Reed-Muller Transformation,
				// deren Transformationsmatrix per flipud gespiegelt
				// wurde.
				if (fwd)
					v[t] += v[s];
				else
					v[t] -= v[s];
				//v[t] += fwd ? v[s] : -v[s];
			}
	}
	return v;
}

/**
 * Isotopomer-nach-Massen-Isotopomer/EMU-Konvertierung
 * Wandelt einen vollständigen Vektor von Isotopomeren in einen Vektor von
 * Massenisotopomeren um. Die Transformation kann nicht rückgängig gemacht
 * werden.
 *
 * Beispiel:
 *       iso -> 8x1-Matrix
 *       atsel -> [1,3]
 *
 * entspricht T*iso mit Matrix T:
 *    | 1 0 1 0 0 0 0 0 | = m+0
 *    | 0 1 0 1 1 0 1 0 | = m+1
 *    | 0 0 0 0 0 1 0 1 | = m+2
 *
 * @param iso vollständiger Vektor der Isotopomere
 * @param iso_len Länge des Isotopomer-Vektors
 * @param mask Bit-Maske (gesetzte Bits beschreiben die Atom-Auswahl)
 * @param massiso Array mit Massenisotopomeren (falls 0 wird allokiert)
 * @return Array mit Massenisotopomeren
 */
double * iso2mass_iso(
	double * iso, size_t iso_len,
	uint32_t mask,
	double * massiso = 0
	);

/**
 * Cumomer nach Massen-Isotopomer Umwandlung.
 *
 * @see iso2mass_iso
 * @param cumu vollständiges Array von Cumomer-Fractions
 * @param cumu_len Länge des Cumomer-Arrays
 * @param mask Bit-Maske (gesetzte Bits beschreiben die Atom-Auswahl)
 * @param massiso Array mit Massenisotopomeren (falls 0 wird allokiert)
 * @return Array mit Massenisotopomeren
 */
double * cumu2mass_iso(
	double * cumu, size_t cumu_len,
	uint32_t mask,
	double * massiso = 0
	);

/**
 * Diskrete Faltung (Cauchy-Produkt) C = A (*) B.
 * Theta(n^2) Rechenschritte.
 * Input-Side-Algorithmus.
 *
 * @param A erstes Quell-Array
 * @param A_len Länge des ersten Quell-Arrays
 * @param B zweites Quell-Array
 * @param B_len Länge des zweiten Quell-Arrays
 * @param C Zeiger auf das Ziel-Array mit Länge A_len+B_len-1 (optional)
 * @return Zeiger auf das Ergebnis-Array (möglicherweise allokierter Speicher)
 */
template< typename T > T * convolve(
	T * A, size_t A_len,
	T * B, size_t B_len,
	T * C = 0
	)
{
	size_t i,j;
	if (C == 0) C = new T[A_len+B_len-1];
	for (i=0; i<A_len+B_len-1; i++) C[i] = T(0);

	for (i=0; i<A_len; i++)
		for (j=0; j<B_len; j++)
			C[i+j] = C[i+j] + A[i]*B[j];
	return C;
}

/**
 * Binärer ggT-Algorithmus (GCD = greatest common divisor).
 * 
 * siehe Donald E. Knuth: The Art Of Computer Programming
 * Part II (Seminumerical Algorithms), S. 321
 *
 * @param u erstes Vielfaches von ggT(u,v)
 * @param v zweites Vielfaches von ggT(u,v)
 * @retval ggT(u,v)
 */
int64_t GCD(int64_t u, int64_t v);

/**
 * Binärer kgV-Algorithmus. Verwendet den binären ggT-Algorithmus.
 *
 * @param u erster Koeffizient des kgV
 * @param v zweiter Koeffizient des kgV
 * @retval kgV(u,v)
 */
inline int64_t LCM(int64_t u, int64_t v)
{
	if (u==0ll or v==0ll) return 0ll;
	return (u/GCD(u,v))*v;
}

/**
 * Ein einfacher "best rational approximation" Algorithmus.
 * Ermittelt ein Kettenbruchdarstellung und fasst sie zu einem
 * Bruch zusammen.
 *
 * @param x zu approximierende Floating-Point-Zahl
 * @param maxd Schranke für Nenner
 * @param n Zähler (Numerator)
 * @param d Nenner (Denominator)
 */
void rationalize(double x, int64_t & n, int64_t & d, int64_t maxd = INT_MAX);

/**
 * CRC-32 Prüfsummenberechnung
 *
 * @param data Zeiger auf Daten
 * @param len Anzahl der Bytes in data
 * @param crc bisherige Prüfsumme
 * @return CRC-32 Prüfsumme
 */
uint32_t update_crc32(void const * data, size_t len, uint32_t crc);

#endif

