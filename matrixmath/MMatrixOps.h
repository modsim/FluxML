#ifndef MMATRIXOPS_H
#define MMATRIXOPS_H

#include "charptr_array.h"
#include "IntegerMath.h"
#include "MMatrix.h"
#include "SMatrix.h"
#include "PMatrix.h"

// maximal 5 Nach-Iterationen erlauben
#define LUMAX_ITER	5

namespace flux {
namespace la {
namespace MMatrixOps {

/**
 * Berechnet die LU(P)-Zerlegung einer Matrix A mit "Partial Pivoting"
 * als Pivotisierungsstrategie. P ist ein Permutationsvektor (wird zu
 * Beginn initialisiert), in dem die Zeilenvertauschungen der Pivotisierung
 * festgehalten werden.
 *
 * @param A zu faktorisierende Matrix
 * @param P (Zeilen-)Permutationsvektor der Pivotisierung
 * @return true, falls Zerlegung erfolgreich; false bei singulärer Matrix
 */
bool _LUfactor(
	MMatrix & A,
	PMatrix & P
	);

/**
 * Berechnet die LU-Zerlegung einer Matrix A ohne jede Pivotisierungsstrategie.
 * Das funktioniert (stabil) nur auf diagonaldominanten Matrizen A, d.h. bei
 * Matrizen, bei denen ein Partial-Pivoting ohnehin nie eine Zeilenvertauschung
 * vornehmen würde.
 *
 * @param A zu faktorisierende Matrix
 */
void _LUfactor_nopivot(
	MMatrix & A
	);

/**
 * Abschätzung der Konditionszahl kappa_inf(A) in O(n^2) Schritten.
 * Nach: Gene H. Golub, Charles F. van Loan; Matrix Computations (3rd Edition)
 *
 * @param LU LU-Zerlegung einer Matrix A
 * @param P Permutationsvektor der LU-Zerlegung
 * @param norm_inf_A der Wert der Inf-Norm von A (NICHT VON LU!)
 * @return geschätzte Konditionszahl kappa_inf(A)
 */
double _LUestimateCondition(
	MMatrix const & LU,
	PMatrix const & P,
	double norm_inf_A
	);

/**
 * Gleichungssystemlöser mit Laufzeit O(N^2) (Single-RHS-Version).
 *
 * @param LU LU(P)-Zerlegung einer NxN-Matrix A
 * @param P Permutationsvektor zu LU (Länge N)
 * @param b Konstantenvektor der Länge N
 * @param x Lösungsvektor der Lange N
 */
void _LUsolve(
	MMatrix const & LU,
	PMatrix const & P,
	MVector const & b,
	MVector & x
	);

/**
 * Gleichungssystemlöser mit Laufzeit O(N^2) (Multi-RHS-Version).
 *
 * @param LU LU(P)-Zerlegung einer NxN-Matrix A
 * @param P Permutationsvektor zu LU (Länge N)
 * @param B Konstantenmatrix NxM
 * @param X Lösungsmatrix NxM
 */
void _LUsolve(
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix const & B,
	MMatrix & X
	);

/**
 * Allgemeiner Gleichungssystemlöser (Single-RHS-Version).
 * Löst ein Gleichungssystem auf Basis seiner LU(P)-Zerlegung mit
 * Laufzeit O(N^2).
 *
 * @param LU LU(P)-faktorisierte NxN-Matrix A
 * @param P Permutationsvektor zu LU (Länge N)
 * @param b Konstanten- und Lösungsvektor der Länge N
 */
void _LUsolve(
	MMatrix const & LU,
	PMatrix const & P,
	MVector & b
	);

/**
 * Gleichungssystemlöser für diagonaldominante Systeme (Single-RHS-Version).
 * Löst ein Gleichungssystem auf Basis seiner LU-Zerlegung mit
 * Laufzeit O(N^2).
 *
 * @param LU LU-faktorisierte NxN-Matrix A
 * @param b Konstanten- und Lösungsvektor der Länge N
 */
void _LUsolve_nopivot(
	MMatrix const & LU,
	MVector & b
	);

/**
 * Gleichungssystemlöser mit Laufzeit O(N^2) (Multi-RHS-Version).
 *
 * @param LU LU(P)-Zerlegung einer NxN-Matrix A
 * @param P Permutationsvektor zu LU (Länge N)
 * @param B Konstanten- und Lösungsmatrix mit N Zeilen
 */
void _LUsolve(
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix & B
	);

/**
 * Matrizen-Inversion auf Basis der LU-Zerlegung.
 *
 * @param LU LU-Zerlegung einer Matrix A (in)
 * @param P Permutationsmatrix der LU-Zerlegung (in)
 * @param I Inverse der Matrix A (out)
 */
void _LUinvert(
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix & I
	);

/**
 * Korrigiert in O(N^2) Schritten die Lösung eines Gleichungssystems,
 * dessen LU(P)-Zerlegung gegeben ist (Single-RHS-Version).
 *
 * @param A Original System-Matrix (NxN) des Gleichungssystems
 * @param LU LU-zerlegte Systemmatrix (NxN)
 * @param P Permutationsvektor der Pivotisierung von LU (Länge N)
 * @param b Konstantenvektor des Gleichungssystems (Länge N)
 * @param x geschätzte Lösung / korrigierte Lösung (Länge N)
 */
void _LUiterativeImprovement(
	MMatrix const & A,
	MMatrix const & LU,
	PMatrix const & P,
	MVector const & b,
	MVector & x
	);

/**
 * Korrigiert in O(N^2) Schritten die Lösung eines Gleichungssystems,
 * dessen LU(P)-Zerlegung gegeben ist (Multi-RHS-Version).
 *
 * @param A Original System-Matrix (NxN) des Gleichungssystems
 * @param LU LU-zerlegte Systemmatrix (NxN)
 * @param P Permutationsvektor der Pivotisierung von LU (Länge N)
 * @param B Konstantenmatrix des Gleichungssystems (NxM)
 * @param X geschätzte Lösung / korrigierte Lösung (NxM)
 */
void _LUiterativeImprovement(
	MMatrix const & A,
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix const & B,
	MMatrix & X
	);

/**
 * "Automatischer" Gauss-LU Gleichungsystemlöser. Löst ein Gleichungssystem
 * A.x=b über LU(P)-Zerlegung und einer heuristisch bestimmten Anzahl von
 * Nach-Iterationen zur Ergebnisverbesserung. Zurückgegen wird die
 * ebenfalls über eine Heuristik berechnete, geschätzte Konditionszahl von A.
 * 
 * Laufzeit und Reihenfolge der Arbeitsschritte:
 * <ol>
 *   <li>LU(P)-Zerlegung mit Partial Pivoting: O(N^3)</li>
 *   <li>Konditionszahl auf Basis von LU schätzen: O(N^2)</li>
 *   <li>Anzahl der Nach-Iterationen abschätzen: O(1)</li>
 *   <li>Gleichungssystem auf Basis von LU lösen: O(N^2)</li>
 *   <li>Nach-Iterationen auf Basis von LU: O(N^2)
 * </ol>
 *
 * @param A Systemmatrix (NxN) des Gleichungssystems
 * @param LU LU-Zerlegung der Matrix A
 * @param P Permutationsmatrix zur LU-Zerlegung von A
 * @param b Konstantenvektor (N) des Gleichungssystems
 * @param x Ergebnisvektor (N)
 * @param digits_needed erwünschte Genauigkeit als Anzahl von Dezimalstellen
 * @param maxiter Maximale Anzahl von Nach-Iterationen
 * @return geschätzte Konditionszahl von A; -1 bei singulärem A
 */
double _LUsolveDigits(
	MMatrix const & A,
	MMatrix const & LU,
	PMatrix const & P,
	MVector const & b,
	MVector & x,
	int & digits,
	int max_iter = LUMAX_ITER
	);

/**
 * "Automatischer" Gauss-LU Gleichungsystemlöser. Löst ein Gleichungssystem
 * A.X=B über LU(P)-Zerlegung und einer heuristisch bestimmten Anzahl von
 * Nach-Iterationen zur Ergebnisverbesserung. Zurückgegen wird die
 * ebenfalls über eine Heuristik berechnete, geschätzte Konditionszahl von A.
 * 
 * Laufzeit und Reihenfolge der Arbeitsschritte:
 * <ol>
 *   <li>LU(P)-Zerlegung mit Partial Pivoting: O(N^3)</li>
 *   <li>Konditionszahl auf Basis von LU schätzen: O(N^2)</li>
 *   <li>Anzahl der Nach-Iterationen abschätzen: O(1)</li>
 *   <li>Gleichungssystem auf Basis von LU lösen: O(N^2)</li>
 *   <li>Nach-Iterationen auf Basis von LU: O(N^2)
 * </ol>
 *
 * @param A Systemmatrix (NxN) des Gleichungssystems
 * @param LU LU-Zerlegung der Matrix A
 * @param P Permutationsmatrix zur LU-Zerlegung von A
 * @param B Konstantenmatrix (NxM) des Gleichungssystems
 * @param X Ergebnismatrix (NxM)
 * @param digits_needed erwünschte Genauigkeit als Anzahl von Dezimalstellen
 * @param maxiter Maximale Anzahl von Nach-Iterationen
 * @return geschätzte Konditionszahl von A; -1 bei singulärem A
 */
double _LUsolveDigits(
	MMatrix const & A,
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix const & B,
	MMatrix & X,
	int & digits,
	int max_iter = LUMAX_ITER
	);

/**
 * Schätzung der korrekten Dezimalstellen einer Lösung eines Gleichungssystems
 * bei gegebener Konditionszahl kappa_inf und gegebener Anzahl von
 * Nach-Iterationen. Es werden die Heuristiken II und III aus dem Kap. 3
 * von "Golub, van Loan; Matrix Computations" verwendet.
 *
 * @param kappa_inf Konditionszahl einer Systemmatrix eines Gleichungssystems
 * @param k Anzahl der Nach-Iterationen [0,n]
 * @return geschätzte Anzahl der korrekten Nachkommastellen der Lösung
 */
int _LUestimateDigits(
	double kappa_inf, int k
	);

/**
 * Allgemeiner Gauss-Jordan-Algorithmus zum Lösen eines Gleichungssystems mit
 * Rang-Defizit. Es wird eine Gesamt-Pivot-Suche durchgeführt (jeweils die
 * komplette Restmatrix wird nach dem besten Pivot-Wert abgesucht). Es wird
 * das Gleichungssystem A.K=bK gelöst, wobei K die spezielle und homogene
 * Lösung beschreibt. K ist eine Matrix mit cols(A) Zeilen und rank(A)+1
 * Spalten. In der ersten Spalte steht die spezielle Lösung x von A.x=b. Hat
 * A ein Rang-Defizit so enthalten die weiteren Spalten von K die homogene,
 * von cols(A)-rank freien Variablen abhängige Lösung. Wird A mit K
 * multipliziert, so ist das Ergebnis eine Matrix bK, deren erste Spalte der
 * Vektor b ist und die in den restlichen Spalten nur noch 0-Vektoren hat.
 *
 *      |x0 b11 b12 ... b1k |  c = Spaltenzahl von A; cols(A)
 *      |x1 b21 b22 ... b2k |  k = cols(A)-rank(A); Anz. freier Variable
 * K := |.  ...             |
 *      |.  .               |
 *      |xn bc1 bc2 ... bck |
 * 
 * Der Permutationsvektor Pc beschreibt, welche Variablen-Indizes als frei bzw.
 * abhängig gewählt wurden. Dabei sind Indizes 0..rank(A)-1 abhängige Variablen
 * und die Indizes rank(A)..(cols(A)-rank(A)) freie Variablen. Die Indizes der
 * freien Variablen entsprechen den Spalten der K-Matrix +1.
 *
 * Umgekehrt kann man aus einer Belegung der freien Variablen die Werte der
 * abhängigen Variablen berechen, indem man dem Vektor der freien
 * Variablenwerte eine 1 voranstellt [ 1 , v_free ]' und ihn auf die Matrix
 * von rechts aufmultipliziert.
 *
 * BEISPIEL:
 *
 * A=                b=        K=         Pc=         rank=
 *   7   0  -7   0     0         0 1/3       1 0 0 0       3
 *   8   1  -5  -2     0   ==>   0  1   ,    0 1 0 0
 *   0   1  -3   0     0         0 1/3       0 0 1 0
 *   0   3  -6  -1     0         0  1        0 0 0 1
 *
 * wenn die Spalten von A die Abhängigkeiten von den Variablen a, b, c, d
 * beschreiben, dann ist wegen rank=3 und Pc "d" die freie Variable, d.h. die
 * Zeilen der zweite Spalte von K beschreiben die Abhängigkeit der abhängigen
 * Variablen von d: a=1/3*d, b=d, c=1/3*d, d=d. Kontrolle:
 *   7/3*d + 0 -7/3*d = 0
 *   8/3*d + d -5/3*d -2*d = 0
 *   0*d + 3*d -6/3*d -d = 0
 *
 * Ist beim Start Pcf(j)=0, so wird die Variable j freigehalten. Ist nach dem
 * Aufruf Pcf(j)=k und k<rank, so ist Variable j eine abhängige Variable. Ist
 * umgekeht Pcf(j)=k und k>=rank, so wurde j zur freien Variablen.
 *
 * Falls ein Fehler auftritt, gibt die Funktion false zurück. In diesem Fall
 * codiert rank die Art des Fehlers als negative Zahl:
 *   rank == -2 => zu viele freie Variablen für vorgefundenen Rangabfall
 *   rank == -3 => freie Variablen konnten nicht freigehalten werden
 *   rank == -4 => das System hat keine Lösung
 *
 * Im Fall von rank = -3 enthält Pc einen Vorschlag für abhängige Variablen.
 * In diesem Fall steht in Pc(j) eine 2 falls die Variable ein möglicher
 * Kandidat für eine abhängige Variable ist.
 *
 * @param A eine quadratische oder "liegende" Matrix (Spalten>Zeilen) (in/out)
 * @param b ein Konstantenvektor (ggfs. auch 0-Vektor) (in/out)
 * @param K Matrix mit spezieller und homogener Lösung (out)
 * @param Pcf Spalten-Permutationsmatrix (in/out)
 * @param rank Rang der Matrix A bzw. Fehlercode (in/out)
 * @return true, bei Erfolg, false bei einem Fehler (siehe rank)
 */
bool gaussJordan(
	MMatrix & A,
	MVector & b,
	MMatrix & K,
	PMatrix & Pcf,
	int & rank
	);

/**
 * Ganzzahl-Version von gaussJordan().
 * Soweit kein Overflow auftritt, wird der exakte Rang ermittelt.
 * In den Diagonalelementen diag(A(1:rank,1:rank)) befinden sich nach dem
 * Aufruf Skalierungsfaktoren, welche >1 sind, falls keine ganzzahlige
 * Lösung existiert. Die spezielle und die homogenen Lösungen der abhängigen
 * Unbekannten x(i) ergeben sich für i=0:rank-1 als K(Pcf(i),:)/A(i,Pcf(i)).
 * 
 * for (k=0; k<rank; k++)
 * 	K(Pcf(k),:) /= A(k,Pcf(k));
 *
 * @param A eine quadratische oder "liegende" Matrix (Spalten>Zeilen) (in/out)
 * @param b ein Konstantenvektor (ggfs. auch 0-Vektor) (in/out)
 * @param K Matrix mit spezieller und homogener Lösung (in/out)
 * @param Pcf Spalten-Permutationsmatrix (in/out)
 * @param rank Rang der Matrix A bzw. Fehlercode (in/out)
 * @return true, bei Erfolg, false bei einem Fehler (siehe rank)
 */
bool gaussJordan(
	GMatrix< int64_t > & A,
	GVector< int64_t > & b,
	GMatrix< int64_t > & K,
	PMatrix & Pcf,
	int & rank
	);

/**
 * Wrapper für gaussJordan zum Finden freier Variablen.
 * Sucht eine Auswahl freier Flüsse die einer (u.U.) ungültigen Auswahl
 * in Pc am nähesten kommt. Die Parameter und Fehlercodes entsprechen
 * denen von gaussJordan. Der zusätzliche Parameter d_max ermöglicht es
 * die maximal erlaubte Distanz zur vorherigen Auswahl (Pc) anzugeben.
 *
 * Falls Variable c_max übergeben wird, enhält sie nach dem Aufruf die
 * Anzahl der getesteten Kombinationen. Enhält sie vor dem Aufruf eine
 * Zahl >0, dann wird die Anzahl der Tests auf *c_max beschränkt.
 *
 * @param A eine quadratische oder "liegende" Matrix (Spalten>Zeilen)
 * @param b ein Konstantenvektor (ggfs. auch 0-Vektor)
 * @param S Matrix mit spezieller und homogener Lösung
 * @param Pc Spalten-Permutationsmatrix / Auswahl freier/abh. Variablen (in/out)
 * @param rank Rang von A bzw. der entsprechende Fehlercode von gaussJordan
 * @param d_max maximal erlaubte Distanz zur alten Auswahl
 * @param c_max maximal erlaubte Anzahl zu testender Kombinationen (optional)
 * @see gaussJordan
 * @return true, falls Auswahl gefunden
 */
bool gaussJordanMatch(
	MMatrix & A,
	MVector & b,
	MMatrix & S,
	PMatrix & Pc,
	int & rank,
	size_t d_max,
	int * c_max = 0
	);

/**
 * Rangbestimmung per SVD.
 * Es wird dasselbe Verfahren wie in Octave verwendet -- der Rang einer
 * Matrix entspricht also der Anzahl der Singulärwerte die größer als
 * eine vorgegebene Schwelle "tol" sind. Wird der Funktion der Parameter
 * tol mit dem Default-Wert 0 übergeben, so wird tol nach folgender
 * Formel berechnet:
 *   tol = Zeilen * smax * MACHEPS;
 * 
 * Dabei ist smax der größte Singulärwert und MACHEPS die kleinste von
 * 0 unterscheidbare double-Zahl.
 *
 * @param A eine Matrix, deren Rang bestimmt werden soll
 * @param log10_cond log10(Konditionszahl) (optional)
 * @param tol ein Toleranzwert (optional)
 * @return der numerische Rang der Matrix A
 */
int rank(MMatrix A, double * log10_cond = 0, double tol = 0.);

/**
 * Exakte Rangbestimmung per Integer-Rechnung (gaussJordan()).
 *
 * @param A eine Integer-Matrix
 */
int rank(GMatrix< int64_t > A);


/**
 * Berechnet eine Zeilen-Reduktion (row reduction) einer Matrix A zum
 * homogenen Gleichungsystem A*x=0:
 * Falls rank(A)<rows(A) wird A mittels Gauss-Jordan-Algorithmus in eine
 * reduzierte Zeilen-Stufen-Form B umgerechnet, wobei rank(B)=rows(B)=rank(A)
 * und ebenfalls B*x=0. Die Funktion dient etwa dazu, eine Stöchiometrie
 * "redundanzfrei" zu machen.
 * 
 * @param A eine Matrix, nach Aufruf die zeilenreduzierte Matrix B
 */
void rowReduce(MMatrix & A);

/**
 * Zeilenreduktion mittels QR-Algorithmus (numerische Rangbestimmung).
 * Es gibt keine Einschränkungen bzgl. der Dimension von A.
 *
 * @see	rowReduce
 * @param A eine Matrix, nach Aufruf eine zeilenreduzierte Matrix B
 */
void rowReduceQR(MMatrix & A);

/**
 * Zeilenreduktion mittels QR-Algorithmus (numerische Rangbestimmung).
 * Es gibt keine Einschränkungen bzgl. der Dimension von A, b.
 *
 * @see	rowReduce
 * @param A eine Matrix, nach Aufruf eine zeilenreduzierte Matrix
 * @param b ein Vektor, nach Aufruf ein dimensionsreduzierter Vektor
 */
void rowReduceQR(MMatrix & A, MVector & b);

/**
 * rowReduce()-Variante mit Ganzzahlrechnung (exakt!).
 *
 * @param A eine Matrix, nach Aufruf die zeilenreduzierte Matrix B
 */
void rowReduce(GMatrix< int64_t > & A);

/**
 * Berechnet eine Zeilen-Reduktion (row reduction) einer Matrix A und eines
 * Vektors b zum Gleichungsystem A*x=b:
 * Falls rank(A)<rows(A) wird A mittels Gauss-Jordan-Algorithmus in eine
 * reduzierte Zeilen-Stufen-Form A' umgerechnet, wobei rank(A')=rows(A')=rank(A)
 * und ebenfalls A'*x=b'. Die Funktion dient etwa dazu, eine Stöchiometrie
 * "redundanzfrei" zu machen.
 * 
 * @param A eine Matrix, nach Aufruf die zeilenreduzierte Matrix A'
 * @param b ein Vektor, nach Aufruf der zeilenreduzierte Vektor b'
 */
void rowReduce(MMatrix & A, MVector & b);

/**
 * rowReduce()-Variante mit Ganzzahlrechnung (exakt!).
 *
 * @param A eine Matrix, nach Aufruf die zeilenreduzierte Matrix A'
 * @param b ein Vektor, nach Aufruf der zeilenreduzierte Vektor b'
 */
void rowReduce(GMatrix< int64_t > & A, GVector< int64_t > & b);


/**
 * Berechnet die Kovarianz-Matrix aus der Jacobi-Matrix J und der
 * Diagonalmatrix Sigma, die Varianzen zu den Messungen angibt.
 *
 *  (J^T.Sigma^(-1).J)^(-1) = [ (sqrt(Sigma)^(-1).J)^T.(sqrt(Sigma)^(-1).J) ]^(-1)
 *                          = (A^T.A)^(-1), mit A = sqrt(Sigma)^(-1).J
 *
 * Die Berechnung basiert auf der QR(P)-Zerlegung A.P=Q.R der skalierten
 * Jacobi-Matrix A.
 *
 *  (A^T.A)^(-1) = P.(R^T.R)^(-1).P^T
 *
 * Das Verfahren stammt aus MINPACK.
 * ACHTUNG: Die Jacobi-Matrix wird hier als bereits skaliert angenommen
 *
 * @param J bereits skalierte Jacobi-Matrix (in), Kovarianzmatrix (out)
 * @param tol Toleranzwert für Rangbestimmung
 * @return Indizes unbestimmbarer Variablen
 */
GVector< int > covarianceFromJacobian(
	MMatrix & J,
	double tol// = MACHEPS
	);

} // namespace flux::la::MMatrixOps
} // namespace flux::la
} // namespace flux

#endif

