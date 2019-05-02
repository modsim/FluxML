#ifndef GLABELMATRIX_H
#define GLABELMATRIX_H

#include "Error.h"
#include "charptr_map.h"
#include "GMatrix.h"

namespace flux {
namespace la {

/**
 * Klasse zur Abbildung einer generischen Matrix mit Zeilen- und
 * Spaltenbeschriftungen.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
template< typename T > class GLabelMatrix : public GMatrix< T >
{
protected:
	/** Zeilenbeschiftungen */
	char const ** row_labels_;
	/** Abbildung von Zeilenbeschriftungen auf Indizes */
	charptr_map< size_t > row_map_;
	/** Spaltenbeschriftungen */
	char const ** col_labels_;
	/** Abbildung von Spaltenbeschriftungen auf Indizes */
	charptr_map< size_t > col_map_;

public:
	/**
	 * Default-Constructor.
	 */
	GLabelMatrix() : GMatrix< T >(), row_labels_(0), col_labels_(0) { }
	
	/**
	 * Constructor.
	 *
	 * @param rows Anzahl der Zeilen
	 * @param cols Anzahl der Spalten
	 * @param init Initialisierungswert für Elemente
	 */
	GLabelMatrix(size_t rows, size_t cols, T const & init)
		: GMatrix< T >(rows,cols,init)
	{
		size_t i,j;
		row_labels_ = new char const*[rows+1];
		col_labels_ = new char const*[cols+1];
		for (i=0; i<=rows; ++i)
			row_labels_[i] = 0;
		for (j=0; j<=cols; ++j)
			col_labels_[j] = 0;
	}

	/**
	 * Constructor.
	 *
	 * @param rows Anzahl der Zeilen
	 * @param cols Anzahl der Spalten
	 */
	GLabelMatrix(size_t rows, size_t cols)
		: GMatrix< T >(rows,cols)
	{
		size_t i,j;
		row_labels_ = new char const*[rows+1];
		col_labels_ = new char const*[cols+1];
		for (i=0; i<=rows; ++i)
			row_labels_[i] = 0;
		for (j=0; j<=cols; ++j)
			col_labels_[j] = 0;
	}

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	GLabelMatrix(GLabelMatrix< T > const & copy)
		: GMatrix< T >(copy),
		  row_map_(copy.row_map_),
		  col_map_(copy.col_map_)
	{
		size_t k;
		charptr_map< size_t >::const_iterator l;
		row_labels_ = new char const*[this->rows()+1];
		col_labels_ = new char const*[this->cols()+1];
		for (k=0; k<=this->rows(); ++k)
			row_labels_[k] = 0;
		for (k=0; k<=this->cols(); ++k)
			col_labels_[k] = 0;
		for (l=row_map_.begin(); l!=row_map_.end(); ++l)
			row_labels_[l->value] = l->key;
		for (l=col_map_.begin(); l!=col_map_.end(); ++l)
			col_labels_[l->value] = l->key;
	}


	/**
	 * Destructor.
	 */
	~GLabelMatrix()
	{
		delete[] row_labels_;
		delete[] col_labels_;
	}

public:
	/**
	 * Setzt eine Zeilenbeschriftung.
	 *
	 * @param i Zeilenindex
	 * @param label Zeilenbeschriftung
	 */
	void setRowLabel(size_t i, char const * label)
	{
		fASSERT(i<this->rows());
		if (label)
		{
			charptr_map< size_t >::iterator l = row_map_.insert(label,i);
			row_labels_[l->value] = l->key;
		}
		else if (row_labels_[i])
		{
			row_map_.erase(row_labels_[i]);
			row_labels_[i] = 0;
		}
	}

	/**
	 * Setzt eine Spaltenbeschriftung.
	 *
	 * @param j Spaltenindex
	 * @param Spaltenbeschriftung
	 */
	void setColumnLabel(size_t j, char const * label)
	{
		fASSERT(j<this->cols());
		if (label)
		{
			charptr_map< size_t >::iterator l = col_map_.insert(label,j);
			col_labels_[l->value] = l->key;
		}
		else if (col_labels_[j])
		{
			col_map_.erase(col_labels_[j]);
			col_labels_[j] = 0;
		}
	}

