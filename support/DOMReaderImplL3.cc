/* vim:set ft=cpp:syn on */
#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/StdInInputSource.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include "Error.h"
#include "readstream.h"
#include "UnicodeTools.h"
#include "DOMReaderImplL3.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

// Inkrement der Puffergröße beim Auslesen einer Datei
#define FREAD_BUFFER_SIZE	2048
#define ERROR_MAXCHARS		1024

namespace flux {
namespace xml {

DOMReaderImplL3::DOMReaderImplL3()
	: DOM_doc_(0)
{
	static const XMLCh utf_LS[] = { XN chLatin_L, XN chLatin_S, XN chNull };
	impl_ = XN DOMImplementationRegistry::getDOMImplementation(utf_LS);
	parser_ = ((XN DOMImplementationLS*)impl_)->createLSParser(
		XN DOMImplementationLS::MODE_SYNCHRONOUS, 0
		);

	XN DOMConfiguration * config = parser_->getDomConfig();
	config->setParameter(XN XMLUni::fgDOMNamespaces, true);
	config->setParameter(XN XMLUni::fgDOMValidate, true);
	config->setParameter(XN XMLUni::fgXercesSchema, true);
//	config->setParameter(XN XMLUni::fgXercesSchemaFullChecking, false);
	config->setParameter(XN XMLUni::fgDOMValidateIfSchema, true);
	config->setParameter(XN XMLUni::fgDOMDatatypeNormalization, true);
	config->setParameter(XN XMLUni::fgDOMErrorHandler, &dom_err_handler_);
	config->setParameter(XN XMLUni::fgDOMResourceResolver, &entity_resolver_);
}

DOMReaderImplL3::DOMReaderImplL3(
	XN DOMImplementation * impl
	) : impl_(impl), DOM_doc_(0)
{
	parser_ = ((XN DOMImplementationLS*)impl_)->createLSParser(
		XN DOMImplementationLS::MODE_SYNCHRONOUS, 0
		);

	XN DOMConfiguration * config = parser_->getDomConfig();
	config->setParameter(XN XMLUni::fgDOMNamespaces, true);
	config->setParameter(XN XMLUni::fgDOMValidate, true);
	config->setParameter(XN XMLUni::fgXercesSchema, true);
//	config->setParameter(XN XMLUni::fgXercesSchemaFullChecking, false);
	config->setParameter(XN XMLUni::fgDOMValidateIfSchema, true);
	config->setParameter(XN XMLUni::fgDOMDatatypeNormalization, true);
	config->setParameter(XN XMLUni::fgDOMErrorHandler, &dom_err_handler_);
	config->setParameter(XN XMLUni::fgDOMResourceResolver, &entity_resolver_);
}

DOMReaderImplL3::~DOMReaderImplL3()
{
	// den Parser und den DOM-Tree löschen
	parser_->release();
}

void DOMReaderImplL3::parseFromURI(
	char const * uri
	)
{
	parser_->resetDocumentPool();

	try
	{
		// die XML-Datei parsen ...
		DOM_doc_ = parser_->parseURI(uri);
	}
	catch (XN XMLException const & e)
	{
		U2A u2a(e.getMessage());
		fTHROW(XMLException,"%s", (char const *)u2a);
	}
	catch (XN DOMException const & e)
	{
		XMLCh errText[2048];
		errText[0] = XN chNull;
		XN DOMImplementation::loadDOMExceptionMsg(e.code, errText, 2047);
		U2A u2a(errText);
		fTHROW(XMLException,"%s", (char const *)u2a);
	}
	catch (...)
	{
		fTHROW(XMLException,"unexpected exception during parsing");
	}
	if (DOM_doc_ == 0)
		fTHROW(XMLException,"error downloading XML from URI [%s]", uri);
	if (DOM_doc_->getDocumentElement() == 0)
		fTHROW(XMLException,"downloaded XML not well-formed?!");
}

void DOMReaderImplL3::parseFromMemory(
	unsigned char const * buf,
	size_t len
	)
{
	static const char * fake_sys_id = "fake";
	
	try
	{
		XN Wrapper4InputSource is(
			new XN MemBufInputSource(
				(const XMLByte *)buf,
				len,
				fake_sys_id,
				false
				)
			);
		parseFromInputSourceWrapper(is);
	}
	catch (XN XMLException const & e)
	{
		U2A asc_msg(e.getMessage());
		fTHROW(XMLException,"%s", (char const*)asc_msg);
	}
	catch (...)
	{
		fTHROW(XMLException,"unexpected exception during parsing");
	}
}

void DOMReaderImplL3::parseFromStream(
	FILE * in_file
	)
{
	char * buffer = 0;
	size_t buffer_length = 0;

	if (in_file == 0)
		fTHROW(XMLException,"0-pointer is not a valid file input stream!");

	buffer = readstream(in_file, buffer_length);

	if (buffer == 0)
		fTHROW(XMLException,"error reading file descriptor");
	if (buffer_length == 0)
	{
		delete[] buffer;
		fTHROW(XMLException,"empty file");
	}

	// DOMInputSource erzeugen und den XML-Parser anwerfen
	try
	{
		XN Wrapper4InputSource is(
			new XN MemBufInputSource(
				reinterpret_cast< const XMLByte *>(buffer),
				buffer_length,
				"fake", false
				)
			);
		parseFromInputSourceWrapper(is);
	}
	catch (XN XMLException const & e)
	{
		delete[] buffer;
		U2A asc_msg(e.getMessage());
		fTHROW(XMLException,"%s", (char const*)asc_msg);
	}
	catch (XMLException const & e)
	{
		throw e;
	}
	catch (...)
	{
		delete[] buffer;
		fTHROW(XMLException,"unexpected exception during parsing");
	}
	delete[] buffer;
}

void DOMReaderImplL3::parseFromFile(
	char const * file_name
	)
{
	A2U utf_file_name(file_name);
	try
	{
		// die Wrapper4InputSource-Klasse möchte das übergebene InputSource-
		// Objekt selbst löschen!
		XN Wrapper4InputSource is(new XN LocalFileInputSource(utf_file_name));
		parseFromInputSourceWrapper(is);
	}
	catch (XN XMLException const & e)
	{
		U2A asc_msg(e.getMessage());
		fTHROW(XMLException,"%s", (char const*)asc_msg);
	}
	catch (XMLException & e)
	{
		throw e;
	}
	catch (...)
	{
		fTHROW(XMLException,"unexpected exception during parsing");
	}
}

void DOMReaderImplL3::parseFromStdIn()
{
	try
	{
		// die Wrapper4InputSource-Klasse möchte das übergebene InputSource-
		// Objekt selbst löschen!
		XN Wrapper4InputSource is(new XN StdInInputSource());
		parseFromInputSourceWrapper(is);
	}
	catch (XN XMLException const & e)
	{
		U2A asc_msg(e.getMessage());
		fTHROW(XMLException,"%s", (char const*)asc_msg);
	}
	catch (XMLException const & e)
	{
		throw e;
	}
	catch (...)
	{
		fTHROW(XMLException,"unexpected exception during parsing");
	}
}

bool DOMReaderImplL3::mapEntity(
	char const * from,
	char const * to
	)
{
	return entity_resolver_.map(from,to);
}

bool DOMReaderImplL3::mapEntity(
	XMLCh const * from,
	XMLCh const * to
	)
{
	return entity_resolver_.map(from,to);
}

void DOMReaderImplL3::parseFromInputSourceWrapper(
	XN Wrapper4InputSource & is
	)
{
	parser_->resetDocumentPool();
        

	try
	{
		// die XML-Datei parsen ...
		DOM_doc_ = parser_->parse(&is);
	}
	catch (XN XMLException const & e)
	{
		U2A u2a(e.getMessage());
		fTHROW(XMLException,"%s", (char const *)u2a);
	}
	catch (XN DOMException const & e)
	{
		XMLCh errText[2048];
		errText[0] = XN chNull;
		XN DOMImplementation::loadDOMExceptionMsg(e.code, errText, 2047);
		U2A u2a(errText);
		fTHROW(XMLException,"%s", (char const *)u2a);
	}
	catch (...)
	{
		fTHROW(XMLException,"unexpected exception during parsing");
	}

	if (DOM_doc_ == 0)
		fTHROW(XMLException,"error with XML input source");
}

void DOMReaderImplL3::setResolveXInclude(bool tf)
{
	f_resolve_xinclude_ = tf;
	if (f_resolve_xinclude_)
	{
		XN DOMConfiguration * config = parser_->getDomConfig();
		if(config->canSetParameter(XN XMLUni::fgXercesDoXInclude, true))
			config->setParameter(XN XMLUni::fgXercesDoXInclude, true);
		else
			f_resolve_xinclude_ = false;
	}
}

} // namespace flux::xml
} // namespace flux

