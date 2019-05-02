#ifndef SMATRIX_H
#define SMATRIX_H

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <exception>
#include "Error.h"
#include "fhash_map.h"
#include "hash_functions.h"
#include "MatrixInterface.h"
#if 0
#include "GSMatrix.h"
#endif

namespace flux {
namespace la {
	class MVector;
	class MMatrix;
	class PMatrix;
}}

namespace flux {
namespace la {

#if 0
class SMatrix : public GSMatrix< double >
{
public:
	inline SMatrix(size_t rows, size_t cols, double expct_fill = 0.05)
		: GSMatrix< double >(rows,cols,expct_fill) { }
	inline SMatrix(SMatrix const & copy)
		: GSMatrix< double >(copy){ }
	inline SMatrix(MMatrix const & copy)
		: GSMatrix< double >(copy) { }
	inline SMatrix()
		: GSMatrix< double >() { }

	virtual operator MMatrix () const
	{
		MMatrix M(rows(),cols());
		for (iterator a_ij=begin(); a_ij!=end(); a_ij++)
			M.set(a_ij->row,a_ij->col,a_ij->value);
		return M;
	}
};
#endif

/**
 * Klasse SMatrix -- modelliert eine dünn besetzte Matrix (sparse matrix).
 * Zur Speicherung der Elemente wird ein schnelles Hash verwendet.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class SMatrix : public MatrixInterface< double >
{
private:
	/** der double-Wert 0. */
	static double zero_;
	/** Anzahl der Zeilen */
	size_t rows_;
	/** Anzahl der Spalten */
	size_t cols_;
	/** interne Hash-Datenstruktur */
	fhash_map< mxkoo,double,mxkoo_hashf > * hash_;
public:
	/**
	 * Ein Referenzen-Tripel aus (Zeile,Spalte,Wert).
	 * Referenziert einen Eintrag in der Sparse-Matrix, wobei die
	 * Koordinaten konstant sind, der Wert aber änderbar bleibt.
	 *
	 * @author Michael Weitzel <info@13cflux.net>
	 */
	struct Elem
	{
		size_t const & row;
		size_t const & col;
		double & value;
	
		/** Constructor -- wird von SMatrix::iterator aufgerufen */
		Elem(size_t const & r, size_t const & c, double & v)
			: row(r), col(c), value(v) {}
	
		/**
		 * Dereferenzierungs-Operator, Struct-Zugriff.
		 * Implementierung für den "->"-Operator der SMatrix::iterator-Klasse.
		 * Das hier funktioniert, weil der "->"-Operator in C++ als
		 * transitiv definiert ist.
		 */
		Elem const * operator->() { return this; }
	};
	
	/**
	 * Ein Referenzen-Tripel aus (Zeile,Spalte,Wert).
	 * Referenziert einen Eintrag in der Sparse-Matrix, wobei die
	 * Koordinaten konstant sind, der Wert aber änderbar bleibt.
	 *
	 * @author Michael Weitzel <info@13cflux.net>
	 */
	struct ConstElem
	{
		size_t const & row;
		size_t const & col;
		double const & value;
	
		/** Constructor -- wird von SMatrix::iterator aufgerufen */
		ConstElem(size_t const & r, size_t const & c, double const & v)
			: row(r), col(c), value(v) {}
	
		/**
		 * Dereferenzierungs-Operator, Struct-Zugriff.
		 * Implementierung für den "->"-Operator der SMatrix::iterator-Klasse.
		 * Das hier funktioniert, weil der "->"-Operator in C++ als
		 * transitiv definiert ist.
		 */
		ConstElem const * operator->() { return this; }
	};
	
	class iterator;
	/**
	 * Ein Iterator zum Iterieren über die Elemente der Sparse-Matrix.
	 *
	 * @author Michael Weitzel <info@13cflux.net>
	 */
	class const_iterator
	{
		/** die SMatrix-Klasse hat Zugriff auf den privaten Constructor */
		friend class SMatrix;
		friend class iterator;
	private:
		/** das SMatrix-Objekt, auf das der Iterator "zeigt". */
		SMatrix const * pobj_;
		/** aktuelle Position im Hash-Objekt */
		fhash_map< mxkoo,double,mxkoo_hashf >::const_iterator pos_;

