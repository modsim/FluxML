extern "C"
{
#include <stdint.h>
}
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include "Sort.h"
#include "Combinations.h"
#include "charptr_array.h"
#include <stdio.h>
/*
 * Logarithmus dualis
 */
unsigned int log_base_2(unsigned int n)
{
	unsigned int l=0;
	while (n>1) { n >>= 1; l++; }
	return l;
}

/*
 * Berechnung der nächsten Zweierpotenz.
 */
unsigned int next_pwr_2(unsigned int n)
{
	unsigned int pwr2 = 1;
	while (pwr2<n) pwr2<<=1;
	return pwr2;
}

/*
 * Zweier-Potenz-Prädikat
 */
bool is_pwr_2(unsigned int n)
{
	return ((unsigned int)(1)<<log_base_2(n)) == n;
}

/*
 * ermittelt die Permutation aus zwei vorgegeben Strings
 * Beispiel: abcdef -> cdafeb => P=[2,5,0,1,4,3]
 */
// REMARK: Atom-Mapping => abcdef -> cdafeb => P=[2,5,0,1,4,3]
bool get_perm(char const * L, char const * R, int ** P)
{
	size_t i, j;
	size_t n_bits = strlen(L);

	if (n_bits == 0 || n_bits != strlen(R) || P == 0 || *P == 0)
		return false;
        
//	printf("Permutation=[");
	// Aufbau der Permutation
	for (i=0; i<n_bits; i++)
		for ((*P)[i]=-1,j=0; j<n_bits; j++)
			if (L[i] == R[j]) 
                        {
                            (*P)[i] = j;
//                            printf("%d, ", int(j));
                        }
//        printf("]\n");
	
	// abschließende "Sinnüberprüfung" der Permutation
	unsigned int * Q = new unsigned int[n_bits];
	Sort< int >::sortPerm(*P,Q,n_bits);
	for (i=0; i<n_bits; i++)
	{
		if ((*P)[Q[i]] != int(i))
		{
			delete[] Q;
			return false;
		}
	}
	delete[] Q;
	return true;
}
// REMARK: Atom-Mapping => "C#1@1 C#2@2 C#3@3" -> "C#1@2 C#2@3 C#3@1"
/* "C#1@1 C#2@2 C#3@3" -> "C#1@2 C#2@3 C#3@1" */
bool get_perm(charptr_array const & L, charptr_array const & R, int ** P)
{
	size_t i, j;
	size_t n_bits = L.size();

	if (n_bits == 0 || n_bits != R.size() || P == 0 || *P == 0)
		return false;
	
//        printf("Permutation=[");
	// Aufbau der Permutation
	charptr_array::const_iterator u,v;
	for (i=0,u=L.begin(); u!=L.end(); i++,u++)
		for ((*P)[i]=-1,j=0,v=R.begin(); v!=R.end(); j++,v++)
			if (strcmp(*u,*v) == 0)
                        {
				(*P)[i] = j;
//                                printf("%d, ", int(j));
                        }
//	printf("]\n");
	// abschließende "Sinnüberprüfung" der Permutation
	unsigned int * Q = new unsigned int[n_bits];
	Sort< int >::sortPerm(*P,Q,n_bits);
	for (i=0; i<n_bits; i++)
	{
		if ((*P)[Q[i]] != int(i))
		{
			delete[] Q;
			return false;
		}
	}
	delete[] Q;
	return true;
}

bool get_bond_perm(char const * L, char const * R, int ** P)
{
	size_t L_len = strlen(L);
	size_t R_len = strlen(R);
	size_t i,j;
	
	if (*P == 0)
		return false;

	// Aufbau der Permutation
	for (j=0; j<R_len; j++)
		for ((*P)[j]=-1,i=0; i<L_len; i++)
			if (L[i] != '?' && L[i] == R[j]) (*P)[j] = i;
	return true;
}

uint64_t bin_coeff(uint64_t n, uint64_t k)
{
	int64_t d;
	if (n<k) return 0;
	if (k==0 || n==k) return 1;
	// die schnellere Alternative zu "C(n,k)=C(n-1,k-1)+C(n-1,k)":
	d = GCD(n,k);
	return ( bin_coeff(n-1,k-1) / (k/d) ) * (n/d);
}

char * bin2str(uint32_t n, uint16_t bits)
{
	char *str = new char[bits+1];
	uint16_t i = 0;
	while (i<bits)
	{
		str[/*bits-*/i/*-1*/] = TSTBIT(n,i)?'1':'x';
		i++;
	}
	str[bits] = '\0';
	return str;
}

