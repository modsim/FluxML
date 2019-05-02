#ifndef SORT_H
#define SORT_H

#include <cstddef>
#include <cstdlib>
#include <cmath>
extern "C"
{
#include <stdint.h>
}
#include "Error.h"

namespace flux { namespace la { class PMatrix; } }

/**
 * Generischer Swap-Operator (Funktor)
 */
template< typename T > struct swap_op_generic
{
	virtual inline void operator() (T & i, T & j) const
	{
		T tmp(i); i=j; j=tmp;
	}
	virtual inline ~swap_op_generic() {}
};

/**
 * Funktor für aufsteigende Sortierung.
 */
template< typename T > struct ASCENDING
{
	virtual inline bool operator()(T const & i, T const & j) const
	{
		return i<j;
	}
	virtual inline ~ASCENDING() {}
};

/**
 * Funktor für absteigende Sortierung
 */
template< typename T > struct DESCENDING
{
	virtual inline bool operator()(T const & i, T const & j) const
	{
		return i>j;
	}
	virtual inline ~DESCENDING() {}
};

/**
 * Implementierungen von verschiedenen Sortier- und Permutierverfahren:
 * <ul>
 *  <li>konventionelles Heap-Sort zum Sortieren von Arrays</li>
 *  <li>dereferenzierendes Heap-Sort zum Sortieren von Zeiger-Arrays</li>
 *  <li>implizites Heap-Sort zum Generieren einer Sortier-Permutation</li>
 *  <li>stabiles Merge-Sort</li>
 *  <li>Unique-Test</li>
 *  <li>In-Situ-Permutationen</li>
 *  <li>Zufalls-Permutation</li>
 *  <li>Integer-Partitionen</li>
 * </ul>
 *
 * Von der Klasse Sort kann niemals ein Objekt erzeugt werden.
 * Es handelt sich bei Sort um eine Klasse, da die heapify_-Methoden
 * nach außen hin verborgen sein sollen.
 *
 * @param T Datentyp des zu sortierenden/permutierenden Arrays
 * @param L Funktor für Wertvergleich
 * @param S Funktor für Wertvertauschung
 * @author Michael Weitzel <info@13cflux.net>
 */
