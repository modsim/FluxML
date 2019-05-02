/* vim:set ft=cpp:syn on */
#include <cstdio>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include "Error.h"
#include "UnicodeTools.h"
#include "DOMWriterImplL3.h"

// Xerces-C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

namespace flux {
namespace xml {

DOMWriterImplL3::DOMWriterImplL3(
	char const * namespaceURI,	// 0 oder "http://www.w3.org/1998/Math/MathML"
	char const * qualifiedName,	// Name des Root-Elements (math,fluxml,cgraph)
	char const * publicId,		// 0 oder "-//W3C//DTD MathML 2.0//EN"
	char const * systemId		// DTD
	)
	: xml_string_(0), release_doc_(true)
{
	// "Core" funktioniert genauso gut wie "LS"
	static const XMLCh utf_LS[] = { chLatin_L, chLatin_S, chNull };
	impl_ = DOMImplementationRegistry::getDOMImplementation(utf_LS);

	if (impl_ == 0)
		fTHROW(XMLException,"failed to aquire DOM implementation \"LS\"");
	if (qualifiedName == 0)
		fTHROW(XMLException,"qualified name must not be NULL");

	A2U utf_nsURI(namespaceURI);
	A2U utf_qName(qualifiedName);
	A2U utf_pId(publicId);
	A2U utf_sId(systemId);

	doc_ = impl_->createDocument(
			utf_nsURI,
			utf_qName,
			systemId ? impl_->createDocumentType(utf_qName,utf_pId,utf_sId) : 0
			);
}

DOMWriterImplL3::~DOMWriterImplL3()
{
	if (release_doc_) doc_->release();
	if (xml_string_ != 0) delete[] xml_string_;
}

// eigentl. lokale Datei :-[
void DOMWriterImplL3::writeToURI(char const * uri) const
{
	try
	{
		XERCES_CPP_NAMESPACE_QUALIFIER DOMLSSerializer *
			theSerializer = ((DOMImplementationLS*)impl_)->createLSSerializer();
		XERCES_CPP_NAMESPACE_QUALIFIER DOMLSOutput *
			theOutput     = ((DOMImplementationLS*)impl_)->createLSOutput();

		bool gSplitCdataSections = false;
		bool gDiscardDefaultContent = true;
		bool gFormatPrettyPrint = f_pretty_print_;
		bool gWriteBOM = false;

		// Einstellungen am Serializer vornehmen
		XERCES_CPP_NAMESPACE_QUALIFIER DOMConfiguration *
			config = theSerializer->getDomConfig();

		config->setParameter(XMLUni::fgDOMWRTSplitCdataSections, gSplitCdataSections);
		config->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, gDiscardDefaultContent);
		config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, gFormatPrettyPrint);
		config->setParameter(XMLUni::fgDOMWRTBOM, gWriteBOM);
	
		A2U a2u(uri);
		XMLFormatTarget * myFormatTarget =
			new LocalFileFormatTarget(a2u);
		theOutput->setByteStream(myFormatTarget);
		theSerializer->write(doc_, theOutput);
		
		delete myFormatTarget;
		delete theSerializer;
	}
	catch (::XERCES_CPP_NAMESPACE_QUALIFIER XMLException const & e)
	{
		U2A u2a(e.getMessage());
		fTHROW(XMLException,"%s", (char const *)u2a);
	}
}