char * bin2str_rev(uint32_t n, uint16_t bits)
{
	char *str = new char[bits+1];
	uint16_t i = 0;
	while (i<bits)
	{
		str[bits-i-1] = TSTBIT(n,i)?'1':'x';
		i++;
	}
	str[bits] = '\0';
	return str;
}

uint32_t str2bin(char const * s)
{
	uint32_t v = 0;
	int i,l = strlen(s)-1;

	// MSB = erstes Bit, LSB = letztes Bit
	for (v=0,i=0; i<=l; i++)
		if (s[i]=='1') v = v|(1<<(l-i));
	return v;
}

uint32_t str2bin_rev(char const * s)
{
	char *x, *w = (char *)s;
	uint32_t v;

	// MSB = letztes Bit, LSB = erstes Bit
	// für die '0' wird auch 'x' oder 'X' akzeptiert
	while (not (*w=='1'||*w=='0'||*w=='x'||*w=='X')) w++;
	for (x=w, v=0; *x!='\0'; x++)
		if (*x=='1') v=v|(1<<(x-w));
	return v;
}

uint32_t count_ones(uint32_t n)
{
	//int k = 0;
	//while (n) { if (n&1) k++; n>>=1; }
	//return k;
	
	// der absolute Hacker-Divide&Conquer-Algorithmus:
	n = ((n >> 1) & 0x55555555) + (n & 0x55555555);
	n = ((n >> 2) & 0x33333333) + (n & 0x33333333);
	n = ((n >> 4) & 0x0F0F0F0F) + (n & 0x0F0F0F0F);
	n = ((n >> 8) & 0x00FF00FF) + (n & 0x00FF00FF);
	return ((n >> 16) & 0x0000FFFF) + (n & 0x0000FFFF);
}

char * makeCumomerName(char const * poolname, uint32_t cumo, uint16_t atoms)
{
	char * cn = new char[strlen(poolname)+atoms+2];
	char * bits = bin2str(cumo,atoms);
	strcpy(cn,poolname);
	strcat(cn,"#");
	strcat(cn,bits);
	delete[] bits;
	return cn;
}

char * makeIsotopomerName(char const * poolname, uint32_t iso, uint16_t atoms)
{
	char * cn = makeCumomerName(poolname,iso,atoms);
	char * w = strchr(cn,'#');
	while (*(++w) != '\0')
		if (*w == 'x') *w = '0';
	return cn;
}

char * makeEMUName(char const * poolname, uint32_t emu, uint16_t atoms)
{
	int i,j;
	char atomnr[16];
	char * en = new char[strlen(poolname)+17*ONES(emu)+5];
	strcpy(en,poolname);
	strcat(en,"_");
	if (emu == 0)
	{
		strcat(en,"#");
		return en;
	}
	for (i=1,j=0; emu; i++,emu>>=1)
	{
		if (emu & 1)
		{
			snprintf(atomnr,16,"%i",i);
			if (j>0) strcat(en,",");
			strcat(en,atomnr);
			j++;
		}
	}
	return en;
}

/*
 * Ermittelt die lexikographisch nächste Kombination.
 * Wird nicht benötigt (siehe next_comb_colex).
 */
/*
uint32_t next_comb_lex(uint8_t n, uint8_t k)
{
	uint32_t i,j;
	uint32_t val;
	static int8_t c[33] = {-1,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
		18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};

	for (val=0,i=k; i; i--)
		val = SETBIT(val,c[i]-1);
	//for (i=k; i; i--) printf(" %c", "ABCDEFG"[c[i]-1]); putchar('\n');

	j = k;
	while (c[j] == n - k + j) j--;

	c[j]++;

	for (i = j + 1; i <= k; i++)
		c[i] = c[i-1] + 1;

	// Reinitialisierung von Feld "c"
	if (!j)
	{
		c[0] = -1;
		for (i=1; i <= k; i++) c[i] = i;
	}

	return val;
}
*/

/**
 * Der (Bit-)"Colex"-Algorithmus. Erzeugt alle Kombinationen aufsteigend
 * in Dualzahl-Zählreihenfolge. Die Funktion speichert den letzten Zustand
 * in internen static-Variablen. Jeder Aufruf erzeugt eine neue der
 * (n über k) Kombinationen.
 *
 * @param n Anzahl der Bits insgesamt
 * @param k Anzahl der eingeschalteten Bits
 * @param init auf true setzen, wenn (Re-)Initialisierung erwünscht
 */
