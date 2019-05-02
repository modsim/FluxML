#ifndef PMATRIX_H
#define PMATRIX_H

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include "Error.h"
#include "GMatrix.h"
#include "GVector.h"
#include "Sort.h"

namespace flux {
namespace la {

/**
 * Klasse zur Abbildung einer Permutationsmatrix.
 * Die Permutationsmatrix wird intern als Vektor von unsigned int
 * abgespeichert und bietet eine Matrizen-ähnliche Schnittstelle.
 *
 * Die Abbildung des Vektors auf die Permutationsmatrix erfolgt per
 * Konvention "von oben". Das bedeutet, wenn im Vektor das i-te Element
 * den Wert j hat, dann ist in der Permutationsmatrix an Index (i,j)
 * eine 1 zu finden.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class PMatrix
	: public GVector< unsigned int >
{
private:
	static unsigned int one_;
	static unsigned int zero_;

public:
	/**
	 * Default-Constructor
	 */
	inline PMatrix() : GVector< unsigned int >() { }

	/**
	 * Constructor.
	 *
	 * @param dim Dimension der Permutationsmatrix
	 * @param init true, falls Permutationsmatrix mit Identität initialisiert werden soll
	 */
	inline PMatrix(size_t dim, bool init=false)
		: GVector< unsigned int >(dim)
	{
		if (init) initIdent();
	}
	
	/**
	 * Constructor.
	 *
	 * @param P Permutationsvektor
	 * @param dim Länge des Permutationsvektors
	 */
	inline PMatrix(unsigned int * P, size_t dim)
		: GVector< unsigned int >(dim)
	{
		for (size_t i=0; i<dim; i++)
			vector_storage_[i] = P[i];
	}

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	inline PMatrix(PMatrix const & copy)
		: GVector< unsigned int > (copy) { }


public:
	/**
	 * Elementzugriff auf Permutationsvektor; Lesen / Schreiben.
	 *
	 * @param i Index
	 * @return Referenz auf Wert an Index i
	 */
	inline unsigned int & operator() (size_t i)
	{
		return GVector< unsigned int >::operator()(i);
	}

	/**
	 * Elementzugriff auf Matrix; nur Lesen.
	 *
	 * @param i Zeile der Permutationsmatrix
	 * @param j Spalte der Permutationsmatrix
	 * @return const-Referenz auf Wert an Index (i,j)
	 */
	inline unsigned int const & operator() (size_t i, size_t j) const
	{
		return get(i,j);
	}

	/**
	 * Elementzugriff auf Matrix; nur Lesen.
	 *
	 * @param i Zeile der Permutationsmatrix
	 * @param j Spalte der Permutationsmatrix
	 * @return const-Referenz auf Wert an Index (i,j)
	 */
	inline unsigned int const & get(size_t i, size_t j) const
	{
		return vector_storage_[i]==j ? one_ : zero_;
	}

	/**
	 * Elementzugriff auf Permutationsvektor; nur Lesen
	 *
	 * @param i Index
	 * @return const-Referenz auf Wert an Index i
	 */
	inline unsigned int const & get(size_t i) const
	{
		return GVector< unsigned int >::get(i);
	}

	/**
	 * Anzahl der Zeilen (Dimension).
	 *
	 * @return Anzahl der Zeilen der Permutationsmatrix
	 */
	inline virtual size_t rows() const { return dim_; }

	/**
	 * Anzahl der Spalten (Dimension).
	 *
	 * @return Anzahl der Spalten der Permutationsmatrix
	 */
	inline virtual size_t cols() const { return dim_; }
public:
	/**
	 * Die Vertauschung zweier Zeilen.
	 *
	 * @param i erster Zeilenindex
	 * @param j zweiter Zeilenindex
	 */
	inline void swapRows(
		size_t i,
		size_t j
		) { swap(i,j); }

	/**
	 * Die aufwändigere Vertauschung zweier Spalten.
	 *
	 * @param i erster Spaltenindex
	 * @param j zweiter Spaltenindex
	 */
	inline void swapCols(
		size_t i,
		size_t j
		) { invert(); swap(i,j); invert(); }
	
	/**
	 * Berechnung der Inversen der Permutationsmatrix.
	 *
	 * @return inverse Permutationsmatrix
	 */
	PMatrix inverse() const;
	
	/**
	 * In-Situ-Inversion der Permutationsmatrix.
	 */
	void invert();

	/**
	 * Berechnung der Inversen der Permutationsmatrix.
	 *
	 * @param Pinv Speicherplatz für inverse Permutationsmatrix
	 */
	void inverse(
		PMatrix & Pinv
		) const;

	/**
	 * Initialisierung der Identitätspermutation.
	 */
	void initIdent();

