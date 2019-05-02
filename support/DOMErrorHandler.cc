#include <cstdio>
#include <xercesc/util/XMLString.hpp>
#include "Error.h"
#include "UnicodeTools.h"
#include "XMLException.h"
#include "DOMErrorHandler.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

bool DOMErrorHandler::handleError(XN DOMError const & domError)
{
	U2A asc_msg(domError.getMessage());
	U2A asc_uri(domError.getLocation()->getURI());

	switch (domError.getSeverity())
	{
	case XN DOMError::DOMError::DOM_SEVERITY_WARNING:
		fWARNING("XML parser (warning): %s in %s; row: %i, column: %i",
				(char const *)asc_msg,
				(char const *)asc_uri,
				int(domError.getLocation()->getLineNumber()),
				int(domError.getLocation()->getColumnNumber())
				);
		break;
	case XN DOMError::DOMError::DOM_SEVERITY_ERROR:
		{
		XMLException E(
#if XERCES_VERSION_MAJOR >= 3
				domError.getLocation()->getRelatedNode(),
#endif
				"XML parser (error): %s in %s; row: %i, column: %i",
				(char const *)asc_msg,
				(char const *)asc_uri,
				int(domError.getLocation()->getLineNumber()),
				int(domError.getLocation()->getColumnNumber())
				);
		E.setXMLLine(domError.getLocation()->getLineNumber());
		E.setXMLColumn(domError.getLocation()->getColumnNumber());
		throw E;
		}
	case XN DOMError::DOMError::DOM_SEVERITY_FATAL_ERROR:
		{
		XMLException E(
#if XERCES_VERSION_MAJOR >= 3
				domError.getLocation()->getRelatedNode(),
#endif
				"XML parser (fatal error): %s in %s; row: %i, column: %i",
				(char const *)asc_msg,
				(char const *)asc_uri,
				int(domError.getLocation()->getLineNumber()),
				int(domError.getLocation()->getColumnNumber())
				);
		E.setXMLLine(domError.getLocation()->getLineNumber());
		E.setXMLColumn(domError.getLocation()->getColumnNumber());
		throw E;
		}
	}
	return true;
}

} // namespace flux::xml
} // namespace flux