	/**
	 * Gibt eine Zeilenbeschriftung zurück.
	 *
	 * @param i Zeilenindex
	 * @return Zeilenbeschriftung
	 */
	char const * getRowLabel(size_t i) const
	{
		fASSERT(i<this->rows());
		return row_labels_[i];
	}
	
	/**
	 * Gibt eine Spaltenbeschriftung zurück.
	 *
	 * @param j Spaltenindex
	 * @return Spaltenbeschriftung
	 */
	char const * getColumnLabel(size_t j) const
	{
		fASSERT(j<this->cols());
		return col_labels_[j];
	}

	/**
	 * Setzt alle Zeilenbeschriftungen.
	 *
	 * @param row_labels Array mit Zeilenbeschriftungen
	 */
	void setRowLabels(char const ** row_labels)
	{
		size_t i;
		for (i=0; *row_labels; ++i,++row_labels)
			setRowLabel(i,*row_labels);
		fASSERT(i == this->rows());
	}

	/**
	 * Setzt alle Spaltenbeschriftungen.
	 *
	 * @param col_labels Array mit Spaltenbeschriftungen
	 */
	void setColumnLabels(char const ** col_labels)
	{
		size_t j;
		for (j=0; *col_labels; ++j,++col_labels)
			setColumnLabel(j,*col_labels);
		fASSERT(j == this->cols());
	}
	
	/**
	 * Gibt ein null-terminiertes Array mit Zeilenbeschriftungen zurück.
	 *
	 * @return Array mit Zeilenbeschriftungen
	 */
	char const ** getRowLabels() const { return row_labels_; }

	/**
	 * Gibt ein null-terminiertes Array mit Spaltenbeschriftungen zurück.
	 *
	 * @return Array mit Spaltenbeschriftungen
	 */
	char const ** getColumnLabels() const { return col_labels_; }

	/**
	 * Sucht den Zeilenindex zu einer gegebenen Beschriftung.
	 *
	 * @param label Beschriftung
	 * @return Zeilenindex oder -1, falls nicht gefunden
	 */
	int getRowIndex(char const * label) const
	{
		size_t * idxp = row_map_.findPtr(label);
		if (idxp) return *idxp;
		return -1;
	}

	/**
	 * Sucht den Spaltenindex zu einer gegebenen Beschriftung.
	 *
	 * @param label Beschriftung
	 * @return Spaltenindex oder -1, falls nicht gefunden
	 */
	int getColumnIndex(char const * label) const
	{
		size_t * idxp = col_map_.findPtr(label);
		if (idxp) return *idxp;
		return -1;
	}

	/**
	 * Zuweisungsoperator.
	 *
	 * @param copy zu kopierendes Objekt
	 * @return Referenz auf *this
	 */
	GLabelMatrix< T > & operator=(GLabelMatrix< T > const & copy)
	{
		size_t k;
		charptr_map< size_t >::const_iterator l;
		row_map_ = copy.row_map_;
		col_map_ = copy.col_map_;
		delete[] row_labels_;
		delete[] col_labels_;
		GMatrix< T >::operator=(copy);
		row_labels_ = new char const*[this->rows()+1];
		col_labels_ = new char const*[this->cols()+1];
		for (k=0; k<=this->rows(); ++k)
			row_labels_[k] = 0;
		for (k=0; k<=this->cols(); ++k)
			col_labels_[k] = 0;
		for (l=row_map_.begin(); l!=row_map_.end(); ++l)
			row_labels_[l->value] = l->key;
		for (l=col_map_.begin(); l!=col_map_.end(); ++l)
			col_labels_[l->value] = l->key;
		return *this;
	}

}; // class GLabelMatrix< T >

} // namespace flux::la
} // namespace flux

#endif

