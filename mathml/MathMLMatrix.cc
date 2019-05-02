#include <list>
#include <string>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "ExprTree.h"
#include "MathMLDocument.h"
#include "XMLException.h"
#include "MathMLContentObject.h"
#include "MathMLUnicodeConstants.h"
#include "MathMLExpression.h"
#include "MathMLMatrix.h"
#include "XMLElement.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

using namespace flux::symb;

namespace flux {
namespace xml {

MathMLMatrix::MathMLMatrix(int rows, int cols)
	: rows_(rows), cols_(cols), is_symbolic_(false)
{
	sym_matrix_ = new ExprTree**[rows_];
	for (int i=0; i<rows_; i++)
	{
		sym_matrix_[i] = new ExprTree*[cols_];
		for (int j=0; j<cols_; j++)
			sym_matrix_[i][j] = 0;
	}
}

MathMLMatrix::MathMLMatrix(MathMLMatrix const & copy)
	: rows_(copy.rows_), cols_(copy.cols_)
{
	sym_matrix_ = new ExprTree**[rows_];
	for (int i=0; i<rows_; i++)
	{
		sym_matrix_[i] = new ExprTree*[cols_];
		for (int j=0; j<cols_; j++)
			sym_matrix_[i][j] = 0;
	}
	copyFrom(copy.sym_matrix_);
}

MathMLMatrix::~MathMLMatrix()
{
	for (int i=0; i<rows_; i++)
	{
		for (int j=0; j<cols_; j++)
			if (sym_matrix_[i][j] != 0) delete sym_matrix_[i][j];
		delete[] sym_matrix_[i];
	}
	delete[] sym_matrix_;
}

MathMLContentObject * MathMLMatrix::parse(
	MathMLDocument * doc,
	DOMNode * node
	)
{
	DOMNode * child;
	std::list< std::list< ExprTree * > * > rowList;
	std::list< ExprTree* > * colList;
	ExprTree * elem;
	int rows = -1, cols = -1;

	if (not XMLElement::match(node,mml_matrix,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (matrix) expected");

	child = node->getFirstChild();
	while (child != 0)
	{
		// nächstes <matrixrow>-Element suchen:
		while (child != 0 && child->getNodeType() != DOMNode::ELEMENT_NODE)
			child = child->getNextSibling();
		if (child != 0)
		{
			rowList.push_back( parseMatrixRow(child,cols) );
			// zur nächsten Matrix-Zeile vorrücken:
			child = child->getNextSibling();
		}
	}

	// Zeilen aus rowList übertragen
	std::list< std::list< ExprTree * > * >::iterator i = rowList.begin();
	std::list< ExprTree * >::iterator j;
	int ii = 0, jj;
	rows = rowList.size();

	MathMLMatrix * M = doc->createMatrix(rows,cols);

	while (i != rowList.end())
	{
		colList = *(i++);
		j = colList->begin();
		jj = 0;

		if (colList->size() != (unsigned int)cols)
			fTHROW(XMLException,node,"matrix row lengths do not match");
		while (j != colList->end())
		{
			elem = *(j++);
			M->sym_matrix_[ii][jj] = elem;
			if (elem != 0)
				M->is_symbolic_ |= (elem->isLiteral() == false);
			jj++;
		}
		ii++;
		delete colList;
	}

	return M;
}

std::list<ExprTree*> * MathMLMatrix::parseMatrixRow(DOMNode * node, int & cols)
{
	DOMNode * child;
	std::list< ExprTree * > * operandList = new std::list< ExprTree* >();
	ExprTree * expr;

	if (not XMLElement::match(node,mml_matrixrow,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (matrixrow) expected");

	// erster Nachfolger des <matrixrow>-Elements:
	child = node->getFirstChild();
	while (child != 0)
	{
		// das nächste Element suchen:
		while (child != 0 && child->getNodeType() != DOMNode::ELEMENT_NODE)
			child = child->getNextSibling();
		
		if (child != 0)
		{
			expr = MathMLExpression::parseExpression(child);
			if (not (expr->isLiteral() && expr->getDoubleValue() == 0.))
				operandList->push_back(expr);
			else
			{
				delete expr;
				operandList->push_back(0);
			}
			child = child->getNextSibling();
		}
	}

	if (cols == -1)
		cols = operandList->size();
	return operandList;
}

void MathMLMatrix::copyFrom(double ** array)
{
	is_symbolic_ = false;
	for (int i=0; i<rows_; i++)
	{
		for (int j=0; j<cols_; j++)
		{
			if (sym_matrix_[i][j] != 0)
			{
				delete sym_matrix_[i][j];
				sym_matrix_[i][j] = 0;
			}
			if (array[i][j] != 0.)
				sym_matrix_[i][j] = new ExprTree(array[i][j]);
		}
	}
}

void MathMLMatrix::copyFrom(ExprTree *** array)
{
	is_symbolic_ = false;
	for (int i=0; i<rows_; i++)
	{
		for (int j=0; j<cols_; j++)
		{
			if (sym_matrix_[i][j] != 0)
			{
				delete sym_matrix_[i][j];
				sym_matrix_[i][j] = 0;
			}
			if (array[i][j] != 0)
			{
				sym_matrix_[i][j] = array[i][j]->clone();
				is_symbolic_ |= not array[i][j]->isLiteral();
			}
		}
	}
}

void MathMLMatrix::serializeContentMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	DOMElement * matrix = doc->createElementNS(mml_xmlns_uri,mml_matrix);
	DOMElement * matrixrow;
	ExprTree zeroValue(0.), * expr;

	node->appendChild(matrix);
	for (int i=0; i<rows_; i++)
	{
		matrixrow = doc->createElementNS(mml_xmlns_uri,mml_matrixrow);
		matrix->appendChild(matrixrow);
		for (int j=0; j<cols_; j++)
		{
			expr = sym_matrix_[i][j];
			if (expr == 0) expr = &zeroValue;
			MathMLExpression mmE(expr);
			mmE.serializeContentMathML(doc, matrixrow);
		}
	}
}

void MathMLMatrix::serializePresentationMathML(
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
	
	for (int i=0; i<rows_; i++)
	{
		mtr = doc->createElementNS(mml_xmlns_uri,mml_mtr);
		mtable->appendChild(mtr);
		for (int j=0; j<cols_; j++)
		{
			mtd = doc->createElementNS(mml_xmlns_uri,mml_mtd);
			mtr->appendChild(mtd);

			expr = sym_matrix_[i][j];
			if (expr == 0) expr = &zeroValue;
			MathMLExpression mmE(expr);
			mmE.serializePresentationMathML(doc, mtd);
		}
	}
}

ExprTree const * MathMLMatrix::get(int i, int j) const
{
	return sym_matrix_[i][j];
}

void MathMLMatrix::set(int i, int j, ExprTree * value)
{
	sym_matrix_[i][j] = value->clone();
	is_symbolic_ |= (value->isLiteral() == false);
}

void MathMLMatrix::set(int i, int j, double value)
{
	if (sym_matrix_[i][j] != 0)
		delete sym_matrix_[i][j];
	sym_matrix_[i][j] = new ExprTree(value);
}

std::string MathMLMatrix::toString() const
{
	std::string str;
	if (rows_ == -1 || cols_ == -1 || sym_matrix_ == 0)
		return str;
	str += "[";
	for (int i=0; i<rows_; i++)
	{
		for (int j=0; j<cols_; j++)
		{
			str += sym_matrix_[i][j]->toString();
			if (j+1<cols_) str += ",";
		}
		str += "; ";
	}
	str += "]";
	return str;
}

} // namespace flux::xml
} // namespace flux