	private:
		/**
		 * Privater Constructor;
		 * wird von der SMatrix-Klasse in SMatrix::begin() verwendet
		 * und ist ansonsten von außen nicht zugreifbar.
		 * 
		 * @param pobj das SMatrix-Objekt, auf dem der Iterator
		 * 	arbeitet
		 */
		const_iterator(SMatrix const * pobj) : pobj_(pobj)
		{
			pos_ = pobj_->hash_->begin();
		}

		/**
		 * Privater Constructor.
		 *
		 * @param pobj das SMatrix-Objekt, auf dem der Iterator
		 * 	arbeitet
		 * @param pos aktuelle Position im Hash
		 */
		const_iterator(
			SMatrix const * pobj,
			fhash_map< mxkoo,double,mxkoo_hashf >::const_iterator const & pos
			) : pobj_(pobj), pos_(pos) { }

	public:
		/**
		 * Constructor.
		 * Erzeugt einen Iterator, der ins Nirvana "zeigt"
		 */
		const_iterator() : pobj_(0) {}

	public:
		/**
		 * Prä-Inkrement-Operator.
		 * Setzt den Iterator auf das nächste Element im Hash.
		 *
		 * @return true, falls Inkrementierung möglich war.
		 */
		inline const_iterator & operator++()
		{
			++pos_;
			return *this;
		}

		/**
		 * Post-Inkrement-Operator.
		 * Setzt den Iterator auf das nächste Element im Hash.
		 *
		 * @return true, falls Inkrementierung möglich war.
		 */
		inline const_iterator operator++(int)
		{
			const_iterator tmp(*this);
			++*this;
			return tmp;
		}

		/**
		 * Dereferenzierungs-Operator.
		 * Simuliert eine Dereferenzierung des Iterators und gibt ein
		 * Elem-Objekt mit Koordinaten und Wert des Eintrages zurück.
		 *
		 * @return Elem-Objekt für den SMatrix-Eintrag
		 */
		inline ConstElem operator*() const
		{
			if (!pobj_ || pos_ == pobj_->hash_->end())
				throw std::exception();
			return ConstElem(pos_->key.i_, pos_->key.j_, pos_->value);
		}

		/**
		 * Dereferenzierungs-Operator.
		 * Wegen der Transitivität von -> wird hier der ->-Operator
		 * von ConstElem verwendent:
		 *
		 * @return Elem-Objekt für den SMatrix-Eintrag
		 */
		inline ConstElem operator->() const { return operator*(); }

		/**
		 * Vergleichs-Operator.
		 * Bei Gleichheit von zwei Iteratoren entsprechen sich die
		 * Kollisionslisten-Zeiger
		 *
		 * @param rval zu vergleichendes Iterator-Objekt
		 * @return true, falls Gleichheit
		 */
		inline bool operator==(const_iterator const & rval) const
		{
			return pos_==rval.pos_;
		}

		/**
		 * Vergleichs-Operator (Ungleichheit).
		 * Siehe operator==().
		 *
		 * @param rval zu vergleichendes Iterator-Objekt
		 * @return true, falls Ungleichheit
		 */
		inline bool operator!=(const_iterator const & rval) const
		{
			return pos_!=rval.pos_;
		}

	};
	
	/**
	 * Ein Iterator zum Iterieren über die Elemente der Sparse-Matrix.
	 *
	 * @author Michael Weitzel <info@13cflux.net>
	 */
	class iterator
	{
		/** die SMatrix-Klasse hat Zugriff auf den privaten Constructor */
		friend class SMatrix;
	private:
		/** das SMatrix-Objekt, auf das der Iterator "zeigt". */
		SMatrix * pobj_;
		/** aktuelle Position im Hash-Objekt */
		fhash_map< mxkoo,double,mxkoo_hashf >::iterator pos_;
	private:
		/**
		 * Privater Constructor;
		 * wird von der SMatrix-Klasse in SMatrix::begin() verwendet
		 * und ist ansonsten von außen nicht zugreifbar.
		 * 
		 * @param pobj das SMatrix-Objekt, auf dem der Iterator
		 * 	arbeitet
		 */
		iterator(SMatrix * pobj) : pobj_(pobj)
		{
			pos_ = pobj_->hash_->begin();
		}

