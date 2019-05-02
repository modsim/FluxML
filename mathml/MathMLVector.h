#ifndef MATHMLVECTOR_H
#define MATHMLVECTOR_H

#include <xercesc/dom/DOM.hpp>
#include "ExprTree.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/** Typ eines MathML-Vektors: Zeilen- oder Spalten-Vektor */
enum VectorType { v_row, v_column };
	
/**
 * Klasse zur Abbildung eines Vektors in MathML-Darstellung.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class MathMLVector : public MathMLContentObject
{
	friend class MathMLDocument;
private:
	/** Länge des Vektors */
	int dim_;
	/** interne Repräsentation des symbolischen Vektors */
	symb::ExprTree ** sym_vector_;
	/** true, falls der Vektor symbolische Werte enthält */
	bool is_symbolic_;
	/** default lt. MathML-Spec ist der Spaltenvektor */
	VectorType vtype_;

private:
	/**
	 * Constructor zum Anlegen eines Vektors
	 *
	 * @param dim Dimension des Vektors
	 * @param vtype Typ des Vektors
	 */
	MathMLVector(int dim, VectorType vtype = v_column);

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierender Vektor
	 */
	MathMLVector(MathMLVector const & copy);

	/**
	 * Destructor
	 */
	virtual ~MathMLVector();

public:
	/**
	 * Parst einen Vektor aus einem Knoten eines Content-MathML-Dokuments.
	 * <vector>
	 *  [operand] [operand] ... [operand]
	 * </vector>
	 *
	 * @param node Knoten des Content-MathML-Dokuments
	 */
	static MathMLContentObject * parse(
		MathMLDocument * doc,
		XN DOMNode * node
		);

public:
	/**
	 * Übernahme der Inhalte (double) eines C-Arrays.
	 *
	 * @param array Array mit double-Werten
	 */
	void copyFrom(double * array);
	
	/**
	 * Übernahme der Inhalte (ExprTree*) eines 2d-C-Arrays.
	 *
	 * @param array 2d Array mit ExprTree*-Werten
	 */
	void copyFrom(symb::ExprTree ** array);
	
	/**
	 * Serialisiert den Vektor in einem DOM-Tree eines Content-MathML
	 * Dokuments.
	 *
	 * @param doc DOM-Tree des Content-MathML-Dokuments
	 * @param node Knoten des Dokuments, in den der Vektor serialisert wird
	 */
	void serializeContentMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;
	
	/**
	 * Serialisiert den Vektor in einem DOM-Tree eines Presentation-MathML
	 * Dokuments.
	 *
	 * @param doc DOM-Tree des Presentation-MathML-Dokuments
	 * @param node Knoten des Dokuments, in den der Vektor serialisert wird
	 */
	void serializePresentationMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;
	
	/**
	 * Gibt den Typ das MathMLContentObjects zurück - in diesem Fall
	 * immer co_vector.
	 *
	 * @retval der Aufzählungstyp co_vector
	 */
	Type getType() const { return co_vector; };
	
	/**
	 * Gibt die Dimension des Vektors zurück.
	 *
	 * @retval Dimension des Vektors
	 */
	int dim() const { return dim_; }
	
	/**
	 * Gibt einen Zeiger auf eine Komponente des Vektors zurück.
	 *
	 * @param i Index der Komponente
	 * @retval Zeiger auf die i-te Komponente des Vektors
	 */
	symb::ExprTree const * get(int i) const;
	
	/**
	 * Setzt eine Komponente des Vektors auf einen (symbolischen) Wert.
	 *
	 * @param i Index der Komponente
	 * @param value symbolischer Wert der Komponente
	 */
	void set(int i, symb::ExprTree * value);

	/**
	 * Setzt eine Komponente des Vektors auf einen Wert.
	 *
	 * @param i Index der Komponente
	 * @param value Wert der Komponente
	 */
	void set(int i, double value);

	/**
	 * Gibt true zurück, falls es sich bei dem Vektor um einen Spaltenvektor
	 * (<it>stehender</it> Vektor) handelt.
	 *
	 * @retval true, falls der Vektor ein Spaltenvektor ist
	 */
	bool isColumnVector() const;

	/**
	 * Kennzeichnet den Vektor als Zeilen-Vektor (d.h. liegender Vektor).
	 */
	inline void setRowVector() { vtype_ = v_row; }

	/**
	 * Kennzeichnet den Vektor als Spalten-Vektor (d.h. stehender Vektor).
	 */
	void setColumnVector() { vtype_ = v_column; }

	/**
	 * Erzeugt aus dem Vektor eine String-Darstellung, wie man sie aus
	 * MatLab oder Octave oder SciLab kennt
	 *
	 * @retval String-Darstellung des Vektors
	 */
	std::string toString() const;
}; // class MathMLVector

} // namespace flux::xml
} // namespace flux

#endif

