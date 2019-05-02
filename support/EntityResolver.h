#ifndef DTDRESOLVER_H
#define DTDRESOLVER_H

/* vim:set ft=cpp:syn on */

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMLSResourceResolver.hpp>
#include "fhash_map.h"
#include "XMLException.h"

typedef XMLCh const * XMLChptr;

size_t XMLCh_hashf(XMLChptr const & val);

struct XMLChptr_eq : public std::equal_to< XMLChptr >
{
	bool operator()(XMLChptr const & x, XMLChptr const & y) const
	{ return XERCES_CPP_NAMESPACE_QUALIFIER XMLString::equals(x,y); }
};

namespace flux {
namespace xml {

class EntityResolver
	: public XERCES_CPP_NAMESPACE_QUALIFIER DOMLSResourceResolver
{
private:
	fhash_map< XMLChptr,XMLChptr,XMLCh_hashf,XMLChptr_eq > map_;

public:
	EntityResolver() { }
	virtual ~EntityResolver();

public:
	bool map(
		XMLChptr const & from,
		XMLChptr const & to
		);

	bool map(
		char const * from,
		char const * to
		) ;

	XERCES_CPP_NAMESPACE_QUALIFIER DOMLSInput * resolveResource(
		const XMLCh * const resourceType,
		const XMLCh * const namespaceUri,
		const XMLCh * const publicId,
		const XMLCh * const systemId,
		const XMLCh * const baseURI
		);
};

} // namespace flux::xml
} // namespace flux

#endif