#define	COLEX_LEAST_ITEM(I)	((I)&-(I))
uint32_t next_comb_colex(uint8_t n, uint8_t k, bool init)
{
	static uint32_t last_comb = 0;
	static uint32_t comb = 0;
	uint32_t hibit, lobit;
	
	if (init || last_comb == 0 || comb == last_comb)
	{
		comb = COLEX_START(n,k);
		last_comb = comb << (n-k);
		return comb;
	}

	lobit = COLEX_LEAST_ITEM(comb);
	comb += lobit;
	hibit = COLEX_LEAST_ITEM(comb);
	hibit -= lobit;
	while (!(hibit&1)) hibit >>= 1;
	comb |= hibit >> 1;

	return comb;
}

uint32_t next_comb_colex_stateless(uint8_t n, uint8_t k, uint32_t comb)
{
	uint32_t hibit, lobit;
	
	if (n<k) return 0;

	// Selbst-Initialisierung:
	//   Wird comb==0 übergeben, oder im letzten Durchlauf die
	//   lexikographisch größte Kombination erzeugt, so wird
	//   reinitialisiert:
	if (comb == 0 || comb >= COLEX_STOP(n,k))
		return COLEX_START(n,k);

	lobit = COLEX_LEAST_ITEM(comb);
	comb += lobit;
	hibit = COLEX_LEAST_ITEM(comb);
	hibit -= lobit;
	while (!(hibit&1)) hibit >>= 1;
	comb |= hibit >> 1;

	return comb;
}

uint32_t prev_comb_colex_stateless(uint8_t n, uint8_t k, uint32_t comb)
{
	comb = next_comb_colex_stateless(n,n-k,~comb & ONEMSK(0,n-1));
	return ~comb & ONEMSK(0,n-1);
}

uint32_t next_comb_colex_mask(uint32_t mask, uint8_t k, uint32_t mcomb)
{
	int i, j;
	uint32_t hibit, lobit;
	uint32_t comb;
	uint8_t n = ONES(mask);
	bool init = false;
	
	if (n<k) return 0;

	// Selbst-Initialisierung:
	//   Wird mcomb==0 übergeben, oder im letzten Durchlauf die
	//   lexikographisch größte Kombination erzeugt, so wird
	//   reinitialisiert:
	if (mcomb == 0)
		init = true;
	else
	{
		comb = 0;
		for (i=0,j=0; i<32; i++)
		{
			if (TSTBIT(mask,i))
			{
				if (TSTBIT(mcomb,i)) comb = SETBIT(comb,j);
				j++;
			}
		}
		if (comb >= COLEX_STOP(n,k))
			init = true;
	}

	if (init)
		comb = COLEX_START(n,k);
	else
	{
		lobit = COLEX_LEAST_ITEM(comb);
		comb += lobit;
		hibit = COLEX_LEAST_ITEM(comb);
		hibit -= lobit;
		while (!(hibit&1)) hibit >>= 1;
		comb |= hibit >> 1;
	}
	mcomb = 0;
	for (i=0,j=0; i<32; i++)
	{
		if (TSTBIT(mask,i))
		{
			if (TSTBIT(comb,j)) mcomb = SETBIT(mcomb,i);
			j++;
		}
	}
	return mcomb;
}

uint32_t prev_comb_colex_mask(uint32_t mask, uint8_t k, uint32_t mcomb)
{
	int i, j;
	uint32_t comb;
	uint8_t n = ONES(mask);
	
	if (n<k) return 0;
	
	// Selbst-Initialisierung:
	if (mcomb == 0)
		return COLEX_MASK_STOP(mask,k);

	comb = 0;
	for (i=0,j=0; i<int(8*sizeof(mask)); i++)
	{
		if (TSTBIT(mask,i))
		{
			if (TSTBIT(mcomb,i)) comb = SETBIT(comb,j);
			j++;
		}
	}

	comb = prev_comb_colex_stateless(n,k,comb);
	mcomb = 0;
	for (i=0,j=0; i<int(8*sizeof(mask)); i++)
	{
		if (TSTBIT(mask,i))
		{
			if (TSTBIT(comb,j)) mcomb = SETBIT(mcomb,i);
			j++;
		}
	}
	return mcomb;
}

uint32_t colex_mask_stop(uint32_t mask, uint8_t k)
{
	uint32_t comb = mask;
	uint8_t i,d,m1 = ONES(mask);

	if (k > m1)
		return comb;
	for (i=0,d=m1-k; d; i++)
	{
		if (TSTBIT(comb,i))
		{
			comb = CLRBIT(comb,i);
			d--;
		}
	}
	return comb;
}

/**
 * erzeugt (n über k) Kombinationen in Gray-Code-Reihenfolge
 * und codiert sie als Bits in einem 64 Bit-Integer.
 */