template<
	typename T = bool,
	typename L = ASCENDING<T>,
	typename S = swap_op_generic<T>
	> class Sort
{
	friend class flux::la::PMatrix;
private:
	/** Privater Constructor (Objekterzeugung verboten). */
	Sort();
	/** Privater Copy-Constructor (Kopieren verboten). */
	Sort(Sort const &);
	/** Privater Zuweisungsoperator (Zuweisen verboten). */
	Sort & operator=(Sort const &);

public:
	/**
	 * Konventionelles HeapSort zum Sortieren eines Arrays
	 * 
	 * @param A Array mit vergleichbaren Objekten (<)
	 * @param elements Anzahl der zu sortierenden Elemente
	 */
	static void sort(
		T * A,
		size_t elements
		);

	/**
	 * Konventionelles HeapSort zum Sortieren eines Arrays
	 * 
	 * @param A Array mit vergleichbaren Objekten (<)
	 * @param lo unteres Offset
	 * @param hi oberes Offset
	 */
	inline static void sort(
		T * A,
		size_t lo,
		size_t hi
		) { sort(&(A[lo]),hi-lo+1); }

	/**
	 * HeapSort mit Dereferenzierung.
	 * Zum Sortieren eines Arrays von Pointern geeignet
	 * (Dereferenzierung vor Sortierung).
	 *
	 * @param A Array von Pointern auf vergleichbare Objekte (<)
	 * @param elements Anzahl der zu sortierenden Elemente
	 */
	static void sortDeref(
		T ** A,
		size_t elements
		);

	/**
	 * Implizites HeapSort (Berechnung einer Permutation zur Sortierung).
	 * Es wird eine Permutation zum Zugriff auf A sortiert; d.h. A[i]
	 * selbst bleibt unsortiert, während A[P[i]] sortiert ist.
	 *
	 * @param A Array mit vergleichbaren Objekten
	 * @param P Integer-Array (Permutations-Vektor)
	 * @param elements Anzahl der zu sortierenden Elemente
	 */
	static void sortPerm(
		T const * A,
		unsigned int * P,
		size_t elements
		);

	/**
	 * Ein stabiles Merge-Sort (benötigt zusätzlichen, temporären Speicher).
	 *
	 * @param A zu sortierendes Array
	 * @param tmp temporärer Speicher
	 * @param len Array-Länge
	 */
	static void stableSort(
		T * A,
		T * tmp,
		size_t len
		) { stable_merge_sort(A,tmp,0,len-1); }

	/**
	 * Ein stabiles Merge-Sort (benötigt zusätzlichen, temporären Speicher).
	 *
	 * @param A zu sortierendes Array
	 * @param tmp temporärer Speicher
	 * @param lo unterer Index
	 * @param hi oberer Index
	 */
	static void stableSort(
		T * A,
		T * tmp,
		size_t lo,
		size_t hi
		) { stable_merge_sort(A,tmp,lo,hi); }

	/**
	 * Prüft, ob Werte in einem Array nicht doppelt vorkommen.
	 * Das übergebene Array wird dazu sortiert.
	 *
	 * @param A Array
	 * @param elements Länge des Arrays A
	 * @return true, falls kein Wert im Array A doppelt vorkommt
	 */
	static bool isUnique(
			T * A,
			size_t elements
			);

	/**
	 * In-Situ-Permutation.
	 *
	 * @param A Array
	 * @param elements Array-Länge
	 * @param P Integer-Array (Permutations-Vektor)
	 */
	static void permInSitu(
		T * A,
		unsigned int const * P,
		size_t elements
		);

	/**
	 * Berechnet die Rückwärtspermutation:
	 * P^(-1) = P^T mit P^(-1)[P[i]] = i
	 *
	 * @param P Permutationsvektor
	 * @param elements Länge des Permutationsvektors
	 */
	static void invPermInSitu(unsigned int * P, size_t elements);

private:
	/**
	 * Die private "Heapify"-Funktion des konventionellen HeapSorts
	 * für Arrays (Methode Sort::sort).
	 *
	 * @param A Array, in dem die Heap-Eigenschaft hergestellt werden soll
	 * @param j Index, bis zu dem der Heap erstellt wird
	 * @param size Größe des Heaps
	 */
	static void heapify(
		T * A,
		int j,
		int size
		);

	/**
	 * Die private "Heapify"-Funktion des dereferenzierenden HeapSorts
	 * für Arrays von Zeigern (Methode Sort::sortDeref).
	 *
	 * @param A Array, in dem die Heap-Eigenschaft hergestellt werden soll
	 * @param j Index, bis zu dem der Heap erstellt wird
	 * @param size Größe des Heaps
	 */
	static void heapify_deref(
		T ** A,
		int j,
		int size
		);

	/**
	 * Die private "Heapify"-Funktion des impliziten HeapSorts
	 * für Arrays (Methode Sort::sortPerm). Vertauschungen, die zur
	 * Sortierung notwendig sind, werden in einem Permutationsvektor
	 * gespeichert.
	 *
	 * @param A Array, in dem die Heap-Eigenschaft hergestellt werden soll
	 * @param P Permutationsvektor
	 * @param j Index, bis zu dem der Heap erstellt wird
	 * @param size Größe des Heaps
	 */
	static void heapify_perm(
		T const * A,
		unsigned int * P,
		int j,
		int size
		);
	
	/**
	 * Stabiles Merge-Sort
	 *
	 * @param A zu sortierendes Array
	 * @param tmp temporärer Speicher
	 * @param lo unterer Array-Index (=0)
	 * @param hi oberer Array-Index (Anzahl der Elemente -1)
	 */
	static void stable_merge_sort(
		T * A,
		T * tmp,
		size_t lo,
		size_t hi
		);

	/**
	 * IsLeader-Funkion der Knuth/Fich in-situ-Permutation
	 */
	static bool is_leader_ISP(
		unsigned int const * P,
		unsigned int i,
		unsigned int remaining
		);
};

