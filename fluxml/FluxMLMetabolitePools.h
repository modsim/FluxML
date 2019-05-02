#ifndef FLUXMLMETABOLITEPOOLS_H
#define FLUXMLMETABOLITEPOOLS_H

#include <xercesc/dom/DOM.hpp>
#include "XMLException.h"
#include "charptr_map.h"
#include "Pool.h"
#include "FluxMLContentObject.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zum Parsen / Serialisieren eines metabolitepools-Elements eines
 * FluxML-Dokuments.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */ 
    
class FluxMLMetabolitePools : public FluxMLContentObject
{
public:
	/**
	 * Erzeugt ein FluxMLMetabolitePools-Objekt durch Parsen aus einem
	 * DOM-Tree.
	 *
	 * @param node Knoten im DOM-Tree
	 */
	inline FluxMLMetabolitePools(
		FluxMLDocument * doc,
		XN DOMNode * node
		)
		: FluxMLContentObject(doc)
	{
		parseMetabolitePools(node);
	}

	/**
	 * Erzeugt ein FluxMLMetabolitePools-Objekt aus einer Abbildung
	 * Pool-Name -> Pool.
	 *
	 * @param pool_map Abbildung Pool-Name nach Pool
	 */
	inline FluxMLMetabolitePools(FluxMLDocument * doc)
		: FluxMLContentObject(doc) { }

	/**
	 * Virtueller Destructor.
	 */
	inline virtual ~FluxMLMetabolitePools() { }

	/**
	 * Gibt den Typ des FluxMLContentObject zurück. In diesem Fall immer
	 * CO_METABOLITE_POOLS.
	 *
	 * @return der Wert FluxMLContentObject.CO_METABOLITE_POOLS
	 */
	inline int getType()
	{
		return FluxMLContentObject::CO_METABOLITE_POOLS;
	}

private:
	/**
	 * Parst die Pools einer Netzwerkspezifikation aus einem
	 * metabolitepools-Element-Knoten eines DOM-Dokuments
	 *
	 * @param node metabolitepools-Element-Knoten eines DOM-Dokuments
	 */
	void parseMetabolitePools(XN DOMNode * node);

}; // class FluxMLMetabolitePools

} // namespace flux::xml
} // namespace flux

#endif

