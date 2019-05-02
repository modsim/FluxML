#include <string>
#include <list>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "XMLException.h"
#include "MathMLDocument.h"
#include "MathMLExpression.h"
#include "MathMLUnicodeConstants.h"
#include "MathMLContentObject.h"
#include "MathMLVector.h"
#include "XMLElement.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

using namespace flux::symb;

namespace flux {
namespace xml {

MathMLVector::MathMLVector(int dim, VectorType vtype)
	: dim_(dim), is_symbolic_(false), vtype_(vtype)
{
	sym_vector_ = new ExprTree*[dim_];
	for (int i=0; i<dim_; i++) sym_vector_[i] = 0;
}

MathMLVector::MathMLVector(MathMLVector const & copy)
	: dim_(copy.dim_), vtype_(copy.vtype_)
{
	sym_vector_ = new ExprTree*[dim_];
	for (int i=0; i<dim_; i++) sym_vector_[i] = 0;
	copyFrom(copy.sym_vector_);
}

MathMLVector::~MathMLVector()
{
	for (int i=0; i<dim_; i++)
		if (sym_vector_[i] != 0) delete sym_vector_[i];
	delete[] sym_vector_;
}

MathMLContentObject * MathMLVector::parse(
	MathMLDocument * doc,
	DOMNode * node
	)
{
	int i;
	DOMNode * child;
	DOMAttr * attrNode;
	DOMNamedNodeMap * nnm;
	std::list< ExprTree * > operandList;
	VectorType vtype = v_column;
	int dim;

	if (not XMLElement::match(node,mml_vector,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (vector) expected");

	// handelt es sich um einen Spaltenvektor (default), oder
	// um einen Zeilenvektor. Der Wert des type-Attributs kann
	// auch den Typ der Komponenten beschreiben (z.B. "real";
	// IMHO sehr unordentlich!), was aber wg. der symbolischen
	// Verarbeitung nebensächlich ist.
	nnm = node->getAttributes();
	attrNode = (DOMAttr*)(nnm->getNamedItem(mml_type));
	if (attrNode != 0 && XMLString::equals(attrNode->getValue(),mml_row))
		vtype = v_row;

	// erster Nachfolger des <vector>-Elements:
	child = node->getFirstChild();
	while (child != 0)
	{
		// das nächste Element suchen:
		while (child != 0 && child->getNodeType() != DOMNode::ELEMENT_NODE)
			child = child->getNextSibling();
		
		if (child != 0)
		{
			operandList.push_back(
				MathMLExpression::parseExpression(child)
				);
			child = child->getNextSibling();
		}
	}

	dim = operandList.size();
	MathMLVector * V = doc->createVector(dim);
	V->vtype_ = vtype;
	
	std::list<ExprTree*>::iterator iter = operandList.begin();
	i = 0;
	while (iter != operandList.end())
	{
		ExprTree * value = *(iter++);
		V->is_symbolic_ |= not value->isLiteral();
		V->sym_vector_[i++] = value;
	}

	return V;
}

void MathMLVector::copyFrom(double * array)
{
	is_symbolic_ = false;
	for (int i=0; i<dim_; i++)
	{
		if (sym_vector_[i] != 0)
		{
			delete sym_vector_[i];
			sym_vector_[i] = 0;
		}
		if (array[i] != 0.)
			sym_vector_[i] = new ExprTree(array[i]);
	}
}

void MathMLVector::copyFrom(ExprTree ** array)
{
	is_symbolic_ = false;
	for (int i=0; i<dim_; i++)
	{
		if (sym_vector_[i] != 0)
		{
			delete sym_vector_[i];
			sym_vector_[i] = 0;
		}
		if (array[i] != 0)
		{
			sym_vector_[i] = array[i]->clone();
			is_symbolic_ |= not array[i]->isLiteral();
		}
	}
}

void MathMLVector::serializeContentMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	DOMElement * vector = doc->createElementNS(mml_xmlns_uri,mml_vector);
	ExprTree zeroValue(0.), * expr;
	
	if (vtype_ == v_row)
		vector->setAttribute(mml_type, mml_row);
	node->appendChild(vector);
	for (int i=0; i<dim_; i++)
	{
		expr = sym_vector_[i];
		if (expr == 0) expr = &zeroValue;
		MathMLExpression mmE(expr);
		mmE.serializeContentMathML(doc, vector);
	}
}

void MathMLVector::serializePresentationMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{	
	DOMElement * mtable = doc->createElementNS(mml_xmlns_uri,mml_mtable);
	DOMElement * mo_open = doc->createElementNS(mml_xmlns_uri,mml_mo);
	DOMElement * mo_close = doc->createElementNS(mml_xmlns_uri,mml_mo);
	DOMElement * mtr, * mtd;
	ExprTree zeroValue(0.), * expr;

	mo_open->appendChild(doc->createTextNode(mml_OpenParen));
	mo_close->appendChild(doc->createTextNode(mml_CloseParen));
	node->appendChild(mo_open);
	node->appendChild(mtable);
	node->appendChild(mo_close);

	if (vtype_ == v_column)
	{
		// stehender Vektor
		for (int i=0; i<dim_; i++)
		{
			mtr = doc->createElementNS(mml_xmlns_uri,mml_mtr);
			mtd = doc->createElementNS(mml_xmlns_uri,mml_mtd);
			mtable->appendChild(mtr);
			mtr->appendChild(mtd);
			expr = sym_vector_[i];
			if (expr == 0) expr = &zeroValue;
			MathMLExpression mmE(expr);
			mmE.serializePresentationMathML(doc, mtd);
		}
	}
	else
	{
		// liegender Vektor
		mtr = doc->createElementNS(mml_xmlns_uri,mml_mtr);
		mtable->appendChild(mtr);
		for (int j=0; j<dim_; j++)
		{
			mtd = doc->createElementNS(mml_xmlns_uri,mml_mtd);
			mtr->appendChild(mtd);
			expr = sym_vector_[j];
			if (expr == 0) expr = &zeroValue;
			MathMLExpression mmE(expr);
			mmE.serializePresentationMathML(doc, mtd);
		}
	}
}

ExprTree const * MathMLVector::get(int i) const
{
	return sym_vector_[i];
}
    
void MathMLVector::set(int i, ExprTree * value)
{
	sym_vector_[i] = value->clone();
	if (is_symbolic_ == false && not value->isLiteral())
		is_symbolic_ = true;
}

void MathMLVector::set(int i, double value)
{
	if (sym_vector_[i] != 0) delete sym_vector_[i];
	sym_vector_[i] = new ExprTree(value);
}

bool MathMLVector::isColumnVector() const
{
	if (sym_vector_ == 0)
		fTHROW(XMLException,"vector still uninitialized");
	return vtype_ == v_column;
}
    
std::string MathMLVector::toString() const
{
	std::string str("");
	if (dim_ == -1 || sym_vector_ == 0)
		return str;
	str += "[";
	for (int i=0; i<dim_; i++)
	{
		str += sym_vector_[i]->toString();
		if (i+1<dim_) str += (vtype_==v_row) ? "," : ";";
	}
	str += "]";
	return str;
}

} // namespace flux::xml
} // namespace flux
