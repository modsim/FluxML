#ifndef GVECTOR_H
#define GVECTOR_H

#include <cstddef>
#include "Error.h"
#include "VectorInterface.h"

namespace flux {
namespace la {

/**
 * Generische Vektor-Klasse.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
template< typename T > class GVector : public VectorInterface<T>
{
protected:
	/** Dimension des Vektors */
	size_t dim_;
	/** Speicher der Vektorelemente */
	T * vector_storage_;
public:
	/**
	 * Constructor.
	 */
	inline GVector() : dim_(0), vector_storage_(0) { }

	/**
	 * Constructor.
	 * Elemente werden nicht initialisiert.
	 *
	 * @param d Dimension des Vektors
	 */
	inline GVector(size_t dim) : dim_(dim)
	{
		vector_storage_ = new T[dim_];
		for (size_t i=0; i<dim_; ++i)
			vector_storage_[i] = T();
	}

	/**
	 * Constructor.
	 * Elemente werden mit einem Parameterwert initialisiert.
	 *
	 * @param d Dimension des Vektors
	 * @param init Initialisierungswert
	 */
	inline GVector(size_t dim, T const & init) : dim_(dim)
	{
		vector_storage_ = new T[dim_];
		while (dim) vector_storage_[--dim] = T(init);
	}

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierender Vektor
	 */
	inline GVector(GVector const & copy)
		: dim_(copy.dim_)
	{
		vector_storage_ = new T[dim_];
		for (size_t i=0; i<dim_; i++)
			vector_storage_[i] = T(copy.vector_storage_[i]);
	}

	/**
	 * Constructor mit Initialisierung über Array.
	 *
	 * @param copy zu kopierendes Array
	 * @param d Dimension des Vektors
	 */
	inline GVector(T const * copy, size_t dim)
		: dim_(dim)
	{
		vector_storage_ = new T[dim_];
		for (size_t i=0; i<dim_; i++)
			vector_storage_[i] = T(copy[i]);
	}

	/**
	 * Destructor.
	 */
	virtual ~GVector() { delete[] vector_storage_; }

public:
	/**
	 * Zuweisung.
	 * Länge des Vektors wird angepasst (so eine Art Copy-Constructor).
	 *
	 * @param copy zu kopierender Vektor
	 * @return Referenz auf die Kopie
	 */
	inline GVector< T > & operator= (GVector const & copy)
	{
		if (dim_ != copy.dim_)
		{
			if (vector_storage_)
				delete[] vector_storage_;
			dim_ = copy.dim_;
			vector_storage_ = new T[dim_];
		}
		for (size_t i=0; i<dim_; i++)
			vector_storage_[i] = copy.vector_storage_[i];
		return *this;
	}

	/**
	 * Elementzugriff (Lesen/Schreiben)
	 *
	 * @param i Index
	 * @return Referenz auf i-tes Element
	 */
	inline T & operator() (size_t i)
	{
		fASSERT(i<dim_);
		return vector_storage_[i];
	}

	/**
	 * Schreiben eines Elements.
	 *
	 * @param i Index
	 * @param val const-Referenz auf Objekt
	 */
	inline virtual void set(size_t i, T const & val)
	{
		fASSERT(i<dim_);
		vector_storage_[i] = T(val);
	}

	/**
	 * Auslesen eines Elements.
	 *
	 * @param i Index
	 * @return const-Referenz auf Objekt an Index i
	 */
	inline virtual T const & get(size_t i) const
	{
		fASSERT(i<dim_);
		return vector_storage_[i];
	}
	
	/**
	 * Zuweisung eines Arrays.
	 *
	 * @param cpy zu kopierendes Array
	 * @param dim Array-Länge
	 * @return Referenz auf Kopie
	 */
	inline GVector< T > & copy(T const * cpy, size_t dim)
	{
		if (dim != dim_)
		{
			delete[] vector_storage_;
			dim_ = dim;
			vector_storage_ = new T[dim_];
		}
		for (size_t i=0; i<dim_; i++)
			vector_storage_[i] = T(cpy[i]);
		return *this;
	}
	
	/**
	 * Zuweisung eines Arrays.
	 * Als Array-Länge wird die aktuelle Vektor-Länge angenommen.
	 *
	 * @param cpy zu kopierendes Array
	 * @return Referenz auf Kopie
	 */
	inline GVector< T > & copy(T const * cpy)
	{
		for (size_t i=0; i<dim_; i++)
			vector_storage_[i] = T(cpy[i]);
		return *this;
	}

	/**
	 * Gibt die Dimension (Länge) des Vektors zurück.
	 *
	 * @return Länge des Vektors
	 */
	inline virtual size_t dim() const { return dim_; }

