#ifndef GMATRIX_H
#define GMATRIX_H

#include <cstddef>
#include <cstring>
#include "Error.h"
#include "MatrixInterface.h"
#include "GVector.h"

namespace flux {
namespace la {

/**
 * Generische Matrizen-Klasse.
 * Die Elemente der Matrix werden in Fortran-Reihenfolge abgespeichert,
 * d.h. spaltenweise von links nach rechts in einem fortlaufenden
 * Speicherbereich.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
template< typename T > class GMatrix : public MatrixInterface<T>
{
protected:
	/** Anzahl der Zeilen */
	size_t rows_;
	/** Anzahl der Spalten */
	size_t cols_;
	/** Speicher für Matrixelemente */
	T * matrix_storage_;

public:
	/**
	 * Constructor
	 */
	inline GMatrix()
		: rows_(0), cols_(0), matrix_storage_(0) { }

	/**
	 * Constructor.
	 *
	 * @param r Anzahl der Zeilen
	 * @param c Anzahl der Spalten
	 */
	inline GMatrix(size_t r, size_t c)
		: rows_(r), cols_(c)
	{
		size_t i = rows_*cols_;
		matrix_storage_ = new T[i];
		while (i) matrix_storage_[--i] = T();
	}
	
	/**
	 * Constructor mit Initialisierung der Matrix.
	 *
	 * @param r Anzahl der Zeilen
	 * @param c Anzahl der Spalten
	 * @param init Initialisierungswert
	 */
	inline GMatrix(size_t r, size_t c, T const & init)
		: rows_(r), cols_(c)
	{
		size_t i = rows_*cols_;
		matrix_storage_ = new T[i];
		while (i) matrix_storage_[--i] = T(init);
	}

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	inline GMatrix(GMatrix const & copy)
		: rows_(copy.rows_), cols_(copy.cols_)
	{
		size_t i = rows_*cols_;
		matrix_storage_ = new T[i];
		while (i)
		{
			i--;
			matrix_storage_[i] = T(copy.matrix_storage_[i]);
		}
	}

	/**
	 * Destructor.
	 */
	virtual inline ~GMatrix() { delete[] matrix_storage_; }

public:
	/**
	 * Zuweisungsoperator.
	 * Für die Kopie der Elemente wird der Zuweisungsoperator der Klasse T
	 * verwendet.
	 * 
	 * @param copy GMatrix mit zu kopierenden Inhalten
	 *
	 */
	inline GMatrix & operator= (GMatrix const & copy)
	{
		size_t i = copy.rows_*copy.cols_;
		if (i != rows_*cols_)
		{
			if (matrix_storage_)
				delete[] matrix_storage_;
			matrix_storage_ = new T[i];
		}
		rows_ = copy.rows_;
		cols_ = copy.cols_;
		while (i>0)
		{
			i--;
			matrix_storage_[i] = copy.matrix_storage_[i];
		}
		return *this;
	}

	/**
	 * Elementzugriff (Lesen/Schreiben).
	 *
	 * @param i Zeile
	 * @param j Spalte
	 * @return Referenz auf das Objekt mit Index (i,j)
	 */
	inline T & operator() (size_t i, size_t j)
	{
		fASSERT(i<rows_ and j<cols_);
		return matrix_storage_[j*rows_+i];
	}

	/**
	 * Elementzugriff (Schreiben).
	 *
	 * @param i Zeile
	 * @param j Spalte
	 * @param val Wert
	 */
	inline virtual void set(size_t i, size_t j, T const & val)
	{
		fASSERT(i<rows_ and j<cols_);
		matrix_storage_[j*rows_+i] = val;
	}

	/**
	 * Elementzugriff (Lesen).
	 *
	 * @param i Zeile
	 * @param j Spalte
	 * @return const-Referenz auf das Objekt mit Index (i,j)
	 */
	inline virtual T const & get(size_t i, size_t j) const
	{
		fASSERT(i<rows_ and j<cols_);
		return matrix_storage_[j*rows_+i];
	}

	/**
	 * Rückgabe der Anzahl der Zeilen.
	 *
	 * @return Anzahl der Zeilen
	 */
	inline virtual size_t rows() const { return rows_; }

	/**
	 * Rückgabe der Anzahl der Spalten.
	 *
	 * @return Anzahl der Spalten
	 */
	inline virtual size_t cols() const { return cols_; }

	/**
	 * Füllt die Matrix mit einem Wert.
	 *
	 * @param val Wert
	 */
	inline virtual void fill(T const & val)
	{
		for (size_t i = 0; i<rows_*cols_; i++)
			matrix_storage_[i] = val;
	}

	/**
	 * Gibt die Matrix in einem Fortran-kompatiblen Format zurück.
	 *
	 * @return rohe Matrix in Fortran-Ordnung
	 */
	inline virtual operator T * () const { return matrix_storage_; }

	/**
	 * Spaltenvertauschung
	 *
	 * @param j1 erste Spalte
	 * @param j2 zweite Spalte
	 */
	inline virtual void swapColumns(size_t j1, size_t j2)
	{
		fASSERT(j1<cols_ and j2<cols_);
		T tmp;
		size_t i;
		for (i=0; i<rows_; i++)
		{
			tmp = get(i,j1);
			set(i,j1,get(i,j2));
			set(i,j2,tmp);
		}
	}
	
