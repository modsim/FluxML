/* vim:set ft=cpp:syn on */
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include <xercesc/framework/URLInputSource.hpp>
#include <xercesc/util/XMLURL.hpp>
#include "Error.h"
#include "charptr_array.h"
#include "XMLException.h"
#include "UnicodeTools.h"
#include "EntityResolver.h"

XERCES_CPP_NAMESPACE_USE

size_t XMLCh_hashf(XMLChptr const & val)
{
	size_t i,a = 0;
	size_t len = sizeof(XMLCh) * XMLString::stringLen(val);
	char const * str = reinterpret_cast< char const * >(val);

	for (i=0; i<len; i++)
	{
		a += str[i]<<4;
		a += str[i]>>4;
		a *= 11;
	}
	return a;
}

namespace flux {
namespace xml {

EntityResolver::~EntityResolver()
{
	fhash_map< XMLChptr,XMLChptr,XMLCh_hashf,XMLChptr_eq >::iterator i;
	for (i=map_.begin(); i!=map_.end(); i++)
	{
		XMLString::release(const_cast< XMLCh** >(&(i->key)));
		XMLString::release(const_cast< XMLCh** >(&(i->value)));
	}
}

bool EntityResolver::map(
	XMLChptr const & from,
	XMLChptr const & to
	)
{
	if (not map_.exists(from))
	{
		XMLCh * fromcpy = XMLString::replicate(from);
		XMLCh * tofixed = new XMLCh[XMLString::stringLen(to)+10*sizeof(XMLCh)];
		XMLString::fixURI(to,tofixed);
		XMLCh * tocpy = XMLString::replicate(tofixed);
		delete[] tofixed;

		map_.insert(fromcpy,tocpy);
		return true;
	}
	return false;
}

bool EntityResolver::map(
	char const * from,
	char const * to
	)
{
	return map(A2U(from),A2U(to));
}

DOMLSInput * EntityResolver::resolveResource(
	const XMLCh * const resourceType,
	const XMLCh * const namespaceUri,
	const XMLCh * const publicId,
	const XMLCh * const systemId,
	const XMLCh * const baseURI
	)
{
	XMLCh * newBaseURI = new XMLCh[XMLString::stringLen(baseURI)+10*sizeof(XMLCh)];
	XMLString::fixURI(baseURI,newBaseURI);
	U2A a_newBaseURI(newBaseURI);
	XMLURL xmlurl(newBaseURI,systemId);
	delete[] newBaseURI;

	// 1. Matching mit baseURI+systemId
	XMLCh const ** result = map_.findPtr(xmlurl.getURLText());
	// 2. Zweite Chance nur mit systemId
	if (result == 0) result = map_.findPtr(systemId);
	
	if (result)
	{
		XMLURL redirectURL(*result);
		return new Wrapper4InputSource(
			new URLInputSource(redirectURL.getURLText())
			);
	}
	return 0;
}

} // namespace flux::xml
} // namespace flux

