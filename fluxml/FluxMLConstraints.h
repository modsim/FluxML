#ifndef FLUXMLCONSTRAINTS_H
#define FLUXMLCONSTRAINTS_H

#include <list>
#include <xercesc/dom/DOM.hpp>
#include "ExprTree.h"
#include "Constraint.h"
#include "Configuration.h"
#include "FluxMLContentObject.h"
#include "FluxMLDocument.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zum Parsen / Serialisieren von Constraints in
 * fluxml-Element-Knoten von FluxML-Dokumenten.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */
    
class FluxMLConstraints : public FluxMLContentObject
{
private:
	data::Configuration * cfg_;
public:	
	/**
	 * Constructor.
	 * Erzeugt ein FluxMLConstraints-Objekt durch Parsen aus einem
	 * DOM-Tree.
	 *
	 * @param node Knoten im DOM-Tree
	 */
	inline FluxMLConstraints(
		FluxMLDocument * doc,
		XN DOMNode * node,
		char const * cfg_name
		)
		: FluxMLContentObject(doc)
	{
		cfg_ = doc->getConfiguration(cfg_name, false);
		parseConstraints(node);
	}
	
	/**
	 * Constructor.
	 * Übernimmt ein bestehendes Constraint-Element aus einem
	 * FluxMLDocument-Objekt. Zur Serialisierung.
	 *
	 * @param doc Zeiger auf das FluxMLDocument mit den Constraint-Daten
	 */
	inline FluxMLConstraints(
		FluxMLDocument * doc,
		char const * cfg_name
		) : FluxMLContentObject(doc)
	{
		cfg_ = doc->getConfiguration("__root__", false);
	}
		   

	/**
	 * Virtueller Destructor.
	 * Da alle Allokation im umgebenden FluxMLDocument-Objekt stattfindet,
	 * gibt es hier nichts zu tun.
	 */
	inline virtual ~FluxMLConstraints() { }
	
	/**
	 * Gibt den Typ des FluxMLContentObject zurück. In diesem Fall immer
	 * CO_CONSTRAINTS.
	 *
	 * @return der Wert FluxMLContentObject.CO_CONSTRAINTS
	 */
	inline int getType() { return FluxMLContentObject::CO_CONSTRAINTS; }
	
private:
	/**
	 * Parst die Pools einer Netzwerkspezifikation aus einem
	 * Element-Knoten eines DOM-Dokuments.
	 *
	 * @param node Element-Knoten eines DOM-Dokuments
	 */
	void parseConstraints(XN DOMNode * node);
	
	/**
	 * Parst Constraint-Spezifikationen in MathML.
	 *
	 * @param node math-Elementknoten
	 * @param is_netto true, falls Netto-Constraint
	 */
	void parseConstraintsMathML(XN DOMNode * node, data::ParameterType parameter_type);

	/**
	 * Parst textuelle Constraint-Spezifikationen.
	 *
	 * @param node textual-Elementknoten
	 * @param is_netto true, falls Netto-Constraint
	 */
	void parseConstraintsTextual(XN DOMNode * node, data::ParameterType parameter_type);

}; // class FluxMLConstraints

} // namespace flux::xml
} // namespace flux

#endif