template< typename T,typename L,typename S >
void Sort< T,L,S >::sort(
	T * A,
	size_t elements
	)
{
	int i, size=elements-1;
	S swap;

	// Erstelle den Heap. heapify wird auf den inneren
	// Knoten aufgerufen (index < elements/2). Der rückwärts
	// laufende Index im Heap entspricht einem Aufstieg zur
	// Wurzel. Komplexität: O(N*ld(N)) (genauer sogar: O(N))
	for (i=elements/2-1; i>=0; i--)
		heapify(A, i, elements);

	// Sortierung. Prinzip: Nach jedem heapify steht der
	// größte Index immer an der Wurzel => fortlaufendes
	// Austauschen der Wurzel. O(N*ld(N))
	for (i=elements-1; i>=1; i--)
	{
		swap(A[0],A[i]);

		// Heap-Eigenschaft für nicht-sortierte Elemente
		// wiederherstellen: O(ld(N))
		heapify(A, 0, size);
		size--;
	}
}

template< typename T,typename L,typename S >
void Sort< T,L,S >::sortDeref(
	T ** A,
	size_t elements
	)
{
	int i, size=elements-1;
	swap_op_generic<T*> swap;

	// Erstelle den Heap. heapify wird auf den inneren
	// Knoten aufgerufen (index < elements/2). Der rückwärts
	// laufende Index im Heap entspricht einem Aufstieg zur
	// Wurzel. Komplexität: O(N*ld(N)) (genauer sogar: O(N))
	for (i=elements/2-1; i>=0; i--)
		heapify_deref(A, i, elements);

	// Sortierung. Prinzip: Nach jedem heapify steht der
	// größte Index immer an der Wurzel => fortlaufendes
	// Austauschen der Wurzel. O(N*ld(N))
	for (i=elements-1; i>=1; i--)
	{
		//swap(reinterpret_cast< void*& >(A[0]),(void*&)A[i]);
		swap(A[0],A[i]);

		// Heap-Eigenschaft für nicht-sortierte Elemente
		// wiederherstellen: O(ld(N))
		heapify_deref(A, 0, size);
		size--;
	}
}

template< typename T,typename L,typename S >
void Sort< T,L,S >::sortPerm(
	T const * A,
	unsigned int * P,
	size_t elements
	)
{
	int i, size=elements-1;
	swap_op_generic< unsigned int > swap;

	// Permutations initialisieren
	for (size_t j=0; j<elements; j++)
		P[j] = j;

	// Erstelle den Heap. heapify wird auf den inneren
	// Knoten aufgerufen (index < elements/2). Der rückwärts
	// laufende Index im Heap entspricht einem Aufstieg zur
	// Wurzel. Komplexität: O(N*ld(N)) (genauer sogar: O(N))
	for (i=elements/2-1; i>=0; i--)
		heapify_perm(A, P, i, elements);

	// Sortierung. Prinzip: Nach jedem heapify steht der
	// größte Index immer an der Wurzel => fortlaufendes
	// Austauschen der Wurzel. O(N*ld(N))
	for (i=elements-1; i>=1; i--)
	{
		swap(P[0],P[i]);

		// Heap-Eigenschaft für nicht-sortierte Elemente
		// wiederherstellen: O(ld(N))
		heapify_perm(A, P, 0, size);
		size--;
	}
}

template< typename T,typename L,typename S >
bool Sort< T,L,S >::isUnique(
	T * A,
	size_t elements
	)
{
	size_t i;
	bool unique = true;
	unsigned int * P = new unsigned int[elements];
	sortPerm(A,P,elements);
	for (i=1; unique && i<elements; i++)
		if (A[P[i-1]] == A[P[i]]) unique = false;
	delete[] P;
	return unique;
}

template< typename T,typename L,typename S >
void Sort< T,L,S >::permInSitu(
	T * A,
	unsigned int const * P,
	size_t elements
	)
{
	unsigned int i, p, length, remaining=elements;
	S swap;

	for (i=0; i<elements; i++)
	{
		if (is_leader_ISP(P, i, remaining))
		{
			length = 0;
			p = i;
			do
			{
				p = P[p];
				length++;
				swap(A[i],A[p]);
			}
			while (p!=i);
			remaining -= length;
			if (remaining==0) break;
		}
	}
}

