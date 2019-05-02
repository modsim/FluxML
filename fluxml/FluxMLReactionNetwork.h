#ifndef FLUXMLREACTIONNETWORK_H
#define FLUXMLREACTIONNETWORK_H

#include <list>
#include "charptr_map.h"
#include "Pool.h"
#include "IsoReaction.h"
#include "XMLException.h"
#include "FluxMLContentObject.h"
#include "FluxMLDocument.h"

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zum Parsen / Serialisieren eines Reaktionsnetzwerkes in FluxML.
 *
 * <reactionnetwork type="(isotopomer|bondomer)">
 *   <metabolitepools>
 *     ... FluxMLMetabolitePools ...
 *   </metabolitepools>
 *   <!-- mittels FluxMLReaction: -->
 *   <reaction>...</reaction>
 *   <reaction>...</reaction>
 *   ...
 * </reactionnetwork>
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */ 
  
class FluxMLReactionNetwork : public FluxMLContentObject
{
public:
	/**
	 * Constructor.
	 *
	 * @param doc Zeiger auf das FluxML-Dokument mit Pool- und Reaktions-Daten
	 */
	inline FluxMLReactionNetwork(FluxMLDocument * doc)
		: FluxMLContentObject(doc) { }
	
	/**
	 * Constructor.
	 * Erzeugt ein FluxMLReactionNetwork durch Parsen aus einem DOM-Tree.
	 * 
	 * @param node Knoten im DOM-Tree
	 */
	inline FluxMLReactionNetwork(FluxMLDocument * doc, XN DOMNode * node)
		: FluxMLContentObject(doc)
	{
		parseReactionNetwork(node);
	}

	/**
	 * Destructor.
	 */
	inline virtual ~FluxMLReactionNetwork()	{ }

	/**
	 * Parst ein Reaktion-Network aus einem FluxML-Dokument
	 * 
	 * @param node reactionnetwork-Element-Knoten des DOM-Dokuments
	 */
	inline void parse(XN DOMNode * node)
	{
		parseReactionNetwork(node);
	}

	/**
	 * Gibt den Typ des FluxMLContentObject zurück. In diesem Fall immer
	 * CO_REACTION_NETWORK.
	 *
	 * @return der Wert FluxMLContentObject.CO_REACTION_NETWORK
	 */
	inline int getType()
	{
		return FluxMLContentObject::CO_REACTION_NETWORK;
	}

private:
	/**
	 * Parst ein Reaktionsnetwerk aus einem DOM-Tree.
	 *
	 * @param node Knoten des DOM-Tree
	 */
	void parseReactionNetwork(XN DOMNode * node);
	
}; // class FluxMLReactionNetwork

} // namespace flux::xml
} // namespace flux

#endif

