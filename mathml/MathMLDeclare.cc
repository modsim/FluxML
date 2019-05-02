#include <string>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "MathMLDeclare.h"
#include "MathMLElement.h"
#include "XMLException.h"
#include "MathMLContentObject.h"
#include "MathMLExpression.h"
#include "MathMLMatrix.h"
#include "MathMLVector.h"
#include "MathMLLambdaExpression.h"
#include "MathMLUnicodeConstants.h"
#include "UnicodeTools.h"
#include "XMLElement.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

namespace flux {
namespace xml {

MathMLDeclare::MathMLDeclare(
	MathMLContentObject * def_value,
	std::string const & def_name
	) : def_name_(def_name), def_value_(def_value) {}

MathMLDeclare::~MathMLDeclare() { }

bool MathMLDeclare::isAnonymous() const
{
	return def_name_.size()==0;
}

MathMLContentObject * MathMLDeclare::parse(
	MathMLDocument * doc,
	DOMNode * node
	)
{
	DOMNode * child, *childchild;
	DOMNamedNodeMap * nnm = 0;
	DOMAttr * attrNode = 0;
	char * text;
	std::string name("");

	if (not XMLElement::match(node,mml_declare,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (declare) expected");

	// Falls ein type-Attribut gesetzt ist, wird untersucht, ob
	// der Typ überhaupt verarbeitet werden kann:
	nnm = node->getAttributes();
	if (nnm != 0)
		attrNode = (DOMAttr*)(nnm->getNamedItem(mml_type));
	if (attrNode != 0)
	{
		if (XMLString::equals(attrNode->getValue(), mml_vector))
			// Typ "vector" nicht unterstützt
			fTHROW(XMLException,node,
			"declaration of \"vector\"s not yet supported");
		/* [...] TODO [...] */
	}

	// Alle nicht-Element-Knoten überspringen
	child = node->getFirstChild();
	while (child != 0 && child->getNodeType() != DOMNode::ELEMENT_NODE)
		child = child->getNextSibling();

	// es muß jetzt ein Identifier folgen
	// den Identifier parsen
	if (XMLElement::match(child,mml_ci,mml_xmlns_uri))
	{
		childchild = child->getFirstChild();
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child,"text node (identifier) expected");
		text = XMLString::transcode(((DOMText*)childchild)->getData());
		strtrim_inplace(text);
		name = text;
		XMLString::release(&text);
	}
	else fTHROW(XMLException,node,"declaration needs identifier!");
	
	// alle nicht-Element-Knoten überspringen
	child = child->getNextSibling();
	while (child != 0 && child->getNodeType() != DOMNode::ELEMENT_NODE)
		child = child->getNextSibling();

	// es muß jetzt ein <apply> oder etwas Vergleichbares folgen, da die
	// Deklaration ansonsten nichts deklariert.
	if (child == 0)
		fTHROW(XMLException,node,"declaration does not declare anything");

	// den Wert der Definition parsen
	MathMLContentObject * co = 0;

	if (XMLElement::match(child,mml_apply,mml_xmlns_uri)
		or XMLElement::match(child,mml_ci,mml_xmlns_uri)
		or XMLElement::match(child,mml_cn,mml_xmlns_uri))
	{
		// einen Ausdruck parsen
		co = MathMLExpression::parse(doc,child);
	}
	else if (XMLElement::match(child,mml_matrix,mml_xmlns_uri))
	{
		// eine Matrix parsen
		co = MathMLMatrix::parse(doc,child);
	}
	else if (XMLElement::match(child,mml_vector,mml_xmlns_uri))
	{
		// einen Vektor parsen
		co = MathMLVector::parse(doc,child);
	}
	else if (XMLElement::match(child,mml_lambda,mml_xmlns_uri))
	{
		// einen Lambda-Ausdruck parsen
		co = MathMLLambdaExpression::parse(doc,child);
	}
	else if (XMLElement::match(child,mml_annotation,mml_xmlns_uri))
	{
		// es wurde ein zusätzlicher annotation-Abschnitt gefunden,
		// in dem Daten stehen, die nicht geparst werden - das kann
		// z.B. Maple oder LaTeX-Notation sein.

		//DOMNamedNodeMap * nnm = child->getAttributes();
		//DOMAttr * attrNode = (DOMAttr*)(nnm->getNamedItem(mml_encoding));
		
		// das encoding [attrNode->getValue()] wird nicht unterstützt!
		return 0;
	}
	else if (MathMLElement::isPresentationNode(child))
	{
		// Presentation-MathML wird nicht geparst:
		return 0;
	}
	else fTHROW(XMLException,"content type unsupported");
	
	// Deklaration erzeugen:
	return doc->createDeclare(co, name);
}

void MathMLDeclare::serializeContentMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	if (isAnonymous())
		def_value_->serializeContentMathML(doc, node);
	else
	{
		DOMElement * declare = doc->createElementNS(mml_xmlns_uri,mml_declare);
		node->appendChild(declare);
		DOMElement * ci = doc->createElementNS(mml_xmlns_uri,mml_ci);
		declare->appendChild(ci);
		A2U def_name_utf(def_name_);
		DOMText * name = doc->createTextNode(def_name_utf);
		ci->appendChild(name);
		def_value_->serializeContentMathML(doc, declare);
	}
}

void MathMLDeclare::serializePresentationMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	if (isAnonymous())
	{
		DOMElement * mrow = doc->createElementNS(mml_xmlns_uri,mml_mrow);
		node->appendChild(mrow);
		def_value_->serializePresentationMathML(doc, mrow);
	}
	else
	{
		DOMElement * mrow = doc->createElementNS(mml_xmlns_uri,mml_mrow);
		node->appendChild(mrow);
		DOMElement * mi = doc->createElementNS(mml_xmlns_uri,mml_mi);
		mrow->appendChild(mi);
		A2U def_name_utf(def_name_);
		DOMText * name = doc->createTextNode(def_name_utf);
		mi->appendChild(name);
		DOMElement * mo = doc->createElementNS(mml_xmlns_uri,mml_mo);
		A2U assign_utf(":=");
		DOMText * assign = doc->createTextNode(assign_utf);
		mo->appendChild(assign);
		mrow->appendChild(mo);
		DOMElement * mrowrow = doc->createElementNS(mml_xmlns_uri,mml_mrow);
		mrow->appendChild(mrowrow);
		def_value_->serializePresentationMathML(doc, mrowrow);
	}
}

std::string MathMLDeclare::toString() const
{
	std::string defstr;
	if (isAnonymous())
		defstr = "<anon>";
	else
		defstr = def_name_;
	defstr += " := " + def_value_->toString();
	return defstr;
}

std::string const & MathMLDeclare::getName() const
{
	return def_name_;
}

MathMLContentObject const * MathMLDeclare::getValue() const
{
	return def_value_;
}

} // namespace flux::xml
} // namespace flux

