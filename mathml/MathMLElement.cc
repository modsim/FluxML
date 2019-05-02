#include <cstring>
#include "Error.h"
#include "cstringtools.h"
#include "XMLException.h"
#include "UnicodeTools.h"
#include "MathMLElement.h"

/** alphabetisch sortierte Liste der Content-MathML-Elemente */
#define content_elements_LENGTH		144
static char content_elements_[][20] = {
	"abs", "and", "annotation", "annotation-xml", "apply", "approx",
	"arccos", "arccosh", "arccot", "arccoth", "arccsc", "arccsch",
	"arcsec", "arcsech", "arcsin", "arcsinh", "arctan", "arctanh", "arg",
	"bvar", "card", "cartesianproduct", "ceiling", "ci", "cn", "codomain",
	"complexes", "compose", "condition", "conjugate", "cos", "cosh",	// 32
	"cot", "coth", "csc", "csch", "csymbol", "curl", "declare",
	"degree", "determinant", "diff", "divergence", "divide", "domain",
	"domainofapplication", "emptyset", "eq", "equivalent", "eulergamma",
	"exists", "exp", "exponentiale", "factorial", "factorof", "false",
	"floor", "forall", "gcd", "geq", "grad", "gt", "ident", "image",	// 32
	"imaginary", "imaginaryi", "implies", "in", "infinity", "int",
	"integers", "intersect", "interval", "inverse", "lambda",
	"laplacian", "lcm", "leq", "limit", "list", "ln", "log", "lowlimit",
	"lt", "matrix", "matrixrow", "max", "mean", "median", "min",
	"minus", "mode", "moment", "momentabout", "naturalnumbers", "neq",	// 32
	"not", "notanumber", "notin", "notprsubset", "notsubset", "or",
	"otherwise", "outerproduct", "partialdiff", "pi", "piece",
	"piecewise", "plus", "power", "primes", "product", "prsubset",
	"quotient", "rationals", "real", "reals", "rem", "root",
	"scalarproduct", "sdev", "sec", "sech", "selector", "semantics",
	"sep", "set", "setdiff", "sin", "sinh", "subset", "sum", "tan",
	"tanh", "tendsto", "times", "transpose", "true", "union", "uplimit",
	"variance", "vector", "vectorproduct", "xor"				// 48
};

/** alphabetisch sortierte Liste der unären Operatoren/Funktionen */
#define unary_operators_LENGTH		49
static char unary_operators_[][12] = {
	"abs", "arccos", "arccosh", "arccot", "arccoth", "arccsc", "arccsch",
	"arcsec", "arcsech", "arcsin", "arcsinh", "arctan", "arctanh",
	"arg", "card", "ceiling", "codomain", "conjugate", "cos", "cosh",
	"cot", "coth", "csc", "csch", "curl", "determinant", "divergence",
	"domain", "exp", "factorial", "floor", "grad", "ident", "image",
	"imaginary", "inverse", "laplacian", "ln", "log", "minus", "not",
	"real", "sec", "sech", "sin", "sinh", "tan", "tanh", "transpose"
};


/** alphabetisch sortierte Liste der binären Operatoren/Funktionen */
#define binary_operators_LENGTH		13
static char binary_operators_[][14] = {
	"approx", "divide", "equivalent", "implies", "minus", "neq",
	"outerproduct", "power", "quotient", "rem", "scalarproduct",
	"setdiff", "vectorproduct"
};
    
/** alphabetisch sortierte Liste der n-ären Operatoren/Funktionen */
#define n_ary_operators_LENGTH		31
static char n_ary_operators_[][17] = {
	"and", "cartesianproduct", "compose", "diff", "eq", "exists", "forall",
	"gcd", "geq", "gt", "int", "intersect", "lcm", "leq", "lt", "max",
	"mean", "median", "min", "mode", "or", "partialdiff", "plus", "product",
	"sdev", "selector", "sum", "times", "union", "variance", "xor"
};

/** alphabetisch sortierte Liste der Presentation-MathML-Elemente */
#define presentation_elements_LENGTH	31
static char presentation_elements_[][14] = {
	"maction", "maligngroup", "malignmark", "menclose", "merror", "mfenced",
	"mfrac", "mglyph", "mi", "mlabeledtr", "mmultiscripts", "mn", "mo",
	"mover", "mpadded", "mphantom", "mroot", "mrow", "ms", "mspace",
	"msqrt", "mstyle", "msub", "msubsup", "msup", "mtable", "mtd",
	"mtext", "mtr", "munder", "munderover"					// 32
};

#define SEARCH_STRING(A,l,name)	\
	int mid,lo=0,hi=l;while ((mid=(lo+hi+1)/2)!=hi)\
	if (strcmp(name,A[mid])<0)hi=mid;else lo=mid;\
	return strcmp(A[lo], name)==0;

/*
static bool searchString(char *array, size_t alen, char const * name)
{
	int mid, lo = 0, hi = alen;
	while ((mid = (lo+hi+1)/2) != hi)
		if (strcmp(name, array[mid]) < 0)
			hi = mid;
		else
			lo = mid;
	return strcmp(array[lo], name) == 0;
}
*/

