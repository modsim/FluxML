#include <list>
#include <string>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "ExprTree.h"
#include "fRegEx.h"
#include "DOMWriter.h"
#include "DOMWriterImpl.h"
#include "MathMLDeclare.h"
#include "MathMLExpression.h"
#include "MathMLUnicodeConstants.h"
#include "MathMLElement.h"
#include "MathMLDocument.h"
#include "XMLElement.h"

using namespace std;

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

using namespace flux::symb;

namespace flux {
namespace xml {

MathMLDocument::MathMLDocument(
	XN DOMDocument * doc
	)
	: writer_(0)
{
	// MathML-Dokumentstruktur in Definitionsliste umsetzen:
	parseMath(doc->getDocumentElement());
}

MathMLDocument::MathMLDocument(
	XN DOMNode * math
	)
	: writer_(0)
{
	// MathML-Dokumentstruktur in Definitionsliste umsetzen:
	parseMath(math);
}

MathMLDocument::MathMLDocument(): writer_(0) { }

MathMLDocument::~MathMLDocument()
{
	std::list< MathMLDeclare* >::iterator i;
	std::list< MathMLContentObject* >::iterator j;
	
	for (i=def_list_.begin(); i!=def_list_.end(); i++)
		delete *i;
	// Dank virtueller Destruktoren können die Content-Objekte ohne
	// type-cast gelöscht werden:
	for (j=cont_list_.begin(); j!=cont_list_.end(); j++)
		delete *j;

	if (writer_) delete writer_;
}

MathMLExpression * MathMLDocument::createExpression(
	ExprTree const * expr
	)
{
	MathMLExpression * E = new MathMLExpression(expr);
	cont_list_.push_back(E);
	return E;
}

MathMLExpression * MathMLDocument::createExpression( // Kopie
	MathMLExpression const & copy
	)
{
	MathMLExpression * E = new MathMLExpression(copy);
	cont_list_.push_back(E);
	return E;
}

MathMLMatrix * MathMLDocument::createMatrix(
	int rows,
	int cols
	)
{
	MathMLMatrix * M = new MathMLMatrix(rows,cols);
	cont_list_.push_back(M);
	return M;
}

MathMLMatrix * MathMLDocument::createMatrix(
	MathMLMatrix const & copy
	)
{
	MathMLMatrix * M = new MathMLMatrix(copy);
	cont_list_.push_back(M);
	return M;
}

MathMLVector * MathMLDocument::createVector(
	int dim
	)
{
	MathMLVector * V = new MathMLVector(dim);
	cont_list_.push_back(V);
	return V;
}

MathMLVector * MathMLDocument::createVector(
	MathMLVector const & copy
	)
{
	MathMLVector * V = new MathMLVector(copy);
	cont_list_.push_back(V);
	return V;
}

MathMLLambdaExpression * MathMLDocument::createLambdaExpression(
	std::list< std::string > const & vars,
	MathMLExpression * expr
	)
{
	MathMLLambdaExpression * L = new MathMLLambdaExpression(vars,expr);
	cont_list_.push_back(L);
	return L;
}

MathMLLambdaExpression * MathMLDocument::createLambdaExpression(
	MathMLLambdaExpression const & copy
	)
{
	MathMLExpression * E = createExpression(copy.getLambdaExpr()->get());
	MathMLLambdaExpression * L = new MathMLLambdaExpression(
		copy.getVarList(),E);
	cont_list_.push_back(L);
	return L;
}

MathMLDeclare * MathMLDocument::createDeclare(
	MathMLContentObject * cont_obj,
	std::string const & name
	)
{
	MathMLDeclare * D = new MathMLDeclare(cont_obj,name);
	def_list_.push_back(D);
	return D;
}

MathMLDeclare * MathMLDocument::createDeclare(
	MathMLDeclare const & copy
	)
{
	MathMLContentObject * co = 0;
	switch (copy.getValue()->getType())
	{
	case co_mathml_doc: return 0; // FEHLER; TODO!
	case co_expression:
		co = createExpression( *(static_cast< MathMLExpression const* >(copy.getValue())) );
		break;
	case co_matrix:
		co = createMatrix( *(static_cast< MathMLMatrix const* >(copy.getValue())) );
		break;
	case co_vector:
		co = createVector( *(static_cast< MathMLVector const* >(copy.getValue())) );
		break;
	case co_lambda:
		co = createLambdaExpression( *(static_cast< MathMLLambdaExpression const* >(copy.getValue())) );
		break;
	case co_declare: return 0; // FEHLER; TODO!
	}
	MathMLDeclare * D = new MathMLDeclare(co,copy.getName());
	def_list_.push_back(D);
	return D;
}

/**
 * Gibt alle Definitionen mit einem bestimmten Typ in Form einer
 * Liste zurück.
 *
 * @param type Content-Typ
 * @return Liste der Deftinionen mit dem Typ type
 */
std::list< MathMLDeclare const * > MathMLDocument::getDefinitionsByType(
	MathMLContentObject::Type type
	) const
{
	std::list< MathMLDeclare const * > result_list;
	std::list< MathMLDeclare * >::const_iterator iter
		= def_list_.begin();
	while (iter != def_list_.end())
	{
		MathMLDeclare const * mml_def = *(iter++);
		if (mml_def->getValue()->getType() == type)
			result_list.push_back(mml_def);
	}
	return result_list;
}

/**
 * Gibt alle Definitionen zurück, deren Name auf einen regulären Ausdruck
 * passt.
 *
 * @param regexp Regulärer Ausdruck
 * @return alle Definitionen, deren name auf regexp passt
 */
std::list< MathMLDeclare const * > MathMLDocument::getDefinitionsByRegExp(
	std::string const & regexp
	) const
{
	std::list< MathMLDeclare const * > result_list;
	try
	{
		lib::RegEx rx(regexp.c_str());
		std::list< MathMLDeclare * >::const_iterator iter
			= def_list_.begin();

		while (iter != def_list_.end())
		{
			MathMLDeclare const * mml_def = *(iter++);
			if (rx.matches(mml_def->getName().c_str()))
				result_list.push_back(mml_def);
		}
	}
	catch (lib::RegExException &)
	{
		fTHROW(XMLException,"parse error in regular expression");
	}
	return result_list;
}

/**
 * Gibt alle Definitionen zurück, deren Name auf einen regulären
 * Ausdruck passt und die einen bestimmten Typ (MathMLContentObject)
 * haben.
 *
 * @param regexp Regulärer Ausdruck für Definitionsname
 * @param type MathMLContentObject-Typ
 */
std::list< MathMLDeclare const * > MathMLDocument::getDefinitionsByRegExpType(
	std::string const & regexp,
	MathMLContentObject::Type type
	)
{
	std::list< MathMLDeclare const * > result_list;
	try
	{
		lib::RegEx rx(regexp.c_str());
		std::list< MathMLDeclare * >::const_iterator iter
			= def_list_.begin();
		while (iter != def_list_.end())
		{
			MathMLDeclare const * mml_def = *(iter++);
			if (rx.matches(mml_def->getName().c_str())
				&& mml_def->getValue()->getType() == type)
				result_list.push_back(mml_def);
		}
	}
	catch (lib::RegExException &)
	{
		fTHROW(XMLException,"parse error in regular expression");
	}
	return result_list;
}

void MathMLDocument::serializeToDom(
	DOMDocument * doc,
	MathMLType type
	) const
{
	switch (type)
	{
	case mml_content:
		serializeContentMathML(doc,doc->getDocumentElement());
		break;
	case mml_presentation:
		serializePresentationMathML(doc,doc->getDocumentElement());
		break;
	case mml_content_with_presentation:
		serializeContentWithPresentationMathML(
				doc,doc->getDocumentElement());
		break;
	case mml_presentation_with_content:
		serializePresentationWithContentMathML(
				doc,doc->getDocumentElement());
		break;
	}
}

/**
 * Parst das math-Element des MathML-Dokuments (das Wurzel-Element)
 *
 * @param node Knoten mit den Wurzel-Element math
 */
void MathMLDocument::parseMath(
	DOMNode * node
	)
{
	if (not XMLElement::match(node,mml_math,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (math) expected");
	DOMNode * child = node->getFirstChild();
	while (child != 0 && child->getNodeType() != DOMNode::ELEMENT_NODE)
		child = child->getNextSibling();
	if (XMLElement::match(child,mml_semantics,mml_xmlns_uri))
		parseSemantics(child);
	else
	{
		// versuche, die Content-Objekte außerhalb eines <semantics>-
		// Elements in die Definitionsliste einzutragen
		while (child != 0)
		{
			// Content-Element parsen
			parseContent(child);
			// zum nächsten Element navigieren
			do
				child = child->getNextSibling();
			while (child != 0
				&& child->getNodeType() != DOMNode::ELEMENT_NODE);
		}
	}
}

/**
 * Parst Content-MathML-Elemente, die an der Wurzel des Content-MathML,
 * direkt hinter math oder annotated-xml, stehen können
 *
 * @param node Wurzel-Knoten des Content-MathML
 */
void MathMLDocument::parseContent(
	DOMNode * node
	)
{
	if (XMLElement::match(node,mml_declare,mml_xmlns_uri))
	{
		// eine Deklaration parsen
		MathMLDeclare::parse(this,node);
		return;
	}

	// je nach Content den passenden Content-Parser starten
	// und eine anonymous-Deklaration anlegen:
	MathMLContentObject * co = 0;
	
	if (XMLElement::match(node,mml_apply,mml_xmlns_uri)
		or XMLElement::match(node,mml_ci,mml_xmlns_uri)
		or XMLElement::match(node,mml_cn,mml_xmlns_uri))
	{
		// einen Ausdruck parsen
		co = MathMLExpression::parse(this,node);
	}
	else if (XMLElement::match(node,mml_matrix,mml_xmlns_uri))
	{
		// eine Matrix parsen
		co = MathMLMatrix::parse(this,node);
	}
	else if (XMLElement::match(node,mml_vector,mml_xmlns_uri))
	{
		// einen Vektor parsen
		co = MathMLVector::parse(this,node);
	}
	else if (XMLElement::match(node,mml_lambda,mml_xmlns_uri))
	{
		// einen Lambda-Ausdruck parsen
		co = MathMLLambdaExpression::parse(this,node);
	}
	else if (XMLElement::match(node,mml_annotation,mml_xmlns_uri))
	{
		// es wurde ein zusätzlicher annotation-Abschnitt gefunden,
		// in dem Daten stehen, die nicht geparst werden - das kann
		// z.B. Maple oder LaTeX-Notation sein.

		//DOMNamedNodeMap * nnm = node->getAttributes();
		//DOMAttr * attrNode = (DOMAttr*)(nnm->getNamedItem(mml_encoding));
		
		// das encoding [attrNode->getValue()] wird nicht unterstützt!
		return;
	}
	else if (MathMLElement::isPresentationNode(node))
	{
		// Presentation-MathML wird nicht geparst:
		return;
	}
	else fTHROW(XMLException,node,"content type unsupported");
	
	if (co) createDeclare(co);
}

/**
 * Parst das semantics-Element von MathML, in das Presentation-MathML
 * und Content-MathML eingebettet sein können.
 *
 * @param node Element-Knoten des semantics-Elements
 */
void MathMLDocument::parseSemantics(
	DOMNode * node
	)
{
	DOMNode * child, * grandchild;

	if (not XMLElement::match(node,mml_semantics,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (semantics) expected");

	child = node->getFirstChild();

	while (child != 0)
	{
		// Navigiere zum ersten Knoten, der Element-Knoten ist und kein
		// Presentation-MathML-Knoten ist (also der Knoten eines
		// Content-MathML-Elements).
		if (child->getNodeType() != DOMNode::ELEMENT_NODE
			|| MathMLElement::isPresentationNode(child))
		{
			child = child->getNextSibling();
			continue;
		}

		if (not MathMLElement::isContentNode(child))
			fTHROW(XMLException,node,"Content-MathML node expected");

		// bei child handelt es sich mit Sicherheit um einen
		// Content-MathML-Node
		if (XMLElement::match(child,mml_annotation_xml,mml_xmlns_uri))
		{
			DOMNamedNodeMap * nnm = child->getAttributes();
			DOMAttr * attrNode = (DOMAttr *)(nnm->getNamedItem(mml_encoding));

			// falls es kein encoding-Attribut gibt. Alternativ könnte man
			// hier auch eine Exception werfen -- die Strategie ist aber,
			// das zu parsen, was man versteht und nicht das zu bemängeln,
			// was man nicht versteht ;-)
			if (attrNode == 0)
			{
				child = child->getNextSibling();
				continue;
			}

			if (XMLString::equals(attrNode->getValue(),mml_MathML_Content))
			{
				grandchild = child->getFirstChild();
				while (grandchild != 0)
				{
					// zum nächsten Element-Knoten navigieren:
					while (grandchild != 0
						&& grandchild->getNodeType() != DOMNode::ELEMENT_NODE)
						grandchild = grandchild->getNextSibling();

					if (grandchild != 0)
					{
						// Content-Element parsen
						parseContent(grandchild);
						grandchild = grandchild->getNextSibling();
					}
				} // while
			} // if (MathML-Content)

			// Der Inhalt des <annotation-xml>-Elements wurde geparst.
			// Auf child-Ebene geht es jetzt zum nächsten Element
			child = child->getNextSibling();
			continue;
		} // if (annotation-xml)
		
		// child ist jetzt immer noch ein Content-MathML-Element und
		// wird verarbeitet:
		parseContent(child);
		child = child->getNextSibling();
	}
}

/**
 * Schreibt die Definitionen aus der Definitionsliste in den
 * DOM-Tree des MathML-Dokuments.
 *
 * @param node Wurzel-Element (math) des MathML-Dokuments
 */
void MathMLDocument::serializeContentMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	std::list< MathMLDeclare * >::const_iterator iter = def_list_.begin();
	while (iter != def_list_.end())
	{
		MathMLDeclare * mml_def = *(iter++);
		mml_def->serializeContentMathML(doc, node);
	}
}

/**
 * Schreibt die Definitionen aus der Definitionsliste in den
 * DOM-Tree des Presentation-MathML-Dokuments.
 * 
 * @param node Wurzel-Element (math) des MathML-Dokuments
 */
void MathMLDocument::serializePresentationMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	if (def_list_.size() == 0)
		return;
	else if (def_list_.size() == 1)
	{
		// für eine einzelne Deklaration wird eine mrow generiert
		DOMElement * mrow = doc->createElementNS(mml_xmlns_uri,mml_mrow);
		node->appendChild(mrow);

		MathMLDeclare * mml_decl = *(def_list_.begin());
		mml_decl->serializePresentationMathML(doc, mrow);
	}
	else
	{
		// für >= 2 Deklarationen wird eine mehrzeilige mtable generiert
		DOMElement * mtable = doc->createElementNS(mml_xmlns_uri,mml_mtable);
		DOMElement * mtr, * mtd;
		std::list< MathMLDeclare * >::const_iterator iter = def_list_.begin();
		node->appendChild(mtable);

		while (iter != def_list_.end())
		{
			MathMLDeclare * mml_decl = *(iter++);
			mtr = doc->createElementNS(mml_xmlns_uri,mml_mtr);
			mtd = doc->createElementNS(mml_xmlns_uri,mml_mtd);
			mtable->appendChild(mtr);
			mtr->appendChild(mtd);
			mml_decl->serializePresentationMathML(doc, mtd);
		}
	}
}

/**
 * Schreibt die Definitionen aus der Definitionsliste in den
 * DOM-Tree eines kombinierten Content/Presentation-MathML-Dokuments.
 * 
 * @param node Wurzel-Element (math) des MathML-Dokuments
 */
void MathMLDocument::serializeContentWithPresentationMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	DOMElement * semantics = doc->createElementNS(mml_xmlns_uri,mml_semantics);
	node->appendChild(semantics);
	serializeContentMathML(doc,semantics);
	DOMElement * presentation = doc->createElementNS(mml_xmlns_uri,mml_annotation_xml);
	// in "annotation-xml" das Attribut "encoding" auf "MathML-Presentation"
	// setzen:
	presentation->setAttribute(mml_encoding, mml_MathML_Presentation);
	semantics->appendChild(presentation);
	serializePresentationMathML(doc,presentation);
	// evtl. weitere Daten in <annotation> schreiben
}

void MathMLDocument::serializePresentationWithContentMathML(
	XN DOMDocument * doc,
	XN DOMNode * node
	) const
{
	DOMElement * semantics = doc->createElementNS(mml_xmlns_uri,mml_semantics);
	node->appendChild(semantics);
	serializePresentationMathML(doc,semantics);
	DOMElement * content = doc->createElementNS(mml_xmlns_uri,mml_annotation_xml);
	// in "annotation-xml" das Attribut "encoding" auf "MathML-Content"
	// setzen:
	content->setAttribute(mml_encoding, mml_MathML_Content);
	semantics->appendChild(content);
	serializeContentMathML(doc,content);
	// evtl. weitere Daten in <annotation> schreiben
}

DOMWriter & MathMLDocument::getDOMWriter()
{
	if (writer_)
		return *writer_;
	writer_ = new DOMWriterImpl(
		"http://www.w3.org/1998/Math/MathML",
		"math",
		0, // publicId: -//W3C//DTD MathML 2.0//EN
		0 // systemId: mathml2.DTD; 0 => kein <!DOCTYPE...> erzeugen
		);
	return *writer_;
}

} // namespace flux::xml
} // namespace flux
