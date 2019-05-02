#ifndef DOMREADER_H
#define DOMREADER_H

#include <xercesc/dom/DOM.hpp>
#include <cstdio>
#include <string>
#include "XMLException.h"
#include "UnicodeTools.h"

namespace flux {
namespace xml {

/**
 * Schnittstellen-Klasse zum Ansteuern des XML-Parsers. Der Constructor nimmt
 * die Initialisierung des XML-DOM-Parsers vor.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class DOMReader
{
protected:
	/** Filtern/Auflösen von XInclude-Anweisungen */
	bool f_resolve_xinclude_;

public:
	/**
	 * Constructor.
	 */
	DOMReader() : f_resolve_xinclude_(false) { }

	/**
	 * Virtueller Destructor
	 */
	virtual ~DOMReader() { }
	
	/**
	 * Parsen eines FluxML-Dokuments hinter einer angegebenen URI
	 *
	 * @param uri URI des FluxML-Dokuments
	 */
	virtual void parseFromURI(char const * uri) = 0;

	/**
	 * Parsen eines FluxML-Dokuments aus einem String.
	 *
	 * @param fluxml ein String mit Content-FluxML
	 */
	virtual void parseFromMemory(
		unsigned char const * buf,
		size_t len
		) = 0;

	/**
	 * Liest ein FluxML-Dokument von einem Stream (Details der C++-Impl noch
	 * festzulegen)
	 *
	 * @param in_file Stream, von dem das FluxML-Dokument gelesen wird
	 */
	virtual void parseFromStream(FILE * in_file) = 0;

	/**
	 * Liest ein FluxML-Dokument aus der Standard-Eingabe
	 */
	virtual void parseFromStdIn() = 0;
 
	/**
	 * Liest ein FluxML-Dokument aus einer lokalen Datei.
	 * Diese Methode gibt es in der Java-Implementierung nicht.
	 *
	 * @param file_name Dateiname mit vollständigem Pfad
	 */
	virtual void parseFromFile(char const * file_name) = 0;

	/**
	 * Rückgabe des FluxML-Dokuments
	 *
	 * @return geparstes FluxML-Dokument
	 */
	virtual XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *
		getDOMDocument() = 0;

	/**
	 * Rückgabe des DOMImplementation-Objekts.
	 *
	 * @return von DOMReader verwendetes DOMImplementation-Objekt
	 */
	virtual XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementation *
		getDOMImplementation() = 0;

	/**
	 * Abbildung einer System-Id auf eine andere System-Id.
	 *
	 * @param from Quell-Resource
	 * @param to Ziel-Resource
	 */
	virtual bool mapEntity(
		char const * from,
		char const * to
		) = 0;
	
	/**
	 * Abbildung einer System-Id auf eine andere System-Id.
	 *
	 * @param from Quell-Resource
	 * @param to Ziel-Resource
	 */
	virtual bool mapEntity(
		XMLCh const * from,
		XMLCh const * to
		) = 0;
	
	/**
	 * Optionale Auflösung der XInclude Anweisungen
	 *
	 * @param tf true, falls XInclude-Anweisungen aufgelöst werden sollen
	 */
	virtual void setResolveXInclude(bool tf) { f_resolve_xinclude_ = tf; }
};

} // namespace flux::xml
} // namespace flux

#endif

