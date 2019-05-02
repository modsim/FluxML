#include "Error.h"
#include "cstringtools.h"
#include "charptr_array.h"
#include "UnicodeTools.h"
#include "XMLElement.h"
#include "XMLException.h"

namespace flux {
namespace xml {

XMLException::XMLException(
	XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node,
	char const * fmt, ...
	) : xml_line_(-1), xml_column_(-1)
{
	int len;
	va_list ap;

	va_start(ap,fmt);
	len = vsnprintf(0,0,fmt,ap);
	va_end(ap);
	
	msg_ = new char[len+1];
	
	va_start(ap,fmt);
	vsnprintf(msg_,len+1,fmt,ap);
	va_end(ap);

	char const * path = get_node_path(node);
	char const * info = XMLElement::nodeToString(node);
	fWARNING("XML exception: %s", msg_);
	fWARNING("     location: %s", path);
	fWARNING(" node details: %s", info);
	delete[] info;
	delete[] path;
}

XMLException::XMLException(
	char const * fmt, ...
	) : xml_line_(-1), xml_column_(-1)
{
	int len;
	va_list ap;

	va_start(ap,fmt);
	len = vsnprintf(0,0,fmt,ap);
	va_end(ap);
	
	msg_ = new char[len+1];
	
	va_start(ap,fmt);
	vsnprintf(msg_,len+1,fmt,ap);
	va_end(ap);

	fWARNING("XML exception: %s", msg_);
}

XMLException::XMLException(
	XMLException const & copy
	) : xml_line_(copy.xml_line_), xml_column_(copy.xml_column_)
{
	msg_ = strdup_alloc(copy.msg_);
}

XMLException::~XMLException()
{
	delete[] msg_;
}

char const * XMLException::get_node_path(
	XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node
	)
{
	charptr_array P;

	while (node != 0)
	{
		U2A asc_nn(node->getNodeName());
		P.addFront(asc_nn);
		node = node->getParentNode();
	}
	return strdup_alloc(P.concat("/"));
}

} // namespace flux::xml
} // namespace flux

