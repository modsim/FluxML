#ifndef MATHMLDECLARE_H
#define MATHMLDECLARE_H

#include <string>
#include <xercesc/dom/DOM.hpp>
#include "XMLException.h"
#include "MathMLContentObject.h"

#include "MathMLExpression.h"
#include "MathMLMatrix.h"
#include "MathMLVector.h"
#include "MathMLLambdaExpression.h"
#include "MathMLDocument.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/**
 * Abbildung einer Definition (Deklaration) in MathML
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class MathMLDeclare : public MathMLContentObject
{
	friend class MathMLDocument;
private:
	/** Name der Definition */
	std::string def_name_;
	/** Wert der Definition */
	MathMLContentObject * def_value_;

private:
	/**
	 * Erzeugt eine Definition aus einem Namen und einem Wert
	 *
	 * @param value Wert der Definition
	 * @param name Name der Definition
	 */
	MathMLDeclare(
		MathMLContentObject * def_value,
		std::string const & def_name = ""
		);

	/**
	 * Destructor
	 */
	virtual ~MathMLDeclare();

public:
	/**
	 * Gibt true zurück, falls die Definition keinen Namen hat.
	 *
	 * @return true, falls die Definition anonym (unbenamt) ist.
	 */
	inline bool isAnonymous() const;

	inline Type getType() const { return co_declare; }

public:
	/**
	 * Parst eine MathML-Definition (Deklaration):
	 * <declare>
	 *   <ci>Identifier</ci>
	 *   <[operand]/>
	 * </declare>
	 *
	 * @param node Knoten mit Definition
	 */
	static MathMLContentObject * parse(
		MathMLDocument * doc,
		XN DOMNode * node
		);

public:
	/**
	 * Serialisiert die Definition nach Content-MathML.
	 *
	 * @param doc DOM-Tree, in den serialisiert werden soll
	 * @param node Knoten des DOM-Trees, in den serialisiert werden soll
	 */
	void serializeContentMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Serialisiert die Definition nach Presentation-MathML.
	 *
	 * @param doc DOM-Tree, in den serialisiert werden soll
	 * @param node Knoten des DOM-Trees, in den serialisiert werden soll
	 */
	void serializePresentationMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Erzeugt eine String-Repräsentation der Definition
	 */
	std::string toString() const;

	/**
	 * Gibt den Namen der Definition zurück.
	 *
	 * @return Name der Definition
	 */
	std::string const & getName() const;

	/**
	 * Gibt den Wert der Definition zurück.
	 *
	 * @return Wert der Definition
	 */
	MathMLContentObject const * getValue() const;
};

} // namespace flux::xml
} // nameapace flux

#endif

