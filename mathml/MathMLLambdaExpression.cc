#include <list>
#include "Error.h"
#include "MathMLUnicodeConstants.h"
#include "MathMLExpression.h"
#include "UnicodeTools.h"
#include "MathMLLambdaExpression.h"
#include "MathMLDocument.h"
#include "XMLElement.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

namespace flux {
namespace xml {

MathMLContentObject * MathMLLambdaExpression::parse(
	MathMLDocument * doc,
	DOMNode * node)
{
	DOMNode * child, * bvar, *bvarchild;
	std::list< std::string > lambda_vars;
	char * text;

	if (not XMLElement::match(node,mml_lambda,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (lambda) expected");

	child = node->getFirstChild();
	while (child != 0)
	{
		// nächsten Element-Eintrag suchen
		while (child != 0 && child->getNodeType() != DOMNode::ELEMENT_NODE)
			child = child->getNextSibling();

		if (child != 0)
		{
			// wird keine weitere gebundene Variable gefunden, so
			// wird als nächstes ein Operand (apply, etc.) erwartet
			if (not XMLElement::match(child,mml_bvar,mml_xmlns_uri))
			{
				if (XMLElement::match(child,mml_apply,mml_xmlns_uri))
					break;
				// es MUSS nach den geb. Variablen ein Operand kommen!
				fTHROW(XMLException,child,"expression expected after list of bound variables");
			}
			bvar = child->getFirstChild();
			while (bvar != 0 && bvar->getNodeType() != DOMNode::ELEMENT_NODE)
				bvar = bvar->getNextSibling();
			
			// leeres <bvar></bvar> ?
			if (bvar == 0)
				fTHROW(XMLException,child,"empty <bvar> element node?");
			
			if (XMLElement::match(bvar,mml_ci,mml_xmlns_uri))
			{
				// den Identifier aus "ci" parsen
				bvarchild = bvar->getFirstChild();
				if (bvarchild == 0 || bvarchild->getNodeType() != DOMNode::TEXT_NODE)
					fTHROW(XMLException,bvar,"text node (identifier) expected");
				text = XMLString::transcode(((DOMText*)bvarchild)->getData());
				strtrim_inplace(text);
				lambda_vars.push_back(text);
				XMLString::release(&text);
			}
			else
				fTHROW(XMLException,bvar,"identifier (ci) expected");
			
			// zur nächsten Variablen vorrücken
			child = child->getNextSibling();
		}
	}

	// nachdem die Variablenliste geparst wurde, wird nun der Ausdruck
	// selbst geparst:
	if (child == 0)
		// Variablenliste da, Ausdruck aber nicht!?
		fTHROW(XMLException,"found bound variables but no expression");

	return doc->createLambdaExpression(
		lambda_vars,
		static_cast< MathMLExpression* >(MathMLExpression::parse(doc,child))
		);
}

std::string MathMLLambdaExpression::toString() const
{
	std::string str = "lambda( (";
	std::list<std::string>::const_iterator iter = lambda_vars_.begin();

	while (iter != lambda_vars_.end())
	{
		std::string const & var = *(iter++);
		str += var;
		if (iter != lambda_vars_.end())
			str += ",";
	}
	str += ") , " + lambda_expr_->toString() + " )";
	return str;
}

void MathMLLambdaExpression::serializeContentMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	DOMElement * lambda = doc->createElementNS(mml_xmlns_uri,mml_lambda);
	node->appendChild(lambda);

	// Parameterliste
	// Iterator iter = lambda_vars_.iterator();
	std::list<std::string>::const_iterator iter = lambda_vars_.begin();
	while (iter != lambda_vars_.end())
	{
		DOMElement * bvar = doc->createElementNS(mml_xmlns_uri,mml_bvar);
		DOMElement * ci = doc->createElementNS(mml_xmlns_uri,mml_ci);
		A2U b_a2u(*(iter++));
		DOMText * value = doc->createTextNode(b_a2u);
		ci->appendChild(value);
		bvar->appendChild(ci);
		lambda->appendChild(bvar);
	}

	// arithmetischer Ausdruck
	lambda_expr_->serializeContentMathML(doc,lambda);
}

void MathMLLambdaExpression::serializePresentationMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	DOMElement * mfenced = doc->createElementNS(mml_xmlns_uri,mml_mfenced);
	node->appendChild(mfenced);

	// Parameterliste
	std::list<std::string>::const_iterator iter = lambda_vars_.begin();
	while (iter != lambda_vars_.end())
	{
		DOMElement * mi = doc->createElementNS(mml_xmlns_uri,mml_mi);
		A2U b_a2u(*(iter++));
		DOMText * value = doc->createTextNode(b_a2u);
		mi->appendChild(value);
		mfenced->appendChild(mi);
	}

	// welche Entity passt für "|->" und wird von Mozilla akzeptiert?
	DOMElement * mo = doc->createElementNS(mml_xmlns_uri,mml_mo);
	//DOMEntityReference * arrow = doc->createEntityReference(mml_rarr);
	DOMEntityReference * arrow = doc->createEntityReference(mml_RightTeeArrow);
	//DOMText * arrow = doc->createTextNode("->");
	mo->appendChild(arrow);
	node->appendChild(mo);

	// arithmetischer Ausdruck
	lambda_expr_->serializePresentationMathML(doc,node);
}

} // namespace flux::xml
} // namespace flux
