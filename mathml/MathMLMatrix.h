#ifndef MATHMLMATRIX_H
#define MATHMLMATRIX_H

#include <list>
#include <xercesc/dom/DOM.hpp>
#include "ExprTree.h"
#include "XMLException.h"
#include "MathMLContentObject.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/**
 * Repräsentiert eine Matrix mit möglicherweise
 * symbolischen Einträgen in MathML.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class MathMLMatrix : public MathMLContentObject
{
	friend class MathMLDocument;
private:
	/** Anzahl der Zeilen */
	int rows_;
	/** Anzahl der Spalten */
	int cols_;
	/** true, falls die Matrix symbolische Werte enthält */
	bool is_symbolic_;
	/** interne Repräsentation der symbolischen Matrix */
	symb::ExprTree *** sym_matrix_;

private:
	/**
	 * Constructor. Erzeugt eine leere Matrix.
	 *
	 * @param rows Anzahl der Zeilen
	 * @param cols Anzahl der Spalten
	 */
	MathMLMatrix(int rows, int cols);

	/**
	 * Copy-Constructor
	 */
	MathMLMatrix(MathMLMatrix const & copy);

	/**
	 * Destructor.
	 */
	virtual ~MathMLMatrix();

public:
	/**
	 * Parst eine Matrix aus einem Content-MathML-Dokument.
	 *
	 * @param node Knoten des Content-MathML-Dokuments
	 */
	static MathMLContentObject * parse(
		MathMLDocument * doc,
		XN DOMNode * node
		);

private:
	/**
	 * Parst eine Zeile einer Matrix aus einem Content-MathML-Dokument
	 *
	 * @param node Knoten des Content-MathML-Dokuments
	 * @return Verkettete Liste der Elemente in der Matrixzeile
	 */
	static std::list< symb::ExprTree* > * parseMatrixRow(
		XN DOMNode * node,
		int & cols
		);

public:
	/**
	 * Übernahme der Inhalte (double) eines 2d-C-Arrays.
	 *
	 * @param array 2d Array mit Double-Werten
	 */
	void copyFrom(double ** array);

	/**
	 * Übernahme der Inhalte (ExprTree*) eines 2d-C-Arrays.
	 *
	 * @param array 2d Array mit ExprTree*-Werten
	 */
	void copyFrom(symb::ExprTree *** array);
	
	/**
	 * Serialisiert die Matrix in einen DOM-Tree eines Content-MathML
	 * Dokuments.
	 *
	 * @param doc DOM-Tree des Content-MathML-Dokuments
	 * @param node Knoten des Dokuments, in den die Matrix serialisiert wird
	 */
	void serializeContentMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Serialisiert die Matrix in einen DOM-Tree eines Presentation-MathML
	 * Dokuments
	 *
	 * @param doc DOM-Tree des Presentation-MathML-Dokuments
	 * @param node Knoten des Dokuments, in den die Matrix serialisiert wird
	 */
	void serializePresentationMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Gibt den Typ das MathMLContentObjects zurück - in diesem Fall immer
	 * der den Aufzählungswert co_matrix.
	 *
	 * @retval der Aufzählungswert co_matrix
	 */
	Type getType() const { return co_matrix; }

	/**
	 * Gibt einen Zeiger auf eine Komponente der Matrix zurück.
	 *
	 * @param i Zeile der Komponente
	 * @param j Spalte der Komponente
	 * @retval Referenz auf Komponente (i,j) der Matrix
	 */	
	symb::ExprTree const * get(int i, int j) const;

	/**
	 * Setzt eine Komponente der Matrix auf einen (symbolischen) Wert.
	 *
	 * @param i Zeile der Komponente
	 * @param j Spalte der Komponente
	 * @param value symbolischer Wert der Komponente
	 */
	void set(int i, int j, symb::ExprTree * value);

	/**
	 * Setzt eine Komponente der Matrix auf einen Wert.
	 *
	 * @param i Zeile der Komponente
	 * @param j Spalte der Komponente
	 * @param value Wert der Komponente
	 */
	void set(int i, int j, double value);

	/**
	 * Gibt die Anzahl der Zeilen der Matrix zurück
	 *
	 * @retval Anzahl der Zeilen der Matrix
	 */
	int getRows() const { return rows_; }

	/**
	 * Gibt die Anzahl der Spalten der Matrix zurück
	 *
	 * @retval Anzahl der Spalten der Matrix
	 */
	int getCols() const { return cols_; }

	/**
	 * Erzeugt aus der Matrix eine String-Darstellung, wie man sie aus
	 * MatLab oder Octave oder SciLab kennt.
	 *
	 * @retval String-Darstellung der Matrix
	 */
	std::string toString() const;

	/**
	 * Gibt true zurück, falls die Matrix symbolische Komponenten enthält.
	 *
	 * @retval true, falls die Matrix symbolische Komponenten enthält
	 */
	bool isSymbolic() const { return is_symbolic_; }
};

} // namespace flux::xml
} // namespace flux

#endif
