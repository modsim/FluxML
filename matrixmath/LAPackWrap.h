#ifndef LAPACKWRAP_H
#define LAPACKWRAP_H

#include "MMatrix.h"
#include "MVector.h"
#include "PMatrix.h"


/**
 * Der FORTRAN-Typ COMPLEX*16 entspricht einem Struct
 * von double-Werten für real und imag:
 */
struct COMPLEX16
{
	double re_, im_;
	double real() const { return re_; }
	double imag() const { return im_; }
	COMPLEX16() : re_(0.), im_(0.) { }
	COMPLEX16(double re, double im ) : re_(re), im_(im) { }
};

namespace flux {
namespace la {
namespace lapack {

/**
 * Singulärwertzerlegung.
 *
 * @param A MxN-Matrix (nach dem Aufruf zerstört) (in/out)
 * @param s Vektor mit berechneten Singulärwerten (Dimension min(M,N)) (out)
 */
void svd(MMatrix & A, MVector & s);

/**
 * Singulärwertzerlegung. A=U . diag(s) . VT
 * Vektor s kann als die Diagonale einer MxN-Matrix angesehen werden.
 * Da die Diagonale maximal min(M,N) Einträge hat, ist die Dimension
 * von Vektor s dim(s)=min(M,N).
 *
 * @param A MxN-Matrix
 * @param U MxM-Matrix
 * @param s Vektor mit berechneten Singulärwerten (Dimension min(M,N))
 * @param VT NxN-Matrix
 */
void svd(MMatrix & A, MMatrix & U, MVector & s, MMatrix & VT);

/**
 * LU-Faktorisierung. Die übergebene Matrix wird LU(P)-zerlegt.
 * Die Permutation wird vom Fortran-Format (Array-Start bei 1)
 * ins C-Format konvertiert (Array-Start bei 0).
 *
 * @param A Matrix (LU-Zerlegung) (in/out)
 * @param P Permutation der LU-Zerlegung (out)
 * @return false bei singulärer Matrix A, sonst true
 */
bool LUfactor(MMatrix & A, PMatrix & P);

/**
 * LU-Lösung.
 *
 * @param LU LU(P) faktorisierte Matrix A
 * @param P Permutation der LU-Faktorisierung
 * @param b rechte Seite (in) und Lösung (out)
 * @return true, falls richtig implementiert ( also immer ;) )
 */
bool LUsolve(MMatrix & LU, PMatrix & P, MVector & b);

/**
 * LU-Lösung (multi-RHS).
 *
 * @param LU LU(P) faktorisierte Matrix A
 * @param P Permutation der LU-Faktorisierung
 * @param B rechte Seite (in) und Lösung (out)
 * @return true, falls richtig implementiert ( also immer ;) )
 */
bool LUsolve(MMatrix & LU, PMatrix & P, MMatrix & B);

/**
 * Matrix-Inversion über LU(P)-Zerlegung mit DGETRF+DGETRI.
 *
 * @param A Matrix
 * @return false bei singulärer Matrix A, sonst true
 */
bool invert(MMatrix & A);

/**
 * Multi-RHS-Gleichungssystemlöser (LU, Blackbox).
 * Die Matrix A wird durch den Aufruf zerstört.
 *
 * @param A NxN-Matrix (in/out)
 * @param XB rechte Seite (in) / Lösung (out)
 * @return false bei singulärer Matrix A, sonst true
 */
bool linsolve(MMatrix & A, MMatrix & XB);

/**
 * Gleichungssystemlöser (LU, Blackbox).
 * Die Matrix A wird durch den Aufruf zerstört.
 *
 * @param A NxN-Matrix (in/out)
 * @param xb rechte Seite (in) / Lösung (out)
 * @return false bei singulärer Matrix A, sonst true
 */
bool linsolve(MMatrix & A, MVector & xb);

/**
 * QR-Faktorisierung. A = Q.R
 * Q und R werden als Zeiger übergeben, falls für Q und R 0-Zeiger übergeben
 * werden, wird nur die in A 'verpackte' Form berechnet.
 *
 * @param A MxN-Matrix
 * @param Q Zeiger auf orthogonale MxM-Matrix
 * @param R Zeiger auf MxN-Matrix
 * @return false im Fehlerfall
 */
bool qr(MMatrix & A, MMatrix * Q = 0, MMatrix * R = 0);

/**
 * QR-Faktorisierung. A = Q.R
 * Alternative Schnittstelle.
 *
 * @param A MxN-Matrix
 * @param Q Orthogonale MxM-Matrix
 * @param R MxN-Matrix
 * @return false im Fehlerfall
 */
inline bool qr(MMatrix & A, MMatrix & Q, MMatrix & R) { return qr(A,&Q,&R); }

/**
 * Rangbestimmung mit QR(P)-Faktorisierung.
 *
 * @return rang(A) oder -1 bei Fehler
 */
int qr_rank(MMatrix A);

/**
 * QR(P)-Faktorisierung (QR-Faktorisierung mit Spalten-Pivotisierung). A.P=Q.R
 *
 * @param A MxN-Matrix
 * @param Q Orthogonale MxM-Matrix
 * @param R MxN-Matrix
 * @param P NxN-Permutationsmatrix der Spaltenpermutation
 * @return false im Fehlerfall
 */
bool qrp(MMatrix & A, MMatrix & Q, MMatrix & R, PMatrix & P);

#if 0
/**
 * VORSICHT: PROBLEME?
 * QR-Least-Squares für beliebige Systeme A.X=B (DGELSY)
 *
 * @param A MxN-Matrix
 * @param B MxL-Konstantenmatrix (rechte Seite)
 * @param X NxL-Matrix für die Lösung
 * @param JPVT Integer-Vektor Länge N; Präferenz freier/abhängiger Spalten/Variablen
 * @param RCOND Parameter zur Rangbestimmung
 * @return Rang der Matrix A, -1 bei Fehler
 */
int lssolve(MMatrix & A, MMatrix const & B, MMatrix & X, GVector< int > JPVT, double RCOND = 0.001);

/**
 * VORSICHT: PROBLEME?
 * QR-Least-Squares für beliebige Systeme A.x=b (DGELSY)
 *
 * @param A MxN-Matrix
 * @param b M-Konstantenvektor (rechte Seite)
 * @param x N-Vektor für die Lösung
 * @param JPVT Integer-Vektor Länge N; Präferenz freier/abhängiger Spalten/Variablen
 * @param RCOND Parameter zur Rangbestimmung
 * @return Rang der Matrix A, -1 bei Fehler
 */
int lssolve(MMatrix & A, MVector const & b, MVector & x, GVector< int > JPVT, double RCOND = 0.001);
#endif

/**
 * QR-Least-Squares-Solver für beliebige Systeme (DGELS).
 * Matrix A darf nicht rank-deficient sein.
 *
 * @param A MxN-Matrix
 * @param b M-Konstantenvektor
 * @param x N-Lösungsvektor
 * @return true, falls alles ok
 */
bool lssolve(MMatrix & A, MVector const & b, MVector & x);

/**
 * QR-Least-Squares-Solver für beliebige Systeme (DGELS).
 * Matrix A darf nicht rank-deficient sein.
 *
 * @param A MxN-Matrix
 * @param B MxL-Konstantenmatrix
 * @param X NxL-Lösungsmatrix
 * @return true, falls alles ok
 */
bool lssolve(MMatrix & A, MMatrix const & B, MMatrix & X);

#if 0
// NICHT FERTIG?!
/**
 * SVD-Least-Squares-Solver für beliebige Systeme (DGELSS).
 * Matrix A darf rank-deficient sein.
 *
 * RCOND: Singular values S(i) <= RCOND*S(1) are treated as zero.
 *        If RCOND < 0, machine precision is used instead.
 *
 * @param A MxN-Matrix
 * @param b M-Konstantenvektor
 * @param x N-Lösungsvektor
 * @param RANK numerischer Rang der Matrix A (out)
 * @param RCOND Schranke für Singulärwerte
 * @return true, falls alles ok, false, falls SVD nicht konvergiert
 */
bool lssolve_svd(MMatrix & A, MVector const & b, MVector & x, int & RANK, double RCOND = 0.01);
#endif

/**
 * Schur-Zerlegung A = Z.T.Z^H.
 *
 * @param A komplexe Matrix; wird mit T überschrieben (in/out)
 * @param w komplexer Vektor mit Eigenwerten (out)
 * @param Z Unitäre Matrix Z
 * @return true, falls alles ok
 */
bool schur(GMatrix< COMPLEX16 > & A, GVector< COMPLEX16 > & w, GMatrix< COMPLEX16 > & Z);

/**
 * Cholesky-Zerlegung A = U^T.U einer symmetrischen, positiv
 * definiten Matrix A.
 *
 * @param A NxN-Matrix (in/out)
 * @return true, falls alles ok
 */
bool cholesky(MMatrix & A);

/**
 * Berechnet die Eigenwerte einer symmetrischen Matrix.
 *
 * @param A symmetrische Matrix (in)
 * @param W aufsteigend sortierte Eigenwerte (out)
 */
bool symmeig(MMatrix & A, MVector & W);

} // namespace flux::la::lapack
} // namespace flux::la
} // namespace flux

#endif

