#ifndef ARRAY_H
#define ARRAY_H

#include <cstddef>
#include "Error.h"
#include "Combinations.h"

/**
 * Einfache Abstraktion eines Puffers.
 * Durch den Cast-Operator verhält sich ein Objekt wie ein Array.
 * Der Multiplikationsoperator entspricht einer diskreten Faltung.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
template< typename T > class Array
{
private:
	T * buf_;
	size_t len_;

public:
	/**
	 * Default-Constructor.
	 */
	inline Array() : buf_(0), len_(0) {}

	/**
	 * Copy-Constructor.
	 */
	inline Array(Array< T > const & copy) : buf_(new T[copy.len_]), len_(copy.len_)
	{
		for (size_t i=0; i<len_; i++)
			buf_[i] = copy.buf_[i];
	}
	
	/**
	 * Constructor.
	 *
	 * @param len Länge des Arrays
	 */
	inline Array(size_t len) : buf_(new T[len]), len_(len) {}

	/**
	 * Constructor.
	 *
	 * @param len Länge des Arrays
	 * @param v Initialisierungswert
	 */
	inline Array(size_t len, T const & v) : buf_(new T[len]), len_(len)
	{
		for (size_t i=0; i<len_; i++)
			buf_[i] = v;
	}

	/**
	 * Destructor.
	 */
	inline ~Array() { if (buf_) delete[] buf_; }
	
public:
	/**
	 * Cast-Operator nach (double*).
	 *
	 * @return Zeiger auf internen Puffer
	 */
	inline operator T * () const { return buf_; }
	
	/**
	 * Multiplikationsoperator (diskrete Faltung).
	 *
	 * @param rval rechtes Argument
	 * @return Cauchy-Produkt
	 */
	inline Array< T > operator*(Array< T > const & rval) const
	{
		Array< T > C(len_+rval.len_-1);
		convolve< T >(*this,len_,rval,rval.len_,C);
		return C;
	}

	/**
	 * Elementweise Addition.
	 *
	 * @param rval rechtes Argument
	 * @return Summen-Array
	 */
	inline Array< T > operator+(Array< T > const & rval) const
	{
		fASSERT(len_ == rval.len_);
		Array< T > C(len_);
		for (size_t i=0; i<len_; i++)
			C[i] = buf_[i] + rval.buf_[i];
		return C;
	}
	
	/**
	 * Elementweise Subtraktion.
	 *
	 * @param rval rechtes Argument
	 * @return Differenz-Array
	 */
	inline Array< T > operator-(Array< T > const & rval) const
	{
		fASSERT(len_ == rval.len_);
		Array< T > C(len_);
		for (size_t i=0; i<len_; i++)
			C[i] = buf_[i] - rval.buf_[i];
		return C;
	}

	/**
	 * Multiplikation mit Skalar.
	 *
	 * @param rval rechtes Argument
	 * @return Produkt-Array
	 */
	inline Array< T > operator*(T const & rval) const
	{
		Array< T > C(len_);
		for (size_t i=0; i<len_; i++)
			C.buf_[i] = buf_[i] * rval;
		return C;
	}

	/**
	 * Division durch Skalar.
	 *
	 * @param rval rechtes Argument
	 * @return Quotienten-Array
	 */
	inline Array< T > operator/(T const & rval) const
	{
		Array< T > C(len_);
		for (size_t i=0; i<len_; i++)
			C.buf_[i] = buf_[i] / rval;
		return C;
	}
	
	/**
	 * Addition eines Skalars.
	 *
	 * @param rval rechtes Argument
	 * @return Summen-Array
	 */
	inline Array< T > operator+(T const & rval) const
	{
		Array< T > C(len_);
		for (size_t i=0; i<len_; i++)
			C.buf_[i] = buf_[i] + rval;
		return C;
	}

	/**
	 * Subtraktion eines Skalars.
	 *
	 * @param rval rechtes Argument
	 * @return Differenz-Array
	 */
	inline Array< T > operator-(T const & rval) const
	{
		Array< T > C(len_);
		for (size_t i=0; i<len_; i++)
			C.buf_[i] = buf_[i] - rval;
		return C;
	}
	
	/**
	 * Unäres Minus.
	 *
	 * @return Produkt
	 */
	inline Array< T > operator-() const
	{
		Array< T > C(len_);
		for (size_t i=0; i<len_; i++)
			C.buf_[i] = -buf_[i];
		return C;
	}

	/**
	 * Zuweisungs-Operator.
	 *
	 * @param rval rechtes Argument
	 * @return Referenz auf *this
	 */
	inline Array< T > & operator=(Array< T > const & rval)
	{
		if (buf_) delete[] buf_;
		len_ = rval.len_;
		buf_ = new T[len_];
		for (size_t i=0; i<len_; i++)
			buf_[i] = rval.buf_[i];
		return *this;
	}

	/**
	 * Abfrage der Array-Länge.
	 *
	 * @return Länge des Arrays
	 */
	size_t size() const { return len_; }
};

/**
 * Multiplikation mit Skalar von links.
 *
 * @param lval linkes Argument (Skalar)
 * @param rval rechtes Argument (Skalar)
 * @return elementweises Produkt von Skalar und Array
 */
template< typename T > Array< T > operator*(T const & lval, Array< T > const & rval)
{
	Array< T > C(rval.size());
	for (size_t i=0; i<rval.size(); i++)
		C[i] = lval * rval[i];
	return C;
}

/**
 * Elementweise Division mit Skalar von links.
 *
 * @param lval linkes Argument (Skalar)
 * @param rval rechtes Argument (Skalar)
 * @return elementweiser Quotient von Skalar und Array
 */
template< typename T > Array< T > operator/(T const & lval, Array< T > const & rval)
{
	Array< T > C(rval.size());
	for (size_t i=0; i<rval.size(); i++)
		C[i] = lval / rval[i];
	return C;
}

/**
 * Elementweise Addition von Skalar von links.
 *
 * @param lval linkes Argument (Skalar)
 * @param rval rechtes Argument (Skalar)
 * @return elementweise Summe von Skalar und Array
 */
template< typename T > Array< T > operator+(T const & lval, Array< T > const & rval)
{
	Array< T > C(rval.size());
	for (size_t i=0; i<rval.size(); i++)
		C[i] = lval + rval[i];
	return C;
}

/**
 * Elementweise Subtraktion mit Skalar von links.
 *
 * @param lval linkes Argument (Skalar)
 * @param rval rechtes Argument (Skalar)
 * @return elementweise Differenz von Skalar und Array
 */
template< typename T > Array< T > operator-(T const & lval, Array< T > const & rval)
{
	Array< T > C(rval.size());
	for (size_t i=0; i<rval.size(); i++)
		C[i] = lval - rval[i];
	return C;
}

#endif