template< typename T,typename L,typename S >
void Sort< T,L,S >::invPermInSitu(
	unsigned int * P,
	size_t elements
	)
{
	int i, j, k, n;

	for (n=elements-1; n>=0; n--)
	{
		i = P[n];
		if (i<0)
			P[n] = (unsigned int)(-1-i);
		else if (i!=n)
		{
			k = n;
			for (;;)
			{
				j = P[i];
				P[i] = (unsigned int)(-1-k);
				if (j==n)
				{
					P[n] = (unsigned int)i;
					break;
				}
				k = i;
				i = j;
			}
		}
	}
}

template< typename T,typename L,typename S >
void Sort< T,L,S >::stable_merge_sort(
	T * A,
	T * tmp,
	size_t lo,
	size_t hi
	)
{
	size_t i, j, k, mid;
	L is_less;
	if (lo>=hi)
		return;

	mid = (lo+hi)>>1;
	stable_merge_sort(A,tmp,lo,mid);
	stable_merge_sort(A,tmp,mid+1,hi);

	for (k=lo; k<=hi; k++) tmp[k] = A[k];

	for (i=j=lo,k=mid+1; i<=hi; i++)
	{
		if (j<=mid && k<=hi)
		{
			if (is_less(tmp[k],tmp[j])) A[i] = tmp[k++];
			else A[i] = tmp[j++];
		}
		else if (k<=hi) A[i] = tmp[k++];
		else A[i] = tmp[j++];
	}
}

template< typename T,typename L,typename S >
void Sort< T,L,S >::heapify(
	T * A,
	int j,
	int size
	)
{
	int l, r;
	int largest;
	S swap;
	L is_less;

	for (;;)
	{
		l = (j<<1)+1; r = (j<<1)+2;
		largest = (l<size && is_less(A[j],A[l]))?l:j;
		if (r<size && is_less(A[largest],A[r])) largest = r;
		if (largest != j)
		{
			swap(A[j],A[largest]);

			// getauschten Wert so tief wie nötig im Heap
			// "versickern" lassen:
			j = largest;
		}
		else break;
	}
}

template< typename T,typename L,typename S >
void Sort< T,L,S >::heapify_deref(
	T ** A,
	int j,
	int size
	)
{
	int l, r;
	int largest;
	swap_op_generic<T*> swap;
	L is_less;

	for (;;)
	{
		l = (j<<1)+1; r = (j<<1)+2;
		largest = (l<size && is_less(*(A[j]),*(A[l])))?l:j;
		if (r<size && is_less(*(A[largest]),*(A[r]))) largest = r;
		if (largest != j)
		{
			//swap((void*&)A[j],(void*&)A[largest]);
			swap(A[j],A[largest]);

			// getauschten Wert so tief wie nötig im Heap
			// "versickern" lassen:
			j = largest;
		}
		else break;
	}
}

template< typename T,typename L,typename S >
void Sort< T,L,S >::heapify_perm(
	T const * A,
	unsigned int * P,
	int j,
	int size
	)
{
	int l, r;
	int largest;
	swap_op_generic< unsigned int > swap;
	L is_less;

	for (;;)
	{
		l = (j<<1)+1; r = (j<<1)+2;
		largest = (l<size && is_less(A[P[j]],A[P[l]]))?l:j;
		if (r<size && is_less(A[P[largest]],A[P[r]])) largest = r;
		if (largest != j)
		{
			swap(P[j],P[largest]);

			// getauschten Wert so tief wie nötig im Heap
			// "versickern" lassen:
			j = largest;
		}
		else break;
	}
}

template< typename T,typename L,typename S >
bool Sort< T,L,S >::is_leader_ISP(
	unsigned int const * P,
	unsigned int i,
	unsigned int remaining
	)
{
	unsigned int j = i, steps = 0;
	bool flag = true;

	do
	{
		j = P[j];
		steps++;
		if (i>j || steps>remaining)
		{
			flag = false;
			break;
		}
	}
	while (j!=i);
	return flag;
}

#endif

