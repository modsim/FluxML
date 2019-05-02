#ifndef FLUXMLINFO_H
#define FLUXMLINFO_H

#include <xercesc/dom/DOM.hpp>
#include "Info.h"
#include "FluxMLContentObject.h"
#include "XMLException.h"
#include "FluxMLDocument.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zum Parsen / Serialisieren eines info-Elements eines FluxML-
 * Dokuments.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */    
class FluxMLInfo : public FluxMLContentObject
{
public:	
	/**
	 * Constructor.
	 * Parst ein info-Element aus einem DOM-Tree eines FluxML-Dokuments.
	 *
	 * @param node Knoten des info-Elements
	 */
	inline FluxMLInfo(FluxMLDocument * doc, XN DOMNode * node)
		: FluxMLContentObject(doc)
	{
		parseInfo(node);
	}

	/**
	 * Constructor.
	 * Erzeugt ein FluxMLInfo-Objekt aus einem Info-Objekt
	 *
	 * @param doc Zeiger auf das FluxML-Dokument mit Info-Daten
	 */
	inline FluxMLInfo(FluxMLDocument * doc)	: FluxMLContentObject(doc) { }

	/**
	 * Destructor.
	 */
	inline virtual ~FluxMLInfo() { }
	
	inline int getType() { return FluxMLContentObject::CO_INFO; }
	
private:
	/**
	 * Parst ein info-Element aus einem DOM-Tree eines FluxML-Dokuments.
	 *
	 * @param node Knoten des info-Elements
	 */
	void parseInfo(XN DOMNode * node);
	
	/**
	 * Parsen eines Text-Node.
	 *
	 * @param node Knoten, dessen Nachfolger ein Text-Knoten ist
	 */
	XMLCh const * parseOptionalTextNode(XN DOMNode * node);

	/**
	 * Datums-Konsistenzprüfung.
	 * Prüft, ob ein Datum (Zeichenkette) in der Form yyyy-mm-dd ist und ob
	 * die Felder yyyy, mm, dd gültige Werte gemäß Kalendersystem enthalten.
	 *
	 * @param date Datums-Zeichenkette
	 * @return true, falls das Datum gültig ist
	 */
	bool checkDate(char const * date);

}; // class FluxMLInfo

} // namespace flux::xml
} // namespace flux

#endif