/*
uint64_t next_comb_gray(uint32_t n, uint32_t k, bool init)
{
	uint32_t j;
	uint64_t val;
	static uint32_t i;
	static uint32_t t;
	static int32_t p[65], g[65];
	static int32_t gray_init = 0;

	if (n>64 || k>n) return 0;

	if (gray_init==0 || init)
	{
		for (j = 1; j <= k; j++)
		{
			g[j] = 1;
			p[j] = j + 1;
		}
		for (j = k+1; j <= n+1; j++)
		{
			g[j] = 0;
			p[j] = j+1;
		}

		p[1] = k+1;
		t = k;
		i = 0;

		gray_init = 1;
	}
	
	if (i==n+1)
	{
		gray_init = 0;
		return 0;
	}

	for (val=0,j=n; j; j--) val |= (g[j]<<(j-1));
	// for (j=n; j; j--) printf(" %2d", g[j]); putchar('\n');

	i = p[1];
        p[1] = p[i];
        p[i] = i + 1;

	if (g[i] == 1)
	{
		if (t)
			g[t] = !g[t];
		else
			g[i-1] = !g[i-1];
		t++;
	}
	else
	{
		if (t != 1)
			g[t-1] = !g[t-1];
		else
			g[i-1] = !g[i-1];
		t--;
	}

	g[i] = !g[i];

	if (t == i-1 || t == 0)
		t++;
	else
	{
		t -= g[i-1];
		p[i-1] = p[1];
		if (t)
			p[1] = t + 1;
		else
			p[1] = i - 1;
	}
	return val;
}
*/

// Anwendungsbeispiel (Alle Kombinationen der Kombinationen n=..., k=2):
//
// #define COLEX_FINISHED(C,n,k)   ((C)[0]==(n)-(k))
//
// uint32_t C1[n], C2[n];
// 
// C1[0] = n;
// do
// {
// 	next_comb_colex(n,k,C1);
// 	C2[0] = n;
// 	do
// 	{
// 		next_comb_colex(n,k,C2);
// 		printf("%i %i <-> %i %i\n", C1[0],C1[1],C2[0],C2[1]);
// 	}
// 	while (not COLEX_FINISH(C2,n,k));
// }
// while (not COLEX_FINISH(C1,n,k));
//
uint32_t * next_comb_colex(uint32_t n, uint32_t k, uint32_t *C, bool init)
{
	uint32_t i,j;
	
	if (C[0] < n-k && not init)
	{
		j=0;
		while (C[j]+1 >= C[j+1]) j++;
		C[j]++;
		for (i=0; i<j; i++) C[i]=i;
	}
	else for (i=0; i<k; i++) C[i]=i; // Initialisierung
	return C;
}

/* Funktionen zur Verarbeitung von Bondomer-Permutationen */

/*
 * Zählt die Einträge einer Permutation, die != -1 sind.
 * (als "Gewicht" der Permutation)
 *
 * @param P Permutationsvektor
 * @param P_len Länge des Permutationsvektors
 * @return Gewicht der Permutation
 */
size_t bond_perm_weight(int * P, size_t P_len)
{
	size_t i;
	size_t weight;
	for (i=0,weight=0; i<P_len; i++)
		if (P[i] != -1) weight++;
	return weight;
}

/*
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
uint32_t bond_comb_shuffle(uint32_t comb, int * P, size_t P_len, size_t P_weight)
{
	size_t i,j;
	uint32_t shuffle = 0;
	
	// es müssen perm_weight Bits geshuffled werden:
	//   j zählt die Bits in "comb" auf
	//   i zählt die Indizes der Permutation auf
	for (i=0,j=0; i<P_len; i++)
	{
		// Falls der Permutationsvektor an Index i eine
		// -1 eingetragen hat (=neues Bond), wird eine 0 in
		// in shuffle eingetragen; steht an Index i KEINE
		// -1, wird getestet, ob Bit j von comb gesetzt ist
		if (P[i] != -1)
		{
			// Falls Bit j von comb gesetzt ist, wird Bit i
			// in shuffle gesetzt:
			if (TSTBIT(comb,j)) shuffle = SETBIT(shuffle, i);
			
			// es wurde gerade ein Bit aus k_comb verarbeitet;
			// j hochzählen:
			j++;
		}
	}

	// an dieser Stelle gilt j==P_weight, dann j muß alle Indizes von
	// comb besucht haben.
	return shuffle;
}

/*
 * Permutiert mit Hilfe einer Bondomer-Permutation
 *
 * @param in Konkatenierte Bits des Edukt-Seite
 * @param P Permutationsvektor zum Zusammenbau der Produkt-Seite
 * @param P_len Länge des Permutationsvektors (=Länge der Produkt-Seite)
 * @return konkatenierte Produktseite (=permutierte Eduktseite)
 */
