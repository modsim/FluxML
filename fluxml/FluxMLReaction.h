#ifndef FLUXMLREACTION_H
#define FLUXMLREACTION_H

#include <list>
#include <string>
#include <xercesc/dom/DOM.hpp>
#include "FluxMLContentObject.h"
#include "IsoReaction.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zum Parsen einer Reaktion in FluxML
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */ 
    
class FluxMLReaction : public FluxMLContentObject
{
private:
	struct Variant
	{
		std::string cfg;
		double ratio;
		Variant() : ratio(-1.) { }
	};

	struct Reactant
	{
		std::string name;
		std::list< Variant > variants;
		std::list< Variant >::const_iterator iter;
		int index;
	};
	
	std::list< Reactant > in_;
	std::list< Reactant > out_;
        bool bidirectional_;

public:
	/**
	 * Constructor.
	 * Erzeugt eine FluxMLReaction durch Parsen aus einem DOM-Tree.
	 * 
	 * @param doc das FluxMLDocument-Objekt
	 * @param node Knoten im DOM-Tree
	 * @param is_isotopomer_reaction true, falls Isotopomer-Reaktion
	 */
	inline FluxMLReaction(
		FluxMLDocument * doc,
		XN DOMNode * node)
		: FluxMLContentObject(doc)
	{
		parseReaction(node);
	}

	/**
	 * Destructor.
	 */
	inline virtual ~FluxMLReaction() { }

	/**
	 * Gibt den Typ des FluxMLContentObject zurück. In diesem Fall immer
	 * CO_REACTION.
	 * 
	 * @return der Wert FluxMLContentObject.CO_REACTION
	 */
	inline int getType() { return FluxMLContentObject::CO_REACTION; }

private:
	/**
	 * Parst eine Reaktionsspezifikation:
	 * <reaction>
	 *   <educt id="..." cfg="..."/>
	 *   <educt .../>
	 *   ...
	 *   <product id="..." cfg="..."/>
	 *   <product .../>
	 * </reaction>
	 *
	 * @param node Knoten mit reaction-Element
	 * @param is_isotopomer_reaction true, falls Isotopomer-Reaktion
	 */
	void parseReaction(
		XN DOMNode * node
		);

	/**
	 * Parsen eines einzelnen Reaktions-Edukts/Produkts.
	 * 
	 * @param node Element-Knoten mit reduct/rproduct-Element
	 * @return Anzahl der Kohlenstoffatome (Länge des cfg-Attributs)
	 */
	Reactant parseReactant(
		XN DOMElement * reactant
		);
	
}; // class FluxMLReaction

} // namespace flux::xml
} // namespace flux

#endif

