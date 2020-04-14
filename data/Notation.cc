#include <cstdlib>
#include <climits>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include "Error.h"
#include "cstringtools.h"
#include "charptr_array.h"
#include "BitArray.h"
#include "Combinations.h"
#include "Sort.h"
#include "ExprTree.h"
#include "SimLimits.h"
#include "Notation.h"

/*
 * Parser-Funktionen zum Zerlegen der Kurznotation im Messmodell
 *
 * @author Michael Weitzel <info@13cflux.net>
 */

namespace flux {
namespace data {
namespace Notation {

struct Lexeme
{
	enum Type {
		T_ETX, T_ERROR, T_MINUS, T_COMMA, T_COLON, T_HASH,
		T_OBRACKET, T_CBRACKET, T_OPAREN, T_CPAREN, T_AT,
		T_INTEGER, T_ID
	} ttype;
	long int integer;
	struct { char * start; char * stop; } str; 
};

static bool match(char ** s, Lexeme & L, Lexeme::Type LT)
{
	char * endptr;
	char * old_s = *s;
	int token = strtoutf8(*s,s);

	L.integer = -1;
	L.str.start = old_s;
	L.str.stop = *s;
	
	if (token == 0) L.ttype = Lexeme::T_ETX;
	else if (token >= '0' && token <= '9')
	{
		*s = old_s;
		L.ttype = Lexeme::T_INTEGER;
		errno = 0;
		L.integer = strtol(*s, &endptr, 10);
		if (((errno == ERANGE) && (L.integer == LONG_MAX || L.integer == LONG_MIN))
			|| (errno != 0 && L.integer == 0) || L.integer < 0)
		{
			L.ttype = Lexeme::T_ERROR;
		}
		*s = endptr;
	}
	else if (token == '-') L.ttype = Lexeme::T_MINUS;
	else if (token == ',') L.ttype = Lexeme::T_COMMA;
	else if (token == ':') L.ttype = Lexeme::T_COLON;
	else if (token == '#') L.ttype = Lexeme::T_HASH;
	else if (token == '[') L.ttype = Lexeme::T_OBRACKET;
	else if (token == ']') L.ttype = Lexeme::T_CBRACKET;
	else if (token == '(') L.ttype = Lexeme::T_OPAREN;
	else if (token == ')') L.ttype = Lexeme::T_CPAREN;
	else if (token == '@') L.ttype = Lexeme::T_AT;
	else
	{
		L.str.start = old_s;
		*s = old_s;
		do token=strtoutf8(*s,s); while (token != 0 && token != '[' && token != '#');
		(*s)--;
		L.str.stop = *s;
		L.ttype = Lexeme::T_ID;
	}
	return L.ttype == LT;
}

/**
 * Rückgabewerte:
 *  0 := kein Fehler
 *  1 := Parse-Error
 *  2 := ungültiger Bereich
 */
static int msfrag_range(char ** s, int * range_lo, int * range_hi)
{
	char * old_s;
	Lexeme L;

	*range_lo = -1; *range_hi = -1;
	if (match(s,L,Lexeme::T_INTEGER))
	{
		old_s = *s;
		*range_lo = (int)L.integer;
		if (match(s,L,Lexeme::T_COMMA))
		{
			*s = old_s; *range_hi = *range_lo;
		}
		else if (L.ttype == Lexeme::T_MINUS)
		{
			if (match(s,L,Lexeme::T_INTEGER))
				*range_hi = (int)L.integer;
			else return 1;
		}
		else if (L.ttype == Lexeme::T_CBRACKET || L.ttype == Lexeme::T_COLON || L.ttype == Lexeme::T_ETX)
		{
			*s = old_s; *range_hi = *range_lo;
		}
		else return 1;
	}
	else return 1;
	if (*range_lo > *range_hi || *range_hi>LIMIT_MAX_ATOMS)
	{
		*range_lo = *range_hi = -1;
		return 2;
	}
	return 0;
}

/**
 * Parser für Bereichsnotation,
 * z.B: "1-3,5,7-9"
 *
 * 0 := kein Fehler
 * 1 := Parse-Error
 * 2 := Ungültige Bereiche (msfrag_range)
 * 3 := Überlappende Bereiche
 */
int parse_range_spec(
	char * s,
	BitArray & mask
	)
{
	int lo, hi, catoms = 0;
	BitArray old_mask;
	int status;
	Lexeme L;

	mask = BitArray(LIMIT_MAX_ATOMS);
	do
	{
		status = msfrag_range(&s, &lo, &hi);
		if (status != 0) break;
		old_mask = mask;
		mask.ones(lo-1,hi-1);
		// bei Bereichsüberlappung vergrößert sich die Anzahl
		// der 1en nicht erwartungsgemäß:
		if (old_mask.countOnes()+(hi-lo+1) > mask.countOnes())
		{
			status = 3;
			break;
		}
		catoms += hi-lo+1;
	}
	while (match(&s,L,Lexeme::T_COMMA));

	if (catoms == 0)
		status = 2;
	if (status != 0)
	{
		mask.resize(0,false);
		return status;
	}
	return 0;
}

char * mask_to_range(
	BitArray const & mask
	)
{
	charptr_array outp;
	size_t i,j;

	i=0;
	while (i<mask.size())
	{
		if (not mask.get(i))
		{
			i++;
			continue;
		}

		j=i+1;
		while (j<mask.size() and mask.get(j))
			j++;
		j--;

		if (i!=j)
			outp.add("%i-%i",i+1,j+1);
		else
			outp.add("%i", i+1);
		i=j+1;
	}
	return strdup_alloc(outp.concat(","));
}

/**
 * Parser für MS-Fragment-Notation:
 *   Pool#Mn
 *   Pool#Mn,m,...
 *   Pool[m,n-o,p,q]#Mu,v,w
 *   ...
 *
 * In mask sind alle Bits gesetzt, die von der Bereichsspezifikation
 * abgedeckt werden (falls vorhanden).
 *
 * Rückgabewerte:
 * 0 := kein Fehler
 * 1 := Parse-Error
 * 2 := Ungültige Bereiche (msfrag_range)
 * 3 := Überlappende Bereiche
 * 4 := Weniger Markierungspositionen als Markierungen
 * 5 := Ungültige Gewichtsspezifikation
 */
int parse_MS_spec(
	char * s,
	char ** mname, // Metabolitname (allokiert)
	int ** weights, // sortierte Liste der Gewichte, mit -1 terminiert, allokiert!
	BitArray & mask // Bit-Maske der markierbaren Positionen
	)
{
	char * old_s;
	int lo, hi, catoms = 0;
	int status;
	Lexeme L;
	BitArray old_mask;

	mask = BitArray(LIMIT_MAX_ATOMS);
	if (not match(&s,L,Lexeme::T_ID))
	{
		*mname = 0;
		*weights = 0;
		return 1;
	}

	// Metabolitname
	*mname = new char[L.str.stop-L.str.start+1];
	for (lo=0; lo<L.str.stop-L.str.start; lo++)
		(*mname)[lo] = *(L.str.start+lo);
	(*mname)[lo] = '\0';

	// Klammer mit Bereichsangabe
	old_s = s;
	if (match(&s,L,Lexeme::T_OBRACKET))
	{
		do
		{
			status = msfrag_range(&s, &lo, &hi);
			if (status != 0) break;
			old_mask = mask;
			mask.ones(lo-1,hi-1);
			// bei Bereichsüberlappung vergrößert sich die Anzahl
			// der 1en nicht erwartungsgemäß:
			if (old_mask.countOnes()+(hi-lo+1) > mask.countOnes())
			{
				status = 3;
				break;
			}

			catoms += hi-lo+1;
		}
		while (match(&s,L,Lexeme::T_COMMA));
		if (status == 0 && L.ttype != Lexeme::T_CBRACKET)
			status = 1;
		if (catoms == 0)
			status = 2;
		if (status != 0)
		{
			delete[] *mname;
			*mname = 0;
			*weights = 0;
			mask.resize(0,false);
			return status;
		}
	}
	else
	{
		s = old_s;
		// es wird angenommen, dass der Metabolit LIMIT_MAX_ATOMS
		// Atome hat, falls keine Maske angegeben wurde -- das muss
		// später verifiziert werden!
		catoms = LIMIT_MAX_ATOMS;
		// Maske auf Einsen setzen:
		mask.ones();
	}
	// #Mn
	if (not match(&s,L,Lexeme::T_HASH))
		status = 1;
	else
		status = (match(&s,L,Lexeme::T_ID) && (*(L.str.start)=='M')) ? 0 : 1;

	if (status != 0)
	{
		delete [] *mname;
		*mname = 0;
		*weights = 0;
		mask.resize(0,false);
		return status;
	}
	// M überspringen:
	s = L.str.start+1;
	// Eine Liste von Gewichten -- Länge ermitteln (Anzahl der Kommas+1)
	hi = 1;
	old_s = s;
	while (*s != '\0') if (*(s++) == ',') hi++;
	s = old_s;
	*weights = new int[hi+1];
	(*weights)[hi] = -1; // Terminator
	lo = 0;
	do
	{
		if (match(&s,L,Lexeme::T_INTEGER))
		{
			status = 0;
			(*weights)[lo++] = L.integer;
			if (L.integer > catoms)
				status = 4;
			else if (lo<hi && !match(&s,L,Lexeme::T_COMMA))
				status = 5;
		}
		else status = 5;
	}
	while (status == 0 && lo < hi);
	if (status != 0)
	{
		delete[] *weights;
		*weights = 0;
	}
	else
	{
		Sort< int >::sort(*weights,hi);
		for (lo=1; lo<hi; lo++)
			if ((*weights)[lo-1] == (*weights)[lo])
			{
				delete[] *weights;
				*weights = 0;
				status = 5;
				break;
			}
	}
	// String-Ende
	if (status == 0)
		status = match(&s,L,Lexeme::T_ETX) ? 0 : 1;
	// Semantik-Check: können im Bereich überhaupt #Mn Atome markiert werden?
	if (status == 0)
	{
		if (mask.countOnes() != size_t(catoms)/* && catoms != LIMIT_MAX_ATOMS*/)
			status = 3;
	}
	if (status != 0)
	{
		delete [] *mname;
		*mname = 0;
		*weights = 0;
		mask.resize(0,false);
		return status;
	}
	return 0;
}

/**
 * Parser für multi-isotopic Tracer MS-Fragment-Notation:
 *   Pool#Mn
 *   Pool#Mn,m,...
 *   Pool[m,n-o,p,q]#Mu,v,w
 *   ...
 *
 * In mask sind alle Bits gesetzt, die von der Bereichsspezifikation
 * abgedeckt werden (falls vorhanden).
 *
 * Rückgabewerte:
 * 0 := kein Fehler
 * 1 := Parse-Error
 * 2 := Ungültige Bereiche (msfrag_range)
 * 3 := Überlappende Bereiche
 * 4 := Weniger Markierungspositionen als Markierungen
 * 5 := Ungültige Gewichtsspezifikation
 */
int parse_MIMS_spec(
	char * s,
	char ** mname, // Metabolitname (allokiert)
	int ** weights, // sortierte Liste der Gewichte, mit -1 terminiert, allokiert!
	BitArray & mask, // Bit-Maske der markierbaren Positionen
	int * NUM_ISOTOPES // Anzahl der Isotopes
        )
{
	char * old_s;
	int lo, hi, catoms = 0;
        size_t numbIsotopes; // anzahl der verwendeten Isotopes
	int status;
	Lexeme L;
	BitArray old_mask;

	mask = BitArray(LIMIT_MAX_ATOMS);
	if (not match(&s,L,Lexeme::T_ID))
	{
		*mname = 0;
                *NUM_ISOTOPES=0;
		*weights = 0;
		return 1;
	}

	// Metabolitname
	*mname = new char[L.str.stop-L.str.start+1];
	for (lo=0; lo<L.str.stop-L.str.start; lo++)
		(*mname)[lo] = *(L.str.start+lo);
	(*mname)[lo] = '\0';

	// Klammer mit Bereichsangabe
	old_s = s;
	if (match(&s,L,Lexeme::T_OBRACKET))
	{
		do
		{
			status = msfrag_range(&s, &lo, &hi);
			if (status != 0) break;
			old_mask = mask;
			mask.ones(lo-1,hi-1);
			// bei Bereichsüberlappung vergrößert sich die Anzahl
			// der 1en nicht erwartungsgemäß:
			if (old_mask.countOnes()+(hi-lo+1) > mask.countOnes())
			{
				status = 3;
				break;
			}

			catoms += hi-lo+1;
		}
		while (match(&s,L,Lexeme::T_COMMA));
		if (status == 0 && L.ttype != Lexeme::T_CBRACKET)
			status = 1;
		if (catoms == 0)
			status = 2;
		if (status != 0)
		{
			delete[] *mname;
			*mname = 0;
                        *NUM_ISOTOPES=0;
			*weights = 0;
			mask.resize(0,false);
			return status;
		}
	}
	else
	{
		s = old_s;
		// es wird angenommen, dass der Metabolit LIMIT_MAX_ATOMS
		// Atome hat, falls keine Maske angegeben wurde -- das muss
		// später verifiziert werden!
		catoms = LIMIT_MAX_ATOMS;
		// Maske auf Einsen setzen:
		mask.ones();
	}
	// #Mn
	if (not match(&s,L,Lexeme::T_HASH))
		status = 1;
	else
		status = (match(&s,L,Lexeme::T_ID) && (*(L.str.start)=='M')) ? 0 : 1;

	if (status != 0)
	{
		delete [] *mname;
		*mname = 0;
                *NUM_ISOTOPES=0;
		*weights = 0;
		mask.resize(0,false);
		return status;
	}
	// M überspringen:
	s = L.str.start+1;
	// Eine Liste von Gewichten -- Länge ermitteln (Anzahl der Kommas+1)
	
        // Eine Liste von Gewichtspaaren -- Länge ermitteln (Anzahl der ')'+1)
	hi = 0;
	old_s = s;
	while (*s != '\0') if (*(s++) == ')') hi++;
	s = old_s;
        
        // Eine Liste von Gewichtspaaren -- Länge ermitteln (Anzahl der ')'+1)
        numbIsotopes=1;
        old_s = s;
        while (*s != ')') 
            if (*(s++) == ',') numbIsotopes++;
	s = old_s;
        
//        if (mask.countOnes() != size_t(numbIsotopes))
//                    status = 5;
//        if (status != 0)
//	{
//		delete [] *mname;
//		*mname = 0;
//                *NUM_ISOTOPES=0;
//		*weights = 0;
//		mask.resize(0,false);
//		return status;
//	}
        
        *NUM_ISOTOPES = numbIsotopes;
	*weights = new int[numbIsotopes*hi+1];
	(*weights)[numbIsotopes*hi] = -1; // Terminator
	lo = 0;
	do
	{
		if (match(&s,L,Lexeme::T_OPAREN))
		{
                    for(size_t k=0; k<numbIsotopes;k++)
                    {
                        status = 0;
                        if (status == 0 && match(&s,L,Lexeme::T_INTEGER))
                        {
                                (*weights)[numbIsotopes*lo+k] = L.integer;
                                if (L.integer > catoms)
                                        status = 4;
                        }
                        if (status == 0 && (k!=(numbIsotopes-1)))
                                status = match(&s,L,Lexeme::T_COMMA) ? 0 : 5;
                    }
                    lo++;
                    if (status == 0)
                            status = match(&s,L,Lexeme::T_CPAREN) ? 0 : 5;
                    if (status == 0 && lo < hi)
                            status = match(&s,L,Lexeme::T_COMMA) ? 0 : 5;
		}
		else status = 5;
	}
	while (status == 0 && lo < hi);
	if (status != 0)
	{
		delete[] *weights;
		*weights = 0;
	}
	else
	{
		unsigned int * P = new unsigned int[hi];
		int * W = new int[hi];
		for (lo=0; lo<hi; lo++)
		{
                    for(size_t k=0; k<numbIsotopes;k++)
			W[lo] = ((((*weights)[numbIsotopes*lo+k]) << 6) + ((*weights)[numbIsotopes*lo+k+1]));
                    P[lo] = lo;
		}
            
		Sort< int >::sortPerm(W,P,hi);
		for (lo=1; lo<hi; lo++)
		{
			// unique?
			if ((*weights)[numbIsotopes*P[lo-1]] == (*weights)[numbIsotopes*P[lo]]
				&& (*weights)[numbIsotopes*P[lo-1]+1] == (*weights)[numbIsotopes*P[lo]+1])
			{
				delete[] *weights;
				*weights = 0;
				status = 5;
				break;
			}
		}
            
		if (status == 0)
		{
			// sortieren
			int * nweights = new int[(2*numbIsotopes)*hi+1];
			nweights[numbIsotopes*hi] = -1;
			for (lo=0; lo<hi; lo++)
			{
                            for(size_t k=0; k<numbIsotopes;k++)
				nweights[numbIsotopes*lo+k] = (*weights)[numbIsotopes*P[lo]+k];
			}
			delete[] *weights;
			*weights = nweights;
		}
		delete[] W;
		delete[] P;
	}
	// String-Ende
	if (status == 0)
		status = match(&s,L,Lexeme::T_ETX) ? 0 : 1;
        
        if (status == 0)
            
	// Semantik-Check: können im Bereich überhaupt #Mn Atome markiert werden?
	if (status == 0)
	{
		if (mask.countOnes() != size_t(catoms)/* && catoms != LIMIT_MAX_ATOMS*/)
			status = 3;
	}
        
	if (status != 0)
	{
		delete [] *mname;
		*mname = 0;
		*NUM_ISOTOPES = 0;
		*weights = 0;
		mask.resize(0,false);
		return status;
	}
	return 0;
}


/**
 * Parser für MS-MS-Fragment-Notation:
 *   Pool#Mn
 *   Pool#Mn,m,...
 *   Pool[m,n-o,p,q:a-b]#M(u,v),(w,x),...
 *   ...
 *
 * In mask sind alle Bits gesetzt, die von der Bereichsspezifikation
 * abgedeckt werden (falls vorhanden).
 *
 * Rückgabewerte:
 * 0 := kein Fehler
 * 1 := Parse-Error
 * 2 := Ungültige Bereiche (msfrag_range)
 * 3 := Überlappende Bereiche
 * 4 := Weniger Markierungspositionen als Markierungen
 * 5 := Ungültige Gewichtsspezifikation
 */
int parse_MSMS_spec(
	char * s,
	char ** mname, // Metabolitname (allokiert)
	int ** weights, // Liste der Gewichte, mit -1 terminiert, allokiert!
	BitArray & mask1, // Bit-Maske der markierbaren Positionen
	BitArray & mask2
	)
{
	char * old_s;
	int lo, hi, catoms1 = 0, catoms2 = 0;
	int status;
	Lexeme L;
	BitArray old_mask;

	mask1 = BitArray(LIMIT_MAX_ATOMS);
	mask2 = BitArray(LIMIT_MAX_ATOMS);
	if (not match(&s,L,Lexeme::T_ID))
	{
		*mname = 0;
		*weights = 0;
		return 1;
	}

	// Metabolitname
	*mname = new char[L.str.stop-L.str.start+1];
	for (lo=0; lo<L.str.stop-L.str.start; lo++)
		(*mname)[lo] = *(L.str.start+lo);
	(*mname)[lo] = '\0';

	// Klammer mit Bereichsangabe
	old_s = s;
	if (match(&s,L,Lexeme::T_OBRACKET))
	{
		do
		{
			status = msfrag_range(&s, &lo, &hi);
			if (status != 0) break;
			old_mask = mask1;
			mask1.ones(lo-1,hi-1);
			// bei Bereichsüberlappung vergrößert sich die Anzahl
			// der 1en nicht erwartungsgemäß:
			if (old_mask.countOnes()+(hi-lo+1) > mask1.countOnes())
			{
				status = 3;
				break;
			}
			catoms1 += hi-lo+1;
		}
		while (match(&s,L,Lexeme::T_COMMA));
		if (status == 0 && L.ttype == Lexeme::T_COLON)
		{
			do
			{
				status = msfrag_range(&s, &lo, &hi);
				if (status != 0) break;
				old_mask = mask2;
				mask2.ones(lo-1,hi-1);
				// s.o.
				if (old_mask.countOnes()+(hi-lo+1) > mask2.countOnes())
				{
					status = 3;
					break;
				}
				catoms2 += hi-lo+1;
			}
			while (match(&s,L,Lexeme::T_COMMA));
		}
		else status = 2;

		if (status == 0 && L.ttype != Lexeme::T_CBRACKET)
			status = 1;
		if (catoms1 == 0 || catoms2 == 0)
			status = 2;
		// mask2 muß teilmenge von mask1 sein!
		if ((mask1 | mask2) != mask1)
			status = 3;
		if (status != 0)
		{
			delete[] *mname;
			*mname = 0;
			*weights = 0;
			mask1.resize(0,false);
			mask2.resize(0,false);
			return status;
		}
	}
	else
	{
		s = old_s;
		// es wird angenommen, dass der Metabolit LIMIT_MAX_ATOMS
		// Atome hat, falls keine Maske angegeben wurde -- das muss
		// später verifiziert werden!
		catoms1 = catoms2 = LIMIT_MAX_ATOMS;
		// maske auf Einsen setzen
		mask1.ones();
		mask2.ones();
	}
	// #Mn
	if (not match(&s,L,Lexeme::T_HASH))
		status = 1;
	else
		status = (match(&s,L,Lexeme::T_ID) && (*(L.str.start)=='M')) ? 0 : 1;

	if (status != 0)
	{
		delete [] *mname;
		*mname = 0;
		*weights = 0;
		mask1.resize(0,false);
		mask2.resize(0,false);
		return status;
	}
	// M überspringen:
	s = L.str.start+1;
	// Eine Liste von Gewichtspaaren -- Länge ermitteln (Anzahl der ')'+1)
	hi = 0;
	old_s = s;
	while (*s != '\0') if (*(s++) == ')') hi++;
	s = old_s;
	*weights = new int[2*hi+1];
	(*weights)[2*hi] = -1; // Terminator
	lo = 0;
	do
	{
		if (match(&s,L,Lexeme::T_OPAREN))
		{
			status = 0;
			if (match(&s,L,Lexeme::T_INTEGER))
			{
				(*weights)[2*lo] = L.integer;
				if (L.integer > catoms1)
					status = 4;
			}
			if (status == 0)
				status = match(&s,L,Lexeme::T_COMMA) ? 0 : 5;
			if (status == 0 && match(&s,L,Lexeme::T_INTEGER))
			{
				(*weights)[2*lo+1] = L.integer;
				lo++;
				if (L.integer > catoms2)
					status = 4;
				if (status == 0)
					status = match(&s,L,Lexeme::T_CPAREN) ? 0 : 5;
				if (status == 0 && lo < hi)
					status = match(&s,L,Lexeme::T_COMMA) ? 0 : 5;
			}
		}
		else status = 5;
	}
	while (status == 0 && lo < hi);
	if (status != 0)
	{
		delete[] *weights;
		*weights = 0;
	}
	else
	{
		unsigned int * P = new unsigned int[hi];
		int * W = new int[hi];
		for (lo=0; lo<hi; lo++)
		{
			W[lo] = ((((*weights)[2*lo]) << 6) + ((*weights)[2*lo+1]));
			P[lo] = lo;
		}
		Sort< int >::sortPerm(W,P,hi);
		for (lo=1; lo<hi; lo++)
		{
			// unique?
			if ((*weights)[2*P[lo-1]] == (*weights)[2*P[lo]]
				&& (*weights)[2*P[lo-1]+1] == (*weights)[2*P[lo]+1])
			{
				delete[] *weights;
				*weights = 0;
				status = 5;
				break;
			}
		}
		if (status == 0)
		{
			// sortieren
			int * nweights = new int[4*hi+1];
			nweights[2*hi] = -1;
			for (lo=0; lo<hi; lo++)
			{
				nweights[2*lo] = (*weights)[2*P[lo]];
				nweights[2*lo+1] = (*weights)[2*P[lo]+1];
			}
			delete[] *weights;
			*weights = nweights;
		}
		delete[] W;
		delete[] P;
	}
	// String-Ende
	if (status == 0)
		status = match(&s,L,Lexeme::T_ETX) ? 0 : 1;
	// Semantik-Check: können im Bereich überhaupt #Mn Atome markiert werden?
	if (status == 0)
	{
		if (mask1.countOnes() != size_t(catoms1) || mask2.countOnes() != size_t(catoms2))
			status = 3;
	}
	if (status != 0)
	{
		delete [] *mname;
		*mname = 0;
		*weights = 0;
		mask1.resize(0,false);
		mask2.resize(0,false);
		return status;
	}
	return 0;
}

/* Rückgabewerte:
 *
 * 1 := Parse Error (ungültiger Poolname)
 * 2 := Fehler in Positionsliste
 * 3 := Positionen doppelt angegeben
 */
int parse_1HNMR_spec(
	char * s,
	char ** mname,
	int ** poslst
	)
{
	char * old_s;
	int i,j;
	int status;
	Lexeme L;

	if (not match(&s,L,Lexeme::T_ID))
	{
		*mname = 0;
		*poslst = 0;
		return 1;
	}
	// Metabolitname
	*mname = new char[L.str.stop-L.str.start+1];
	for (i=0; i<L.str.stop-L.str.start; i++)
		(*mname)[i] = *(L.str.start+i);
	(*mname)[i] = '\0';

	// #P
	if (not match(&s,L,Lexeme::T_HASH)
		or not (match(&s,L,Lexeme::T_ID) and (*(L.str.start)=='P'))
		)
	{
		delete[] *mname;
		*mname = 0;
		*poslst = 0;
		return 1;
	}
	// P überspringen:
	s = L.str.start+1;

	// Eine Liste von Gewichten -- Länge ermitteln (Anzahl der Kommas+1)
	j = 1;
	old_s = s;
	while (*s != '\0') if (*(s++) == ',') j++;
	s = old_s;
	*poslst = new int[j+1];
	(*poslst)[j] = -1; // Terminator
	i = 0;
	do
	{
		if (match(&s,L,Lexeme::T_INTEGER))
		{
			status = 0;
			// 0 nicht erlaubt
			if (L.integer == 0)
				status = 2;
			(*poslst)[i++] = L.integer;
			if (i<j)
			{
				if (not match(&s,L,Lexeme::T_COMMA))
					status = 2;
				else if (*s == 'P')
					s++;
			}
		}
		else status = 2;
	}
	while (status == 0 && i < j);
	if (status == 0)
	{
		Sort< int >::sort(*poslst,j);
		for (i=1; i<j; i++)
			if ((*poslst)[i-1] == (*poslst)[i])
			{
				status = 3;
				break;
			}
	}
	if (status != 0)
	{
		delete[] *mname;
		*mname = 0;
		delete[] *poslst;
		*poslst = 0;
	}
	return status;
}

/* type := S->1, DL->2, DR->3, DD->4, T->5
 *
 * Rückgabewerte:
 *
 * 1 := Parse Error (ungültiger Poolname)
 * 2 := Fehler in Positionsliste
 * 3 := Positionen doppelt angegeben
 */
int parse_13CNMR_spec(
	char * s,
	char ** mname,
	int ** poslst,
	int ** typelst
	)
{
	char * old_s;
	int i,j,k;
	int status = 0;
	Lexeme L;
	int type = -1, prevtype = -1;

	*mname = 0;
	*poslst = 0;
	*typelst = 0;

	if (not match(&s,L,Lexeme::T_ID))
		return 1;
	// Metabolitname
	*mname = new char[L.str.stop-L.str.start+1];
	for (i=0; i<L.str.stop-L.str.start; i++)
		(*mname)[i] = *(L.str.start+i);
	(*mname)[i] = '\0';

	if (!match(&s,L,Lexeme::T_HASH))
	{
		delete[] *mname;
		*mname = 0;
		return 1;
	}

	// Eine Liste von Positionen -- Länge ermitteln (Anzahl der Kommas+1)
	j = 1;
	old_s = s;
	while (*s != '\0') if (*(s++) == ',') j++;
	s = old_s;
	*poslst = new int[j+1];
	*typelst = new int[j+1];
	(*poslst)[j] = -1; // Terminator
	(*typelst)[j] = -1;

	// die Liste parsen
	i = 0;
	do
	{
		prevtype = type;
		type = -1;

		if (*s == 'S') type = 1;
		else if (*s == 'T') type = 5;
		else if (*s == 'D')
		{
			s++;
			if (*s == 'L') type = 2;
			else if (*s == 'R') type = 3;
			else if (*s == 'D') type = 4;
			else
			{
				delete[] *mname;
				delete[] *poslst;
				delete[] *typelst;
				*mname = 0;
				*poslst = 0;
				*typelst = 0;
				return 2;
			}
		}
		
		if (type == -1)
			type = prevtype;
		else
			s++;
	
		if (match(&s,L,Lexeme::T_INTEGER))
		{
			status = 0;
			// 0 nicht erlaubt; DL oder T bei pos==1 nicht erlaubt!
			if (L.integer == 0 || ((type == 2 || type == 5) && L.integer == 1))
				status = 2;
			(*poslst)[i] = L.integer;
			(*typelst)[i] = type;
			i++;
			if (i<j && !match(&s,L,Lexeme::T_COMMA))
				status = 2;
		}
		else if (not (*s == 'S' || *s == 'D' || *s == 'T'))
			status = 2;
	}
	while (status == 0 && i < j);

	if (status == 0)
	{
		i = 0;
		while (i<j && (*typelst)[i] != -1)
		{
			k = i;
			while ((*typelst)[i]==(*typelst)[k+1])
				k++;
			Sort< int >::sort(*poslst,i,k);

			for (i=i+1; i<k; i++)
			{
				if ((*poslst)[i-1] == (*poslst)[i])
				{
					delete[] *mname;
					delete[] *poslst;
					delete[] *typelst;
					*mname = 0;
					*poslst = 0;
					*typelst = 0;
					return 3;
				}
			}
			i = k+1;
		}
	}
	if (status != 0)
	{
		delete[] *mname;
		delete[] *typelst;
		delete[] *poslst;
		*mname = 0;
		*typelst = 0;
		*poslst = 0;
	}
	return status;
}

int parse_cumomer_spec(
	char * s,
	char ** mname,
	BitArray & xmask,
	BitArray & mask
	)
{
	int i,j;
	Lexeme L;

	*mname = 0;

	if (not match(&s,L,Lexeme::T_ID))
		return 1;
	
	// Metabolitname
	*mname = new char[L.str.stop-L.str.start+1];
	for (i=0; i<L.str.stop-L.str.start; i++)
		(*mname)[i] = *(L.str.start+i);
	(*mname)[i] = '\0';

	if (!match(&s,L,Lexeme::T_HASH))
	{
		delete[] *mname;
		*mname = 0;
		return 1;
	}

	// Eine Liste von 'x'-Positionen -- Länge ermitteln
	j = strlen(s);
	mask = BitArray(j);
	xmask = BitArray(j);
	for (i=0; i<j; i++)
	{
		if (s[i] == 'x')
			xmask.set(i);
		else if (s[i] == '1')
			mask.set(i);
	}
	return 0;
}

// Scanner zum Identifizieren der Notation einbinden
// (int identify_notation(char *s))
#include "Notation.inc"

int check_spec(char * s, bool * valid, int * dim, char * pname)
{
	int i,type = identify_notation(s);

	if (dim)
		*dim = -1;
	if (valid == 0)
		return type;
	if (type == -1)
	{
		*valid = false;
		return -1;
	}

	*valid = false;
	// MS=1,MSMS=2,NMR1H=3,NMR13C=4,generic=5, MI_MS=6
	switch (type)
	{
	case 1: // MS
		{
			char * mname;
			int * weights;
			BitArray mask;
			if (parse_MS_spec(s,&mname,&weights,mask) == 0)
			{
				i = 0;
				while (weights[i]!=-1) i++;
				if (dim) *dim = i;
//                                if(pname) pname = strdup_alloc(mname);
                                delete[] mname;
				delete[] weights;
				*valid = true;
			}
		}
		break;
	case 2: // MSMS
		{
			char * mname;
			int * weights;
			BitArray mask1, mask2;
			if (parse_MSMS_spec(s,&mname,&weights,mask1,mask2) == 0)
			{
				i = 0;
				while (weights[i]!=-1) i++;
				if (dim) *dim = i/2;
                                if(pname) pname = strdup_alloc(mname);
				delete[] mname;
				delete[] weights;
				*valid = true;
			}
		}
		break;
	case 3: // NMR1H
		{
			char * mname;
			int * poslst;
			if (parse_1HNMR_spec(s,&mname,&poslst) == 0)
			{
				i = 0;
				while (poslst[i]!=-1) i++;
				if (dim) *dim = i;
				delete[] mname;
				delete[] poslst;
				*valid = true;
			}
		}
		break;
	case 4: // NMR13C
		{
			char * mname;
			int * poslst;
			int * typelst;
			if (parse_13CNMR_spec(s,&mname,&poslst,&typelst) == 0)
			{
				i = 0;
				while (poslst[i]!=-1) i++;
				if (dim) *dim = i;
				delete[] mname;
				delete[] poslst;
				delete[] typelst;
				*valid = true;
			}
		}
		break;
	case 5: // generic Cumomer
		{
			char * mname;
			BitArray xmask;
			BitArray mask;
			if (parse_cumomer_spec(s,&mname,xmask,mask) == 0)
			{
				// dim ist hier immer gleich 1:
				if (dim) *dim = 1;
				delete[] mname;
				*valid = true;
			}
		}
		break;
        case 6: // MI_MS
		{
			char * mname;
			int * weights;
			BitArray mask;
                        int numOfIsotope = 1;
			if (parse_MIMS_spec(s,&mname,&weights,mask,&numOfIsotope) == 0)
			{
				i = 0;
				while (weights[i]!=-1) i++;
				if (dim) *dim = i/numOfIsotope;
                                if(pname) pname = strdup_alloc(mname);
				delete[] mname;
				delete[] weights;
				*valid = true;
			}
		}
		break;
	default:
		return -1;
	}
	return type;
}

int parse_perm_spec_tag(
	char * s,
	char ** element,
	int * atomnr,
	char ** posid
	)
{
	int i;
	Lexeme L;

	if (not match(&s,L,Lexeme::T_ID))
	{
		// Element-Name erwartet
		*element = 0;
		*atomnr = 0;
		*posid = 0;
		return 1;
	}

	*element = new char[L.str.stop-L.str.start+1];
	for (i=0; i<L.str.stop-L.str.start; i++)
		(*element)[i] = *(L.str.start+i);
	(*element)[i] = '\0';

	if ((not match(&s,L,Lexeme::T_HASH))
		or (not match(&s,L,Lexeme::T_INTEGER)))
	{
		// #[atomnr] erwartet
		delete[] *element;
		*element = 0;
		*atomnr = 0;
		*posid = 0;
		return 2;
	}
	*atomnr = L.integer;

	if (not match(&s,L,Lexeme::T_AT))
	{
		// @[posid] erwartet
		delete[] *element;
		*element = 0;
		*atomnr = 0;
		*posid = 0;
		return 3;
	}

	*posid = strdup_alloc(s);
	if (*posid == 0 or strlen(*posid) == 0)
	{
		delete[] *element;
		*element = 0;
		*atomnr = 0;
		if (*posid)
			delete[] *posid;
		*posid = 0;
		return 4;
	}
	return 0;
}

#include "Notation2.inc"

int perm_spec_length(char const * s)
{
	int type;
	if (s == 0 or *s == '\0')
		return 0;

	type = identify_perm_spec(s);
	if (type == -1)
		return -1;
	if (type == 1)
		return strlen(s);

	size_t len = 0, b = 0;
	char const * p = s;
	char const delims[] = " \t\r\n\f\v";
	do
	{
		b = strspn(p,delims);
		p += b;
		b = strcspn(p,delims);
		if (b == 0)
			break;
		len++;
		p += b;
	}
	while (*p != '\0');
	return len;
}

int check_perm_spec(charptr_array const & spec)
{
	int vote_s = 0, vote_l = 0, vote_a = 0;
	charptr_array::const_iterator si;

	for (si=spec.begin(); si!=spec.end(); ++si)
	{
		switch (identify_perm_spec(*si))
		{
		case 0: vote_a++; break;
		case 1: vote_s++; break;
		case 2: vote_l++; break;
		case -1: return -1;
		default:
			 fASSERT_NONREACHABLE();
		}
	}

	if (vote_s > 0 and vote_l > 0) // widersprüchlich?
		return -1;
	if (vote_s == 0 and vote_l == 0 and vote_a > 0) // unbestimmt?
		return 0;
	if (vote_s > 0) // kurz?
		return 1;
	if (vote_l > 0) // lang?
		return 2;
	return -1; // illegal!
}

#include "Notation3.inc"

} // namespace flux::data::Notation
} // namespace flux::data
} // namespace flux