namespace flux {
namespace xml {

/**
 * Stellt fest, ob es sich bei einem angegebenen MathML-Element um
 * ein Content-MathML-Element handelt.
 *
 * @param name Name des Elements
 * @return true, falls das Element ein Content-MathML-Element ist
 */
bool MathMLElement::isContentElement(char const * name)
{
	SEARCH_STRING(content_elements_,content_elements_LENGTH,name);
}

/**
 * Stellt fest, ob es sich bei einem angegebenen MathML-Element um
 * ein Presentation-MathML-Element handelt.
 *
 * @param name Name des Elements
 * @return true, falls das Element ein Presentation-MathML-Element ist
 */
bool MathMLElement::isPresentationElement(char const * name)
{
	SEARCH_STRING(presentation_elements_,presentation_elements_LENGTH,name);
}

/**
 * Stellt fest, ob es sich bei einem angegebenen DOM-Knoten um
 * ein Content-MathML-Element handelt.
 *
 * @param node DOM-Knoten
 * @return true, falls das Element ein Content-MathML-Element ist
 */
bool MathMLElement::isContentNode(XN DOMNode * node)
{
	U2A u2a(node->getNodeName());
	return isContentElement(u2a);
}

/**
 * Stellt fest, ob es sich bei einem angegebenen DOM-Knoten um
 * ein Presentation-MathML-Element handelt.
 *
 * @param node DOM-Knoten
 * @return true, falls das Element ein Presentation-MathML-Element ist
 */    
bool MathMLElement::isPresentationNode(XN DOMNode * node)
{
	U2A u2a(node->getNodeName());
	return isPresentationElement(u2a);
}

/**
 * Stellt fest, ob es sich bei einem Operator um einen binären Operator
 * handelt.
 *
 * @param name Name es Operator-Elements
 * @return true, falls es sich um einen n-ären-Operator handelt
 */
bool MathMLElement::isBinaryOperator(char const * name)
{
	SEARCH_STRING(binary_operators_,binary_operators_LENGTH,name);
}

/**
 * Stellt fest, ob es sich bei einem Operator um einen unären Operator
 * handelt.
 *
 * @param name Name es Operator-Elements
 * @return true, falls es sich um einen n-ären-Operator handelt
 */
bool MathMLElement::isUnaryOperator(char const * name)
{
	SEARCH_STRING(unary_operators_,unary_operators_LENGTH,name);
}

/**
 * Stellt fest, ob es sich bei einem Operator um einen n-ären Operator
 * handelt.
 *
 * @param name Name es Operator-Elements
 * @return true, falls es sich um einen n-ären-Operator handelt
 */
bool MathMLElement::isNAryOperator(char const * name)
{
	SEARCH_STRING(n_ary_operators_,n_ary_operators_LENGTH,name);
}

/**
 * Stellt fest, ob ein Name einem Operator zugeordnet ist.
 *
 * @param name Name
 * @return true, falls name ein Operator ist
 */
bool MathMLElement::isOperator(char const * name)
{
	if (isBinaryOperator(name))
		return true;
	if (isUnaryOperator(name))
		return true;
	if (isNAryOperator(name))
		return true;
	return false;
}

XN DOMNode * MathMLElement::nextNode(XN DOMNode * node)
{
	if (node == 0)
		return 0;
	node = node->getNextSibling();
	return skipJunkNodes(node);
}

XN DOMNode * MathMLElement::nextElementNode(XN DOMNode * node)
{
	if (node == 0)
		return 0;
	node = node->getNextSibling();
	return skipTextNodes(node);
}

XN DOMNode * MathMLElement::skipTextNodes(XN DOMNode * node)
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

XN DOMNode * MathMLElement::skipJunkNodes(XN DOMNode * node)
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
			fTHROW(XMLException,"illegal CDATA section found");
		case XN DOMNode::COMMENT_NODE:
			// Kommentare überlesen ...
			break;
		case XN DOMNode::DOCUMENT_FRAGMENT_NODE:
			fTHROW(XMLException,"embedded document fragment unsupported");
			break;
		case XN DOMNode::DOCUMENT_TYPE_NODE:
			// einfache Regel: Das Dokument ist MathML, oder es ist
			// kaputt...
			break;
		case XN DOMNode::ELEMENT_NODE:
			// ELEMENT_NODEs dürfen nicht überlesen werden ...
			found = true;
			break;
		case XN DOMNode::ENTITY_NODE:
			// TODO ...
			fTHROW(XMLException,"entity nodes not supported for Content-MathML");
			break;
		case XN DOMNode::NOTATION_NODE:
			// NOTATION_NODEs werden überlesen ...
			break;
		case XN DOMNode::TEXT_NODE:
			{
			// TEXT_NODEs dürfen nicht überlesen werden ...
			// Whitespace wird überlesen
			U2A u2a(static_cast< XN DOMText * >(node)->getData());
			char * txt_cont = strdup_alloc(u2a);
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
			fTHROW(XMLException,"unknown node type");
		}
		if (not found)
			node = node->getNextSibling();
	}
	return node;
}

} // namespace flux::xml
} // namespace flux

