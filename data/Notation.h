#ifndef NOTATION_H
#define NOTATION_H

#include "charptr_array.h"
#include "BitArray.h"

/*
 * Parser für MS / MSMS / 1H-NMR / 13C-NMR / Cumomer Kurznotationen
 *
 * @author Michael Weitzel <info@13cflux.net>
 */

namespace flux {
namespace data {
namespace Notation {

/**
 * Parser für Bereichsnotation (Fragmentnotation),
 * z.B: "1-3,5,7-9"
 *
 * Rückgabewerte:
 * 0 := kein Fehler
 * 1 := Parse-Error
 * 2 := Ungültige Bereiche (msfrag_range)
 * 3 := Überlappende Bereiche
 *
 * @param s Zeichenkette (in)
 * @param mask Bitmaske, die das Fragment beschreibt (out)
 * @return Fehlercode
 */
int parse_range_spec(
	char * s,
	BitArray & mask
	);

/**
 * Wandelt eine Bitmaske in einen Range-String um.
 *
 * @param mask Bitmaske
 * @return Range-String (allokiert!)
 */
char * mask_to_range(
	BitArray const & mask
	);

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
 *
 * @param s Zeichenkette (in)
 * @param mname Metabolitname, allokiert (out)
 * @param weights sort. Liste von Gewichten, allokiert, mit -1 terminiert (out)
 * @param mask Bit-Maske der markierbaren Positionen
 * @return Fehlercode
 */
int parse_MS_spec(
	char * s,
	char ** mname,
	int ** weights,
	BitArray & mask
	);

/**
 * Parser für multi-isotopic Tracer MS-Fragment-Notation:
 *   Pool#Mn
 *   Pool#Mn,m,...
 *   Pool[m,n-o,p,q]#M(u,v,w)
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
 *
 * @param s Zeichenkette (in)
 * @param mname Metabolitname, allokiert (out)
 * @param weights sort. Liste von Gewichten, allokiert, mit -1 terminiert (out)
 * @param mask Bit-Maske der markierbaren Positionen
 * @return Fehlercode
 */
int parse_MIMS_spec(
	char * s,
	char ** mname,
	int ** weights,
	BitArray & mask,
        int * NUM_ISOTOPES
	);
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
 *
 * @param s Zeichenkette (in)
 * @param mname Metabolitname, allokiert (out)
 * @param weights Liste der Gewichte, terminiert mit -1, allokiert (out)
 * @param mask1 Bit-Maske des Fragments
 * @param mask2 Bit-Maske des Fragment-Fragments
 * @return Fehlercode
 */
int parse_MSMS_spec(
	char * s,
	char ** mname,
	int ** weights,
	BitArray & mask1,
	BitArray & mask2
	);

/**
 * Parser für 1H-NMR-Spezifikation
 * Rückgabewerte:
 *
 * 1 := Parse Error (ungültiger Poolname)
 * 2 := Fehler in Positionsliste
 * 3 := Positionen doppelt angegeben
 *
 * Die Indizierung in poslst beginnt bei 1!
 *
 * @param s Zeichenkette (in)
 * @param mname Metabolitname, allokiert (out)
 * @param poslst Liste von Atom-Positionen, terminert mit -1, allokiert (out)
 * @return Fehlercode
 */
int parse_1HNMR_spec(
	char * s,
	char ** mname,
	int ** poslst
	);

/**
 * Parser für 13C-NMR-Notaton
 *
 *  Pool#(S|DL|DR|DD|T)positionen
 *
 *  type := S->1, DL->2, DR->3, DD->4, T->5
 *
 * Die Indizierung in poslst beginnt bei 1!
 *
 * Rückgabewerte:
 * 1 := Parse Error (ungültiger Poolname)
 * 2 := Fehler in Positionsliste
 * 3 := Positionen doppelt angegeben
 *
 * @param s Zeichenkette (in)
 * @param mname Metabolitname, allokiert (out)
 * @param poslst Liste von Atom-Positionen, terminiert mit -1, allokiert (out)
 * @param typelst Liste von 13C-NMR-Typen, terminiert mit -1, allokiert (out)
 * @return Fehlercode
 */
int parse_13CNMR_spec(
	char * s,
	char ** mname,
	int ** poslst,
	int ** typelst
	);

/**
 * Parser für die generische Markierungs-Notation
 *
 * Pool#[01x]+
 *
 * Es wird eine Liste von zu variierenden Positionen zurückgegeben,
 * sowie eine Maske, welche die restliche Markierungskonfiguration
 * beschreibt.
 *
 * @param s Zeichenkette (in)
 * @param mname Metabolitname (out)
 * @param xmask Bit-Maske der x-Positionen (out)
 * @param mask Bitmaske der 1-Positionen (out)
 * @return Fehlercode
 */
int parse_cumomer_spec(
	char * s,
	char ** mname,
	BitArray & xmask,
	BitArray & mask
	);

/**
 * Identifiziert eine Kurznotation und prüft ihre semantische Korrektheit.
 *
 * @param s Kurznotation
 * @param valid falls übergeben, wird in valid das Ergebis der
 * 	Korrektheitsprüfung abgelegt
 * @param dim falls übergeben, wird in dim die Anzahl der zu erwartenden
 * 	Messwerte gespeichert
 * @return -1 bei Fehler; ansonsten MS=1,MSMS=2,NMR1H=3,NMR13C=4,generic=5
 */
int check_spec(char * s, bool * valid = 0, int * dim = 0, char * pname= 0);

/**
 * Parst etwas in der Art von C#3@6 in der Form
 * s => [element]#[atomnr]@[posid]
 *
 * @param s
 * @param element
 * @param atomnr
 * @param posid
 */
int parse_perm_spec_tag(
	char * s,
	char ** element,
	int * atomnr,
	char ** posid
	);

/**
 * Identifiziert eine Permutations-Spezifikation.
 *
 * @param s Spezifikation
 * @return 0 für leeren String, 1 für Kurznotation, 2 für Langnotation, -1 für Fehler
 */
int identify_perm_spec(char const * s);

/**
 * Berechnet die Länge (=Anzahl der Atome) einer Permutations-Spezifikation.
 *
 * @param s Spezifikation
 * @return Anzahl der Atome oder -1 bei Fehler
 */
int perm_spec_length(
	char const * s
	);

/**
 * Prüft auf Gültigkeit einer Permutations-Spezifikation einer Edukt- oder
 * Produkt-Seite einer Reaktion und identifiziert die verwendete Notation.
 *
 * @param spec Permutations-Spezifikation der beteiligten Metabolite
 * @return 1 für Kurznotation, 2 für Langnotation, -1 für Fehler
 */
int check_perm_spec(charptr_array const & spec);

/**
 * Prüft auf Gültigkeit eines Variablennamens.
 *
 * @param s etwaiger Variablenname
 * @return true, falls s ein gültiger Variablenname ist
 */
bool is_varname(char const * s);

} // namespace flux::data::Notation
} // namespace flux::data
} // namespace flux

#endif