	/**
	 * Zeilenvertauschung
	 *
	 * @param i1 erste Zeile
	 * @param i2 zweite Zeile
	 */
	inline virtual void swapRows(size_t i1, size_t i2)
	{
		fASSERT(i1<rows_ and i2<rows_);
		T tmp;
		size_t j;
		for (j=0; j<cols_; j++)
		{
			tmp = get(i1,j);
			set(i1,j,get(i2,j));
			set(i2,j,tmp);
		}
	}

	virtual void transpose()
	{
		size_t i,j;
		if (rows_ == cols_)
		{
			T tmp;
			for (i=1; i<rows_; i++)
				for (j=0; j<i; j++)
				{
					tmp = get(i,j);
					set(i,j,get(j,i));
					set(j,i,tmp);
				}
		}
		else
		{
			GMatrix< T > tmpM(*this);
			size_t stmp = rows_;
			rows_ = cols_;
			cols_ = stmp;
			for (i=0; i<cols_; i++)
				for (j=0; j<rows_; j++)
					set(j,i,tmpM.get(i,j));
		}
	}

	inline virtual T * getRow(size_t i, T * row = 0) const
	{
		fASSERT(i<rows_);
		if (row == 0)
			row = new T[cols_];
		for (size_t j=0; j<cols_; j++)
			row[j] = matrix_storage_[j*rows_+i];
		return row;
	}

	inline virtual T * getCol(size_t j, T * col = 0) const
	{
		fASSERT(j<cols_);
		if (col == 0)
			col = new T[rows_];
		for (size_t i=0; i<rows_; i++)
			col[i] = matrix_storage_[j*rows_+i];
		return col;
	}
	
	inline virtual void setRow(size_t i, T const * row)
	{
		fASSERT(i<rows_);
		if (row == 0) return;
		for (size_t j=0; j<cols_; j++)
			matrix_storage_[j*rows_+i] = row[j];
	}

	inline virtual void setCol(size_t j, T const * col)
	{
		fASSERT(j<cols_);
		if (col == 0) return;
		for (size_t i=0; i<rows_; i++)
			matrix_storage_[j*rows_+i] = col[i];
	}

	GMatrix< T > operator+ (GMatrix< T > const & Rval) const
	{
		size_t i,j;
		GMatrix< T > C(rows(), cols());

		for (i=0; i<rows_; i++)
			for (j=0; j<cols_; j++)
				C.set(i,j,get(i,j) + Rval.get(i,j));
		return C;
	}

	GMatrix< T > & operator+= (GMatrix< T > const & Rval)
	{
		size_t i,j;

		for (i=0; i<rows_; i++)
			for (j=0; j<cols_; j++)
				set(i,j,get(i,j) + Rval.get(i,j));
		return *this;
	}

	GMatrix< T > operator- (GMatrix< T > const & Rval) const
	{
		size_t i,j;
		GMatrix< T > C(rows(), cols());

		for (i=0; i<rows_; i++)
			for (j=0; j<cols(); j++)
				C.set(i,j,get(i,j) - Rval.get(i,j));
		return C;
	}

	GMatrix< T > & operator-= (GMatrix< T > const & Rval)
	{
		size_t i,j;

		for (i=0; i<rows_; i++)
			for (j=0; j<cols_; j++)
				set(i,j,get(i,j) - Rval.get(i,j));
		return *this;
	}

	GMatrix< T > operator* (GMatrix< T > const & Rval) const
	{
		size_t i,j,k;
		GMatrix< T > C(rows_,Rval.cols_);

		for (i=0; i<C.rows_; i++)
			for (j=0; j<C.cols_; j++)
			{
				T & c_ij = C(i,j);
				for (k=0; k<cols_; k++)
					c_ij = c_ij + get(i,k) * Rval.get(k,j);
			}
		return C;
	}

	GVector< T > operator* (GVector< T > const & rval) const
	{
		size_t i,k;
		GVector< T > c(rows_);

		for (i=0; i<rows_; i++)
		{
			T & c_i = c(i);
			for (k=0; k<cols_; k++)
				c_i = c_i + get(i,k) * rval.get(k);
		}
		return c;
	}

	GMatrix< T > & operator*= (double rval)
	{
		size_t i,j;

		for (i=0; i<rows_; i++)
			for (j=0; j<cols_; j++)
				set(i,j,get(i,j) * rval);
		return *this;
	}

	GMatrix< T > operator* (double rval) const
	{
		size_t i,j;
		GMatrix< T > M(rows_,cols_);

		for (i=0; i<rows_; i++)
			for (j=0; j < cols(); j++)
				M.set(i, j, get(i, j) * rval);
		return M;
	}

	GMatrix< T > & operator/= (double rval)
	{
		size_t i,j;
		fASSERT(rval != 0.);

		for (i=0; i<rows_; i++)
			for (j=0; j<cols(); j++)
				set(i,j,get(i,j) / rval);
		return *this;
	}

	GMatrix< T > operator/ (double rval) const
	{
		size_t i,j;
		GMatrix< T > M(rows(), cols());
		fASSERT(rval != 0.);

		for (i=0; i<rows_; i++)
			for (j=0; j<cols_; j++)
				M.set(i,j,get(i,j) / rval);
		return M;
	}

};

} // namespace flux::la
} // namespace flux

#endif