	/**
	 * Prüft die Permutation auf Gültigkeit.
	 *
	 * @return true, falls die Permutation gültig ist
	 */
	bool check() const;

	/**
	 * Ausgabe des Permutationsvektors auf stdout.
	 */
	void dump() const;

	/**
	 * Gibt die Permutations-Matrix in Form eines Permutations-Vektors
	 * in einem in MatLab verwendbaren Format in einer Datei aus.
	 *
	 * @param fn Dateiname
	 * @param mn Name der Permutationsmatrix
	 * @retval true, bei Erfolg
	 * @see PMatrix.cc
	 */
	bool dumpMFile(
		char const * fn,
		char const * mn
		) const;

	/**
	 * Sortiert ein Intervall [lo,hi] des Permutationsvektors
	 *
	 * @param lo untere Intervallgrenze
	 * @param hi obere Intervallgrenze
	 */
	void sort(
		size_t lo,
		size_t hi
		);

	/**
	 * Ermittelt die Permutation zum Sortieren eines Intervalls [lo,hi]
	 * des Permutationsvektors. Zurückgegeben wird eine Permutation der
	 * Länge hi-lo+1.
	 *
	 * @param lo untere Intervallgrenze
	 * @param hi obere Intervallgrenze
	 * @retval Permutation zum Sortieren des Intervalls [lo,hi]
	 */
	PMatrix sortPerm(
		size_t lo,
		size_t hi
		);

	/**
	 * In-Situ-Spalten-Permutation.
	 * Entspricht einer Multiplikation mit einer Permutationsmatrix von rechts.
	 *
	 * @param A eine Matrix
	 */
	template< typename T > void permColumns(
		GMatrix< T > & A
		) const;
	
	/**
	 * In-Situ-Zeilen-Permutation.
	 * Entspricht einer Multiplikation mit einer Permutationsmatrix von links.
	 *
	 * @param A eine Matrix
	 */
	template< typename T > void permRows(
		GMatrix< T > & A
		) const;

	/**
	 * Zeilen-Spalten-Permutation (symmetrische Permutation).
	 * Entspricht einer Multiplikation mit einer Permutationsmatrix von
	 * links und rechts: P^T*A*P.
	 * Wird eine optionale temporäre Matrix tmp angegeben, so verbessert sich
	 * die Laufzeit. Andernfalls wird ein etwas langsamer In-Situ-
	 * Permutationsalgorithmus verwendet.
	 *
	 * @param A eine Matrix
	 * @param tmp Zeiger auf eine temporäre Matrix (optional)
	 */
	template< typename T > void symmPerm(
		GMatrix< T > & A,
		GMatrix< T > * tmp = 0
		) const;
};

/*
 * Implementierungen der Template-Methoden
 */

template< typename T > void PMatrix::permColumns(
	GMatrix< T > & A
	) const
{
	fASSERT(A.cols() == dim_);
	size_t i, p, remaining, length;

	for (i=0,remaining=dim_; i<dim_; i++)
	{
		if (Sort< unsigned int >::is_leader_ISP(vector_storage_, i, remaining))
		{
			length = 0;
			p = i;
			do
			{
				p = get(p);
				length++;
				A.swapColumns(i,p);
			}
			while (p!=i);
			remaining -= length;
			if (remaining == 0) break;
		}
	}
}

template< typename T > void PMatrix::permRows(
	GMatrix< T > & A
	) const
{
	fASSERT(A.rows() == dim_);
	size_t i, p, remaining, length;

	for (i=0,remaining=dim_; i<dim_; i++)
	{
		if (Sort< unsigned int >::is_leader_ISP(vector_storage_, i, remaining))
		{
			length = 0;
			p = i;
			do
			{
				p = get(p);
				length++;
				A.swapRows(i,p);
			}
			while (p!=i);
			remaining -= length;
			if (remaining == 0) break;
		}
	}
}

template< typename T > void PMatrix::symmPerm(
	GMatrix< T > & A,
	GMatrix< T > * tmp
	) const
{
	// Wenn tmp gegeben ist, wird es für die Permutation als
	// Temporärspeicher genutzt; ansonsten wird eine in-situ
	// Permutation ausgeführt, die eine geringfügig schlechtere
	// Laufzeit hat (zwischen O(n*log(n)) und O(n^2)).
	if (tmp)
	{
		size_t i,j,p,n = A.rows();
		fASSERT(tmp->rows() == n && tmp->cols() == n);
		for (i=0; i<n; i++)
			for (p=get(i),j=0; j<n; j++)
				tmp->set(p,j,A.get(i,j));
		for (j=0; j<n; j++)
			for (p=get(j),i=0; i<n; i++)
				A.set(i,p,tmp->get(i,j));
		return;
	}
	permRows(A);
	permColumns(A);
}

} // namespace flux::la
} // namespace flux

#endif