		/**
		 * Privater Constructor.
		 *
		 * @param pobj das SMatrix-Objekt, auf dem der Iterator
		 * 	arbeitet
		 * @param pos aktuelle Position im Hash
		 */
		iterator(
			SMatrix * pobj,
			fhash_map< mxkoo,double,mxkoo_hashf >::iterator const & pos
			) : pobj_(pobj), pos_(pos) { }

	public:
		/**
		 * Constructor.
		 * Erzeugt einen Iterator, der ins Nirvana "zeigt"
		 */
		iterator() : pobj_(0) {}
	public:
		/**
		 * Prä-Inkrement-Operator.
		 * Setzt den Iterator auf das nächste Element im Hash.
		 *
		 * @return true, falls Inkrementierung möglich war.
		 */
		inline iterator & operator++()
		{
			++pos_;
			return *this;
		}

		/**
		 * Post-Inkrement-Operator.
		 * Setzt den Iterator auf das nächste Element im Hash.
		 *
		 * @return true, falls Inkrementierung möglich war.
		 */
		inline iterator operator++(int)
		{
			iterator tmp(*this);
			++*this;
			return tmp;
		}

		/**
		 * Dereferenzierungs-Operator.
		 * Simuliert eine Dereferenzierung des Iterators und gibt ein
		 * Elem-Objekt mit Koordinaten und Wert des Eintrages zurück.
		 *
		 * @return Elem-Objekt für den SMatrix-Eintrag
		 */
		inline Elem operator*() const
		{
			if (!pobj_ || pos_ == pobj_->hash_->end())
				throw std::exception();
			return Elem(pos_->key.i_,pos_->key.j_,pos_->value);
		}

		/**
		 * Dereferenzierungs-Operator.
		 * Wegen der Transitivität von -> wird hier der ->-Operator
		 * von hpair verwendent:
		 *
		 * @return Elem-Objekt für den SMatrix-Eintrag
		 */
		inline Elem operator->() const { return operator*(); }

		/**
		 * Vergleichs-Operator.
		 * Bei Gleichheit von zwei Iteratoren entsprechen sich die
		 * Kollisionslisten-Zeiger
		 *
		 * @param rval zu vergleichendes Iterator-Objekt
		 * @return true, falls Gleichheit
		 */
		inline bool operator==(iterator const & rval) const
		{
			return pos_==rval.pos_;
		}

		/**
		 * Vergleichs-Operator (Ungleichheit).
		 * Siehe operator==().
		 *
		 * @param rval zu vergleichendes Iterator-Objekt
		 * @return true, falls Ungleichheit
		 */
		inline bool operator!=(iterator const & rval) const
		{
			return pos_!=rval.pos_;
		}

		/**
		 * Ein Cast-Operator zum casten von iterator nach
		 * const_iterator. Ermöglicht eine Zuweisung vom Typ
		 * iterator=const_iterator.
		 *
		 * @return const_iterator object
		 */
		inline operator const_iterator() const
		{
			return const_iterator(pobj_,pos_);
		}
	};

public:
	/**
	 * Constructor
	 *
	 * @param rows Spalten
	 * @param cols Zeilen
	 * @param expct_fill Erwarteter Füllfaktor der Matrix
	 */
	inline SMatrix(size_t rows, size_t cols, double expct_fill = 0.05)
		: rows_(rows), cols_(cols)
	{
		hash_ = new fhash_map< mxkoo,double,mxkoo_hashf >(
			size_t(1. + double(rows_) * double(cols_) * expct_fill)
			);
	}

	/**
	 * Copy-Constructor. Kopiert das Hash mit den Matrix-Elementen
	 *
	 * @param copy Zu kopierende Sparse-Matrix
	 */
	inline SMatrix(SMatrix const & copy)
		: rows_(copy.rows_), cols_(copy.cols_)
	{
		hash_ = new fhash_map< mxkoo,double,mxkoo_hashf >( *(copy.hash_) );
	}

