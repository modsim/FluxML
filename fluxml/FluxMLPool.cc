#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "cstringtools.h"
#include "Notation.h"
#include "Pool.h"
#include "SimLimits.h"
#include "UnicodeTools.h"
#include "XMLElement.h"
#include "FluxMLUnicodeConstants.h"
#include "FluxMLPool.h"
#include "fRegEx.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

namespace flux {
namespace xml {

void FluxMLPool::parsePool(DOMNode * node)
{
	DOMAttr * attrNode;
	DOMNamedNodeMap * nnm;
	std::string name;
	std::string cfg;
	int natoms;
	double poolsize = 1.;

	if (pool_)
		fTHROW(XMLException,"FluxMLPool object already initialized!");
	
	if (not XMLElement::match(node,fml_pool,fml_xmlns_uri))
		fTHROW(XMLException,node,"element node (pool) expected.");
	
	// eine Liste der Attribute abfragen
	nnm = node->getAttributes();
	
	// das id-Attribut ist erforderlich (#REQUIRED)
	attrNode = static_cast< DOMAttr* >(nnm->getNamedItem(fml_id));
	if (attrNode == 0)
		fTHROW(XMLException,node,"pool lacks the required unique id");
	U2A utf_name(attrNode->getValue());
	if (not data::Notation::is_varname(utf_name))
		fTHROW(XMLException,node,"invalid pool name [%s]",
			(char const *)utf_name);
	name = utf_name;

	// das atoms-Attribut ist optional (default: 0)
	attrNode = static_cast< DOMAttr* >(
			nnm->getNamedItem(fml_atoms));
	if (attrNode != 0)
	{
		if (not XMLElement::parseInt(attrNode->getValue(),natoms))
		{
			U2A utf_natoms(attrNode->getValue());
			fTHROW(XMLException,"error parsing atoms atrribute: %s",
				(char const*)utf_natoms);
		}

		if (natoms < 0 or natoms > LIMIT_MAX_ATOMS)
			fTHROW(XMLException,node,
				"specified number of atoms (%i) out of range [0,%i].",
				natoms, LIMIT_MAX_ATOMS);
	}
	else natoms = 0;

	// das size-Attribut ist optional (default: 1.0)
	attrNode = static_cast< DOMAttr* >(nnm->getNamedItem(fml_size));
	if (attrNode != 0)
	{
		if (not XMLElement::parseDouble(attrNode->getValue(),poolsize))
		{
			U2A utf_size(attrNode->getValue());
			fTHROW(XMLException,node,"error parsing pool size value: %s\n",
				(char const*)utf_size);
		}

		if (poolsize < 0.)
		{
			U2A utf_size(attrNode->getValue());
			fTHROW(XMLException,node,"negative pool size values are not allowed: %s\n",
				(char const*)utf_size);
		}
	}
        
        // das cfg-Attribut ist erforderlich für multi-isotopic Tracer MFA (#OPTIONAL)
	attrNode = static_cast< DOMAttr* >(nnm->getNamedItem(fml_cfg));
	if (attrNode != 0)
	{
            U2A utf_cfg(attrNode->getValue());
            lib::RegEx rx_cfg("(([a-zA-Z]{2,})|([^CNOPH0-9][0-9]+))");
            if(rx_cfg.matches((char const*)utf_cfg))
            {
                   fTHROW(XMLException,node,"error parsing pool attribute [cfg= %s]. \nOnly the following isotopes are supported: C, N."
                           "\nA valid configuration can be speicified as follow: \"C1H2O3N4\" denotes "
                            "that one carbon, two hydrogen, three oxygen and four nitrogen are used by ILE.",
                            (char const *)utf_cfg);
            }
            cfg = utf_cfg;
        }
            
	// Unterhalb des pool-Elements können annotation-Elemente auftauchen.
	// Der FluxML-Parser wertet die Informationen dieser Elemente nicht
	// aus.
	
	// das Pool-Objekt erzeugen:
	pool_ = doc_->createPool(name, natoms, poolsize, cfg);
}

} // namespace flux::xml
} // namespace flux

