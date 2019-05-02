#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

#include <cstddef>
#include <cstring>
#include <functional>

/**
 * Typendefinition eines allgemeinen Pointers
 */
typedef void * genericptr;

/**
 * Struct zum Speichern eines Koordinatenpaars
 */
struct mxkoo
{
	size_t i_,j_;
	
	mxkoo(size_t i, size_t j) : i_(i), j_(j) {}

	inline bool operator== (const mxkoo &r) const
	{
		return r.i_==i_ && r.j_==j_;
	}
};

/**
 * Eine sehr gute Hash-Funktion (ähnlich dem 'R5-Hash' aus dem
 * Linux-Dateisystem ReiserFS). Berechnet aus einem Koordinaten-Paar
 * einen Hash-Wert.
 *
 * @param ij Koordinatenpaar
 * @retval Hash-Wert für das übergebene Koordinatenpaar
 */
size_t mxkoo_hashf(const mxkoo & ij);

/**
 * Struct zum Speichern eines Koordinatentripels
 */
struct mxkooo
{
	size_t i_,j_,k_;
	
	mxkooo(size_t i, size_t j, size_t k) : i_(i), j_(j), k_(k) {}

	inline bool operator== (const mxkooo &r) const
	{
		return r.i_==i_ && r.j_==j_ && r.k_==k_;
	}
};

/**
 * Eine sehr gute Hash-Funktion (ähnlich dem 'R5-Hash' aus dem
 * Linux-Dateisystem ReiserFS). Berechnet aus einem Koordinaten-Tripel
 * einen Hash-Wert.
 *
 * @param ijk Koordinatentripel
 * @retval Hash-Wert für das übergebene Koordinatentripel
 */
size_t mxkooo_hashf(const mxkooo & ijk);

/**
 * Eine Hash-Funktion für Double-Werte
 */
size_t double_hashf(double const & val);

/**
 * Eine Hash-Funktion für Zeiger-Werte
 */
size_t ptr_hashf(genericptr const & val);

/**
 * Eine Hash-Funktion für unsigned int's
 */
size_t uint_hashf(unsigned int const & val);

#endif

