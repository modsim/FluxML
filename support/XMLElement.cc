#include <cstdlib>
#include <cerrno>
extern "C"
{
#include <limits.h>
}
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/XercesVersion.hpp>
#include "charptr_array.h"
#include "cstringtools.h"
#include "UnicodeTools.h"
#include "XMLException.h"
#include "XMLElement.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

XN DOMNode * XMLElement::nextNode(XN DOMNode * node)
{
	if (node == 0)
		return 0;
	node = node->getNextSibling();
	return skipJunkNodes(node);
}

XN DOMElement * XMLElement::nextElementNode(XN DOMNode * node)
{
	if (node == 0)
		return 0;
	node = node->getNextSibling();
	while (node != 0 && node->getNodeType() != XN DOMNode::ELEMENT_NODE)
		node = node->getNextSibling();
	return static_cast< XN DOMElement* >(node);
}

XN DOMNode * XMLElement::skipTextNodes(XN DOMNode * node)
{
	if (node == 0)
		return 0;
	do
	{
		node = skipJunkNodes(node);
		if (node != 0 && node->getNodeType() == XN DOMNode::TEXT_NODE)
			node = node->getNextSibling();
		else
			break;
	}
	while (true);
	return node;
}

XN DOMNode * XMLElement::skipJunkNodes(XN DOMNode * node)
{
	bool found = false;
	while (node != 0 && not found)
	{
		switch (node->getNodeType())
		{
		case XN DOMNode::ATTRIBUTE_NODE:
			// die Attribut-Knoten der Elemente werden nur dann
			// ausgewertet, wenn gezielt danach gesucht wird.
			break;
		case XN DOMNode::CDATA_SECTION_NODE:
			//throw Exception("illegal CDATA section found");
			break;
		case XN DOMNode::COMMENT_NODE:
			// Kommentare überlesen ...
			break;
		case XN DOMNode::DOCUMENT_FRAGMENT_NODE:
			//throw Exception("embedded document fragment unsupported");
			break;
		case XN DOMNode::DOCUMENT_TYPE_NODE:
			// einfache Regel: Das Dokument ist , oder es ist
			// kaputt...
			break;
		case XN DOMNode::ELEMENT_NODE:
			// ELEMENT_NODEs dürfen nicht überlesen werden ...
			found = true;
			break;
		case XN DOMNode::ENTITY_NODE:
			//throw Exception("entity nodes not supported");
			break;
		case XN DOMNode::NOTATION_NODE:
			// NOTATION_NODEs werden überlesen ...
			break;
		case XN DOMNode::TEXT_NODE:
			{
			if (static_cast< XN DOMText * >(node)->isIgnorableWhitespace())
				break;

			// das vorherige Verhalten:

			// TEXT_NODEs dürfen nicht überlesen werden ...
			// Whitespace wird überlesen (TODO: wirklich immer richtig?)
			U2A u2a(static_cast< XN DOMText * >(node)->getData());
			char * txt_cont = strdup_alloc(u2a);
			for (char * w=txt_cont; *w!='\0'; w++)
				if (*w < ' ') *w = ' ';
			strtrim_inplace(txt_cont);
			if (strlen(txt_cont))
				found = true; // TODO
			delete[] txt_cont;
			}
			break;
		case XN DOMNode::PROCESSING_INSTRUCTION_NODE:
			// Processing-Instructions werden überlesen ...
			// obwohl sie eigentlich nichts im Dokument verloren haben.
			break;
		default:
			// sollte nicht vorkommen ...
			//throw Exception("unknown node type");
			return 0;
		}
		if (not found)
			node = node->getNextSibling();
	}
	return node;
}

void XMLElement::dumpNode(XN DOMNode const * node)
{
	if (node == 0)
	{
		printf("(null node)\n");
		return;
	}

	U2A asc_nn(node->getNodeName());
	printf("%s: ", (char const *)asc_nn);
	
	switch (node->getNodeType())
	{
	case XN DOMNode::ATTRIBUTE_NODE:
		{
		U2A u2a(static_cast< XN DOMAttr const* >(node)->getValue());
		printf("(ATTRIBUTE_NODE) {%s}\n", (char const *)u2a);
		}
		break;
	case XN DOMNode::CDATA_SECTION_NODE:
		printf("(CDATA_SECTION_NODE)\n");
		break;
	case XN DOMNode::COMMENT_NODE:
		printf("(COMMENT_NODE)\n");
		break;
	case XN DOMNode::DOCUMENT_FRAGMENT_NODE:
		printf("(DOCUMENT_FRAGMENT_NODE)\n");
		break;
	case XN DOMNode::DOCUMENT_TYPE_NODE:
		printf("(DOCUMENT_TYPE_NODE)\n");
		break;
	case XN DOMNode::ELEMENT_NODE:
		printf("(ELEMENT_NODE)\n");
		break;
	case XN DOMNode::ENTITY_NODE:
		printf("(ENTITY_NODE)\n");
		break;
	case XN DOMNode::NOTATION_NODE:
		printf("(NOTATION_NODE)\n");
		break;
	case XN DOMNode::TEXT_NODE:
		{
		U2A u2a(static_cast< XN DOMText const * >(node)->getData());
		printf("(TEXT_NODE) {%s}\n", (char const *)u2a);
		}
		break;
	case XN DOMNode::PROCESSING_INSTRUCTION_NODE:
		printf("(PROCESSING_INSTRUCTION_NODE)\n");
		break;
#if XERCES_VERSION_MAJOR >= 3
	case XN DOMNode::ENTITY_REFERENCE_NODE:
		printf("(ENTITY_REFERENCE_NODE)\n");
		break;
	case XN DOMNode::DOCUMENT_NODE:
		printf("(DOCUMENT_NODE)\n");
		break;
#endif
	}
}

