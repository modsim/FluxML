#ifndef GMATRIXOPS_H
#define GMATRIXOPS_H

#include "Error.h"
#include "GMatrix.h"
#include "GVector.h"
#include "PMatrix.h"
#include "cstringtools.h"
#include "charptr_array.h"

namespace flux {
namespace la {
namespace GMatrixOps {

template< typename T > bool _LUfactor(
	GMatrix< T > & A,
	PMatrix & P
	)
{
	int i,j,k,n,pk,tmp_i;
	T pivot, tmp;
	
	n = A.rows();
	
	// Initialisiere P:
	P.initIdent();
	
	// LU(P)-Zerlegung: P.A=L.U
	for (k=0; k<=n-2; k++)
	{
		// "normale" Pivot-Suche (Partial Pivoting)
		pk=k; pivot=abs(A(k,k));
		for (i=k+1; i<n; i++)
		{
			tmp = abs(A(i,k));
			if (tmp>pivot) { pivot=tmp; pk=i; }
		}
		
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

template< typename T > void _LUsolve(
	GMatrix< T > const & LU,
	PMatrix const & P,
	GVector< T > & b
	)
{
	int i,k,n = b.dim();
	GVector< T > r(n);

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

template< typename T > void _LUsolve(
	GMatrix< T > const & LU,
	PMatrix const & P,
	GMatrix< T > & B
	)
{
	int i,j,k,n = B.rows();
	GVector< T > r(n);

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

template< typename T > bool gaussJordan(
	GMatrix< T > & A,
	GVector< T > & b,
	GMatrix< T > & K,
	PMatrix & Pcf,
	int & rank
	)
{
	int i,j,k,pr,pc,pk,pj,free,user_free;
	int R = A.rows();
	int C = A.cols();
	T pivot, absval, mult, tolerance;
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
	tolerance = 0; //R * MACHEPS * A.normInf();

	for (k=0; k<R; k++) // ALLE R Zeilen eliminieren (0 .. R-1)
	{
		// Initialisierung der Pivot-Zeilen / -Spalten
		pr = k;
		pc = k;
		pivot = abs(A(pr,pc));

		// Pivot in der Restmatrix suchen.
		// Abbruch, sobald Index j die Spalten der freizuhaltenden
		// Variablen erreicht.
		for (j=k; j<C-user_free; j++)
		{
			for (i=k; i<R; i++)
			{
				absval = abs(A(i,j));
				if (absval > pivot)
				{
					pivot = absval;
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
				if (abs(A(i,j)) > tolerance)
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
					if (abs(A(i,j)) > tolerance)
						zero = false;
				if (not zero)
					// möglicher Pivot-Kandidat gefunden
					Pcf(Pc(j)) = 2;
			}
			rank = -3;
			K = GMatrix< T >();
			return false;
		}
	}

	// Wenn es 0-Zeilen gibt, so müssen die entsprechenden Einträge im
	// b-Vektor jetzt auch 0en enthalten -- ansonsten besitzt das System
	// keine Lösung:
	for (i=rank; i<R; i++)
	{
		if (abs(b(i)) > tolerance)
		{
			K = GMatrix< T >();
			rank = -4;
			return false;
		}
		else
			// klare Verhältnisse herstellen:
			b(i) = T(0.);
	}

	// Der Benutzer will möglicherweise mehr Variablen freihalten als
	// das System hergibt:
	if (user_free > C-rank)
	{
		rank = -2;
		K = GMatrix< T >();
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
	K = GMatrix< T >(C,free+1);
	
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
		if (abs(b(k))>=MACHEPS)
			K(pk,0) = b(k);
		
		for (j=rank; j<C; j++)
		{
			pj = Psfree(j-rank)+1;
			if (abs(A(k,j))>=MACHEPS) K(pk,pj) = - A(k,j);
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

} // namespace flux::la::GMatrixOps
} // namespace flux::la
} // namespace flux

#endif