uint32_t bond_reaction_shuffle(uint32_t in, int * P, size_t P_len)
{
	size_t i;
	uint32_t out = 0;

	// Bits aus "in" gemäß Permutation auf "out" übertragen;
	// dort, wo die Permutation eine "-1" hat, wird in "out"
	// eine "0" eingetragen:
	for (i=0; i<P_len; i++)
		if (P[i] != -1 && TSTBIT(in,P[i]))
			out = SETBIT(out,i);
	return out;
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
 *    | 1 0 1 0 0 0 0 0 |
 *    | 0 1 0 1 1 0 1 0 |
 *    | 0 0 0 0 0 1 0 1 |
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
	double * massiso
	)
{
	// fASSERT( IS_PWR2(iso_len) );
	size_t k = ONES(mask)+1;
	if (massiso == 0)
		massiso = new double[k];
	while (k) massiso[--k] = 0.;
	for (k=0; k<iso_len; k++)
		massiso[ONES(k & mask)] += iso[k];
	return massiso;
}

double * cumu2mass_iso(
	double * cumu, size_t cumu_len,
	uint32_t mask,
	double * massiso
	)
{
	double * iso = new double[cumu_len];
	for (size_t k=0; k<cumu_len; k++)
		iso[k] = cumu[k];
	cumulate(iso,cumu_len,false);
	massiso = iso2mass_iso(iso,cumu_len,mask,massiso);
	delete[] iso;
	return massiso;
}

int64_t GCD(int64_t u, int64_t v)
{
	int64_t k=0;
	int64_t t;

	// Ergebnis von GCD ist immer >=0
	if (u<0) u = -u;
	if (v<0) v = -v;

	// 0 hat beliebige Teiler; der größte Teiler
	// einer von 0 verschiedenen Zahl ist die
	// Zahl selbst. GCD(0,0) ist undefiniert; bzw.
	// die 0:
	if (u==0||v==0) return u|v;

	// finde 2er-Potenz
	while (~(u|v)&1) // solange beide gerade
	{
		k++;
		u>>=1;
		v>>=1;
	}

	if (u&1) // wenn u ungerade: t = -v
	{
		t = -v;
		goto bin_bgcd_B4;
	}
	else t = u;

	do
	{
		t>>=1;
bin_bgcd_B4:	while (~t&1) t>>=1;
		if (t>0) u = t; else v = -t;
		t = u - v;
	}
	while (t!=0);

	return u<<k;
}

#if defined(P_AIX) || defined(__INTEL_COMPILER)
#define TEST_ISINF(x)	isinf(x)
#define TEST_ISNAN(x)	isnan(x)
#else
#define TEST_ISINF(x)	std::isinf(x)
#define TEST_ISNAN(x)	std::isnan(x)
#endif
void rationalize(double x, int64_t & n, int64_t & d, int64_t maxd)
{
	int64_t n1, n2, d1, d2, a, b;
	int64_t n_o, d_o;
	bool minus = false;
	
	if (TEST_ISINF(x))
	{
		n = 1ll; d = 0ll;
		return;
	}

	if (TEST_ISNAN(x))
	{
		n = 0ll; d = 0ll;
		return;
	}

	if (x == 0.)
	{
		n = 0ll; d = 1ll;
		return;
	}

	if (x < 0.) { x = -x; minus = true; }

	n = int64_t(x);
	d = 1ll;

	n1 = 1ll; d1 = 0ll; n2 = 0ll; d2 = 1ll;
	while (not (x==0. or TEST_ISINF(x) or TEST_ISNAN(x)))
	{
		a = int64_t(x);
		x = 1./(x-double(a));


		for (b=(a+1)/2; b<a+1; b++)
		{
			n_o = b*n1+n2;
			d_o = b*d1+d2;
			if (d_o > maxd)
			{
				if (minus) n = -n;
				return;
			}
			n = n_o;
			d = d_o;
		}

		b = a*n1+n2; n2 = n1; n1 = b;
		b = a*d1+d2; d2 = d1; d1 = b;
	}
	if (minus) n = -n;
}

uint32_t update_crc32(void const * data, size_t len, uint32_t crc)
{
	static const uint32_t crc_table[256] =
	{
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
	};

	uint8_t const * ptr = (uint8_t const *)data;
	size_t i;

	if (data == 0)
		return crc;

	for (i=0; i<len; i++)
	{
		crc = (crc << 8) ^ crc_table[
			((crc >> 24) ^ *(ptr++)) % (sizeof(crc_table)/sizeof(uint32_t))
			];
	}
	return crc;
}

