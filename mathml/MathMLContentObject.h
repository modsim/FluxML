#ifndef MATHMLCONTENTOBJECT_H
#define MATHMLCONTENTOBJECT_H

#include <string>
#include <xercesc/dom/DOM.hpp>
#include "XMLException.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/**
 * Schnittstellenklasse für ein Objekt im Content-MathML.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class MathMLContentObject
{
public:
	/** unterschiedliche Content-Objekt-Typen */
	enum Type {
		co_mathml_doc,
		co_expression,
		co_matrix,
		co_vector,
		co_lambda,
		co_declare
	};
	
public:
	/**
	 * Virtueller Destructor.
	 * Anmerkung: Der virtuelle Destructor der Basisklasse darf nicht
	 * rein virtuell sein. Stattdessen wird hier eine (leere)
	 * Implementierung angegeben, die von den abgeleiteten Klassen
	 * überschrieben wird.
	 */
	virtual ~MathMLContentObject() { }
	
public:
	/**
	 * Serialisiert das Objekt in einen Knoten eines DOM-Trees in Form
	 * von Content-MathML.
	 *
	 * @param node Wurzelknoten, in den das Objekt serialisiert wird
	 */
	virtual void serializeContentMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const = 0;

	/**
	 * Serialisiert das Object in einen Knoten eines DOM-Trees in From
	 * von Presentation-MathML
	 *
	 * @param node Wurzelknoten, in den das Objekt serialisiert wird
	 */
	virtual void serializePresentationMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const = 0;

	/**
	 * Gibt den Typ des Objekts zurück.
	 * 
	 * @return Typ des Objekts
	 */
	virtual Type getType() const = 0;

	/**
	 * Gibt eine String-Repräsentation des Objekts zurück
	 */
	virtual std::string toString() const = 0;

};

} // namespace flux::xml
} // namespace flux

#endif

