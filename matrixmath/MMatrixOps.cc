#include <cmath>

#include "Error.h"
#include "Combinations.h"
#include "Sort.h"
#include "BitArray.h"
#include "IntegerMath.h"
#include "SMatrix.h"
#include "PMatrix.h"
#include "MVector.h"
#include "LAPackWrap.h"
#include "MMatrixOps.h"
#include "cstringtools.h"

#include <limits>

// ein double ist etwa auf 15 Dezimal-Stellen genau
#define DBL_DIGITS	15

#ifdef SCALED_PP
#define TINY		10./HUGE_VAL
#endif

namespace flux {
namespace la {
namespace MMatrixOps {

bool _LUfactor(
	MMatrix & A,
	PMatrix & P
	)
{
	int i,j,k,n,pk,tmp_i;
	double pivot, tmp;
	
	n = A.rows();
	
	// Initialisiere P:
	P.initIdent();

#ifdef SCALED_PP
	MVector scale(n);
	double abs, abs_max;
	for (k=0; k<n; k++)
	{
		for (j=0,abs_max=0.; j<n; j++)
		{
			abs = fabs(A.get(k,j));
			if (abs > abs_max) abs_max = abs;
		}
		scale(k) = abs_max;
	}
#endif
	
	// LU(P)-Zerlegung: P.A=L.U
	for (k=0; k<=n-2; k++)
	{
#ifdef SCALED_PP
		// Pivot-Suche (Scaled-Partial-Pivoting)
		pk=k; pivot=fabs(A(k,k))/scale(k);
		for (i=k+1; i<n; i++)
		{
			tmp = fabs(A(i,k));
			if (scale(i) >= TINY*tmp)
			{
				tmp /= scale(i);
				if (tmp>pivot) { pivot=tmp; pk=i; }
			}
		}
#else
		// "normale" Pivot-Suche (Partial Pivoting)
		pk=k; pivot=fabs(A(k,k));
		for (i=k+1; i<n; i++)
		{
			tmp = fabs(A(i,k));
			if (tmp>pivot) { pivot=tmp; pk=i; }
		}
#endif
		
		// falls das Pivot 0 ist, ist die Matrix singulär
		if (pivot==0.) return false;

		// Zeilenvertauschung
		if (pk != k)
		{
			for (i=0; i<n; i++)
			{
				tmp = A(pk,i);
				A(pk,i) = A(k,i);
				A(k,i) = tmp;
			}
			tmp_i = P(pk);
			P(pk) = P(k);
			P(k) = tmp_i;
		}

		// Elimination
		for (i=k+1; i<n; i++)
		{
			A(i,k) /= A(k,k);
			for (j=k+1; j<n; j++)
				A(i,j) -= A(i,k)*A(k,j);
		}
	}
	return true;
}

void _LUfactor_nopivot(
	MMatrix & A
	)
{
	int i,j,k,n = A.rows();
	
	// LU-Zerlegung: A=L.U
	for (k=0; k<=n-2; k++)
	{
		// Elimination
		for (i=k+1; i<n; i++)
		{
			A(i,k) /= A(k,k);
			for (j=k+1; j<n; j++)
				A(i,j) -= A(i,k)*A(k,j);
		}
	}
}

double _LUestimateCondition(
	MMatrix const & LU,
	PMatrix const & P,
	double norm_inf_A
	)
{
	int j,k,n=LU.rows();
	double yk_plus, yk_minus, abs;
	double norm_inf_y, norm_inf_r, norm_inf_z;
	MVector p(n), y(n), r(n), w(n), z(n);
	MVector pk_plus(n);
	MVector pk_minus(n);

	// 2.1.: Löse U^T.y=d (wie estimate_condition; zu Beginn p(0:n-1)=0)
	//       und berechne norm_inf(y):
	for (k=0,norm_inf_y=0.; k<n; k++)
	{
		yk_plus  = (1. - p(k)) / LU.get(k,k);
		yk_minus = (-1. - p(k)) / LU.get(k,k);
		
		for (j=k; j<n; j++)
		{
			// Durch transponierten Zugriff wird hier U^T.y=d gelöst:
			// (es wird auf LU(k,j) zugegr. statt auf LU(j,k) !)
			double LU_kj = LU.get(k,j);
			pk_plus(j) = p(j) + LU_kj * yk_plus;
			pk_minus(j) = p(j) + LU_kj * yk_minus;
		}
		
		if (fabs(yk_plus) + pk_plus.norm1(k,n-1)
			>= fabs(yk_minus) + pk_minus.norm1(k,n-1))
		{
			y(k) = yk_plus;
			for (j=k; j<n; j++) p(j) = pk_plus(j);
		}
		else
		{
			y(k) = yk_minus;
			for (j=k; j<n; j++) p(j) = pk_minus(j);
		}

		// Inf-Norm-Berechnung für y:
		abs = fabs(y(k));
		if (abs > norm_inf_y) norm_inf_y = abs;
	}
	
	// y := y / norm_inf_y
	for (k=0; k<n; k++) y(k) /= norm_inf_y;
	
	// norm_inf(T*y) ist jetzt eine Näherung für 1/norm_inf(T^-1).
	// norm_inf(y)*norm_inf(T) ist eine Näherung für
	// kappa_inf(T):=norm_inf(A)*norm_inf(A^-1).

	// Heuristik 2:
	// bei einem Rundungsfehler (roundoff) von 10^-d und einer
	// Konditionszahl von 10^q ergibt sich aus der Gauß'schen
	// Elimination eine Lösung mit d-q korrekten Ziffern (digits).
	// Beim double-Typ gilt etwa d=15;

	// 2.2.: Löse L^T.r=y; dann L.w=P.r; dann U.z=w
	
	// 2.2.1.: Löse L.r=y; Achtung! L hat auf der Diagonalen 1en!
	r(0) = y(0);
	norm_inf_r = fabs(r(0));
	for (k=1; k<n; k++)
	{
		for (r(k)=y(k),j=0; j<k; j++)
			r(k) -= LU.get(k,j)*r(j);
		abs = fabs(r(k));
		if (abs > norm_inf_r) norm_inf_r = abs;
	}

	// 2.2.2.: Löse L.w=P.r
	w(0) = r(P.get(0));
	for (k=1; k<n; k++)
		for (w(k)=r(P.get(k)),j=0; j<k; j++)
			w(k) -= LU.get(k,j)*w(j);
	
	// 2.2.3.: Löse U.z=w
	for (k=n-1,norm_inf_z=0.; k>=0; k--)
	{
		for (z(k)=w(k),j=n-1; j>k; j--)
			z(k) -= LU.get(k,j)*z(j);
		z(k) /= LU.get(k,k);
		abs = fabs(z(k));
		if (abs > norm_inf_z) norm_inf_z = abs;
	}

	// 2.3.: Berechne Konditionszahl:
	//       kappa_inf = norm_inf(A) . norm_inf(z)/norm_inf(r)
	return norm_inf_A * norm_inf_z / norm_inf_r;
}

void _LUsolve(
	MMatrix const & LU,
	PMatrix const & P,
	MVector const & b,
	MVector & x
	)
{
	int j,k,n = x.dim();
	MVector r(n);

	// Berechne L.r = P.b
	for (k=0; k<n; k++)
		for (r(k)=b.get(P.get(k)),j=0; j<k; j++)
			r(k) -= LU.get(k,j)*r(j);

	// Berechne R.x = r
	for (k=n-1; k>=0; k--)
	{
		// x(k) = r(k) kann entfallen, da r in x gespeichert wird
		x(k) = r(k);
		for (j=n-1; j>k; j--)
			x(k) -= LU.get(k,j)*x(j);
		x(k) /= LU.get(k,k);
	}
}

/**
 * Allgemeiner Gleichungssystemlöser.
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
	)
{
	int i,k,n = b.dim();
	MVector r(n);

	// Forward Substitution
	r(0) = b(P.get(0));
	for (i=1; i<n; i++)
		for (k=0,r(i)=b(P.get(i)); k<i; k++)
			r(i) -= LU.get(i,k)*r(k);

	// Back Substitution
	b(n-1) = r(n-1) / LU.get(n-1,n-1);
	for (i=n-2; i>=0; i--)
	{
		b(i) = r(i);
		for (k=i+1; k<n; k++)
			b(i) -= LU.get(i,k)*b(k);
		b(i) /= LU.get(i,i);
	}
}

void _LUsolve_nopivot(
	MMatrix const & LU,
	MVector & b
	)
{
	int i,k,n = b.dim();
	MVector r(n);

	// Forward Substitution
	r(0) = b(0);
	for (i=1; i<n; i++)
		for (k=0,r(i)=b(i); k<i; k++)
			r(i) -= LU.get(i,k)*r(k);

	// Back Substitution
	b(n-1) = r(n-1) / LU.get(n-1,n-1);
	for (i=n-2; i>=0; i--)
	{
		b(i) = r(i);
		for (k=i+1; k<n; k++)
			b(i) -= LU.get(i,k)*b(k);
		b(i) /= LU.get(i,i);
	}
}

void _LUsolve(
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix & B
	)
{
	int i,j,k,n = B.rows();
	MVector r(n);

	for (j=0; j<(int)B.cols(); j++)
	{
		// Forward Substitution
		r(0) = B(P.get(0),j);
		for (i=1; i<n; i++)
			for (k=0,r(i)=B(P.get(i),j); k<i; k++)
				r(i) -= LU.get(i,k)*r(k);

		// Back Substitution
		B(n-1,j) = r(n-1) / LU.get(n-1,n-1);
		for (i=n-2; i>=0; i--)
		{
			B(i,j) = r(i);
			for (k=i+1; k<n; k++)
				B(i,j) -= LU.get(i,k)*B(k,j);
			B(i,j) /= LU.get(i,i);
		}
	}
}

void _LUsolve(
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix const & B,
	MMatrix & X
	)
{
	int i,j,k,n = B.rows();
	MVector r(n);

	for (j=0; j<(int)B.cols(); j++)
	{
		// Berechne L.r = P.b (Forward Substitution)
		for (i=0; i<n; i++)
			for (r(i)=B.get(P.get(i),j),k=0; k<i; k++)
				r(i) -= LU.get(i,k)*r(k);
	
		// Berechne R.x = r (Back Substitution)
		X(n-1,j) = r(n-1) / LU.get(n-1,n-1);
		for (i=n-2; i>=0; i--)
		{
			// x(i) = r(i) kann entfallen, da r in x gespeichert wird
			X(i,j) = r(i);
			for (k=n-1; k>i; k--)
				X(i,j) -= LU.get(i,k)*X(k,j);
			X(i,j) /= LU.get(i,i);
		}
	}
}

void _LUinvert(
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix & I
	)
{
	size_t i,j;
	size_t n = LU.rows();

	// Vorgehensweise zur Bildung der inversen Matrix ist dieselbe
	// wie beim Lösen von Gleichungssystemen - der Aufwand ist
	// dim-mal so hoch: Die Matrix-Spalten der Einheitsmatrix
	// entsprechen dem b-Vektor. Der Ergebnis-Vektor entspricht
	// einer Spalte der inversen Matrix:
	for (i=0; i<n; i++)
		for (j=0; j<n; j++)
			I.set(i,j,i==j);

	_LUsolve(LU,P,I);
}

void _LUiterativeImprovement(
	MMatrix const & A,
	MMatrix const & LU,
	PMatrix const & P,
	MVector const & b,
	MVector & x
	)
{
	int j,k,n = x.dim();
	MVector r(n), y(n);
	MVector & z = r;
	
	// Berechne Residuum: r = b - A.x (x ist die zuvor berechnete Näherung)
	for (k=0; k<n; k++)
		for (r(k)=b.get(k),j=0; j<n; j++)
			r(k) -= A.get(k,j)*x.get(j);

	// Berechne L.y = P.r
	y(0) = r(P.get(0));
	for (k=1; k<n; k++)
		for (y(k)=r(P.get(k)),j=0; j<k; j++)
			y(k) -= LU.get(k,j)*y(j);
	
	// Berechne U.z = y; x := x+z
	for (k=n-1; k>=0; k--)
	{
		for (z(k)=y(k),j=n-1; j>k; j--)
			z(k) -= LU.get(k,j)*z(j);
		z(k) /= LU.get(k,k);
		x(k) += z(k);
	}
}

// multi-RHS-Version
void _LUiterativeImprovement(
	MMatrix const & A,
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix const & B,
	MMatrix & X
	)
{
	int j,k,l,n = X.rows();
	int nu = X.cols();
	MMatrix R(n,nu), Y(n,nu);
	MMatrix & Z = R;
	
	// Berechne Residuum: R = B - A.X (X ist die zuvor berechnete Näherung)
	for (l=0; l<nu; l++)
		for (k=0; k<n; k++)
			for (R(k,l)=B.get(k,l),j=0; j<n; j++)
				R(k,l) -= A.get(k,j)*X.get(j,l);

	// Berechne L.Y = P.R
	for (l=0; l<nu; l++)
	{
		Y(0,l) = R(P.get(0),l);
		for (k=1; k<n; k++)
			for (Y(k,l)=R(P.get(k),l),j=0; j<k; j++)
				Y(k,l) -= LU.get(k,j)*Y(j,l);
	}
	
	// Berechne U.Z = Y; X := X+Z
	for (l=0; l<nu; l++)
		for (k=n-1; k>=0; k--)
		{
			for (Z(k,l)=Y(k,l),j=n-1; j>k; j--)
				Z(k,l) -= LU.get(k,j)*Z(j,l);
			Z(k,l) /= LU.get(k,k);
			X(k,l) += Z(k,l);
		}
}

int _LUestimateDigits(
	double kappa_inf, int k
	)
{
	int q = (int)round(log10(kappa_inf));

	if (k == 0)
		return DBL_DIGITS-q;
	else
	{
		int c = k*(DBL_DIGITS-q);
		return DBL_DIGITS < c ? DBL_DIGITS : c;
	}
}

double _LUsolveDigits(
	MMatrix const & A,
	MMatrix const & LU,
	PMatrix const & P,
	MVector const & b,
	MVector & x,
	int & digits,
	int max_iter
	)
{
	int iter, digits_iter;
	double kappa_inf_estimated;

	// 1. Mit der Inf-Norm von A die Konditionszahl kappa_inf
	//    schätzen
	kappa_inf_estimated = _LUestimateCondition(LU,P,A.normInf());
	
	// 3. Anzahl der Nach-Iterationen abschätzen
	iter = 0;
	digits_iter = _LUestimateDigits(kappa_inf_estimated,0);
	if (digits_iter < digits)
		do digits_iter = _LUestimateDigits(kappa_inf_estimated,++iter);
		while (digits_iter < digits && iter < max_iter);

	// 4. Gleichungssystem lösen
	_LUsolve(LU,P,b,x);

	// 5. Iterative Improvement
	//printf("Berechne %i Nachiterationen!\n", iter);
	while (iter--) _LUiterativeImprovement(A,LU,P,b,x);

	// erreichte / geschätzte Präzision zurückgeben
	digits = digits_iter;
	// Konditionszahl zurückgeben
	return kappa_inf_estimated;
}

// multi-RHS-Version
double _LUsolveDigits(
	MMatrix const & A,
	MMatrix const & LU,
	PMatrix const & P,
	MMatrix const & B,
	MMatrix & X,
	int & digits,
	int max_iter
	)
{
	int iter, digits_iter;
	double kappa_inf_estimated;

	// 1. Mit der Inf-Norm von A die Konditionszahl kappa_inf
	//    schätzen
	kappa_inf_estimated = _LUestimateCondition(LU,P,A.normInf());
	
	// 3. Anzahl der Nach-Iterationen abschätzen
	iter = 0;
	digits_iter = _LUestimateDigits(kappa_inf_estimated,0);
	if (digits_iter < digits)
		do digits_iter = _LUestimateDigits(kappa_inf_estimated,++iter);
		while (digits_iter < digits && iter < max_iter);

	// 4. Gleichungssystem lösen
	_LUsolve(LU,P,B,X);

	// 5. Iterative Improvement
	//printf("Berechne %i Nachiterationen!\n", iter);
	while (iter--) _LUiterativeImprovement(A,LU,P,B,X);

	// erreichte / geschätzte Präzision zurückgeben
	digits = digits_iter;
	// Konditionszahl zurückgeben
	return kappa_inf_estimated;
}

bool gaussJordan(
	MMatrix & A,
	MVector & b,
	MMatrix & K,
	PMatrix & Pcf,
	int & rank
	)
{
	int i,j,k,pr,pc,pk,pj,free,user_free;
	int R = A.rows();
	int C = A.cols();
	double pivot, abs, mult, tolerance;
	bool zero;
	PMatrix Pc(C);

	fASSERT(Pcf.dim() == A.cols());
	fASSERT(b.dim() == A.rows());

	// Anzahl der vom Benutzer als frei deklarierten Variablen zählen
	for (i=0,user_free=0; i<C; i++)
		if (Pcf(i) == 0) user_free++;

	// Spalten freier Variablen in den rechten Teil der Matrix tauschen.
	// Spalten abhängiger Variablen in den linken Teil der Matrix tauschen.
	Pc.initIdent();
	i=-1;
	j=C;
	for (;;)
	{
		do j--; while (j>i and Pcf(j) == 0);
		do i++; while (i<j and Pcf(i) != 0);

		if (i<j)
		{
			// Tausch in der Spaltenpermutation
			Pc.swap(i,j);
			Pcf.swap(i,j);
			// Spaltentausch
			A.swapColumns(i,j);
		}
		else break;
	}

	// Toleranz für die Rang-Bestimmung:
	// Die rref-Funktion von Octave nimmt hier MAX(R,C) als Multiplikator
	// für MACHEPS*A.normInf(). Das macht aber keinen Sinn, denn die
	// Fehlerfortpflanzung findet entlang der Zeilen statt (Spalten werden
	// "parallel" verarbeitet). Also wird hier mit R multipliziert:
	tolerance = R * std::numeric_limits<double>::epsilon() * A.normInf();

	for (k=0; k<R; k++) // ALLE R Zeilen eliminieren (0 .. R-1)
	{
		// Initialisierung der Pivot-Zeilen / -Spalten
		pr = k;
		pc = k;
		pivot = fabs(A(pr,pc));

		// Pivot in der Restmatrix suchen.
		// Abbruch, sobald Index j die Spalten der freizuhaltenden
		// Variablen erreicht.
		for (j=k; j<C-user_free; j++)
		{
			for (i=k; i<R; i++)
			{
				abs = fabs(A(i,j));
				if (abs > pivot)
				{
					pivot = abs;
					pr = i;
					pc = j;
				}
			}
		}

		// bevor durch 0 geteilt wird hier abgebrochen -- genauere
		// Beurteilung der Situation erfolgt unten
		if (pivot <= tolerance)
			break;
		
		// müssen Zeilen getauscht werden?
		if (pr != k)
		{
			// Zeilentausch k<->pr
			A.swapRows(k,pr);
			// b-Vektor anpassen:
			b.swap(k,pr);
		}

		// müssen Spalten getauscht werden?
		if (pc != k)
		{
			// Spaltentausch k<->pc
			A.swapColumns(k,pc);
			// Permutationsvektor Pc anpassen
			Pc.swap(k,pc);
		}

		// Eliminationsschritt
		for (i=k+1; i<R; i++)
		{
			mult = - A(i,k) / A(k,k);
			b(i) += mult * b(k);
			
			// multipliziere Zeile k mit mult und addiere
			// auf Zeile i
			for (j=k; j<C; j++)
				A(i,j) += mult * A(k,j);
		}
	}

	// Wenn hier k<R ist, dann wurde vorzeitig abgebrochen. Jetzt müssen
	// zwei Fälle unterschieden werden:
	//  1. Die Restmatrix A(k:R-1,C-user_free:C-1) ist eine Nullmatrix. Die
	//     ursprüngliche Matrix A enthielt linear abhängige Zeilen.
	//     Es gibt zusätzlich (R-k) freie Variablen
	//  2. Die Restmatrix A(k:R-1,C-user_free:C-1) ist KEINE Nullmatrix. Die
	//     Elimination hätte fortgesetzt werden können, wenn dafür
	//     als frei markierte Spalten hinzugezogen worden wären.
	//     Die Wahl der freien Variablen ist also illegal. Evtl.
	//     interessant wäre jetzt, welche als frei markierten
	//     Variablen in Frage kämen --> Rückgabe im Permutationsvektor
	rank = k;
	if (rank < R)
	{
		// ist A(k:R-1,C-user_free:C-1) ist eine Nullmatrix?
		for (j=C-user_free,zero=true; j<C and zero; j++)
			for (i=rank; i<R and zero; i++)
				if (fabs(A(i,j)) > tolerance)
					zero = false;
				else
					// klare Verhältnisse schaffen
					A(i,j) = 0.;

		// Fall 2:
		if (not zero)
		{
			// Vorschlag für freie/abhängige Variablen in den
			// Permutationsvektor schreiben. Für abhängige
			// Variablen wird eine 1 eingetragen. Für freie
			// eine 0:
			Pcf.fill(0); // alles frei

			// bisherige abhängige Variablen bleiben abhängig
			for (j=0; j<rank; j++)
				Pcf(Pc(j)) = 1;

			for (; j<C; j++)
			{
				for (i=rank,zero=true; i<R and zero; i++)
					if (fabs(A(i,j)) > tolerance)
						zero = false;
				if (not zero)
					// möglicher Pivot-Kandidat gefunden
					Pcf(Pc(j)) = 2;
			}
			rank = -3;
			K = MMatrix();
			return false;
		}
	}

	// Wenn es 0-Zeilen gibt, so müssen die entsprechenden Einträge im
	// b-Vektor jetzt auch 0en enthalten -- ansonsten besitzt das System
	// keine Lösung:
	for (i=rank; i<R; i++)
	{
		if (fabs(b(i)) > tolerance)
		{
			K = MMatrix();
			rank = -4;
			return false;
		}
		else
			// klare Verhältnisse herstellen:
			b(i) = 0.;
	}

	// Der Benutzer will möglicherweise mehr Variablen freihalten als
	// das System hergibt:
	if (user_free > C-rank)
	{
		rank = -2;
		K = MMatrix();
		return false;
	}

	// Gauss-Jordan-Elimination:
	// Die Zeilen 0 bis rank-1 werden zur Einheitsmatrix eliminiert
	// (reduced echelon form)
	for (k=rank-1; k>=0; k--)
	{
		// Eliminationsschritt
		for (i=k-1; i>=0; i--)
		{
			mult = - A(i,k) / A(k,k);
			b(i) += mult * b(k);
			
			// multipliziere Zeile k mit mult und addiere
			// auf Zeile i
			for (j=k; j<C; j++)
				A(i,j) += mult * A(k,j);
		}
		// Teile die Zeile durch A(k,k), damit eine 1 auf der
		// Diagonalen steht
		mult = 1. / A(k,k);
		A(k,k) = 1.;
		for (j=rank; j<C; j++)
			A(k,j) *= mult;
		b(k) *= mult;
	}

	free = C-rank;
	// in der ersten Spalte der Lösungsmatrix K steht die spezielle
	// Lösung des Gleichungssystems - in den weiteren Spalten stehen
	// die Koeffizienten der freien Variablen (d.h. die Lösung des
	// homogenen Gleichungssystems A.x=b mit b=(0)).
	K = MMatrix(C,free+1);
	
	// die Spalten von K sind in der selben Permutation wie die Spalten
	// der Matrix A nach dem Gauß-Algorithmus -> ent-permutieren
	PMatrix Psfree = Pc.sortPerm(rank, C-1).inverse();

	// K aufbauen:
	for (k=0; k<rank; k++)
	{
		// durch das Spaltentauschen der Pivotisierung verändert
		// sich die Zeilen-Anordnung der Lösungen:
		pk = Pc(k);

		// konstanter Teil der Lösung; beim Zugriff auf b muß nicht
		// permutiert werden, da Zeilenvertauschungen auch auf b
		// durchgeführt wurden.
		if (fabs(b(k))>=std::numeric_limits<double>::epsilon())
			K(pk,0) = b(k);
		
		for (j=rank; j<C; j++)
		{
			pj = Psfree(j-rank)+1;
			if (fabs(A(k,j))>=std::numeric_limits<double>::epsilon()) K(pk,pj) = - A(k,j);
		}
	}

	// für die freien Variablen noch 1en in die K-Matrix eintragen
	for (k=0; k<free; k++)
	{
		pj = Psfree(k)+1;
		K(Pc(k+rank),pj) = 1.;
	}

	// die Spalten von A ent-permutieren falls die Matrix noch gebraucht
	// wird (siehe etwa rowReduce)
	Pc.permColumns(A);

	// abhängige Variablen sortieren (eigentlich unwichtig ...)
	Pc.sort(0,rank-1);
	// Indizes der freien Variablen sortieren
	Pc.sort(rank,C-1);
	// Spaltenpermutation Pc zurückgeben
	Pcf = Pc;
	return true;
}

bool gaussJordan(
	GMatrix< int64_t > & A,
	GVector< int64_t > & b,
	GMatrix< int64_t > & K,
	PMatrix & Pcf,
	int & rank
	)
{
	int i,j,k,pr,pc,pk,pj,free,user_free;
	int R = A.rows();
	int C = A.cols();
	int64_t g, a_ik, a_kk;
	bool zero;
	PMatrix Pc(C);

	fASSERT(Pcf.dim() == A.cols());
	fASSERT(b.dim() == A.rows());

	// Anzahl der vom Benutzer als frei deklarierten Variablen zählen
	for (i=0,user_free=0; i<C; i++)
		if (Pcf(i) == 0) user_free++;

	// Spalten freier Variablen in den rechten Teil der Matrix tauschen.
	// Spalten abhängiger Variablen in den linken Teil der Matrix tauschen.
	Pc.initIdent();
	i=-1;
	j=C;
	for (;;)
	{
		do j--; while (j>i and Pcf(j) == 0);
		do i++; while (i<j and Pcf(i) != 0);

		if (i<j)
		{
			// Tausch in der Spaltenpermutation
			Pc.swap(i,j);
			Pcf.swap(i,j);
			// Spaltentausch
			A.swapColumns(i,j);
		}
		else break;
	}

	for (k=0; k<R; k++) // ALLE R Zeilen eliminieren (0 .. R-1)
	{
		// Initialisierung der Pivot-Zeilen / -Spalten
		pr = k;
		pc = k;

		// Pivot in der Restmatrix suchen. Es wird abgebrochen,
		// sobald ein Eintrag != 0 gefunden wird bzw. Index j
		// die Spalten der freizuhaltenden Variablen erreicht:
		for (j=k; j<C-user_free; j++)
			for (i=k; i<R; i++)
				if (A(i,j) != 0) { pr = i; pc = j; goto gj_check_pivot; }

		// bevor durch 0 geteilt wird hier abgebrochen -- genauere
		// Beurteilung der Situation erfolgt unten
gj_check_pivot:
		if (A(pr,pc) == 0)
			break;

		// müssen Zeilen getauscht werden?
		if (pr != k)
		{
			// Zeilentausch k<->pr
			A.swapRows(k,pr);
			// b-Vektor anpassen:
			b.swap(k,pr);
		}

		// müssen Spalten getauscht werden?
		if (pc != k)
		{
			// Spaltentausch k<->pc
			A.swapColumns(k,pc);
			// Permutationsvektor Pc anpassen
			Pc.swap(k,pc);
		}

		// In jedem Schritt ggT der Restmatrix:
		for (i=k; i<R; i++)
		{
			g = b(i);
			for (j=k; j<C; j++)
				g = GCD(g,A(i,j));
			if (g == 0) break;
			if (g != 1ll)
			{
				b(i) = divI(b(i),g); // b(i) /= g;
				for (j=k; j<C; j++)
					A(i,j) = divI(A(i,j),g); // A(i,j) /= g;
			}
		}

		// Eliminationsschritt
		for (i=k+1; i<R; i++)
		{
			g = GCD(A(k,k),A(i,k));
			fASSERT(g!=0ll);
			a_kk = divI(A(k,k),g); // A(k,k)/g;
			a_ik = divI(A(i,k),g); // A(i,k)/g;

			b(i) = subI(mulI(b(i),a_kk),mulI(b(k),a_ik)); // b(i)*a_kk - b(k)*a_ik;
			A(i,k) = 0ll;
			for (j=k+1; j<C; j++)
				A(i,j) = subI(mulI(A(i,j),a_kk),mulI(A(k,j),a_ik)); // A(i,j)*a_kk-A(k,j)*a_ik;
		}
	}
	// Wenn hier k<R ist, dann wurde vorzeitig abgebrochen. Jetzt müssen
	// zwei Fälle unterschieden werden:
	//  1. Die Restmatrix A(k:R-1,C-user_free:C-1) ist eine Nullmatrix. Die
	//     ursprüngliche Matrix A enthielt linear abhängige Zeilen.
	//     Es gibt zusätzlich (R-k) freie Variablen
	//  2. Die Restmatrix A(k:R-1,C-user_free:C-1) ist KEINE Nullmatrix. Die
	//     Elimination hätte fortgesetzt werden können, wenn dafür
	//     als frei markierte Spalten hinzugezogen worden wären.
	//     Die Wahl der freien Variablen ist also illegal. Evtl.
	//     interessant wäre jetzt, welche als frei markierten
	//     Variablen in Frage kämen --> Rückgabe im Permutationsvektor
	rank = k;
	if (rank < R)
	{
		// ist A(k:R-1,C-user_free:C-1) ist eine Nullmatrix?
		for (j=C-user_free,zero=true; j<C and zero; j++)
			for (i=rank; i<R and zero; i++)
				if (A(i,j) != 0ll)
					zero = false;

		// Fall 2:
		if (not zero)
		{
			// Vorschlag für freie/abhängige Variablen in den
			// Permutationsvektor schreiben. Für abhängige
			// Variablen wird eine 1 eingetragen. Für freie
			// eine 0:
			Pcf.fill(0); // alles frei

			// bisherige abhängige Variablen bleiben abhängig
			for (j=0; j<rank; j++)
				Pcf(Pc(j)) = 1;

			for (; j<C; j++)
			{
				for (i=rank,zero=true; i<R and zero; i++)
					if (A(i,j) != 0ll)
						zero = false;
				if (not zero)
					// möglicher Pivot-Kandidat gefunden
					Pcf(Pc(j)) = 1;
			}
			rank = -3;
			K = GMatrix< int64_t >();
			return false;
		}
	}

	// Wenn es 0-Zeilen gibt, so müssen die entsprechenden Einträge im
	// b-Vektor jetzt auch 0en enthalten -- ansonsten besitzt das System
	// keine Lösung:
	for (i=rank; i<R; i++)
	{
		if (b(i) != 0)
		{
			K = GMatrix< int64_t >();
			rank = -4;
			return false;
		}
	}

	// Der Benutzer will möglicherweise mehr Variablen freihalten als
	// das System hergibt:
	if (user_free > C-rank)
	{
		rank = -2;
		K = GMatrix< int64_t >();
		return false;
	}

	// Eliminationsschritt (Gauss-Jordan)
	for (k=rank-1; k>=0; k--)
	{
		for (i=k-1; i>=0; i--)
		{
			g = GCD(A(k,k),A(i,k));
			fASSERT(g!=0ll);
			a_kk = divI(A(k,k),g); // A(k,k)/g;
			a_ik = divI(A(i,k),g); // A(i,k)/g;

			b(i) = subI(mulI(b(i),a_kk),mulI(b(k),a_ik)); // b(i)*a_kk - b(k)*a_ik;
			for (j=i; j<C; j++)
				A(i,j) = subI(mulI(A(i,j),a_kk),mulI(A(k,j),a_ik)); // A(i,j)*a_kk - A(k,j)*a_ik;
		}

		// fertige Zeile durch den ggT teilen:
		g = GCD(b(k),A(k,k));
		for (j=rank; j<C; j++)
			g = GCD(g,A(k,j));
		// keine negativen Werte auf der Diagonalen
		if (A(k,k)<0ll) g = -g;

		if (g!=1ll)
		{
			b(k) = divI(b(k),g); // b(k) /= g;
			A(k,k) = divI(A(k,k),g); // A(k,k) /= g;
			for (j=rank; j<C; j++) A(k,j) = divI(A(k,j),g); // A(k,j) /= g;
		}
	}

	free = C-rank;
	// in der ersten Spalte der Lösungsmatrix K steht die spezielle
	// Lösung des Gleichungssystems - in den weiteren Spalten stehen
	// die Koeffizienten der freien Variablen (d.h. die Lösung des
	// homogenen Gleichungssystems A.x=b mit b=(0)).
	K = GMatrix< int64_t >(C,free+1,0ll);
	
	// die Spalten von K sind in der selben Permutation wie die Spalten
	// der Matrix A nach dem Gauß-Algorithmus -> ent-permutieren
	PMatrix Psfree = Pc.sortPerm(rank, C-1).inverse();

	// S aufbauen:
	for (k=0; k<rank; k++)
	{
		// durch das Spaltentauschen der Pivotisierung verändert
		// sich die Zeilen-Anordnung der Lösungen:
		pk = Pc(k);

		// konstanter Teil der Lösung; beim Zugriff auf b muß nicht
		// permutiert werden, da Zeilenvertauschungen auch auf b
		// durchgeführt wurden.
		if (b(k) != 0)
			K(pk,0) = b(k);
		
		for (j=rank; j<C; j++)
		{
			pj = Psfree(j-rank)+1;
			if (A(k,j) != 0) K(pk,pj) = - A(k,j);
		}
	}

	// für die freien Variablen noch 1en in die K-Matrix eintragen
	for (k=0; k<free; k++)
	{
		pj = Psfree(k)+1;
		K(Pc(k+rank),pj) = 1ll;
	}

	// die Spalten von A ent-permutieren falls die Matrix noch gebraucht
	// wird (siehe etwa rowReduce)
	Pc.permColumns(A);

	// Im Gegensatz zur Floating-Point-Version von gaussJordan darf die
	// Spalten-Permutation Pc jetzt nicht sortiert werden, da in den
	// Diagonalelementen der abhängigen Variablen noch Koeffizienten !=0
	// stehen (beim FP-gaussJordan stehen da 1en, d.h. es darf sortiert
	// werden).
	
	// Spaltenpermutation Pc zurückgeben
	Pcf = Pc;
	return true;
}

bool gaussJordanMatch(
	MMatrix & A,
	MVector & b,
	MMatrix & K,
	PMatrix & Pc,
	int & rank,
	size_t d_max,
	int * c_max
	)
{
	size_t i,k,m;
	int c;
	MMatrix Aold(A);
	MVector bold(b);
	PMatrix Pcold(Pc);
	BitArray Dmask(Pc.dim());
	BitArray Fmask(Pc.dim());

	// Initiale Auswahl prüfen und bei Erfolg die Suche abbrechen.
	// Es kann sein, dass nicht genug freie variablen gewählt
	// wurden. In diesem Fall wählt gaussJordan zusätzliche
	// freie Variablen und löst das System. Das ist kein Fehler.
	if (gaussJordan(A,b,K,Pc,rank))
	{
		if (c_max) *c_max = 0;
		return true;
	}

	// - hat A mehr Zeilen als Spalten? (systematischer Fehler)
	// - wurden zu viele freie Variablen angegeben?
	// - hat das System keine Lösung?
	if (rank == -1 or rank == -2 or rank == -4)
	{
		if (c_max) *c_max = 0;		
		return false;
	}

	// gibt es eine Beschränkung der maximalen Tests?
	if (c_max)
	{
		// keine Beschränkung:
		if (*c_max <= 0)
			*c_max = INT_MAX;
	}

	// ursprüngliche Auswahl der free/dep Variablen speichern
	for (i=0; i<Pcold.dim(); i++)
	{
		if (Pcold.get(i) == 0)
			Fmask.set(i);
		else
			Dmask.set(i);
	}

	// Suche eingrenzen
	k = Dmask.countOnes();
	m = k<(Pc.dim()-k) ? k : (Pc.dim()-k);
	m = d_max<m ? d_max : m;

	for (i=1,c=0; i<=m; i++)
	{
		BitArray::comb_iterator D = BitArray::firstComb(Dmask,i);
		while (D)
		{
			BitArray::comb_iterator F = BitArray::firstComb(Fmask,i);
			while (F)
			{
				A = Aold;
				b = bold;
				Pc = Pcold;
				b = bold;
				Pc = Pcold;

				for (k=0; k<Pc.dim(); k++)
				{
					if (D->get(k))
						// k wird frei
						Pc(k) = 0;
					if (F->get(k))
						// k wird abhängig
						Pc(k) = 1;
				}

				// eine weitere Kombination getestet:
				c++;

				// Auswahl testen:
				if (gaussJordan(A,b,K,Pc,rank))
				{
					if (c_max) *c_max = c;
					return true;
				}

				// Hier wird nur noch ein Fehlertyp erwartet:
				// Variablen können nicht freigehalten werden
				fASSERT(rank == -3);

				// maximal erlaubte Anzahl von Kombinationen
				// erfolglos getestet?
				if (c_max and c >= *c_max)
				{
					*c_max = c;
					return false;
				}

				F++;
			}
			D++;
		}
	}

	// keine Auswahl innerhalb Distanz d_max gefunden
	rank = -3; // urspr. Rückgabewert von gaussJordan
	if (c_max) *c_max = c; // Anzahl der Tests
	return false;
}

int rank(MMatrix A, double * log10_cond, double tol)
{
	int rank;
	size_t i;
	double smin, smax;
	size_t M = A.rows();
	size_t N = A.cols();
	MVector s(MIN2(M,N));

	// die Singulärwertzerlegung berechnen, U und VT werden
	// nicht benötigt
	// Für die SVD gilt A == U.diag(s).V^T und A^-1 == V.diag(1./s).U^T
	lapack::svd(A,s);

	// minimaler und maximaler Singulärwert; die SVD sortiert
	// die Singulärwerte absteigend
	smin = s(s.dim()-1);
	smax = s(0);
	
	// Toleranz für die Rangbestimmung: Der (numerische) Rank ist hier
	// definiert als die Anzahl der Singulärwerte, deren Wert größer
	// als "tol" ist:
	if (tol == 0.)
		tol = double( MAX2(M,N) ) * smax * std::numeric_limits<double>::epsilon();

	// Rangbestimmung durch Zählen der Singulärwerte > tol:
	for (i=0,rank=0; i<s.dim(); i++)
		if (fabs(s(i)) > tol) rank++;

	// Konditionszahl chi(A) := ||A|| * ||A^-1||
	// Die Spektralnorm ist die Wurzel des maximalen Eigenwerts von
	// A*.A, wobei A* die konjugierte (falls complex), transponierte
	// Matrix ist. Ein Eigenwert a findet sich in A^-1 als 1/a wieder --
	// deshalb entspricht die Konditionszahl für die Spektralnorm dem
	// Quotienten aus (betragsmäßig) maximalem und minimalem Eigenwert:
	//
	// Hier wird die SVD-Konditionszahl als Verhältnis von größtem zu
	// kleinstem Singulärwert.
	// Falls die Konditionszahl INF ist, so gibt es einen Rangabfall;
	// falls ihre Reziproke in der Nähe der Floating-Point-Präzision
	// liegt (laut Numerical Recipes für float bei 10^-6 und double bei
	// 10^-12), ist die Matrix schlecht konditioniert und führt bei
	// Verwendung in einer GLS-Lösung evtl. zu ungenauen Ergebnissen.
	// Da nur die Größenordnung der Konditionszahl interessiert, wird
	// hier Logarithmiert; log10_cond nimmt also für akzeptabel
	// konditionierte Matrizen Werte zwischen 0 und ~6(?) an
	if (log10_cond != 0)
		*log10_cond = log10(smax) - log10(smin);
	
	return rank;
}

int rank(GMatrix< int64_t > A)
{
	bool result;
	int rank = 0;
	GMatrix< int64_t > S;
	GVector< int64_t > b(A.rows());
	PMatrix Pc(A.cols());

	b.fill(0ll);
	Pc.fill(1);

	// Gauss-Jordan-Elimination
	// Soweit es keinen Integer-Overflow gibt, wird hier der Rang
	// extakt bestimmt. Der Overflow wird nach außen weitergegeben...
	result = gaussJordan(A,b,S,Pc,rank);

	// result sollte immer true sein:
	fASSERT( result==true );
	return rank;
}

void rowReduce(MMatrix & A)
{
	int rank_A, svd_rank_A;
	bool result;

	// gaussJordan funktioniert nicht auf überbestimmten Systemen.
	if (A.rows() > A.cols())
	{
		// QR-Algorithmus verwenden (warum nicht gleich?)
		rowReduceQR(A);
		return;
	}

	MVector b(A.rows());
	PMatrix Pc(A.cols());
	Pc.fill(1);
	MMatrix oldA(A);
	MMatrix S;

	// SVD-Rangbestimmung zur Konsistenzprüfung:
	svd_rank_A = rank(oldA);
	oldA = A;
	// Gauss-Jordan-Elimination
	result = gaussJordan(A,b,S,Pc,rank_A);

	// result sollte immer true sein:
	fASSERT( result==true );

	// Uneinigkeit über numerischen Rang?
	if (rank_A != svd_rank_A)
	{
		fWARNING("different opinions about rank -- SVD: %i GJ: %i ...", svd_rank_A, rank_A);
		if (svd_rank_A < rank_A)
		{
			// Stärkere Zeilenreduktion ist möglich. Dennoch ist
			// die von GaussJordan gefundene Lösung gültig:
			fWARNING("... preferring GJ solution");
			// Teilmatrix ausschneiden
			A = A.getSlice(0,0,rank_A-1,A.cols()-1);
		}
		else
		{
			// GaussJordan hat versagt. SVD war cleverer
			fWARNING("... GJ failed. Restoring original matrix");
			A = oldA;
		}
	}
	else
	{
		if (rank_A == (int)(A.rows()))
			// keine Reduktion möglich, A wiederherstellen
			A = oldA;
		else
			// Teilmatrix ausschneiden
			A = A.getSlice(0,0,rank_A-1,A.cols()-1);
	}
}

void rowReduce(GMatrix< int64_t > & A)
{
	int rank_A;
	bool result;

	// gaussJordan funktioniert nicht auf überbestimmten Systemen.
	fASSERT(A.rows() <= A.cols());

	GVector< int64_t > b(A.rows());
	PMatrix Pc(A.cols());
	Pc.fill(1);
	GMatrix< int64_t > S;
	GMatrix< int64_t > oldA(A);

	b.fill(0ll);
	result = gaussJordan(A,b,S,Pc,rank_A);

	// result sollte immer true sein:
	fASSERT( result==true );

	if (rank_A == (int)(A.rows()))
	{
		A = oldA;
		return;
	}
	GMatrix< int64_t > newA(rank_A,A.cols());
	for (int i=0; i<rank_A; i++)
		for (size_t j=0; j<A.cols(); j++)
			newA.set(i,j,A.get(i,j));
	A = newA;
}

void rowReduce(MMatrix & A, MVector & b)
{
	int rank;
	bool result;

	// gaussJordan funktioniert nicht auf überbestimmten Systemen.
	if (A.rows() > A.cols())
	{
		// QR-Algorithmus verwenden (warum nicht gleich?)
		rowReduceQR(A,b);
		return;
	}

	PMatrix Pc(A.cols());
	Pc.fill(1);
	MMatrix oldA(A);
	MVector oldb(b);
	MMatrix S;
	result = gaussJordan(A,b,S,Pc,rank);

	if (not result or (rank == (int)(A.rows())))
	{
		A = oldA;
		b = oldb;
	}
	else
	{
		A = A.getSlice(0,0,rank-1,A.cols()-1);
		b = b.getSlice(0,rank-1);
	}
}

void rowReduce(GMatrix< int64_t > & A, GVector< int64_t > & b)
{
	int rank;
	bool result;

	// gaussJordan funktioniert nicht auf überbestimmten Systemen.
	// In diesem Fall lapack::lssolve_svd verwenden
	fASSERT(A.rows() <= A.cols());

	PMatrix Pc(A.cols());
	Pc.fill(1);
	GMatrix< int64_t > oldA(A);
	GVector< int64_t > oldb(b);
	GMatrix< int64_t > S;
	result = gaussJordan(A,b,S,Pc,rank);

	if (not result or (rank == (int)(A.rows())))
	{
		A = oldA;
		b = oldb;
	}
	else
	{
		GMatrix< int64_t > newA(rank,A.cols());
		GVector< int64_t > newb(rank);
		for (int i=0; i<rank; i++)
		{
			newb.set(i,b.get(i));
			for (size_t j=0; j<A.cols(); j++)
				newA.set(i,j,A.get(i,j));
		}
		A = newA;
		b = newb;
	}
}

void rowReduceQR(MMatrix & A)
{
	const double tol = 1e-10;
	MMatrix At = A.getTranspose();
	int M = At.rows();
	int N = At.cols();
	MMatrix Q(M,M);
	MMatrix R(M,N);
	PMatrix P(N);

	if (!lapack::qrp(At,Q,R,P))
	{
		fWARNING("QR factorization failed");
		return;
	}
	
	// Skalierung für Rangbestimmung
	double r_tol = tol * std::abs(R(1,1));
	int r = 0;
	int r_max = M<N?M:N;
	while (r<r_max && std::abs(R(r,r))>=r_tol)
		r++;

	P.sort(0,r-1);
	MMatrix Ar = MMatrix(r,M);
	for (int i=0; i<r; ++i)
	    for (int j=0; j<M; ++j)
		Ar(i,j) = A(P(i),j);
	A = Ar;
}

void rowReduceQR(MMatrix & A, MVector & b)
{
    int M = A.rows();
    int N = A.cols();
    MMatrix Ab(M,N+1); // erweitertes System Ab = [A,b]

    for (int i=0; i<M; ++i)
    {
	for (int j=0; j<N; ++j)
	    Ab(i,j) = A(i,j);
	Ab(i,N) = b(i);
    }
    rowReduceQR(Ab);
    M = Ab.rows();
    A = MMatrix(M,N);
    b = MVector(M);
    for (int i=0; i<M; ++i)
    {
	for (int j=0; j<N; ++j)
	    A(i,j) = Ab(i,j);
	b(i) = Ab(i,N);
    }
}

GVector< int > covarianceFromJacobian(
	MMatrix & J,
	double tol
	)
{
	int i, j, k, l, ii, jj;
	bool sing;
	double temp;
	double tolR;
	int M = J.rows();
	int N = J.cols();
	
	// Die Jacobi-Matrix darf nicht mehr Spalten (N) als Zeilen (M)
	// haben, da in R (MxN) die Kovarianzmatrix (NxN) erzeugt wird.
	// Es werden hier ggfs. Nullzeilen angehängt.
	if (M < N)
	{
		MMatrix Jnew(N,N);
		for (i=0; i<M; ++i)
			for (j=0; j<N; ++j)
				Jnew(i,j) = J(i,j);
		J = Jnew;
		M = N;
	}
	
	MMatrix Q(M,M),R(M,N);
	PMatrix P(N);
	MVector wa(N);

	// QRP-Zerlegung der Jacobi-Matrix berechnen
	if (not lapack::qrp(J,Q,R,P))
	{
		fWARNING("QRP factorization failed");
		J.fill(0.);
		return GVector< int >();
	}

	// Toleranz für Rangbestimmung
	tolR = tol * fabs(R(0,0));

	// den oberen Teil von R (vollbesetzte obere Dreiecksmatrix)
	// zu R^(-1) umwandeln und den Rang bestimmen
	l = -1;
	for (k=0; k<N; ++k)
	{
		if (fabs(R(k,k)) <= tolR)
			break;
		R(k,k) = 1./R(k,k);
		for (j=0; j<k; ++j)
		{
			temp = R(k,k) * R(j,k);
			R(j,k) = 0.;
			for (i=0; i<=j; ++i)
				R(i,k) -= temp*R(i,j);
		}
		l = k;
	}
	fDEBUG(0,"determined rank=%i for %ix%i-Jacobian",l+1,M,N);

	// Es gilt l=rank(R)-1=rank(J)-1. Jetzt R^(-1) in (R^T.R)^(-1) umwandeln
	for (k=0; k<=l; ++k)
	{
		for (j=0; j<k; ++j)
		{
			temp = R(j,k);
			for (i=0; i<=j; ++i)
				R(i,j) += temp * R(i,k);
		}
		for (i=0,temp=R(k,k); i<=k; ++i)
			R(i,k) *= temp;
	}

	// Indizes unbestimmbarer Variablen aufzeichnen
	GVector< int > indet(N-(l+1));

	// form the full lower triangle of the covariance matrix
	// in the strict lower triangle of R and in wa.
	for (j=0; j<N; ++j)
	{
		jj = P(j);
		sing = j > l;
		if (sing)
			indet(j-(l+1)) = jj;
		for (i=0; i<=j; ++i)
		{
			if (sing) R(i,j) = 0.;
			ii = P(i);
			if (ii > jj) R(ii,jj) = R(i,j);
			if (ii < jj) R(jj,ii) = R(i,j);
		}
		wa(jj) = R(j,j);
	}

	// symmetrize the covariance matrix in R.
	for (j=0; j<N; ++j)
	{
		for (i=0; i<=j; ++i)
			R(i,j) = R(j,i);
		R(j,j) = wa(j);
	}

	// Kovarianzmatrix herauskopieren
	J = R.getSlice(0,0,N-1,N-1);

	return indet;
}

} // namespace flux::la::MatrixOps
} // namespace flux::la
} // namespace flux