	/**
	 * Vertauschung der Indizes i und j.
	 * Bei Permutationsmatrizen/Vektoren entspricht das einer Vertauschung
	 * der Zeilen i und j.
	 * 
	 * @param i (Zeilen-)Index
	 * @param j (Zeilen-)Index
	 */
	inline virtual void swap(
		size_t i,
		size_t j
		)
	{
		fASSERT(i<dim_ and j<dim_);
		T tmp = vector_storage_[i];
		vector_storage_[i] = vector_storage_[j];
		vector_storage_[j] = tmp;
	}


	/**
	 * Füllt den Vektor mit einem Wert
	 *
	 * @param val Wert
	 */
	inline virtual void fill(T const & val)
	{
		for (size_t i=0; i<dim_; i++)
			vector_storage_[i] = val;
	}

	/**
	 * Gibt die Matrix in einem Fortran-kompatibelen Format zurück
	 *
	 * @return rohe Matrix in Fortran-Ordnung
	 */
	inline virtual operator T * () const { return vector_storage_; }

	/**
	 * Skalarprodukt-Operator.
	 */
	T operator*(GVector< T > const & rval) const
	{
		size_t i;
		T p(); // setzt einen funktionierenden def-Constructor voraus
		fASSERT(rval.dim_ == dim_);
		for (i=0; i<dim_; i++)
			p = p + get(i)*rval.get(i);
		return p;
	}

	/**
	 * Produkt aus Vektor und Skalar.
	 *
	 * @param rval Skalar
	 * @return Produktvektor
	 */
	GVector< T > operator*(T const & rval) const
	{
		GVector< T > p(dim_);
		for (size_t i=0; i<dim_; i++)
			p(i) = get(i)*rval;
		return p;
	}

	/**
	 * Produkt aus Vektor und Skalar.
	 *
	 * @param rval Skalar
	 * @return Referenz auf *this
	 */
	GVector< T > & operator*=(T const & rval)
	{
		for (size_t i=0; i<dim_; i++)
			set(i,get(i)*rval);
		return *this;
	}

	/**
	 * Quotient aus Vektor und Skalar.
	 *
	 * @param rval Skalar
	 * @return Produktvektor
	 */
	GVector< T > operator/(T const & rval) const
	{
		GVector< T > p(dim_);
		for (size_t i=0; i<dim_; i++)
			p(i) = get(i)/rval;
		return p;
	}

	/**
	 * Quotient aus Vektor und Skalar.
	 *
	 * @param rval Skalar
	 * @return Referenz auf *this
	 */
	GVector< T > & operator/=(T const & rval)
	{
		for (size_t i=0; i<dim_; i++)
			set(i,get(i)/rval);
		return *this;
	}


	/**
	 * Additions-Operator.
	 *
	 * @param rval rechtes Argument
	 * @return Summenvektor
	 */
	GVector< T > operator+(GVector< T > const & rval) const
	{
		size_t i;
		fASSERT(rval.dim_ == dim_);
		GVector< T > s(dim_);
		for (i=0; i<dim_; i++)
			s(i) = get(i) + rval.get(i);
		return s;
	}

	/**
	 * Additions-Operator.
	 *
	 * @param rval rechtes Argument
	 * @return Referenz auf *this
	 */
	GVector< T > & operator+=(GVector< T > const & rval)
	{
		size_t i;
		fASSERT(rval.dim_ == dim_);
		for (i=0; i<dim_; i++)
			set(i,get(i) + rval.get(i));
		return *this;
	}

	/**
	 * Subtraktions-Operator.
	 *
	 * @param rval rechtes Argument
	 * @return Differrenzvektor
	 */
	GVector< T > operator-(GVector< T > const & rval) const
	{
		size_t i;
		fASSERT(rval.dim_ == dim_);
		GVector< T > d(dim_);
		for (i=0; i<dim_; i++)
			d(i) = get(i) - rval.get(i);
		return d;
	}

	/**
	 * Subtraktions-Operator.
	 *
	 * @param rval rechtes Argument
	 * @return Referenz auf *this
	 */
	GVector< T > & operator-=(GVector< T > const & rval)
	{
		size_t i;
		fASSERT(rval.dim_ == dim_);
		for (i=0; i<dim_; i++)
			set(i,get(i) - rval.get(i));
		return *this;
	}

	/**
	 * Unäres Minus
	 *
	 * @return neuer Vektor mit negierten Elementen
	 */
	GVector< T > operator-() const
	{
		GVector< T > neg(dim_);
		for (size_t i=0; i<dim_; ++i)
			neg(i) = -get(i);
		return neg;
	}
	
};

/**
 * Produkt aus Skalar und Vektor.
 *
 * @param lval linkes Argument, Skalar
 * @param rval rechtes Argument, Vektor
 * @return Produktvektor
 */
template< typename T > GVector< T > operator*(
	T const & lval,
	GVector< T > const & rval
	)
{
	GVector< T > p(rval.dim());
	for (size_t i=0; i<rval.dim(); i++)
		p(i) = lval*rval.get(i);
	return p;
}

} // namespace flux::la
} // namespace flux

#endif

