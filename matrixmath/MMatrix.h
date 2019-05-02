#ifndef MMATRIX_H
#define MMATRIX_H

#include "GMatrix.h"

namespace flux {
namespace la {

class PMatrix;
class MVector;

class MMatrix : public GMatrix< double >
{
public:
	/**
	 * Default-Constructor.
	 */
	MMatrix() : GMatrix< double >() { }

	/**
	 * Constructor.
	 *
	 * @param r Anzahl der Zeilen
	 * @param c Anzahl der Spalten
	 */
	MMatrix(size_t r, size_t c) : GMatrix< double >(r, c, 0.) { }

	/**
	 * Constructor (inkl. Initialisierung).
	 *
	 * @param r Anzahl der Zeilen
	 * @param c Anzahl der Spalten
	 * @param init Initialisierungswert für die Einträge
	 */
	MMatrix(size_t r, size_t c, double init)
		: GMatrix< double >(r, c, init) { }

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MMatrix(MMatrix const & copy) : GMatrix< double >(copy) { }
	
	/**
	 * Copy-Constructor (GMatrix).
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MMatrix(GMatrix< double > const & copy) : GMatrix< double >(copy) { }

	/**
	 * Destructor.
	 */
	virtual ~MMatrix() { }

public:

	/**
	 * Formatiert eine Matrix à la Matlab/Octave.
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
	void dump(FILE * outf = stdout, dump_t dt = dump_default, char const * fmt = "sg") const;

	/**
	 * Zeigt übersichtlich die Besetztheit von Matrizen
	 * (ähnlich wie in MatLab)
	 */
	void spy() const;

	/**
	 * Mißt die Bandbreite der Matrix (teuer!, O(N^2))
	 *
	 * @param lb untere/linke Bandbreite
	 * @param ub obere/rechte Bandbreite
	 */
	void measureBandWidth(int & lb, int & ub) const;

	/**
	 * Gibt eine Zeile der Matrix als Vektor zurück.
	 *
	 * @param r Zeile
	 * @retval Vektor mit den Elementen der Zeile r der Matrix
	 */
//	inline MVector & getRow(size_t r) const;

	/**
	 * Gibt eine Spalte der Matrix als Vektor zurück.
	 *
	 * @param r Zeile
	 * @retval Vektor mit den Elementen der Spalte c der Matrix
	 */
//	inline MVector & getCol(size_t c) const;

	/**
	 * Inf-Norm der Matrix (Maximum der 1-Normen der Zeilenvektoren,
	 * d.h. größte Summe der Beträge der Zeilenvektorelemente).
	 *
	 * @return Inf-Norm der Matrix
	 */
	double normInf() const;
        
        /**
	 * Inf-Norm der Matrix (Maximum der 1-Normen der Spaltenvektoren,
	 * d.h. größte Summe der Beträge der Spaltenvektorelemente).
	 *
	 * @return Inf-Norm der Matrix
	 */
	double normInfCwise() const;
        
        
        /**
	 * 1-Norm der Matrix (Maximum der 1-Normen der Zeilenvektoren,
	 * d.h. größte Summe der Beträge der Zeilenvektorelemente).
	 *
	 * @return Inf-Norm der Matrix
	 */
	double norm1() const;

	/**
	 * 2-norm der Matrix (=sqrt(lambda_max(A^T.A))).
	 * Entspricht dem maximalen Singulärwert
	 *
	 * @return 2-Norm der Matrix
	 */
	double norm2() const;

	/**
	 * Konditionszahl kappa_inf der Matrix (Vorsicht! Rechenaufwändig).
	 *
	 * @return Konditionszahl
	 */
	double conditionKappaInf() const;

	/**
	 * Matrix-Matrix-Addition.
	 *
	 * @param Rval rechtes Argument
	 * @return Summen-Matrix
	 */
	MMatrix operator+ (MMatrix const & Rval) const;

	/**
	 * Matrix-Matrix-Addition.
	 *
	 * @param Rval rechtes Argument
	 * @return Referenz auf *this
	 */
	MMatrix & operator+= (MMatrix const & Rval);

	/**
	 * Matrix-Matrix-Subtraktion.
	 *
	 * @param Rval rechtes Argument
	 * @return Differenz-Matrix
	 */
	MMatrix operator- (MMatrix const & Rval) const;
	
	/**
	 * Matrix-Matrix-Subtraktion.
	 *
	 * @param Rval rechtes Argument
	 * @return Referenz auf *this
	 */
	MMatrix & operator-= (MMatrix const & Rval);

	/**
	 * Matrix-Matrix-Multiplikation.
	 *
	 * @param Rval rechtes Argument (Matrix)
	 * @return Matrizenprodukt (*this)*Rval
	 */
	MMatrix operator* (MMatrix const & Rval) const;

	/**
	 * Matrix-Vektor-Multiplikation
	 *
	 * @param Rval rechtes Argument (Vektor)
	 * @return Matrix-Vektor-Produkt (*this)*Rval
	 */
	MVector operator* (MVector const & rval) const;

	/**
	 * Matrix-Skalar-Multiplikation.
	 *
	 * @param rval Skalar
	 * @return Referenz auf *this
	 */
	MMatrix & operator*= (double rval);

	/**
	 * Matrix-Skalar Multiplikation.
	 *
	 * @param rval Skalar
	 * @return Produktmatrix
	 */
	MMatrix operator* (double rval) const;

	/**
	 * Matrix-Skalar-Division.
	 *
	 * @param rval Skalar
	 * @return Referenz auf *this
	 */
	MMatrix & operator/= (double rval);

	/**
	 * Matrix-Skalar-Division.
	 *
	 * @param rval Skalar
	 * @return Produktmatrix
	 */
	MMatrix operator/ (double rval) const;

	/**
	 * Gibt eine transponierte Kopie der Matrix zurück
	 *
	 * @return transponierte Kopie der Matrix
	 */
	MMatrix getTranspose() const;

	/**
	 * Gibt einen Ausschnitt der Matrix zurück
	 *
	 * @param i1 Zeile der oberen linken Ecke
	 * @param j1 Spalte der oberen linken Ecke
	 * @param i2 Zeile der unteren rechten Ecke
	 * @param j2 Spalte der unteren rechten Ecke
	 */
	MMatrix getSlice(size_t i1, size_t j1, size_t i2, size_t j2) const;
        
        
        /**
	 * Setzt einen Ausschnitt der Matrix
	 *
	 * @param i1 Zeile der oberen linken Ecke
	 * @param j1 Spalte der oberen linken Ecke
	 * @param i2 Zeile der unteren rechten Ecke
	 * @param j2 Spalte der unteren rechten Ecke
	 */
	bool setSlice(size_t i1, size_t j1, size_t i2, size_t j2, MMatrix S);
	
	/**
	 * Erzeugung einer Diagonal-Matrix auf Basis eines Vektors.
	 *
	 * @param d Vektor der Länge min(rows(),cols())
	 * @return Referenz auf *this
	 */
	MMatrix & diag(MVector & d);

	/**
	 * Gibt die Diagonale als Vektor zurück.
	 *
	 * @return Diagonal-Vektor der Matrix
	 */
	MVector diag() const;

};

} // namespace flux::la
} // namespace flux

#endif

