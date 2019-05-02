#ifndef MATHMLEXPRESSION_H
#define MATHMLEXPRESSION_H

#include <string>
#include <xercesc/dom/DOM.hpp>
#include "MathMLContentObject.h"
#include "ExprTree.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

class MathMLDocument;

/**
 * Abbildung eines einfachen arithmetischen Ausdrucks in MathML.
 * Abgebildet werden können einfache Ausdrück - Vektor oder
 * Matrizen-Operationen werden hier nicht unterstützt.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class MathMLExpression : public MathMLContentObject
{
	friend class MathMLDocument;
	friend class MathMLMatrix;
	friend class MathMLVector;
private:
	/** interne Repräsentation eines MathML-Ausdrucks: Arith. Baum */
	symb::ExprTree * expr_;

private:
	/**
	 * Constructor.
	 * Erzeugt eine MathMLExpression aus einem arithmetischen Ausdruck.
	 * Es wird ein Duplikat des arithmetischen Ausdrucks angelegt.
	 *
	 * @param expr arithmetischer Ausdruck
	 */
	MathMLExpression(symb::ExprTree const * expr)
		: expr_(expr->clone()) {}

	/**
	 * Copy-Constructor.
	 */
	MathMLExpression(MathMLExpression const & copy)
		: expr_(copy.expr_->clone()) {}

	/**
	 * Destructor.
	 */
	virtual ~MathMLExpression() { delete expr_; }
public:
	/**
	 * Serialisiert das Objekt in einen Knoten eines DOM-Trees in Form
	 * von Content-MathML.
	 *
	 * @param doc DOM-Tree des Content-MathML Dokuments
	 * @param node Wurzelknoten, in den das Objekt serialisiert wird
	 */
	void serializeContentMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Serialisiert das Object in einen Knoten eines DOM-Trees in From
	 * von Presentation-MathML
	 *
	 * @param node Wurzelknoten, in den das Objekt serialisiert wird
	 */
	void serializePresentationMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Gibt den Typ des Objekts zurück.
	 * 
	 * @return Typ des Objekts
	 */
	inline Type getType() const { return co_expression; }

	/**
	 * Gibt eine String-Repräsentation des Objekts zurück
	 */
	inline std::string toString() const { return expr_->toString(); }

public:
	/**
	 * Gibt den arithmetischen Ausdruck zurück.
	 *
	 * @return arithmetischer Ausdruck
	 */
	symb::ExprTree * get() const { return expr_; }

public:
	/**
	 * Statische Methode zum Parsen eines Ausdrucks.
	 *
	 * @param doc das MathMLDocument-Objekt
	 * @param node ein Knoten im DOM-Tree
	 */
	static MathMLContentObject * parse(
		MathMLDocument * doc,
		XN DOMNode * node
		);
	
	/**
	 * Parst einen arithmetischen Ausdruck aus einem Knoten eines
	 * Content-MathML Dokuments.
	 *
	 * @param node Knoten des Content-MathML-Dokuments
	 * @retval geparster arithmetischer Ausdruck
	 */
	static symb::ExprTree * parseExpression(XN DOMNode * node);

private:
	/**
	 * Parst einen Identifier aus einem Knoten eines Content-MathML Dokuments
	 *
	 * @param node Knoten des Content-MathML Dokuments
	 * @retval geparster Identifier
	 */
	static symb::ExprTree * parseIdentifier(XN DOMNode * node);

	/**
	 * Parst eine Zahl aus einem Knoten eines Content-MathML-Dokuments.
	 * Komplexe Zahlen werden (momentan noch) nicht unterstützt.
	 * Konstanten wie &pi werden nicht unterstützt.
	 * (siehe Appendix C.2 der MathML-Spec!)
	 *
	 * @param node Knoten des Content-MathML Dokuments
	 * @retval geparste Zahl
	 */
	static symb::ExprTree * parseNumber(XN DOMNode * node);

	/**
	 * Parst ein apply-Element (Funktionsanwendung) aus einem Knoten eines
	 * Content-MathML Dokuments.
	 *
	 * <apply>
	 *   <[operator]/>
	 *   <[operand]/>
	 *   <[operand]/>
	 *   ... möglicherweise weitere Operanden ...
	 * </apply>
	 *
	 * @param node Knoten des Content-MathML-Dokuments
	 * @retval arithmetischer Baum mit Operator-Anwendung
	 */
	static symb::ExprTree * parseApply(XN DOMNode * node);

	/**
	 * Parst einen Operator aus einem Content-MathML Dokument.
	 *
	 * @param node Knoten des Content-MathML Dokuments
	 * @return Typ des geparsten Operators
	 */
	static symb::ExprType parseOperator(
		XN DOMNode * node,
		symb::ExprTree ** extra
		);

	/**
	 * Serialisiert einen arithmetischen Ausdruck in einen Knoten eines
	 * Content-MathML Dokuments.
	 *
	 * @param doc DOM-Tree des Content-MathML Dokuments
	 * @param node Knoten des Dokuments, in den serialisiert wird
	 * @param e arithmetischer (Teil-)Ausdruck
	 */
	void exprTree2ContentMathML(
			XN DOMDocument * doc,
			XN DOMNode * node,
			symb::ExprTree * e
			) const;
    
	/**
	 * Serialisiert einen arithmetischen Ausdruck in einen Knoten eines
	 * Presentation-MathML Dokuments.
	 *
	 * @param doc DOM-Tree des Content-MathML Dokuments
	 * @param node Knoten des Dokuments, in den serialisiert wird
	 * @param e arithmetischer (Teil-)Ausdruck
	 */
	void exprTree2PresentationMathML(
			XN DOMDocument * doc,
			XN DOMNode * node,
			symb::ExprTree * e
			) const;
};

} // namespace flux::xml
} // namespace flux

#endif

