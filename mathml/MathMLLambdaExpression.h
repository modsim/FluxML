#ifndef MATHMLLAMBDAEXPRESSION_H
#define MATHMLLAMBDAEXPRESSION_H

#include <string>
#include <list>
#include "ExprTree.h"
#include "MathMLContentObject.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/**
 * Lambda-Ausdrücke in MathML.
 * Ein Lambda-Ausdruck definiert eine Funktion im Sinne einer
 * Programmiersprache. Er ist ein Tupel aus einem Tupel von Parameter-
 * Variablen und einem arithmetischen Ausdruck. Innerhalb des arithmetischen
 * Ausdrucks "überschreiben" die Variablen des Parameter-Tupels evtl.
 * vorhandene "globale" Variablen. ... leider in MathML ungetypt :-(
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class MathMLLambdaExpression : public MathMLContentObject
{
	friend class MathMLDocument;
private:
	/** eine Liste (Tupel) von Parametervariablen */
	std::list<std::string> lambda_vars_;
	/** der arithmetische Ausdruck des Lambda-Ausdrucks */
	MathMLExpression * lambda_expr_;

private:
	/**
	 * Constructor.
	 * Erzeugt einen Lambda-Ausdruck aus einer Parameterliste und einem
	 * arithmetischen Ausdruck.
	 *
	 * @param vars Parameterliste
	 * @param expr arithmetischer Ausdruck
	 */
	MathMLLambdaExpression(
		std::list<std::string> const & vars,
		MathMLExpression * expr
		) : lambda_vars_(vars), lambda_expr_(expr) { }

	/**
	 * Privater Copy-Constructor.
	 */
	MathMLLambdaExpression(MathMLLambdaExpression const & copy)
	: lambda_expr_(0)
	{ }

public:
	/**
	 * Destructor.
	 */
	virtual ~MathMLLambdaExpression() { }

public:
	/**
	 * Parst einen Lambda-Ausdruck aus einem Knoten eines Content-MathML
	 * Dokuments.
	 *
	 * <lambda>
	 *   <bvar><ci>p1</ci></bvar>
	 *   <bvar><ci>p2</ci></bvar>
	 *   ...
	 *   <bvar><ci>pn</ci></bvar>
	 *   <apply>
	 *     <!-- Funktion der gebundenen Variablen p1,p2, ... pn -->
	 *   </apply>
	 * </lambda>
	 *
	 * @param node Wurzelknoten des Lamda-Ausdrucks
	 */
	static MathMLContentObject * parse(
		MathMLDocument * doc,
		XN DOMNode * node
		);
    
public:
	/**
	 * Gibt den Typ des MathMLContentObject zurück. In diesem Fall ist das
	 * immer der Aufzählungswert co_lambda.
	 *
	 * @return Aufzählungswert co_lambda
	 */
	Type getType() const { return co_lambda; }

	/**
	 * Gibt die Variablenliste des Lambda-Audrucks in Form einer verketteten
	 * Liste zurück.
	 *
	 * @retval Variablenliste des Lambda-Ausdrucks
	 */	
	std::list<std::string> const & getVarList() const { return lambda_vars_; }

	/**
	 * Gibt den arithmetischen Ausdruck des Lambda-Ausdrucks zurück.
	 *
	 * @retval arithmetischer Ausdruck des Lambda-Ausdrucks
	 */
	MathMLExpression const * getLambdaExpr() const { return lambda_expr_; }

	/**
	 * Konvertiert den Lambda-Ausdruck in eine String-Darstellung
	 *
	 * @retval String-Darstellung des Lambda-Ausdrucks
	 */
	std::string toString() const;

	/**
	 * Serialisiert den Lambda-Ausdruck in ein Content-MathML Dokument
	 *
	 * @param doc DOM-Tree des Content-MathML Dokuments
	 * @param node Knoten, in den serialisiert werden soll
	 */
	void serializeContentMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Serialisiert den Lambda-Ausdruck in ein Presentation-MathML
	 * Dokument.
	 *
	 * @param doc DOM-Tree des Presentation-MathML Dokuments
	 * @param node Knoten, in den serialisiert werden soll
	 */
	void serializePresentationMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;
};

} // namespace flux::xml
} // namespace flux

#endif

