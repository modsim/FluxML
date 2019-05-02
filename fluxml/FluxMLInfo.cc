#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <ctime>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "Info.h"
#include "UnicodeTools.h"
#include "FluxMLUnicodeConstants.h"
#include "XMLElement.h"
#include "FluxMLInfo.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

namespace flux {
namespace xml {

void FluxMLInfo::parseInfo(DOMNode * node)
{
	DOMNode * child;
	std::string strain, modeler, comment, version, signature;
	struct tm date;

	memset(&date,0,sizeof(struct tm));

	if (doc_->getInfo() != 0)
		fTHROW(XMLException,node,"FluxMLInfo object already initialized!");

	if (not XMLElement::match(node,fml_info,fml_xmlns_uri))
		fTHROW(XMLException,node,"element node (info) expected.");
	
	child = XMLElement::skipJunkNodes(node->getFirstChild());
	do
	{
		// im info-Element ist alles optional
		if (child == 0) break;
                
                // Das Element name wurde gelassen aufgrund der Rückwertskompatibilität 
                if (XMLElement::match(child,fml_strain,fml_xmlns_uri) or XMLElement::match(child,fml_name,fml_xmlns_uri))
		{
			U2A utf_strain(parseOptionalTextNode(child));
			if (utf_strain)
				strain = utf_strain;
		}
                else if (XMLElement::match(child,fml_modeler,fml_xmlns_uri))
		{
			U2A utf_modeler(parseOptionalTextNode(child));
			if (utf_modeler)
				modeler = utf_modeler;
		}
		else if (XMLElement::match(child,fml_version,fml_xmlns_uri))
		{
			U2A utf_version(parseOptionalTextNode(child));
			if (utf_version)
				version = utf_version;
		}
		else if (XMLElement::match(child,fml_date,fml_xmlns_uri))
		{
			U2A utf_date(parseOptionalTextNode(child));
			if (utf_date)
			{
                            char * end_ptr = strptime(utf_date,"%Y-%m-%d %H:%M:%S", &date);
                            if (end_ptr == 0)
                                    fTHROW(XMLException,child,"error parsing timestamp (date) [%s].", (char const*)utf_date);
                            if (*end_ptr != '\0')
                                    fTHROW(XMLException,child,"trailing garbage in timestamp (date) [%s].", end_ptr);
			}
			else
                            fTHROW(XMLException,child,"error parsing timestamp (date): empty date element");
		}
		else if (XMLElement::match(child,fml_comment,fml_xmlns_uri))
		{
			U2A utf_comment(parseOptionalTextNode(child));
			if (utf_comment)
				comment = utf_comment;
		}
		else if (XMLElement::match(child,fml_signature,fml_xmlns_uri))
		{
			U2A utf_signature(parseOptionalTextNode(child));
			if (utf_signature)
				signature = utf_signature;
		} 
		else
			fTHROW(XMLException,child,"unknown element below info element.");
		
		child = XMLElement::nextNode(child);
	}
	while (child != 0);
	
	// das Info-Objekt wird erzeugt. Bitte beachten: der info_-Zeiger ist
	// eine Referenz auf den info_-Zeiger im FluxMLDocument-Objekt!
	doc_->createInfo(modeler,strain,version,mktime(&date),comment,signature);
}

XMLCh const * FluxMLInfo::parseOptionalTextNode(DOMNode * node)
{
	DOMNode * child = XMLElement::skipJunkNodes(node->getFirstChild());

	// weil das name-Element optional ist, darf child hier null sein:
	if (child == 0) return 0;
	if (child->getNodeType() != DOMNode::TEXT_NODE)
		fTHROW(XMLException,child,"#PCDATA element expected.");
	return static_cast< DOMText* >(child)->getData();
}

/**
 * Datums-Konsistenzprüfung.
 * Prüft, ob ein Datum (Zeichenkette) in der Form yyyy-mm-dd ist und ob
 * die Felder yyyy, mm, dd gültige Werte gemäß Kalendersystem enthalten.
 *
 * @param date Datums-Zeichenkette
 * @return true, falls das Datum gültig ist
 */
bool FluxMLInfo::checkDate(char const * date)
{
	int d, m, y, maxday;
	char * endp;

	errno = 0;
	y = strtol(date, &endp, 10);
	if (endp - date != 4) return false;
	errno = 0;
	m = strtol(date+5, &endp, 10);
	if (endp - (date+5) != 2) return false;
	errno = 0;
	d = strtol(date+8, &endp, 10);
	if (endp - (date+8) != 2) return false;
	
	if (!(y > 1900 && y < 2078)) return false;
	if (!(m >= 1 && m <= 12)) return false;

	switch (m)
	{
	case 1: case 3: case 5: case 7: case 8: case 10: case 12:
		maxday = 31;
		break;
	case 2:
		if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)
			maxday = 29; // Schaltjahr
		else
			maxday = 28;
		break;
	default:
		maxday = 30;
	}

	return d>=1 && d<=maxday;
}

} // namespace flux::xml
} // namespace flux

