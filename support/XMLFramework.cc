#include <xercesc/util/PlatformUtils.hpp>
#include "Error.h"
#include "XMLFramework.h"
#include "XMLException.h"
#include "UnicodeTools.h"

namespace flux {
namespace xml {
namespace framework {

void initialize()
{
	try
	{
		XERCES_CPP_NAMESPACE_QUALIFIER
			XMLPlatformUtils::Initialize();
	}
	catch (XERCES_CPP_NAMESPACE_QUALIFIER XMLException const & e)
	{
		U2A asc_msg(e.getMessage());
		fTHROW(XMLException,"Xerces-C init.: %s",
			(char const*)asc_msg);
	}
}

void terminate()
{
	XERCES_CPP_NAMESPACE_QUALIFIER
		XMLPlatformUtils::Terminate();
}

} // namespace flux::xml::framework
} // namespace flux::xml
} // namespace flux

