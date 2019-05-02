/* vim:set ft=cpp:syn on */
#ifndef DOMREADERIMPLL3_H
#define DOMREADERIMPLL3_H

#include <cstdio>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include "XMLException.h"
#include "EntityResolver.h"
#include "DOMReader.h"
#include "DOMErrorHandler.h"

namespace flux {
namespace xml {

/**
 * Xerces L3-DOM-Implementierung der DOMReader-Schnittstelle
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class DOMReaderImplL3 : public DOMReader
{
private:
	/** Xerces L3-DOM-Implementierung */
	XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementation * impl_;

	/** Der Xerces-C++ DOM-Parser */
	XERCES_CPP_NAMESPACE_QUALIFIER DOMLSParser * parser_;

	/** Das DOM-Dokument */
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument * DOM_doc_;

	/** Abbildung der Dokument-DTD auf eine System-DTD */
	EntityResolver entity_resolver_;

	DOMErrorHandler dom_err_handler_;

public:
	DOMReaderImplL3();

	DOMReaderImplL3(XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementation * impl);

	virtual ~DOMReaderImplL3();

	void parseFromURI(char const * uri);

	void parseFromMemory(
		unsigned char const * buf,
		size_t len
		);
    
	void parseFromStream(FILE * in_file);

	void parseFromFile(char const * file_name);

	void parseFromStdIn();

	inline XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *
		getDOMDocument() { return DOM_doc_; }
	
	inline XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementation *
		getDOMImplementation() { return impl_; }

	bool mapEntity(
		char const * from,
		char const * to
		);
	
	bool mapEntity(
		XMLCh const * from,
		XMLCh const * to
		);

private:
	/**
	 * Ein generischer Wrapper für das Parsen von einer
	 * DOMInputSource.
	 *
	 * @param is ein Objekt der Klasse DOMInputSource
	 */
	void parseFromInputSourceWrapper(
		XERCES_CPP_NAMESPACE_QUALIFIER Wrapper4InputSource & is);


	/**
	 * Schaltet die XInclude-Auflösung an/ab
	 *
	 * @param tf true, falls XInclude aufgelöst werden soll
	 */
	virtual void setResolveXInclude(bool tf);
};

} // namespace flux::xml
} // namespace flux

#endif