char const * XMLElement::nodeToString(XN DOMNode const * node)
{
	charptr_array str;
	if (node == 0)
		return strdup_alloc("(null)");

	U2A asc_nn(node->getNodeName());
	
	switch (node->getNodeType())
	{
	case XN DOMNode::ATTRIBUTE_NODE:
		{
		U2A u2a(static_cast< XN DOMAttr const * >(node)->getValue());
		str.add("#attr{%s=%s}",(char const *)asc_nn,(char const *)u2a);
		}
		break;
	case XN DOMNode::CDATA_SECTION_NODE:
		{
		U2A u2a(static_cast< XN DOMCharacterData const * >(node)->getData());
		str.add("#CDATA{%s}", (char const *)u2a);
		}
		break;
	case XN DOMNode::COMMENT_NODE:
		{
		U2A u2a(static_cast< XN DOMCharacterData const * >(node)->getData());
		str.add("#comment{%s}", (char const *)u2a);
		}
		break;
	case XN DOMNode::DOCUMENT_FRAGMENT_NODE:
		str.add("#fragment");
		break;
	case XN DOMNode::DOCUMENT_TYPE_NODE:
		str.add("#<!DOCTYPE>");
		break;
	case XN DOMNode::ELEMENT_NODE:
		{
		XN DOMNamedNodeMap * nnm = node->getAttributes();
		str.add("#<%s", (char const *)asc_nn);
		for (XMLSize_t i=0; i<nnm->getLength(); ++i)
		{
			XN DOMAttr const * attr
				= static_cast< XN DOMAttr const * >(nnm->item(i));
			U2A u2a_n(attr->getName());
			U2A u2a_v(attr->getValue());
			str.add(" %s=\"%s\"",(char const *)u2a_n,(char const *)u2a_v);
		}
		str.add("/>");
		}
		break;
	case XN DOMNode::ENTITY_NODE:
		str.add("#entity");
		break;
	case XN DOMNode::NOTATION_NODE:
		str.add("#notation");
		break;
	case XN DOMNode::TEXT_NODE:
		{
		U2A u2a(static_cast< XN DOMText const * >(node)->getData());
		str.add("#text{%s}", (char const *)u2a);
		}
		break;
	case XN DOMNode::PROCESSING_INSTRUCTION_NODE:
		str.add("#procinst");
		break;
#if XERCES_VERSION_MAJOR >= 3
	case XN DOMNode::ENTITY_REFERENCE_NODE:
		str.add("#entityref");
		break;
	case XN DOMNode::DOCUMENT_NODE:
		str.add("#docnode");
		break;
#endif
	}
	return strdup_alloc(str.concat());
}

bool XMLElement::match(
	XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node,
	XMLCh const * name,
	XMLCh const * xmlns_uri
	)
{
	if (node == 0 or node->getNodeType() != XN DOMNode::ELEMENT_NODE)
		return false;

	XMLCh const * nname = getName(node,xmlns_uri);

	if (nname == 0)
		return false;

	return XN XMLString::equals(nname,name);
}

bool XMLElement::parseDouble(XMLCh const * utf8str, double & value)
{
	char * end_ptr;
	U2A str(utf8str);
	strtrim_inplace(str);

	errno = 0;
	value = strtod(str,&end_ptr);
	if (end_ptr == str or *end_ptr != '\0' or errno == ERANGE)
	{
		value = 0.;
		return false;
	}
	return true;
}

bool XMLElement::parseLongInt(
	XMLCh const * utf8str,
	long int & value,
	unsigned short base
	)
{
	char * end_ptr;
	U2A str(utf8str);
	strtrim_inplace(str);

	errno = 0;
	value = strtol(str,&end_ptr,base);
	if (end_ptr == str or *end_ptr != '\0' or errno == ERANGE)
	{
		value = 0l;
		return false;
	}
	return true;
}

bool XMLElement::parseInt(
	XMLCh const * utf8str,
	int & value,
	unsigned short base
	)
{
	long int livalue;
	if (not parseLongInt(utf8str,livalue,base)
		or livalue > INT_MAX or livalue < INT_MIN)
	{
		value = 0;
		return false;
	}
	value = (int)livalue;
	return true;
}

XMLCh const * XMLElement::getName(
	XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node,
	XMLCh const * xmlns_uri
	)
{
	if (node == 0 or node->getNodeType() != XN DOMNode::ELEMENT_NODE)
		return 0;
	
	XMLCh const * ns_prefix = node->getPrefix();

	// lokalen Namen zurückgeben, falls möglich
	if (xmlns_uri!=0 and ns_prefix!=0)
	{
		XMLCh const * n_xmlns_uri = node->getNamespaceURI();
		if (XN XMLString::equals(n_xmlns_uri,xmlns_uri))
			return node->getLocalName();
	}
	return node->getNodeName();
}

} // namespace flux::xml
} // namespace flux

