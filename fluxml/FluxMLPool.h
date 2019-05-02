#ifndef FLUXMLPOOL_H
#define FLUXMLPOOL_H

#include <xercesc/dom/DOM.hpp>
#include "Pool.h"
#include "XMLException.h"
#include "FluxMLContentObject.h"
#include "FluxMLDocument.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zum Parsen / Serialisieren eines pool-Elements in einem
 * FluxML-Dokument.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */ 
    
class FluxMLPool : public FluxMLContentObject
{
private:
	/** das interne Pool-Objekt */
	data::Pool * pool_;

public:
	/**
	 * Erzeugt ein FluxMLPool-Objekt mit vorgegebenen Eigenschaften.
	 *
	 * @param pool ein Pool-Objekt
	 */
	FluxMLPool(FluxMLDocument * doc, data::Pool * pool)
		: FluxMLContentObject(doc), pool_(pool) { }
	
	/**
	 * Erzeugt ein FluxMLPool-Objekt durch Parsen aus einem
	 * DOM-Tree.
	 *
	 * @param node Knoten im DOM-Tree
	 */
	inline FluxMLPool(
		FluxMLDocument * doc,
		XN DOMNode * node
		)
		: FluxMLContentObject(doc), pool_(0)
	{
		parsePool(node);
	}

	/**
	 * Destructor.
	 */
	inline virtual ~FluxMLPool() { }
	
	/**
	 * Parst ein Reaktion-Network aus einem FluxML-Dokument
	 *
	 * @param node reactionnetwork-Element-Knoten des DOM-Dokuments
	 */
	inline void parse(XN DOMNode * node)
	{
		parsePool(node);
	}
	
	/**
	 * Gibt den Typ des FluxMLContentObject zurück. 
	 * In diesem Fall immer CO_POOL.
	 *
	 * @return der Wert FluxMLContentObject::CO_POOL
	 */
	int getType() { return FluxMLContentObject::CO_POOL; }
	
	/**
	 * Gibt das interne Pool-Objekt zurück.
	 *
	 * @return das interne Pool-Objekt
	 */
	data::Pool * getPool() { return pool_; }

private:
	/**
	 * Parst die Pools einer Netzwerkspezifikation aus einem
	 * pool-Element-Knoten eines DOM-Dokuments
	 *
	 * @param node pool-Element-Knoten eines DOM-Dokuments
	 */
	void parsePool(XN DOMNode * node);
}; // class FluxMLPool

} // namespace flux::xml
} // namespace flux

#endif

