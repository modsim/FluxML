#ifndef MVECTOR_H
#define MVECTOR_H

#include <cmath>
#include "GVector.h"
#include "MatrixInterface.h"

namespace flux {
namespace la {

class MMatrix;

class MVector : public GVector< double >
{
public:
	/**
	 * Constructor
	 */
	MVector()
		: GVector< double >() { }

	/**
	 * Constructor (inkl. Initialisierung)
	 *
	 * @param dim Dimension des Vektors
	 */
	MVector(size_t dim)
		: GVector< double >(dim, 0.) { }

	MVector(size_t dim, double init)
		: GVector< double >(dim, init) { }

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierender Vektor
	 */
	MVector(MVector const & copy)
		: GVector< double >(copy) { }

	/**
	 * Constructor mit Initialisierung über Array.
	 *
	 * @param copy zu kopierendes Array
	 * @param dim Dimension des Vektors
	 */
	MVector(double const * copy, size_t dim)
		: GVector< double >(copy,dim) { }

	/**
	 * Destructor
	 */
	virtual ~MVector() { }

public:
	/**
	 * 1-Norm (Summe der Beträge)
	 *
	 * @param lo unterer Index
	 * @param hi oberer Index
	 */
	double norm1(size_t lo=0, size_t hi=0) const;
	
	/**
	 * 2-Norm (Wurzel der Summe der Quadrate, sqrt(lambda_max(v'*v)))
	 *
	 * @param lo unterer Index
	 * @param hi oberer Index
	 */
	double norm2(size_t lo=0, size_t hi=0) const;
	
	/**
	 * Inf-Norm (Maximal-Betrag)
	 *
	 * @param lo unterer Index
	 * @param hi oberer Index
	 */
	double normInf(size_t lo=0, size_t hi=0) const;

	/**
	 * Euklidische Norm zweier Vektoren (kürzester Abstand zweier
	 * Punkte im kartesischen Raum). Schneller als (L-R).norm2().
	 *
	 * @param L erster Vektor
	 * @param R zweiter Vektor
	 * @return Euklidischer Abstand
	 */
	static double euklid(MVector const & L, MVector const & R);
        
         /**
	 * Max-Wert.
	 *
	 * @return neuer Vektor mit negierten Elementen
	 */
	double max() const;

	/**
	 * Gibt einen Ausschnitt (slice) des Vektors als neuen Vektor zurück.
	 *
	 * @param i untere Grenze
	 * @param j obere Grenze
	 * @return Ausschnitt des Vektors
	 */
	MVector getSlice(size_t i, size_t j) const;

	/**
	 * Formatiert einen Vektor à la Matlab/Octave.
	 * Parameter fmt:
	 * "s"  <=> format short
	 * "l"  <=> format long
	 * "se" <=> format short e
	 * "sl" <=> format long e
	 * "sg" <=> format short g
	 * "sl" <=> format long g
	 *
	 * @param outf Ausgabe-Stream
	 * @param dt dump-Typ (siehe MatrixInterface.h)
	 * @param fmt Formatierungstyp
	 */
	void dump(
		FILE * outf = stdout,
		dump_t dt = dump_default,
		char const * fmt = "sg"
		) const;

	/**
	 * Konstruiert eine Diagonal-Matrix aus dem Vektor.
	 *
	 * @param r Anzahl der Zeilen (>= dim())
	 * @param c Anzahl der Spalten (>= dim())
	 */
	MMatrix diag(size_t r=0, size_t c=0);

	/**
	 * Vektor-Matrix-Multiplikation.
	 *
	 * @param Rval Matrix, rechter Operand
	 * @return Vektor
	 */
	MVector operator*(MMatrix const & Rval) const;

	/**
	 * Skalarprodukt
	 *
	 * @param rval Vektor, rechter Operand
	 * @return Skalarprodukt
	 */
	double operator*(MVector const & rval) const;

	/**
	 * Produkt aus Vektor und Skalar.
	 *
	 * @param rval Skalar, rechter Operand
	 * @return Produkt
	 */
	MVector operator*(double rval) const;
	
	/**
	 * Produkt aus Vektor und Skalar.
	 *
	 * @param rval Skalar, rechter Operand
	 * @return Referenz auf *this
	 */
	MVector & operator*=(double rval);

	/**
	 * Elementweise Division durch einen Skalar.
	 *
	 * @param rval Skalar
	 * @return neuer Vektor
	 */
	MVector operator/(double rval) const;

	/**
	 * Elementweise Division durch einen Skalar.
	 *
	 * @param rval
	 * @return Referenz auf *this
	 */
	MVector & operator/=(double rval);

	/**
	 * Vektor-Addition.
	 *
	 * @param rval Vektor, rechter Operand
	 * @return Vektor
	 */
	MVector operator+(MVector const & rval) const;

	/**
	 * Vektor-Addition.
	 *
	 * @param rval Vektor, rechter Operand
	 * @return Referenz auf *this
	 */
	MVector & operator+=(MVector const & rval);

	/**
	 * Vektor-Subtraktion.
	 *
	 * @param rval Vektor, rechter Operand
	 * @return Vektor
	 */
	MVector operator-(MVector const & rval) const;
	
	/**
	 * Vektor-Subtraktion.
	 *
	 * @param rval Vektor, rechter Operand
	 * @return Referenz auf *this
	 */
	MVector & operator-=(MVector const & rval);

	/**
	 * Unäres Minus.
	 *
	 * @return neuer Vektor mit negierten Elementen
	 */
	MVector operator-() const;
              
};

MVector operator*(double lval, MVector const & rval);

} // namespace flux::la
} // namespace flux

#endif
