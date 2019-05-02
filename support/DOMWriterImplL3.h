#ifndef MATHMLDOMWRITERIMPLXERCESDOM3_H
#define MATHMLDOMWRITERIMPLXERCESDOM3_H

#include <cstdio>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include "XMLException.h"
#include "DOMWriter.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/**
 * DOM-L3 Implementierung der DOMWriter-Schnittstelle
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class DOMWriterImplL3 : public DOMWriter
{
private:
	/** der DOM-Tree */
	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument * doc_;
	/** die DOM-Implementierung */
	XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementation * impl_;
	/** C-String-Repräsentation des XML-Dokuments */
	mutable char * xml_string_;
	/** doc_ im Destructor freigeben? */
	bool release_doc_;

public:
	DOMWriterImplL3(
		char const * namespaceURI,	// 0 oder "http://www.w3.org/1998/Math/MathML"
		char const * qualifiedName,	// Name des Root-Elements (math,fluxml,cgraph)
		char const * publicId,		// 0 oder "-//W3C//DTD MathML 2.0//EN"
		char const * systemId		// DTD
		);

	DOMWriterImplL3(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementation * impl,
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument * doc
		) : doc_(doc), impl_(impl), xml_string_(0), release_doc_(false) { }


	virtual ~DOMWriterImplL3();

	void writeToURI(char const * uri) const;

	void writeToStream(FILE * out_file) const;
	
	char const * writeToString(size_t & bufsize) const;

	XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *
		getDOMDocument() { return doc_; }

	XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementation *
		getDOMImplementation() { return impl_; }
}; // class xml::DOMWriterImplL3

} // namespace flux::xml
} // namespace flux

#endif

