#include <list>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "charptr_map.h"
#include "IsoReaction.h"
#include "XMLException.h"
#include "XMLElement.h"
#include "FluxMLContentObject.h"
#include "FluxMLMetabolitePools.h"
#include "FluxMLReaction.h"
#include "UnicodeTools.h"
#include "FluxMLUnicodeConstants.h"
#include "FluxMLReactionNetwork.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

namespace flux {
namespace xml {

void FluxMLReactionNetwork::parseReactionNetwork(
	DOMNode * node
	)
{
	DOMNode * child;

	if (not XMLElement::match(node,fml_reactionnetwork,fml_xmlns_uri))
		fTHROW(XMLException,node,"element node (reactionnetwork) expected.");

	child = XMLElement::skipJunkNodes(node->getFirstChild());

	if (not XMLElement::match(child,fml_metabolitepools,fml_xmlns_uri))
		fTHROW(XMLException,child,"element node (metabolitepools) expected.");
	FluxMLMetabolitePools metabolite_pools(doc_,child);
	
	// zum nächsten Element-Knoten vorrücken
	child = XMLElement::nextNode(child);

	// mindestends eine Reaktion parsen:
	do
	{
		if (not XMLElement::match(child,fml_reaction,fml_xmlns_uri))
			fTHROW(XMLException,child,"element node (reaction) expected.");

		// eine Reaktion parsen und in die Liste einfügen
		FluxMLReaction fluxml_reaction(
			doc_,
			child
			);

		// zum nächsten Element-Knoten vorrücken
		child = XMLElement::nextNode(child);
	}
	while (child != 0);
}

} // namespace flux::xml
} // namespace flux

