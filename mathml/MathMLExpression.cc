#include <cstdlib>
#include <cerrno>
#include "Error.h"
#include "ExprTree.h"
#include "MathMLUnicodeConstants.h"
#include "UnicodeTools.h"
#include "MathMLDocument.h"
#include "MathMLElement.h"
#include "MathMLExpression.h"
#include "XMLElement.h"

// Xerces-C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

using namespace flux::symb;

namespace flux {
namespace xml {

void MathMLExpression::serializeContentMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	exprTree2ContentMathML(doc, node, expr_);
}

void MathMLExpression::serializePresentationMathML(
	DOMDocument * doc,
	DOMNode * node
	) const
{
	exprTree2PresentationMathML(doc, node, expr_);
}

MathMLContentObject * MathMLExpression::parse(
	MathMLDocument * doc,
	XN DOMNode * node
	)
{
	ExprTree * expr = parseExpression(node);
	MathMLExpression * E = doc->createExpression(expr);
	delete expr;
	return E;
}

ExprTree * MathMLExpression::parseExpression(
	DOMNode * node
	)
{
	ExprTree * expr = 0;

	if (node->getNodeType() != DOMNode::ELEMENT_NODE)
		fTHROW(XMLException,node,"element node expected");

	if (XMLElement::match(node,mml_ci,mml_xmlns_uri))
		expr = parseIdentifier(node);
	else if (XMLElement::match(node,mml_cn,mml_xmlns_uri))
		expr = parseNumber(node);
	else if (XMLElement::match(node,mml_apply,mml_xmlns_uri))
		expr = parseApply(node);
	else
		fTHROW(XMLException,node,"ci|cn|apply expected");

	return expr;
}

ExprTree * MathMLExpression::parseIdentifier(
	DOMNode * node
	)
{
	DOMNode * child = 0;
	ExprTree * Id = 0;
	char * text;

	if (node == 0)
		fTHROW(XMLException,"null pointer");
	if (not XMLElement::match(node,mml_ci,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (ci) expected");

	child = node->getFirstChild();
	if (child->getNodeType() != DOMNode::TEXT_NODE)
		fTHROW(XMLException,node,"text node (identifier) expected");

	text = XMLString::transcode(((DOMText*)child)->getData());
	strtrim_inplace(text);
	Id = new ExprTree(text);
	XMLString::release(&text);
	return Id;
}

/*
 * Komplexe Zahlen werden (momentan noch) nicht unterstützt.
 * Konstanten wie &pi werden nicht unterstützt.
 * (siehe Appendix C.2 der MathML-Spec!)
 */
ExprTree * MathMLExpression::parseNumber(
	DOMNode * node
	)
{
	DOMNode * child = 0;
	double dblval;
	int type = 1; // 0:=(Big)Integer, 1:=Real, 2:=Rational, 3:=e-notation 4:=???
	int radix = 10;
	char * end_ptr;
	ExprTree * first = 0, * second = 0, * value = 0;

	if (not XMLElement::match(node,mml_cn,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (cn) expected");

	DOMNamedNodeMap * nnm = node->getAttributes();
	DOMAttr * typeAttr = (DOMAttr*)nnm->getNamedItem(mml_type);
	DOMAttr * baseAttr = (DOMAttr*)nnm->getNamedItem(mml_base);

	// child auf das erste Text-Element setzen (die Zahl)
	child = node->getFirstChild();
	while (child != 0 && child->getNodeType() != DOMNode::TEXT_NODE)
		child = child->getNextSibling();
	if (child == 0)
		fTHROW(XMLException,node,"number expected");
	
	if (baseAttr != 0)
	{
		// bei gesetztem base-Attribut handelt es sich grundsätzlich um
		// einen Integer.
		type = 0;
		radix = XMLString::parseInt(baseAttr->getValue());
	}
	else
	{
		// wenn weder base- noch type-Attribut gesetzt sind, wird eine
		// real-Zahl geparst
		if (typeAttr == 0)
			type = 1;
		else if (XMLString::equals(typeAttr->getValue(), mml_integer))
			type = 0;
		else if (XMLString::equals(typeAttr->getValue(), mml_real))
			type = 1;
		else if (XMLString::equals(typeAttr->getValue(), mml_rational))
			type = 2;
		else if (XMLString::equals(typeAttr->getValue(), mml_e_notation))
			type = 3;
		else
			type = 4; // nicht untersützter Typ
	}

	// für type="rational" und type="e-notation" werden zunächst zwei
	// Zahlen geparst:
	if (type == 2 || type == 3)
	{
		// ersten Wert parsen (Zähler oder Mantisse)
		U2A f_u2a(((DOMText*)child)->getData());
		dblval = strtod((char*)f_u2a, &end_ptr);
		if (child->getNodeType() != DOMNode::TEXT_NODE || end_ptr == f_u2a)
		{
			fTHROW(XMLException,node,
				"error parsing first number of pair (%s)",
				(char const *)f_u2a);
		}
		first = new ExprTree(dblval);

		// <sep/> suchen:
		while (child != 0 && child->getNodeType() != DOMNode::ELEMENT_NODE)
			child = child->getNextSibling();
		if (child == 0)
			fTHROW(XMLException,node,"missing separator <sep/>?");
		// zum Nenner vorrücken:
		child = child->getNextSibling();
		// das nächste Child muß ein Text-Node sein!
		if (child == 0 || child->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,node,
				"number following separator not found");
		// zweiten Wert parsen (Nenner oder Exponent)
		U2A s_u2a(((DOMText*)child)->getData());
		dblval = strtod(s_u2a, &end_ptr);
		if (end_ptr == s_u2a)
		{
			fTHROW(XMLException,node,
				"error parsing second number of pair (%s)",
				(char const *)s_u2a);
		}
		second = new ExprTree(dblval);
	}

	switch (type)
	{
	case 0: // (Big)Integer
		{
		//strtol
		U2A i_u2a(((DOMText*)child)->getData());
		errno = 0;
		dblval = strtol(i_u2a, &end_ptr, radix);
		if (end_ptr == i_u2a)
		{
			fTHROW(XMLException,node,
				"error parsing long integer value (%s)",
				(char const *)i_u2a);
		}
		value = new ExprTree(dblval);
		}
		break;
	case 1: // Real
		{
		U2A r_u2a(((DOMText*)child)->getData());
		dblval = strtod(r_u2a, &end_ptr);
		if (end_ptr == r_u2a)
		{
			fTHROW(XMLException,node,
				"error parsing real value (%s)",
				(char const *)r_u2a);
		}
		value = new ExprTree(dblval);
		}
		break;
	case 2: // Rational ... Zähler <sep/> Nenner
		value = ExprTree::div(first, second);
		break;
	case 3: // e-Notation ... Mantisse <sep/> Exponent
		// value = first * (10**second)
		second = ExprTree::pow(ExprTree::val(10), second);
		value = ExprTree::mul(first, second);
		break;
	default: // nicht unterstützter Typ
		fTHROW(XMLException,child, "unsupported number format");
	}
	return value;
}

ExprTree * MathMLExpression::parseApply(
	DOMNode * node
	)
{
	DOMNode * child, * opNode;
	ExprType op;
	ExprTree *L, *R, *Rexpr;
	int nargs = 0;

	if (not XMLElement::match(node,mml_apply,mml_xmlns_uri))
		fTHROW(XMLException,node,"element node (apply) expected");

	// alle nicht-Element-Knoten überspringen
	child = XMLElement::skipJunkNodes(node->getFirstChild());

	// es wird ein Operator-Knoten erwartet
	if (child == 0)
		fTHROW(XMLException,node,"apply: operator node expected");

	// der erste Element-Knoten muß ein Operator-Knoten sein
	ExprTree * extra = 0;
	op = parseOperator(child,&extra);
	opNode = child;
	U2A opNodeName(opNode->getNodeName());
	if (extra)
		child = XMLElement::nextNode(child);

	// alle nicht-Element-Knoten überspringen
	child = XMLElement::nextNode(child);

	// wenn es kein weiteres Child gibt, dann liegt hier ein Fehler vor:
	// 0-ärer Operator (laut MathML-Spez. erlaubt!).
	if (child == 0)
		fTHROW(XMLException,node,"apply: too few arguments for operator");
	
	// jetzt folgt der erste Operand (L-value)
	L = parseExpression(child);
	nargs = 1;

	R = 0;

	// zum nächsten Child vorrücken
	child = XMLElement::nextNode(child);

	while (child != 0)
	{
		// parse ein R-Value
		Rexpr = parseExpression(child);

		if (R == 0)
		{
			// Rexpr war das erste R-Value
			if (MathMLElement::isUnaryOperator(opNodeName)
				and not MathMLElement::isBinaryOperator(opNodeName))
			{
				// Hier muß sicher sein, dass es sich um einen Operator
				// handelt, der mit Sicherheit ausschließlich unär ist -
				// ein Gegenbeispiel ist "minus".
				fTHROW(XMLException,node,
					"apply: too many arguments to unary operator.");
			}
			R = Rexpr;
			Rexpr = 0;
		}
		else
		{
			// es gab schon ein Rvalue
			if (MathMLElement::isBinaryOperator(opNodeName))
			{
				// damit gibt es mehr als zwei Argumente für einen binären
				// Operator:
				fTHROW(XMLException,node,
					"apply: too many arguments to binary operator");
			}
			// Konsequenz: opNode ist ein n-ärer Operator!
			R = new ExprTree(op, R, Rexpr);
			Rexpr = 0;
		}
		nargs++;
		
		// zum nächsten Child vorrücken
		child = XMLElement::nextNode(child);
	}

	if (R == 0)
	{
		if (not MathMLElement::isUnaryOperator(opNodeName)
			and (MathMLElement::isBinaryOperator(opNodeName)
			or MathMLElement::isNAryOperator(opNodeName)))
		{
			fTHROW(XMLException,node,
				"apply: too few arguments for binary/n-ary operator");
		}

		if (op == et_op_sub) // unäres Minus
			return new ExprTree(et_op_uminus, L, 0);
		else if (op == et_op_sqrt) // beliebige Wurzeln
			return ExprTree::pow(L,ExprTree::recip(extra));
		else if (op == et_op_log) // beliebige Logarithmen
		{
			if (extra == 0)
				return ExprTree::log(L);
			else if (extra->isLiteral())
			{
				if (extra->getDoubleValue() == 2.)
				{
					delete extra;
					return ExprTree::log_2(L);
				}
				else if (extra->getDoubleValue() == 10.)
				{
					delete extra;
					return ExprTree::log_10(L);
				}
				else
				{
					double d = ::log(extra->getDoubleValue());
					delete extra;
					return ExprTree::div(ExprTree::log(L),ExprTree::val(d));
				}
			}
		}
                else if(op == et_op_sin)
                    return ExprTree::sin(L);
                else if(op == et_op_cos)
                    return ExprTree::cos(L);
	}
	return new ExprTree(op, L, R);
}

ExprType MathMLExpression::parseOperator(
	DOMNode * node,
	ExprTree ** extra
	)
{
	ExprType op;

	*extra = 0;

	if (node->getNodeType() != DOMNode::ELEMENT_NODE)
		fTHROW(XMLException,node,"element node (operator) expected");

	U2A u2a(node->getLocalName());
	if (not MathMLElement::isOperator(u2a))
		fTHROW(XMLException,node,"node is not a valid MathML operator");
	
	if (XMLElement::match(node,mml_plus,mml_xmlns_uri))
		op = et_op_add;
	else if (XMLElement::match(node,mml_minus,mml_xmlns_uri))
		op = et_op_sub;
	else if (XMLElement::match(node,mml_times,mml_xmlns_uri))
		op = et_op_mul;
	else if (XMLElement::match(node,mml_divide,mml_xmlns_uri))
		op = et_op_div;
	else if (XMLElement::match(node,mml_power,mml_xmlns_uri))
		op = et_op_pow;
	else if (XMLElement::match(node,mml_eq,mml_xmlns_uri))
		op = et_op_eq;
	else if (XMLElement::match(node,mml_leq,mml_xmlns_uri))
		op = et_op_leq;
	else if (XMLElement::match(node,mml_lt,mml_xmlns_uri))
		op = et_op_lt;
	else if (XMLElement::match(node,mml_geq,mml_xmlns_uri))
		op = et_op_geq;
	else if (XMLElement::match(node,mml_gt,mml_xmlns_uri))
		op = et_op_gt;
	else if (XMLElement::match(node,mml_neq,mml_xmlns_uri))
		op = et_op_neq;
	else if (XMLElement::match(node,mml_abs,mml_xmlns_uri))
		op = et_op_abs;
	else if (XMLElement::match(node,mml_sin,mml_xmlns_uri))
		op = et_op_sin;
	else if (XMLElement::match(node,mml_cos,mml_xmlns_uri))
		op = et_op_cos;
	else if (XMLElement::match(node,mml_min,mml_xmlns_uri))
		op = et_op_min;
	else if (XMLElement::match(node,mml_max,mml_xmlns_uri))
		op = et_op_max;
	else if (XMLElement::match(node,mml_root,mml_xmlns_uri))
	{
		DOMNode * degree = XMLElement::nextNode(node);
		if (not XMLElement::match(degree,mml_degree,mml_xmlns_uri))
			fTHROW(XMLException,degree,"element node (degree) expected");
		*extra = parseExpression(XMLElement::skipJunkNodes(degree->getFirstChild()));
		op = et_op_sqrt;
	}
	else if (XMLElement::match(node,mml_log,mml_xmlns_uri))
	{
		DOMNode * logbase = XMLElement::nextNode(node);
		if (not XMLElement::match(logbase,mml_logbase,mml_xmlns_uri))
			fTHROW(XMLException,logbase,"element node (logbase) expected");
		*extra = parseExpression(XMLElement::skipJunkNodes(logbase->getFirstChild()));
		op = et_op_log;
	}
	else if (XMLElement::match(node,mml_ln,mml_xmlns_uri))
		op = et_op_log;
	else if (XMLElement::match(node,mml_exp,mml_xmlns_uri))
		op = et_op_exp;
	else
		// nicht-unterstützter Operator
		fTHROW(XMLException,node,
			"operator not supported"
			//"operator \"" + node.getNodeName() + "\" not supported"
			);
	return op;
}

void MathMLExpression::exprTree2ContentMathML(
	DOMDocument * doc,
	DOMNode * node,
	ExprTree * e
	) const
{
	switch (e->getNodeType())
	{
	case et_literal:
		{
		A2U l_a2u(e->toString());
		DOMElement * cn = doc->createElementNS(mml_xmlns_uri,mml_cn);
		DOMText * value = doc->createTextNode(l_a2u);
		cn->appendChild(value);
		node->appendChild(cn);
		}
		return;
	case et_variable:
		{
		A2U v_a2u(e->toString());
		DOMElement * ci = doc->createElementNS(mml_xmlns_uri,mml_ci);
		DOMText * value = doc->createTextNode(v_a2u);
		ci->appendChild(value);
		node->appendChild(ci);
		}
		return;
	default:
		{
		// Sonderbehandlung für rationale Zahlen
		if (e->getNodeType() == et_op_div
			&& e->Lval()->isLiteral()
			&& e->Rval()->isLiteral())
		{
			A2U n_a2u(e->Lval()->toString());
			A2U d_a2u(e->Rval()->toString());
			DOMElement * cn = doc->createElementNS(mml_xmlns_uri,mml_cn);
			DOMElement * sep = doc->createElementNS(mml_xmlns_uri,mml_sep);
			DOMText * num_value = doc->createTextNode(n_a2u);
			DOMText * den_value = doc->createTextNode(d_a2u);
			cn->setAttribute(mml_type, mml_rational);
			cn->appendChild(num_value);
			cn->appendChild(sep);
			cn->appendChild(den_value);
			node->appendChild(cn);
			return;
		}
		DOMElement * apply = doc->createElementNS(mml_xmlns_uri,mml_apply);
		DOMElement * logbase = 0;
		DOMElement * degree = 0;
		DOMElement * power2 = 0;
		node->appendChild(apply);
		XMLCh const * opString = 0;
		switch (e->getNodeType())
		{
		case et_op_sub:
		case et_op_uminus:
			opString = mml_minus;
			break;
		case et_op_add:
			opString = mml_plus;
			break;
		case et_op_mul:
			opString = mml_times;
			break;
		case et_op_div:
			opString = mml_divide;
			break;
		case et_op_pow:
			opString = mml_power;
			break;
		case et_op_eq:
			opString = mml_eq;
			break;
		case et_op_leq:
			opString = mml_leq;
			break;
		case et_op_lt:
			opString = mml_lt;
			break;
		case et_op_geq:
			opString = mml_geq;
			break;
		case et_op_gt:
			opString = mml_gt;
			break;
		case et_op_neq:
			opString = mml_neq;
			break;
		case et_op_abs:
			opString = mml_abs;
			break;
		case et_op_sin:
			opString = mml_sin;
			break;
		case et_op_cos:
			opString = mml_cos;
			break;
                case et_op_exp:
			opString = mml_exp;
			break;
		case et_op_min:
			opString = mml_min;
			break;
		case et_op_max:
			opString = mml_max;
			break;
		case et_op_sqrt:
			opString = mml_root;
			// n-te wurzel von a:
			// <apply>
			//   <root/>
			//   <degree><ci type='integer'> n </ci></degree>
			//   <ci> a </ci>
			// </apply>
			{
			DOMElement * cn = doc->createElementNS(mml_xmlns_uri,mml_cn);
			cn->appendChild(doc->createTextNode(mml_2));
			cn->setAttribute(mml_type, mml_integer);
			degree = doc->createElementNS(mml_xmlns_uri,mml_degree);
			degree->appendChild(cn);
			}
			break;
		case et_op_log:
			opString = mml_ln;
			break;
		case et_op_log2:
		case et_op_log10:
			// log(x) zur basis 3:
			// <apply>
			//   <log/>
			//   <logbase>
			//   <cn> 3 </cn>
			//   </logbase>
			//   <ci> x </ci>
			// </apply>
			opString = mml_log;
			{
			DOMElement * cn = doc->createElementNS(mml_xmlns_uri,mml_cn);
			cn->appendChild(doc->createTextNode(
				e->getNodeType() == et_op_log2 ? mml_2 : mml_10
				));
			logbase = doc->createElementNS(mml_xmlns_uri,mml_logbase);
			logbase->appendChild(cn);
			}
			break;
		case et_op_sqr:
			opString = mml_power;
			power2 = doc->createElementNS(mml_xmlns_uri,mml_cn);
			power2->appendChild(doc->createTextNode(mml_2));
			break;
		default:
			// diff noch nicht implementiert
			fASSERT_NONREACHABLE();
		}
		DOMElement * op = doc->createElementNS(mml_xmlns_uri,opString);
		apply->appendChild(op);

		if (degree != 0)
			apply->appendChild(degree);
		else if (logbase != 0)
			apply->appendChild(logbase);

		exprTree2ContentMathML(doc, apply, e->Lval());
		if (e->isBinaryOp())
			exprTree2ContentMathML(doc, apply, e->Rval());
		if (power2 != 0)
			apply->appendChild(power2);
		}
		return;
	}
}

void MathMLExpression::exprTree2PresentationMathML(
	DOMDocument * doc,
	DOMNode * node,
	ExprTree * e
	) const
{
	switch (e->getNodeType())
	{
	case et_literal:
		{
		A2U l_a2u(e->toString());
		DOMElement * mn = doc->createElementNS(mml_xmlns_uri,mml_mn);
		DOMText * value = doc->createTextNode(l_a2u);
		mn->appendChild(value);
		node->appendChild(mn);
		}
		return;
	case et_variable:
		{
		A2U v_a2u(e->toString());
		DOMElement * mi = doc->createElementNS(mml_xmlns_uri,mml_mi);
		DOMText * value = doc->createTextNode(v_a2u);
		mi->appendChild(value);
		node->appendChild(mi);
		}
		return;
	default:
		{
		switch (e->getNodeType())
		{
		case et_op_add:
			{
			DOMElement * mrow = doc->createElementNS(mml_xmlns_uri,mml_mrow);
			node->appendChild(mrow);
			exprTree2PresentationMathML(doc, mrow, e->Lval());
			DOMElement * mo_add = doc->createElementNS(mml_xmlns_uri,mml_mo);
			mo_add->appendChild(doc->createTextNode(mml_Plus));
			mrow->appendChild(mo_add);
			exprTree2PresentationMathML(doc, mrow, e->Rval());
			}
			break;
		case et_op_sub:
			{
			DOMElement * mrow = doc->createElementNS(mml_xmlns_uri,mml_mrow);
			node->appendChild(mrow);
			exprTree2PresentationMathML(doc, mrow, e->Lval());
			DOMElement * mo_sub = doc->createElementNS(mml_xmlns_uri,mml_mo);
			mo_sub->appendChild(doc->createTextNode(mml_Dash)); // "-"
			mrow->appendChild(mo_sub);
			if (e->Rval()->isOperator())
			{
				switch (e->Rval()->getNodeType())
				{
				case et_op_add:
				case et_op_sub:
					{
					DOMElement * mfenced = doc->createElementNS(mml_xmlns_uri,mml_mfenced);
					mrow->appendChild(mfenced);
					exprTree2PresentationMathML(doc, mfenced, e->Rval());
					}
					break;
				default:
					exprTree2PresentationMathML(doc, mrow, e->Rval());
					break;
				}
			}
			else
				exprTree2PresentationMathML(doc, mrow, e->Rval());
			}
			break;
		case et_op_mul:
			{
			DOMElement * mrow = doc->createElementNS(mml_xmlns_uri,mml_mrow);
			node->appendChild(mrow);
			
			if (e->Lval()->isOperator())
			{
				switch (e->Lval()->getNodeType())
				{
				case et_op_add:
				case et_op_sub:
					{
					DOMElement * mfenced = doc->createElementNS(mml_xmlns_uri,mml_mfenced);
					mrow->appendChild(mfenced);
					exprTree2PresentationMathML(doc, mfenced, e->Lval());
					}
					break;
				default:
					exprTree2PresentationMathML(doc, mrow, e->Lval());
					break;
				}
			}
			else
				exprTree2PresentationMathML(doc, mrow, e->Lval());
			
			// Mozilla kennt die Entity &InvisibleTimes; nur, wenn MathML
			// in XHTML eingebettet wurde:

			//DOMEntityReference * invTimes =
			//	doc->createEntityReference(mml_InvisibleTimes);
			//DOMElement * mo_mul = doc->createElementNS(mml_xmlns_uri,mml_mo);
			//mo_mul->appendChild(invTimes);
			//mrow->appendChild(mo_mul);

			if (e->Rval()->isOperator())
			{
				switch (e->Rval()->getNodeType())
				{
				case et_op_add:
				case et_op_sub:
					{
					DOMElement * mfenced = doc->createElementNS(mml_xmlns_uri,mml_mfenced);
					mrow->appendChild(mfenced);
					exprTree2PresentationMathML(doc, mfenced, e->Rval());
					}
					break;
				default:
					exprTree2PresentationMathML(doc, mrow, e->Rval());
					break;
				}
			}
			else
				exprTree2PresentationMathML(doc, mrow, e->Rval());
			}
			break;
		case et_op_div:
			{
			DOMElement * mfrac = doc->createElementNS(mml_xmlns_uri,mml_mfrac);
			node->appendChild(mfrac);
			exprTree2PresentationMathML(doc, mfrac, e->Lval());
			exprTree2PresentationMathML(doc, mfrac, e->Rval());
			}
			break;
		case et_op_pow:
			{
			DOMElement * power = doc->createElementNS(mml_xmlns_uri,mml_msup);
			node->appendChild(power);
		
			if (e->Lval()->isOperator() && not IS_OP_UMINUS(e->Lval()))
			{
				DOMElement * mfenced = doc->createElementNS(mml_xmlns_uri,mml_mfenced);
				power->appendChild(mfenced);
				exprTree2PresentationMathML(doc, mfenced, e->Lval());
			}
			else
				exprTree2PresentationMathML(doc, power, e->Lval());
			exprTree2PresentationMathML(doc, power, e->Rval());
			}
			break;
		case et_op_uminus:
			{
			DOMElement * mo_uminus = doc->createElementNS(mml_xmlns_uri,mml_mo);
			mo_uminus->appendChild(doc->createTextNode(mml_Dash)); // "-"
			node->appendChild(mo_uminus);
		
			if (e->Lval()->isLeaf())
				exprTree2PresentationMathML(doc, node, e->Lval());
			else
			{
				DOMElement * mfenced = doc->createElementNS(mml_xmlns_uri,mml_mfenced);
				node->appendChild(mfenced);
				exprTree2PresentationMathML(doc, mfenced, e->Lval());
			}
			}
			break;
		case et_op_eq:
		case et_op_leq:
		case et_op_lt:
		case et_op_geq:
		case et_op_gt:
		case et_op_neq:
			{
			XMLCh const * mml_opstr = 0;
			DOMElement * mrow = doc->createElementNS(mml_xmlns_uri,mml_mrow);
			node->appendChild(mrow);
			exprTree2PresentationMathML(doc, mrow, e->Lval());
			switch (e->getNodeType())
			{
			case et_op_eq:  mml_opstr = mml_Equal; break;
			case et_op_leq: mml_opstr = mml__le_; break;
			case et_op_lt:  mml_opstr = mml__lt_; break;
			case et_op_geq: mml_opstr = mml__GreaterEqual_; break;
			case et_op_gt:  mml_opstr = mml__gt_; break;
			case et_op_neq: mml_opstr = mml__NotEqual_; break;
			default: break;
			}
			DOMElement * mo_op = doc->createElementNS(mml_xmlns_uri,mml_mo);
			mo_op->appendChild(doc->createTextNode(mml_opstr));
			mrow->appendChild(mo_op);
			exprTree2PresentationMathML(doc, mrow, e->Rval());
			} // Vergleichsoperatoren
		default:
			// abs, exp, min, max, sqrt noch nicht implementiert
			fASSERT_NONREACHABLE();
		} // switch (e->getNodeType())
	        } // case et_operator
	} // switch (e->getNodeType())
}

} // namespace flux::xml
} // namespace flux

