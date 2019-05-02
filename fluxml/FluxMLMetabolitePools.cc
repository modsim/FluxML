#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "charptr_map.h"
#include "Pool.h"
#include "XMLException.h"
#include "XMLElement.h"
#include "FluxMLContentObject.h"
#include "FluxMLPool.h"
#include "FluxMLUnicodeConstants.h"
#include "UnicodeTools.h"
#include "FluxMLMetabolitePools.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

namespace flux {
namespace xml {

void FluxMLMetabolitePools::parseMetabolitePools(DOMNode * node)
{
	if (not XMLElement::match(node,fml_metabolitepools,fml_xmlns_uri))
		fTHROW(XMLException,node,"element node (metabolitepools) expected.");

	if (doc_->getPoolMap()->size() != 0)
		fTHROW(XMLException,node,"FluxMLMetabolitePools already initialized");

	// erster Nachfolger des metabolitepools-Elements
	DOMNode * child = XMLElement::skipJunkNodes(node->getFirstChild());

	do
	{
		if (not XMLElement::match(child,fml_pool,fml_xmlns_uri))
			fTHROW(XMLException,child,"element node (pool) expected.");
		
		// den Pool parsen:
		FluxMLPool fluxml_pool(doc_, child);

		// zum nächsten Pool (falls vorhanden) weiterrücken
		child = XMLElement::nextNode(child);
	}
	while (child != 0);
}

} // namespace flux::xml
} // namespace flux