void DOMWriterImplL3::writeToStream(FILE * out_file) const
{
	if (out_file == 0)
		fTHROW(XMLException,"0-pointer is not a valid file output stream!");
	
	try
	{
		XERCES_CPP_NAMESPACE_QUALIFIER DOMLSSerializer *
			theSerializer = ((DOMImplementationLS*)impl_)->createLSSerializer();
		XERCES_CPP_NAMESPACE_QUALIFIER DOMLSOutput *
			theOutput     = ((DOMImplementationLS*)impl_)->createLSOutput();

		bool gSplitCdataSections = false;
		bool gDiscardDefaultContent = true;
		bool gFormatPrettyPrint = f_pretty_print_;
		bool gWriteBOM = false;

		// Einstellungen am Serializer vornehmen
		XERCES_CPP_NAMESPACE_QUALIFIER DOMConfiguration *
			config = theSerializer->getDomConfig();

		config->setParameter(XMLUni::fgDOMWRTSplitCdataSections, gSplitCdataSections);
		config->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, gDiscardDefaultContent);
		config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, gFormatPrettyPrint);
		config->setParameter(XMLUni::fgDOMWRTBOM, gWriteBOM);
		
		XMLFormatTarget * myFormatTarget =
			new MemBufFormatTarget();
		theOutput->setByteStream(myFormatTarget);
		theSerializer->write(doc_, theOutput);

		XMLByte const * buffer = static_cast<MemBufFormatTarget*>(myFormatTarget)->getRawBuffer();
		size_t buffer_length = static_cast<MemBufFormatTarget*>(myFormatTarget)->getLen();
		size_t nblocks;
		
		nblocks = fwrite(buffer, buffer_length, 1, out_file);

		if (nblocks != 1 || ferror(out_file))
		{
			clearerr(out_file);
			fTHROW(XMLException,"failed writing buffer containing XML data");
		}

		delete myFormatTarget;
		delete theSerializer;
		delete theOutput;
	}
	catch (::XERCES_CPP_NAMESPACE_QUALIFIER XMLException const & e)
	{
		U2A u2a(e.getMessage());
		fTHROW(XMLException,"%s", (char const *)u2a);
	}
}

char const * DOMWriterImplL3::writeToString(size_t & bufsize) const
{
	// falls xml_string_ schon belegt ist, wird es zunächst freigegeben
	if (xml_string_ != 0)
	{
		delete [] xml_string_;
		xml_string_ = 0;
	}

	try
	{
		XERCES_CPP_NAMESPACE_QUALIFIER DOMLSSerializer *
			theSerializer = ((DOMImplementationLS*)impl_)->createLSSerializer();
		XERCES_CPP_NAMESPACE_QUALIFIER DOMLSOutput *
			theOutput     = ((DOMImplementationLS*)impl_)->createLSOutput();

		bool gSplitCdataSections = false;
		bool gDiscardDefaultContent = true;
		bool gFormatPrettyPrint = f_pretty_print_;
		bool gWriteBOM = false;

		// Einstellungen am Serializer vornehmen
		XERCES_CPP_NAMESPACE_QUALIFIER DOMConfiguration *
			config = theSerializer->getDomConfig();

		config->setParameter(XMLUni::fgDOMWRTSplitCdataSections, gSplitCdataSections);
		config->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, gDiscardDefaultContent);
		config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, gFormatPrettyPrint);
		config->setParameter(XMLUni::fgDOMWRTBOM, gWriteBOM);
	
		XMLFormatTarget * myFormatTarget =
			new MemBufFormatTarget();
		theOutput->setByteStream(myFormatTarget);
		theSerializer->write(doc_, theOutput);

		XMLByte const * buffer = static_cast<MemBufFormatTarget*>(myFormatTarget)->getRawBuffer();
		size_t buffer_length = static_cast<MemBufFormatTarget*>(myFormatTarget)->getLen();

		xml_string_ = new char[buffer_length+1];
		memcpy(xml_string_, buffer, buffer_length);
		xml_string_[buffer_length] = '\0';
		bufsize = buffer_length; // OHNE Terminator!

		// buffer wird beim Zerstören von myFormatTarget zerstört
		delete myFormatTarget;
		delete theSerializer;
	}
	catch (::XERCES_CPP_NAMESPACE_QUALIFIER XMLException const & e)
	{
		U2A u2a(e.getMessage());
		fTHROW(XMLException,"%s", (char const *)u2a);
	}
	return xml_string_;
}

} // namespace flux::xml
} // namespace flux