	/**
	 * "Copy"-Constructor für die MMatrix-Klasse.
	 * Kopiert von 0 verschiedene Elemente in die Sparse-Matrix
	 *
	 * @param copy Zu kopierende Matrix
	 */
	SMatrix(MMatrix const & copy);

	/**
	 * Constructor für Dummy-Objekte
	 */
	inline SMatrix() :
		rows_(0), cols_(0),
		hash_(new fhash_map< mxkoo,double,mxkoo_hashf >) { }

	/**
	 * Destructor. Löscht das Hash mit den Matrix-Elementen
	 */
	inline ~SMatrix() { delete hash_; }
public:
	/**
	 * Ein Cast-Operator von SMatrix nach MMatrix
	 *
	 * @return ein MMatrix-Objekt mit dem Inhalt der Sparse-Matrix
	 */
	operator MMatrix () const;

	/**
	 * SMatrix-MVektor-Multiplikation
	 *
	 * @param rval rechtes Argument
	 * @return Produkt
	 */
	MVector operator*(MVector const & rval) const;
        
        
        /**
	 * SMatrix-MVektor-Multiplikation
	 *
	 * @param rval rechtes Argument
	 * @return Produkt
	 */
	SMatrix operator*= (double rval);
        

	/**
	 * SMatrix-MMatrix-Multiplikation
	 *
	 * @param rval rechtes Argument
	 * @return Produktmatrix
	 */
	MMatrix operator*(MMatrix const & Rval) const;

	/**
	 * Zugriff zum Lesen/Schreiben auf ein Element (i,j). Wenn das Element
	 * (i,j) noch nicht existerte, wird es zunächst erzeugt.
	 *
	 * @param i Zeile
	 * @param j Spalte
	 * @return Referenz auf das evtl. neu erzeugte Element (i,j)
	 */
	inline double & operator() (size_t i, size_t j)
	{
		fASSERT(i<rows_ && j<cols_);
		return hash_->operator[](mxkoo(i,j));
	}

	/**
	 * Zuweisungsoperator.
	 *
	 * @param copy Zu kopierendes Objekt
	 * @return Referenz auf *this
	 */
	inline SMatrix & operator= (SMatrix const & copy)
	{
		delete hash_;
		hash_ = new fhash_map< mxkoo,double,mxkoo_hashf >( *(copy.hash_) );
		rows_ = copy.rows_;
		cols_ = copy.cols_;
		return *this;
	}

	/**
	 * Bevorzugte Schnittstelle zum Schreiben von Elementen
	 *
	 * @param i Zeile
	 * @param j Spalte
	 * @param val Wert
	 */
	inline void set(size_t i, size_t j, double const & val)
	{
		fASSERT(i<rows_ && j<cols_);
		hash_->insert(mxkoo(i,j),val);
	}

	/**
	 * Bevorzugte Schnittstelle zum Lesen von Elementen
	 *
	 * @param i Zeile
	 * @param j Spalte
	 * @return Elementwert oder Default-Wert 0.0, falls Element nicht existiert.
	 */
	inline double const & get(size_t i, size_t j) const
	{
		fASSERT(i<rows_ && j<cols_);
		double * dptr = hash_->findPtr(mxkoo(i,j));
		return dptr?*dptr:zero_;
	}

	/**
	 * Abfrage von Elementen. Wenn das Element (i,j) existiert, wird
	 * ein Zeiger auf seinen Wert zurückgegeben. Wenn es nicht existiert,
	 * wird ein Nullzeiger zurückgegeben.
	 *
	 * @param i Zeile
	 * @param j Spalte
	 * @return Zeiger auf den Wert (i,j) oder Null-Zeiger
	 */
	inline double * refIfSet(size_t i, size_t j) const
	{
		fASSERT(i<rows_ && j<cols_);
		return hash_->findPtr(mxkoo(i,j));
	}

