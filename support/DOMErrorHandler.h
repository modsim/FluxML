#ifndef DOMERRORHANDLER_H
#define DOMERRORHANDLER_H

#include <xercesc/dom/DOMErrorHandler.hpp>

namespace flux {
namespace xml {

class DOMErrorHandler : public XERCES_CPP_NAMESPACE_QUALIFIER DOMErrorHandler
{
public:
	bool handleError(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMError const & domError
		);
};

} // namespace flux::xml
} // namespace flux

#endif