	/**
	 * Löschen von Elementen.
	 * Wenn das Element (i,j) existiert, wird es gelöscht und es wird
	 * <tt>true</tt> zurückgegeben. Existiert (i,j) nicht, wird <tt>false</tt>
	 * zurückgegeben
	 *
	 * @param i Zeile
	 * @param j Spalte
	 * @return true, falls Löschung erfolgreich, false, falls Element nicht gefunden
	 */
	inline bool erase(size_t i, size_t j)
	{
		fASSERT(i<rows_ && j<cols_);
		return hash_->erase(mxkoo(i,j));
	}

	/**
	 * Löscht alle Elemente.
	 */
	inline void eraseAll() { hash_->clear(); }

	/**
	 * Anzahl der Zeilen
	 *
	 * @return Anzahl der Zeilen
	 */
	inline size_t rows() const { return rows_; }

	/**
	 * Anzahl der Spalten
	 *
	 * @return Anzahl der Spalten
	 */
	inline size_t cols() const { return cols_; }

	/**
	 * Füllfaktor (der Matrix, nicht des Hashes)
	 *
	 * @return Füllfaktor der Matrix
	 */
	inline float fillFactor() const
	{
		return float(hash_->size())/float(rows_*cols_);
	}

	/**
	 * Anzahl der Einträge (ungleich 0).
	 *
	 * @return Anzahl der Non-Zero-Einträge
	 */
	inline size_t nnz() const { return hash_->size(); }

	/**
	 * Gibt eine Statistik zum internen Hash zurück.
	 * Berechnet statistische Kenngrößen der Verteilung der
	 * Kollisionslistenlängen. Damit läßt sich die Qualität der
	 * Hash-Funktion bewerten. Die chi-Quadrat-Parameter sind
	 * optional.
	 *
	 * @param N Größe der Hash-Tabelle
	 * @param E Erwartungswert der Kollisionslistenlänge
	 * @param V Varianz der Kollisionlistenlängenverteilung
	 * @param chisq chi-Quadrat-Wert der Kollisionlistenlängenverteilung
	 * @param chidof Anzahl der Freiheitsgrade der chi-Quadrat-Verteilung
	 */
	inline void stats(
		int & N,
		double & E,
		double & V,
		double & chisq,
		int & chidof
		) const
	{
		hash_->stats(N,E,V,chisq,chidof);
	}

public:
	/**
	 * Erzeugt einen iterator und setzt ihn auf das erste
	 * Element der Sparse-Matrix (Hash)
	 *
	 * @return Iterator, der auf das erste Matrix-Element "zeigt"
	 */
	inline iterator begin() { return iterator(this); }
	
	/**
	 * Erzeugt einen const_iterator und setzt ihn auf das erste
	 * Element der Sparse-Matrix (Hash)
	 *
	 * @return Iterator, der auf das erste Matrix-Element "zeigt"
	 */
	inline const_iterator begin() const { return const_iterator(this); }

	/**
	 * Erzeugt einen iterator mit Ende-Markierung.
	 *
	 * @return end-Iterator
	 */
	inline iterator end() { return iterator(); }
	
	/**
	 * Erzeugt einen const_iterator mit Ende-Markierung.
	 *
	 * @return end-Iterator
	 */
	inline const_iterator end() const { return const_iterator(); }

public:
	/**
	 * Zeigt die Besetztheit der Sparse-Matrix
	 */
	void spy() const;

	/**
	 * Gibt die Elemente der Sparse-Matrix auf stdout aus.
	 */
	void dump(FILE * outf = stdout, dump_t dt = dump_default) const;

	/**
	 * Symmetrische Permutation: P^T*A*P
	 *
	 * @param P Permutationsmatrix
	 */
	void symmPerm(PMatrix const & P);

	/**
	 * Messung der Bandbreite einer Sparse-Matrix A.
	 * 
	 * @param lb gemessene untere/linke Bandbreite
	 * @param ub gemessene obere/rechte Bandbreite
	 */
	void measureBandWidth(int & lb, int & ub) const;

	/**
	 * Macht die Matrix sparse: Identifiziert 0-Elemente und löscht sie
	 * aus der Datenstruktur
	 */
	void compress();
        
        MVector diag() const;
        
        double trace() const;
        
        double norm1() const;
};

} // namespace flux::la
} // namespace flux

#endif

