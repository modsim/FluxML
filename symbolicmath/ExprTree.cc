#include <string>
#include <list>
#include <queue>
#include <cmath>
#include <cstdarg>
extern "C"
{
#include <stdint.h>
}
#include "fluxml_config.h"
#include "Error.h"
#include "Combinations.h"
#include "cstringtools.h"
#include "ExprTree.h"

// Fix für Newlib/Cygwin
#ifndef INFINITY
#define INFINITY	HUGE_VAL
#endif

using namespace std;

namespace flux {
namespace symb {

#define IS_INT(v)	((v)-(int64_t)(v) == 0.0)

/**
 * Ein Funktor zum Sortieren von std::list< ExprTree * >
 */
static struct et_list_keycomp_t
{
	bool operator()(ExprTree * L, ExprTree * R) const
	{
		return *L < *R;
	}
} et_list_keycomp;

/*
 * Implementierungen der ExprTree-Klasse:
 */

ExprTree::ExprTree(ExprTree const & copy)
{
	node_type_ = copy.node_type_;
	switch (node_type_)
	{
	case et_variable:
		value_.var_name_ = strdup_alloc(copy.value_.var_name_);
		break;
	case et_literal:
		value_.lit_value_ = copy.value_.lit_value_;
		break;
	default:
		fASSERT( isOperator() );
		if (copy.value_.childs_.Lval_ != 0)
			value_.childs_.Lval_ =
				new ExprTree(*(copy.value_.childs_.Lval_));
		else
			value_.childs_.Lval_ = 0;
		if (copy.value_.childs_.Rval_ != 0)
			value_.childs_.Rval_ =
				new ExprTree(*(copy.value_.childs_.Rval_));
		else
			value_.childs_.Rval_ = 0;
	}
	if (copy.hval_ != 0)
	{
		hval_ = new size_t;
		*hval_ = *(copy.hval_);
	}
	else
		hval_ = 0;
	mark_ = copy.mark_;
}

ExprTree::ExprTree(double ltvalue) : hval_(0)
{
	mark_ = 0;
	if (ltvalue >= 0.)
	{
		node_type_ = et_literal;
		value_.lit_value_ = ltvalue;
	}
	else
	{
		node_type_ = et_op_uminus;
		value_.childs_.Lval_ = new ExprTree(-ltvalue);
		value_.childs_.Rval_ = 0;
	}
	hashValue();
}

ExprTree::~ExprTree()
{
	if (hval_ != 0)
		delete hval_;

	if (isOperator())
	{
		if (value_.childs_.Lval_ != 0)
			delete value_.childs_.Lval_;
		if (value_.childs_.Rval_ != 0)
			delete value_.childs_.Rval_;
	}
	else if (isVariable())
		delete[] value_.var_name_;
}	
	
ExprTree * ExprTree::replaceBy(ExprTree * & E)
{
	fASSERT( E != 0 );
	if (node_type_ == et_variable)
		delete[] value_.var_name_;
	else
	{
		if (value_.childs_.Lval_)
			delete value_.childs_.Lval_;
		if (value_.childs_.Rval_)
			delete value_.childs_.Lval_;
	}
	value_ = E->value_;
	mark_ = E->mark_;
	if (E->hval_)
	{
		if (hval_ == 0)
			hval_ = new size_t;
		*hval_ = *(E->hval_);
	}
	else if (hval_)
	{
		delete hval_;
		hval_ = 0;
	}
	node_type_ = E->node_type_;
	// beim delete das Löschen des Variablennamens verhindern
	// und Variablen zu Literalen machen
	if (node_type_ == et_variable)
		E->node_type_ = et_literal;
	E->value_.childs_.Lval_ = 0;
	E->value_.childs_.Rval_ = 0;
	delete E;
	E = 0;
	return this;
}

bool ExprTree::operator== (ExprTree const & E) const
{
	// Wenn es die selben Bäume sind, dann sind sie gleich
	if (this == &E)
		return true;
	// Wenn die Knotentypen unterschiedlich sind, dann sind die Ausdrücke ungleich
	if (node_type_ != E.node_type_)
		return false;
	// Wenn die Hashwerte ungleich sind, dann sind die Ausdrücke ungleich
	if (hashValue() != E.hashValue())
		return false;

	// Genauerer Vergleich
	switch (node_type_)
	{
	case et_literal:
		return value_.lit_value_ == E.value_.lit_value_;
	case et_variable:
		return strcmp(value_.var_name_, E.value_.var_name_) == 0;
	default:
		// rekursiver Vergleich
		if (not (*(value_.childs_.Lval_) == *(E.value_.childs_.Lval_)))
			return false;
		if (isBinaryOp())
			return *(value_.childs_.Rval_) == *(E.value_.childs_.Rval_);
		return true;
	}
}

bool ExprTree::operator< (ExprTree const & E) const
{
	switch (node_type_)
	{
	case et_literal:
		switch (E.node_type_)
		{
		case et_literal:
			return value_.lit_value_ < E.value_.lit_value_;
		case et_variable:
			return true;
		default:
			return true;
		}
		break;
	case et_variable:
		switch (E.node_type_)
		{
		case et_literal:
			return false;
		case et_variable:
			return strcmp(value_.var_name_,E.value_.var_name_)<0;
		default:
			return true;
		}
		break;
	default:
		switch (E.node_type_)
		{
		case et_literal:
			return false;
		case et_variable:
			return false;
		default:
			return node_type_ < E.node_type_;
		}
	}
	return false;
}

std::string ExprTree::subTreeToString() const
{
	fASSERT( isOperator() and Lval() != 0 );
	std::string s;

	switch (node_type_)
	{
	case et_op_add:
		s += Lval()->toString() + "+" + Rval()->toString();
		break;
	case et_op_sub:
		s += Lval()->toString() + "-";
		if (Rval()->isOperator())
		{
			switch (Rval()->node_type_)
			{
			case et_op_add:
			case et_op_sub:
				s += "(" + Rval()->toString() + ")";
				break;
			default:
				s += Rval()->toString();
			}
		}
		else
			s += Rval()->toString();
		break;
	case et_op_mul:
		{
		if (Lval()->isOperator())
		{
			switch (Lval()->node_type_)
			{
			case et_op_add:
			case et_op_sub:
			case et_op_div:
				s += "(" + Lval()->toString() + ")";
				break;
			default:
				s += Lval()->toString();
			}
		}
		else
			s += Lval()->toString();
		s += "*";
		if (Rval()->isOperator())
		{
			switch (Rval()->node_type_)
			{
			case et_op_add:
			case et_op_sub:
			case et_op_div:
				s += "(" + Rval()->toString() + ")";
				break;
			default:
				s += Rval()->toString();
			}
		}
		else
			s += Rval()->toString();
		} // case et_op_mul
		break;
	case et_op_div:
		{
		if (Lval()->isOperator())
		{
			switch (Lval()->node_type_)
			{
			case et_op_add:
			case et_op_sub:
			case et_op_div:
				s += "(" + Lval()->toString() + ")";
				break;
			default:
				s += Lval()->toString();
			}
		}
		else
			s += Lval()->toString();
		s += "/";
		if (Rval()->isOperator() and Rval()->node_type_ != et_op_uminus)
			s += "(" + Rval()->toString() + ")";
		else
			s += Rval()->toString();
		} // case et_op_div
		break;
	case et_op_pow:
		{
		if (Lval()->isOperator() and Lval()->node_type_ != et_op_uminus)
			s += "(" + Lval()->toString() + ")";
		else
			s += Lval()->toString();
		s += "^";
		if (Rval()->isOperator() and Rval()->node_type_ != et_op_uminus)
			s += "(" + Rval()->toString() + ")";
		else
			s += Rval()->toString();
		} // case et_op_pow
		break;
	case et_op_uminus:
		{
		if (Lval()->isLeaf())
		{
			if (Lval()->isLiteral() and Lval()->getDoubleValue() < 0)
				s += "-(" + Lval()->toString() + ")";

			else
				s += "-" + Lval()->toString();
		}
		else
			s += "-(" + Lval()->toString() + ")";
		}
		break;
	case et_op_eq:
		s += Lval()->toString() + "=" + Rval()->toString();
		break;
	case et_op_leq:
		s += Lval()->toString() + "<=" + Rval()->toString();
		break;
	case et_op_lt:
		s += Lval()->toString() + "<" + Rval()->toString();
		break;
	case et_op_geq:
		s += Lval()->toString() + ">=" + Rval()->toString();
		break;
	case et_op_gt:
		s += Lval()->toString() + ">" + Rval()->toString();
		break;
	case et_op_neq:
		s += Lval()->toString() + "!=" + Rval()->toString();
		break;
	case et_op_abs:
		s += "abs(" + Lval()->toString() + ")";
		break;
	case et_op_exp:
		s += "exp(" + Lval()->toString() + ")";
		break;
	case et_op_max:
		s += "max(" + Lval()->toString() + "," + Rval()->toString() + ")";
		break;
	case et_op_min:
		s += "min(" + Lval()->toString() + "," + Rval()->toString() + ")";
		break;
	case et_op_sqrt:
		s += "sqrt(" + Lval()->toString() + ")";
		break;
	case et_op_log:
		s += "log(" + Lval()->toString() + ")";
		break;
        case et_op_sin:
                s += "sin(" + Lval()->toString() + ")";
		break;
        case et_op_cos:
                s += "cos(" + Lval()->toString() + ")";
		break;
	case et_op_log2:
		s += "log2(" + Lval()->toString() + ")";
		break;
	case et_op_log10:
		s += "log10(" + Lval()->toString() + ")";
		break;
	case et_op_sqr:
		s += "sqr(" + Lval()->toString() + ")";
		break;
	case et_op_diff:
		s += "diff(" + Lval()->toString() + "," + Rval()->toString() + ")";
		break;
	default:
		fASSERT_NONREACHABLE();
	}
	return s;
}

std::string ExprTree::nodeToString() const
{
	switch (node_type_)
	{
	case et_literal:
		{
		// Hier wird ein double in einen String konvertiert.
		// Dabei wird eine automatisch richtige Anzahl von
		// Nachkommastellen verwendet, mit denen sich der
		// Double-Wert ohne Verlust von Genauigkeit rekonstruieren
		// läßt (siehe dbl2str in cstringtools.cc/.h).
		char buf[32];
		dbl2str(buf, value_.lit_value_,sizeof(buf));
		return std::string(buf);
		}
	case et_variable:
		return value_.var_name_;
	case et_op_add:    return "+";
	case et_op_sub:    return "-";
	case et_op_uminus: return "-";
	case et_op_mul:    return "*";
	case et_op_div:    return "/";
	case et_op_pow:    return "^";
	case et_op_eq:     return "==";
	case et_op_neq:    return "!=";
	case et_op_leq:    return "<=";
	case et_op_geq:    return ">=";
	case et_op_lt:     return "<";
	case et_op_gt:     return ">";
	case et_op_abs:    return "abs";
	case et_op_min:    return "min";
	case et_op_max:    return "max";
	case et_op_sqr:    return "sqr";
	case et_op_sqrt:   return "sqrt";
	case et_op_log:    return "log";
        case et_op_sin:    return "sin";
        case et_op_cos:    return "cos";
	case et_op_log2:   return "log2";
	case et_op_log10:  return "log10";
	case et_op_exp:    return "exp";
	case et_op_diff:   return "diff";
	default:
		fASSERT_NONREACHABLE();
		return "?";
	}
}

std::string ExprTree::nodeToSource() const
{
	std::string pfx("ExprTree::");

	switch (node_type_)
	{
	case et_literal:
		{
		// Hier wird ein double in einen String konvertiert.
		// Dabei wird eine automatisch richtige Anzahl von
		// Nachkommastellen verwendet, mit denen sich der
		// Double-Wert ohne Verlust von Genauigkeit rekonstruieren
		// läßt (siehe dbl2str in cstringtools.cc/.h).
		char buf[32];
		dbl2str(buf, value_.lit_value_,sizeof(buf));
		return pfx + "val(" + std::string(buf) + ")";
		}
	case et_variable:
		return pfx + "sym(\"" + std::string(value_.var_name_) + "\")";
	case et_op_add:    return pfx + "add";
	case et_op_sub:    return pfx + "sub";
	case et_op_uminus: return pfx + "minus";
	case et_op_mul:    return pfx + "mul";
	case et_op_div:    return pfx + "div";
	case et_op_pow:    return pfx + "pow";
	case et_op_eq:     return pfx + "eq";
	case et_op_neq:    return pfx + "neq";
	case et_op_leq:    return pfx + "leq";
	case et_op_geq:    return pfx + "geq";
	case et_op_lt:     return pfx + "lt";
	case et_op_gt:     return pfx + "gt";
	case et_op_abs:    return pfx + "abs";
	case et_op_min:    return pfx + "min";
	case et_op_max:    return pfx + "max";
	case et_op_sqr:    return pfx + "sqr";
	case et_op_sqrt:   return pfx + "sqrt";
	case et_op_log:    return pfx + "log";
        case et_op_sin:    return pfx + "sin";
        case et_op_cos:    return pfx + "cos";
	case et_op_log2:   return pfx + "log2";
	case et_op_log10:  return pfx + "log10";
	case et_op_exp:    return pfx + "exp";
	case et_op_diff:   return pfx + "diff";
	default:
		fASSERT_NONREACHABLE();
		return "?";
	}
}

std::string ExprTree::toString() const
{
	switch (node_type_)
	{
	case et_literal:
	case et_variable:
		return nodeToString();
	default: // Operator
		return subTreeToString();
	}
}

/*
 * Algebraische Differentiation
 */
#define EN_(E)	((E)->evalNode(true,dsymmap,true))
ExprTree * ExprTree::deval(
	char const * x,
	charptr_map< charptr_array > const * dsymmap
	) const
{
	if (isLeaf())
	{
		if (isVariable())
		{
			// diff(x,x) ?
			if (strcmp(value_.var_name_,x)==0)
				return val(1);

			// diff(f(x,y,z),x) ?
			if (dsymmap)
			{
				// Existiert ein Eintrag für den Symbolnamen?
				charptr_array * deps = dsymmap->findPtr(value_.var_name_);
				// Hängt der Symbolname von x ab?
				if (deps and deps->find(x) != deps->end())
					return diff(clone(this),x);
			}
		}
		return val(0);
	}

	fASSERT( isOperator() );

	ExprTree * Dx = 0;

	switch (node_type_)
	{
	case et_op_add:
		// (u + v)' = u' + v'
		Dx = add(
			Lval()->deval(x,dsymmap),
			Rval()->deval(x,dsymmap)
			);
		break;
	case et_op_sub:
		// (u - v)' = u' - v'
		Dx = sub(
			Lval()->deval(x,dsymmap),
			Rval()->deval(x,dsymmap)
			);
		break;
	case et_op_mul:
		// (u*v)' = u'v + uv'
		Dx = add(	EN_(mul(Lval()->deval(x,dsymmap),	// u'
					clone(Rval())			// v
					)),
				EN_(mul(clone(Lval()),			// u
					Rval()->deval(x,dsymmap)	// v'
					))
				);
		break;
	case et_op_div:
		// (u/v)' = (u'v - uv')/v^2
		Dx = div(	EN_(sub(EN_(mul(Lval()->deval(x,dsymmap), // u'
						clone(Rval())		// v
						)),
					EN_(mul(clone(Lval()),		// u
						Rval()->deval(x,dsymmap) // v'
						))
					)),
				sqr(clone(Rval()))
				);
		break;
	case et_op_pow:
		{
			// L(x)^R(x)*(R'(x)*ln(L(x))+R(x)*L'(x)/L(x))
			ExprTree * DL = Lval()->deval(x,dsymmap);
			ExprTree * DR = Rval()->deval(x,dsymmap);
			Dx = mul(	clone(this),
					EN_(add(EN_(mul(DR,
							EN_(log(clone(Lval())))
					   	)),
						EN_(div(EN_(mul(clone(Rval()),
								DL
							   	)),
							clone(Lval())
						   	))
					   	))
					);
		}
		break;
	case et_op_uminus:
		Dx = minus(Lval()->deval(x,dsymmap));
		break;
	case et_op_eq:
	case et_op_leq:
	case et_op_lt:
	case et_op_geq:
	case et_op_gt:
	case et_op_neq:
		Dx = new ExprTree(
			node_type_,
			Lval()->deval(x,dsymmap),
			Rval()->deval(x,dsymmap)
			);
		break;
	case et_op_abs:
		// "abs" ist ohne Smoothing nicht differenzierbar
		fTHROW(ExprTreeException);
		break;
	case et_op_exp:
		{
		// exp(f(x)) => f'(x)*exp(f(x))
		ExprTree * DL = Lval()->deval(x,dsymmap);
		Dx = mul(
			EN_(DL),
			clone(this)
			);
		}
		break;
	case et_op_max:
		// "max" ist ohne Smoothing nicht differenzierbar
		fTHROW(ExprTreeException);
		break;
	case et_op_min:
		// "min" ist ohne Smoothing nicht differenzierbar
		fTHROW(ExprTreeException);
		break;
	case et_op_sqrt:
		// sqrt(f(x)) => f'(x)/(2*sqrt(f(x))) = (f'(x)/(2*sqrt(f(x))))
		Dx = div(
			Lval()->deval(x,dsymmap),
			mul(val(2),sqrt(clone(Lval())))
			);
		break;
	case et_op_log:
		// log(f(x)) => f'(x)/f(x)
		Dx = div(
			Lval()->deval(x,dsymmap),
			clone(Lval())
			);
		break;
	case et_op_log2:
		// log2(f(x)) => f'(x)/(f(x)*log(2))
		Dx = div(
			Lval()->deval(x,dsymmap),
			EN_(mul(clone(Lval()),log(val(2.))))
			);
		break;
	case et_op_log10:
		// log10(f(x)) => f'(x)/(f(x)*log(10))
		Dx = div(
			Lval()->deval(x,dsymmap),
			EN_(mul(clone(Lval()),log(val(10.))))
			);
		break;
	case et_op_sqr:
		// (f(x))^2 => 2*f(x)*f'(x)
		Dx = mul(
			val(2),
			EN_(mul(
				clone(Lval()),
				Lval()->deval(x,dsymmap)
				)
			));
		break;
  case et_op_sin:
    // sin(f(x)) => f'(x)*cos(f(x))
    Dx = mul(
         Lval()->deval(x,dsymmap),
         EN_(cos(clone(Lval())))
         );
    break;
  case et_op_cos:
    // cos(f(x)) => -f'(x)*cos(f(x))
    Dx = minus(
         EN_(mul(
           Lval()->deval(x,dsymmap),
           EN_(cos(clone(Lval())))
         )));
    break;
	case et_op_diff:
		// d(diff(f,y))/dx => diff(diff(f,y),x)
		Dx = diff(clone(this),x);
		break;
	case et_literal:
	case et_variable:
		// kann hier nicht mehr vorkommen
		fASSERT_NONREACHABLE();
	}
	
	fASSERT( Dx != 0 );
	// lokale Optimierung:
	Dx->evalNode(true,dsymmap,true);
	return Dx;
}

/*
 * Komprimiert Bäume von Multiplikationen und Divisionen
 *
 *             '/'
 *             / \                   '/'
 *    (oben) '/'  d (unten)          / \
 *           / \              =>    *   *
 *   (oben) c  '/' (unten)         / \ / \
 *             / \                 c b d a
 *    (unten) a   b (oben)
 *
 */
void ExprTree::compressMulDiv()
{
	// Diese Funktion wird nur auf Multiplikations- oder Divisions-
	// Operatoren ausgeführt:
	if (node_type_ != et_op_mul and node_type_ != et_op_div)
		return;

	// die Listen "over" und "under" enthalten die Operatorknoten
	// oder Blätter die später oberhalb und unterhalb eines
	// Bruchstriches stehen:
	list< ExprTree* > over, under;

	// Eine Queue für die Breitensuche. Ein Element besteht aus einem
	// Zeiger auf einen Knoten und einem bool-Wert. Ist der bool-Wert
	// true, dann wird der Knoten zunächst über den Bruchstrich geschrieben,
	// sonst unter den Bruchstrich.
	queue< pair<ExprTree*,bool> > S;

	ExprTree *l=0, *r=0;
	list< ExprTree* >::iterator h,i,j,k;
	ExprTree *lits;

	//
	// Initialisierung: Operatoren am Wurzelknoten werden in die
	// Breitensuche-Queue geschrieben; Blätter am Wurzelknoten
	// werden gleich über / unter den Bruchstich gezogen:
	//

	if (Lval()->node_type_ == et_op_div or Lval()->node_type_ == et_op_mul)
		// ein linker Operator steht zunächst über den Bruchstrich
		S.push( pair<ExprTree*,bool>(Lval(),true) );
	else
		// ein linker Wert steht grundsätzlich über dem Bruchstrich
		over.push_back(Lval());
	
	if (Rval()->node_type_ == et_op_div or Rval()->node_type_ == et_op_mul)
		// ein rechter Operator steht grundsätzlich über dem Bruchstrich,
		// wenn der Wurzel-Operator ein Multiplikationsoperator ist
		S.push( pair<ExprTree*,bool>(Rval(), node_type_ == et_op_mul) );
	else
	{
		// ein rechter Wert steht über dem Bruchstrich, wenn der Wurzel-
		// Operator ein Multiplikationsoperator ist - ansonsten unter
		// dem Bruchstrich
		if (node_type_ == et_op_mul)
			over.push_back(Rval());
		else
			under.push_back(Rval());
	}

	//
	// Breitensuche: Eintragung einer Unterbaum-Knoten in die Listen "over"
	// und "under".
	// 
	while (not S.empty())
	{
		pair<ExprTree*,bool> v = S.front();
		S.pop();

		fASSERT(v.first->node_type_ == et_op_mul or v.first->node_type_ == et_op_div);
		
		l = v.first->Lval();
		r = v.first->Rval();

		if (l->node_type_ == et_op_div or l->node_type_ == et_op_mul)
		{
			// Ist der linke Sohn von "v" ein Operator, so wird an in die
			// Breitensuche-Queue angehängt. Ein linker Sohn übernimmt
			// grundsätzlich das Flag von seinem Vater "v":
			S.push( pair<ExprTree*,bool>(l, v.second) );
		}
		else
		{
			// Wenn der linke Sohn *kein* Mult.- oder Div.-Operator ist,
			// dann handelt es sich um einen Wert, der mit Hilfe des Flags
			// von Vater "v" über oder unter den Bruchstrich gezogen werden
			// kann.
			if (v.second == true)
				over.push_back(l);
			else
				under.push_back(l);
		}

		if (r->node_type_ == et_op_div or r->node_type_ == et_op_mul)
		{
			// Ist der rechte Sohn von "v" ein Operator, so wird das Flag
			// des Vaters nur dann übernommen, wenn es sich beim Vater um
			// einen Multiplikationsoperator handelt; ansonsten wird das
			// Flag negiert:
			if (v.first->node_type_ == et_op_mul)
				S.push( pair<ExprTree*,bool>(r, v.second) );
			else
				// Bei Division wird das Flag des Vaters rechts invertiert
				S.push( pair<ExprTree*,bool>(r, not v.second) );
		}
		else
		{
			// Wenn der rechte Sohn *kein* Mult.- oder Div.-Operator ist,
			// dann handelt es sich um einen Wert oder anderen Operator,
			// der mit Hilfe des Flags von Vater "v" über oder unter den
			// Bruchstrich gezogen werden kann.
			if (v.first->node_type_ == et_op_mul)
			{
				// bei einer Multiplikation wird das Flag des Vaters
				// weitergeleitet:
				if (v.second == true)
					over.push_back(r);
				else
					under.push_back(r);
			}
			else
			{
				// bei einer Division wird das Flag des Vaters invertiert:
				if (v.second == true)
					under.push_back(r);
				else
					over.push_back(r);
			}
		}

		// Der linke und rechte Nachfolger von "v" wurde an diesem Punkt
		// entweder in die Queue geschrieben, falls es sich um einen Mult.- oder
		// Div.- Operator gehandelt hat. Waren die Nachfolger Blätter oder andere
		// Operatoren, so wurden sie in "over" oder "under" geschrieben.
		//
		// "v" selbst wird nicht mehr benötigt. Vor dem Löschen von "v" müssen
		// linker und rechter Sohn von "v" auf 0 gesetzt werden, damit der
		// Destructor nicht die Unterbäume löscht:
		v.first->Lval(0);
		v.first->Rval(0);
		delete v.first;		
	}

	//
	// Zusammenfassen von Konstanten (Zähler)
	//
	i = over.begin();
	lits = 0;
	while (i != over.end())
	{
		if ((*i)->node_type_ == et_literal) // and IS_INT((*i)->value_.lit_value_)
		{
			if (!lits)
			{
				lits = *i;
				i++;
			}
			else
			{
				lits->value_.lit_value_ *= (*i)->value_.lit_value_;
				k = i++;
				delete *k;
				over.erase(k);
			}
		}
		else i++;
	}

	//
	// Zusammenfassen von Konstanten (Nenner)
	//
	i = under.begin();
	lits = 0;
	while (i != under.end())
	{
		if ((*i)->node_type_ == et_literal) // and IS_INT((*i)->value_.lit_value_)
		{
			if (!lits)
			{
				lits = *i;
				i++;
			}
			else
			{
				lits->value_.lit_value_ *= (*i)->value_.lit_value_;
				k = i++;
				delete *k;
				under.erase(k);
			}
		}
		else i++;
	}

	//
	// Kürzen von identischen Ausdrücken im Zähler / Nenner
	//
	bool next_pair;
	i = over.begin();
	while (i != over.end())
	{
		next_pair = true;

		// steht eine 0 im Zähler?
		if ((*i)->isLiteral() and (*i)->value_.lit_value_ == 0.)
		{
			// dann wird der ganze Ausdruck 0
			while (over.size())
			{
				lits = over.front();
				delete lits;
				over.pop_front();
			}
			while (under.size())
			{
				lits = under.front();
				delete lits;
				under.pop_front();
			}
			node_type_ = et_literal;
			value_.lit_value_ = 0.;
			return;
		}

		j = under.begin();
		while (j != under.end())
		{
			if ((*j)->isLiteral() && (*j)->value_.lit_value_ == 0.)
			{
				// der komplette Nenner wird 0
				while (under.size()>0)
				{
					lits = under.front();
					delete lits;
					under.pop_front();
				}
				under.push_back(val(0));
				// das Kürzen ist beendet ...
				goto cMD_abort_loop;
			}
			
			// Kürzen von Literalbrüchen:
			if ((*i)->isLiteral() and (*j)->isLiteral()
				&& IS_INT((*i)->value_.lit_value_)
				&& IS_INT((*j)->value_.lit_value_))
			{
				int64_t ltrunc = (int64_t)(*i)->value_.lit_value_;
				int64_t rtrunc = (int64_t)(*j)->value_.lit_value_;
				int64_t gcd = GCD(ltrunc,rtrunc);

				// wenn gcd==-1, war ltrunc oder rtrunc = 0.
				// dieser Fehler hätte früher abgefangen
				// werden müssen
				fASSERT(gcd != -1);
				
				(*i)->value_.lit_value_ = ltrunc / gcd;
				(*j)->value_.lit_value_ = rtrunc / gcd;

				// Der Bruch i/j wurde gekürzt; sowohl i, als auch j
				// bleiben erhalten => next_pair wird auf true
				// gesetzt
				next_pair = true;
			}
			else if (**i == **j) // Kürzen von strukturell gleichen Ausdrücken
			{
				h = j++;
				// es ist wichtig das ZUERST delete aufgerufen wird,
				// da der Iterator sonst ungültig wird:
				delete *h;
				under.erase(h);
				k = i++;
				delete *k;
				over.erase(k);

				// weil *i und *j weggekürzt werden und bereits
				// hier inkrementiert wurden, dürfen sie nicht
				// noch einmal inkrementiert werden:
				next_pair = false;

				if (over.empty() && under.empty())
				{
					// es hat sich alles weggekürzt
					node_type_ = et_literal;
					value_.lit_value_ = 1.;
					return;
				}
				else if (over.empty())
				{
					over.push_back(val(1));
					i = over.begin();
				}
			}
			else
			{
				// es konnte nichts gekürzt werden:
				next_pair = true;
			}

			if (next_pair)
				j++;
			else
				// durch das Kürzen wurde i bereits inkrementiert
				// und der ursprungliche Wert hinter i ist nicht
				// mehr vorhanden - die j-Schleife wird abgebrochen
				break;
		}

		// soll i inkrementiert werden?
		if (next_pair) i++;
	}
cMD_abort_loop:
	;

/*
	list<ExprTree*>::iterator li;
	for (li=over.begin(); li!=over.end(); li++)
		cout << (*li) << ", ";
	cout << endl << "---" << endl;
	for (li=under.begin(); li!=under.end(); li++)
		cout << (*li) << ", ";
	cout << endl << "---" << endl;
*/

	// Sortieren
	over.sort(et_list_keycomp);
	under.sort(et_list_keycomp);

	//
	// Aufbau der neuen linken und rechten Söhne:
	//
	if (under.empty()) // passiert, wenn nur Multiplikationen vorhanden ...
	{
		// es kann passieren, dass durch das Zusammenfassen nur noch
		// ein einziger Wert in "over" steht (z.B. bei "(a+a)/((a+a)/(b+b))" das
		// "b+b"). In diesem Fall wird der Wurzelknoten von einem Operatorknoten
		// in einen Blattknoten oder einen Operatorknoten umgewandelt:
		if (over.size() == 1)
		{
			l = over.front();
			over.pop_front();

			// l ist ein Literalknoten oder ein Variablen-Knoten
			node_type_ = l->node_type_;
			switch (l->node_type_)
			{
			case et_literal:
				value_ = l->value_;
				delete l;
				break;
			case et_variable:
				value_.var_name_ = strdup_alloc(l->value_.var_name_);
				delete l;
				break;
			default: // Operator
				value_ = l->value_;
				Lval(l->Lval());
				Rval(l->Rval());
				l = Lval();
				r = Rval();
				break;
			}
		}
		else
		{
			node_type_ = et_op_mul;
			l = over.front();
			over.pop_front();

			while (over.size() > 1)
			{
				l = new ExprTree(et_op_mul, l, over.front());
				over.pop_front();
			}
			r = over.front();
			over.pop_front();
		}
	}
	else // Multiplikationen und Divisionen
	{
		// falls nur noch Werte im Zahler und Nenner übrig sind:
		if (over.size() == 1 && under.size() == 1)
		{
			l = over.front();
			r = under.front();

			if (l->isLiteral() && r->isLiteral())
			{
				over.pop_front();
				under.pop_front();

				node_type_ = et_literal;
				value_.lit_value_ = l->value_.lit_value_ / r->value_.lit_value_;
				delete l; delete r;
				return;
			}
		}

		// ansonsten wird der aktuelle Knoten in einen Divisions-Operator
		// umgewandelt und es werden Zähler und Nenner aufgebaut:
		
		node_type_ = et_op_div;
		l = over.front();
		over.pop_front();

		while (over.empty() == false)
		{
			l = new ExprTree(et_op_mul, l, over.front());
			over.pop_front();
		}

		r = under.front();
		under.pop_front();

		while (under.empty() == false)
		{
			r = new ExprTree(et_op_mul, r, under.front());
			under.pop_front();
		}
	}

	// neuen linken und rechten Sohn zuweisen:
	if (isOperator())
	{
		Lval(l);
		Rval(r);
	}
	rebuildHashValue();
}

/*
 * Wie oben, nur mit Additionen und Subtraktionen
 */
void ExprTree::compressAddSub()
{
	// Diese Methode wird nur auf Addition, Subtraktion, oder
	// unärem Minus ausgeführt:
	if ( not (IS_OP_ADD(this) or IS_OP_SUB(this) or IS_OP_UMINUS(this)) )
		return;

	// die Listen "plus" und "minus" enthalten die Operatorknoten
	// oder Blätter die später addiert oder subtrahiert werden:
	list< ExprTree* > plus, minus;

	// Eine Queue für die Breitensuche. Ein Element besteht aus einem
	// Zeiger auf einen Knoten und einem bool-Wert. Ist der bool-Wert
	// true, dann wird der Knoten zunächst mit positiven Vorzeichen
	// gewertet, sonst mit negativem Vorzeichen:
	queue< pair<ExprTree*,bool> > S;

	ExprTree *l=0, *r=0;
	list< ExprTree* >::iterator h,i,j,k;
	ExprTree *lits;

	//
	// Initialisierung: Operatoren am Wurzelknoten werden in die
	// Breitensuche-Queue geschrieben; Blätter am Wurzelknoten
	// werden gleich in plus / minus eingetragen:
	//
	switch (node_type_)
	{
	case et_op_add:
		S.push( pair< ExprTree*,bool >(Lval(),true) );
		S.push( pair< ExprTree*,bool >(Rval(),true) );
		break;
	case et_op_sub:
		S.push( pair< ExprTree*,bool >(Lval(),true) );
		S.push( pair< ExprTree*,bool >(Rval(),false) );
		break;
	case et_op_uminus:
		S.push( pair< ExprTree*,bool >(Lval(),false) );
		break;
	default:
		fASSERT_NONREACHABLE();
		break;
	}

	while (not S.empty())
	{
		pair<ExprTree*,bool> v = S.front();
		S.pop();
		ExprTree * E = v.first;
		bool sign = v.second;

		if (E->isOperator())
		{
			switch (E->node_type_)
			{
			case et_op_add:
				S.push( pair< ExprTree*,bool >(E->Lval(),sign) );
				S.push( pair< ExprTree*,bool >(E->Rval(),sign) );
				E->Lval(0); E->Rval(0);
				delete E;
				continue;
			case et_op_sub:
				S.push( pair< ExprTree*,bool >(E->Lval(),sign) );
				S.push( pair< ExprTree*,bool >(E->Rval(),not sign) );
				E->Lval(0); E->Rval(0);
				delete E;
				continue;
			case et_op_uminus:
				S.push( pair< ExprTree*,bool >(E->Lval(),not sign) );
				E->Lval(0);
				delete E;
				continue;
			default:
				break;
			}
		}

		// Blatt oder anderer Operator
		if (sign)
			plus.push_back(E);
		else
			minus.push_back(E);
	}

	//
	// Zusammenfassen von Konstanten (Plus-Seite)
	//
	i = plus.begin();
	lits = 0;
	while (i != plus.end())
	{
		if ((*i)->isLiteral() && IS_INT((*i)->value_.lit_value_))
		{
			if (!lits)
			{
				lits = *i;
				i++;
			}
			else
			{
				lits->value_.lit_value_ += (*i)->value_.lit_value_;
				k = i++;
				delete *k;
				plus.erase(k);
			}
		}
		else i++;
	}

	//
	// Zusammenfassen von Konstanten (Minus-Seite)
	//
	i = minus.begin();
	lits = 0;
	while (i != minus.end())
	{
		if ((*i)->isLiteral() && IS_INT((*i)->value_.lit_value_))
		{
			if (!lits)
			{
				lits = *i;
				i++;
			}
			else
			{
				lits->value_.lit_value_ += (*i)->value_.lit_value_;
				k = i++;
				delete *k;
				minus.erase(k);
			}
		}
		else i++;
	}

	//
	// Auslöschen von identischen Ausdrücken im Plus- und Minus-Teil
	//
	bool next_pair;
	i = plus.begin();
	while (i != plus.end())
	{
		next_pair = true;

		j = minus.begin();
		while (j != minus.end())
		{	
			// Subtrahieren von Literalen:
			if ((*i)->isLiteral() && (*j)->isLiteral()
				&& IS_INT((*i)->value_.lit_value_)
				&& IS_INT((*j)->value_.lit_value_))
			{
				int64_t ltrunc = (int64_t)(*i)->value_.lit_value_;
				int64_t rtrunc = (int64_t)(*j)->value_.lit_value_;
				int64_t sum = ltrunc - rtrunc;

				if (sum < 0)
				{
					k = i++;
					delete *k;
					plus.erase(k);
					(*j)->value_.lit_value_ = -sum;
					j++;
					next_pair = false;
				}
				else if (sum == 0)
				{
					h = j++;
					delete *h;
					minus.erase(h);
					k = i++;
					delete *k;
					plus.erase(k);

					// weil *i und *j weggelöscht werden und bereits
					// hier inkrementiert wurden, dürfen sie nicht
					// noch einmal inkrementiert werden:
					next_pair = false;
				}
				else // sum > 0
				{
					h = j++;
					delete *h;
					minus.erase(h);
					(*i)->value_.lit_value_ = sum;
					i++;
					next_pair = false;
				}
			}
			else if (**i == **j) // Kürzen von strukturell gleichen Ausdrücken
			{
				h = j++;
				delete *h;
				minus.erase(h);
				k = i++;
				delete *k;
				plus.erase(k);

				// weil *i und *j weggekürzt werden und bereits
				// hier inkrementiert wurden, dürfen sie nicht
				// noch einmal inkrementiert werden:
				next_pair = false;

				if (plus.empty() && minus.empty())
				{
					// es hat sich alles weggelöscht
					node_type_ = et_literal;
					value_.lit_value_ = 0.;
					return;
				}
				else if (plus.empty())
				{
					/*
					plus.push_back(new Val(0));
					i = plus.begin();
					*/
					goto cAS_abort_loop;
				}
			}
			else
			{
				// es konnte nichts gekürzt werden:
				next_pair = true;
			}

			if (next_pair)
				j++;
			else
				// durch das Kürzen wurde i bereits inkrementiert
				// und der ursprungliche Wert hinter i ist nicht
				// mehr vorhanden - die j-Schleife wird abgebrochen
				break;
		}

		// soll i inkrementiert werden?
		if (next_pair) i++;
	}
cAS_abort_loop:
	;

	// Sortieren
	plus.sort(et_list_keycomp);
	minus.sort(et_list_keycomp);

	// positive und negative Anteile zusammenbringen
	if (plus.empty() && minus.empty())
	{
		node_type_ = et_literal;
		value_.lit_value_ = 0.;
	}
	else if (plus.empty() && not minus.empty())
	{
		if (minus.size() == 1) // erzeuge ein unäres Minus
		{
			l = minus.front();
			minus.pop_front();
			r = 0;
			node_type_ = et_op_uminus;
		}
		else // erzeuge eine linke Kette von Subtraktionen
		{
			l = new ExprTree(et_op_uminus, minus.front());
			minus.pop_front();
			while (minus.size() > 1)
			{
				l = new ExprTree(et_op_sub, l, minus.front());
				minus.pop_front();
			}
			r = minus.front();
			minus.pop_front();
			node_type_ = et_op_sub;
		}
	}
	else if (not plus.empty() && minus.empty())
	{
		// es kann passieren, dass durch das Zusammenfassen nur noch
		// ein einziger Wert in "plus" steht (z.B. bei "a*a-(a*a-b*b)"
		// ein "b*b"). In diesem Fall wird der Wurzelknoten von einem
		// Operatorknoten in einen Blattknoten oder Operatorknoten umgewandelt:
		if (plus.size() == 1)
		{
			l = plus.front();
			plus.pop_front();

			// l ist ein Literalknoten oder ein Variablen-Knoten
			node_type_ = l->node_type_;
			switch (l->node_type_)
			{
			case et_literal:
				value_ = l->value_;
				delete l;
				break;
			case et_variable:
				value_.var_name_ = strdup_alloc(l->value_.var_name_);
				delete l;
				break;
			default:
				value_ = l->value_;
				Lval(l->Lval());
				Rval(l->Rval());
				l = Lval();
				r = Rval();
				break;
			}
		}
		else
		{
			l = plus.front();
			plus.pop_front();
			while (plus.size() > 1)
			{
				l = new ExprTree(et_op_add, l, plus.front());
				plus.pop_front();
			}
			r = plus.front();
			plus.pop_front();

			node_type_ = et_op_add;
		}
	}
	else // weder plus noch minus ist leer
	{
		l = plus.front();
		plus.pop_front();
		while (not plus.empty())
		{
			l = new ExprTree(et_op_add, l, plus.front());
			plus.pop_front();
		}
		while (minus.size() > 1)
		{
			l = new ExprTree(et_op_sub, l, minus.front());
			minus.pop_front();
		}
		r = minus.front();
		minus.pop_front();

		node_type_ = et_op_sub;
	}
	
	// neuen linken und rechten Sohn zuweisen:
	if (isOperator())
	{
		Lval(l);
		Rval(r);
	}
	rebuildHashValue();
}

ExprTree * ExprTree::evalNode(
	bool force,
	charptr_map< charptr_array > const * dsymmap,
	bool deleteChilds)
{
	if (not isOperator())
		return this;
	
	// Operator diff
	if (node_type_ == et_op_diff)
	{
		// muss immer gelten:
		if (not Rval()->isVariable())
			fTHROW(ExprTreeException);
		
		// diff([literal],v) = 0
		if (Lval()->isLiteral())
		{
			if (deleteChilds)
			{
				delete Lval();
				delete Rval();
			}
			node_type_ = et_literal;
			value_.lit_value_ = 0.;
			return this;
		}

		if (Lval()->isVariable())
		{
			// diff(v,v) = 1
			if (strcmp(Lval()->getVarName(),Rval()->getVarName())==0)
			{
				if (deleteChilds)
				{
					delete Lval();
					delete Rval();
				}
				node_type_ = et_literal;
				value_.lit_value_ = 1.;
				return this;
			}

			// falls es in dsymmap keinen Eintrag für Lval->getVarName()
			// gibt, dann hängt Lval->getVarName() nicht von
			// Rval->getVarName() ab und wird durch die Ableitung zu 0.
			charptr_array * dsyms = 0;
			if (dsymmap)
				dsyms = dsymmap->findPtr(Lval()->getVarName());
			if (dsyms == 0 or dsyms->find(Rval()->getVarName()) == dsyms->end())
			{
				if (deleteChilds)
				{
					delete Lval();
					delete Rval();
				}
				node_type_ = et_literal;
				value_.lit_value_ = 0.;
				return this;
			}
		}
		return this;
	}

	// Binärer Operator mit zwei Literalen oder
	// unärer Operator mit einem Literal
	if (Lval()->isLiteral() and (Rval() == 0 or Rval()->isLiteral()))
	{
		ExprTree * lval = Lval(), * rval = Rval();
		bool keep = false;
		switch (node_type_)
		{
		case et_op_add:
			fASSERT( Rval() );
			value_.lit_value_ = Lval()->value_.lit_value_
						+ Rval()->value_.lit_value_;
			break;
		case et_op_sub:
			fASSERT( Rval() );
			value_.lit_value_ = Lval()->value_.lit_value_
						- Rval()->value_.lit_value_;
			break;
		case et_op_pow:
			fASSERT( Rval() );
			value_.lit_value_ = ::pow(Lval()->value_.lit_value_,
						Rval()->value_.lit_value_);
			break;
		case et_op_uminus:
			value_.lit_value_ = -Lval()->value_.lit_value_;
			break;
		case et_op_mul:
			fASSERT( Rval() );
			value_.lit_value_ = Lval()->value_.lit_value_
						* Rval()->value_.lit_value_;
			break;
		case et_op_div:
			fASSERT( Rval() );
			if (force == false
				&& IS_INT(Lval()->value_.lit_value_)
				&& IS_INT(Rval()->value_.lit_value_)
				&& fabs(Lval()->value_.lit_value_)<=9.2234E18
				&& fabs(Rval()->value_.lit_value_)<=9.2234E18)
			{
				// Rationalzahl-Division per Bruchrechnung
				int64_t ltrunc = (int64_t)Lval()->value_.lit_value_;
				int64_t rtrunc = (int64_t)Rval()->value_.lit_value_;
				int64_t div = GCD(ltrunc,rtrunc);

				if (div == -1)
				{
					// ltrunc oder rtrunc war 0
					value_.lit_value_ = (double)INFINITY;
				}
				else
				{
					Lval()->value_.lit_value_ = ltrunc / div;
					Rval()->value_.lit_value_ = rtrunc / div;
					if (Rval()->value_.lit_value_ == 1)
					{
						value_.lit_value_ = Lval()->value_.lit_value_;
						keep = false;
					}
					else keep = true;
				}
			}
			else
			{
				// Ist der 64-Bit-Integer ist zu klein, um die
				// double-Zahl aufzunehmen, oder wird es gewünscht
				// mit FP-Arithmetik weiterrechnen:
				value_.lit_value_ = Lval()->value_.lit_value_
							/ Rval()->value_.lit_value_;
			}
			break;
		// Für die Vergleichsoperatoren sind boole'sche Regeln à la MatLab
		// implementiert: Sie lassen sich nur dann auswerten, wenn zwei
		// Literale miteinander verglichen werden
		case et_op_eq:
			value_.lit_value_ =
				(Lval()->value_.lit_value_ == Rval()->value_.lit_value_);
			break;
		case et_op_leq:
			value_.lit_value_ =
				(Lval()->value_.lit_value_ <= Rval()->value_.lit_value_);
			break;
		case et_op_lt:
			value_.lit_value_ =
				(Lval()->value_.lit_value_ < Rval()->value_.lit_value_);
			break;
		case et_op_geq:
			value_.lit_value_ =
				(Lval()->value_.lit_value_ >= Rval()->value_.lit_value_);
			break;
		case et_op_gt:
			value_.lit_value_ =
				(Lval()->value_.lit_value_ > Rval()->value_.lit_value_);
			break;
		case et_op_neq:
			value_.lit_value_ =
				(Lval()->value_.lit_value_ != Rval()->value_.lit_value_);
			break;
		case et_op_abs:
			value_.lit_value_ = ::fabs(Lval()->value_.lit_value_);
			break;
		case et_op_exp:
			value_.lit_value_ = ::exp(Lval()->value_.lit_value_);
			break;
		case et_op_max:
			value_.lit_value_ =
				(Lval()->value_.lit_value_ >= Rval()->value_.lit_value_)
				? Lval()->value_.lit_value_ : Rval()->value_.lit_value_;
			break;
		case et_op_min:
			value_.lit_value_ =
				(Lval()->value_.lit_value_ <= Rval()->value_.lit_value_)
				? Lval()->value_.lit_value_ : Rval()->value_.lit_value_;
			break;
		case et_op_sqrt:
			value_.lit_value_ = ::sqrt(Lval()->value_.lit_value_);
			break;
		case et_op_log:
			value_.lit_value_ = ::log(Lval()->value_.lit_value_);
			break;
                case et_op_sin:
			value_.lit_value_ = ::sin(Lval()->value_.lit_value_);
			break;
                case et_op_cos:
			value_.lit_value_ = ::cos(Lval()->value_.lit_value_);
			break;
		case et_op_log2:
			// im C99-Standard gibt es log2 (unter Cygwin aber nicht)
			value_.lit_value_ = ::log(Lval()->value_.lit_value_)/::log(2.);
			break;
		case et_op_log10:
			// im C99-Standard gibt es log10
			value_.lit_value_ = ::log10(Lval()->value_.lit_value_);
			break;
		case et_op_sqr:
			value_.lit_value_ = Lval()->value_.lit_value_ * Lval()->value_.lit_value_;
			break;
		default:
			fASSERT_NONREACHABLE();
		} // switch (node_type_)

		if (not keep)
		{
			// der Operator-Knoten wird nach dem Auswerten ein Literal-Knoten
			node_type_ = et_literal;
			if (deleteChilds)
			{
				// linken und (bei Bedarf) rechten Sohn löschen
				delete lval;
				if (rval) delete rval;
			}
		}

		rebuildHashValue();
		return this;
	} // if (Lval()->isLiteral() and (Rval() == 0 or Rval()->isLiteral()))

	// Ab hier sind nur noch binäre Operatoren betroffen und es wird
	// davon ausgegangen, dass Rval() != 0 ist.
	if (not isBinaryOp())
		return this;

	fASSERT(Lval());
	fASSERT(Rval());

	// Ein Literal mit Wert 0
	if ((Lval()->isLiteral() and Lval()->value_.lit_value_ == 0.)
		or (Rval()->isLiteral() and Rval()->value_.lit_value_ == 0.))
	{
		ExprType new_node_type_;
		ExprTree * lval = Lval(), * rval = Rval();

		// wenn im Folgenden eine linke oder rechte Seite ein Literal
		// ist, muß es sich dabei auch um die 0 handeln, da der Fall mit
		// zwei Literalen oben schon abgefangen wurde:
		switch (node_type_)
		{
		case et_op_add:
			// a+0 -> 0+a
			if (Rval()->isLiteral())
			{
				swapLvalRval();
				lval = Lval();
				rval = Rval();
			}

			// 0+a = a
			// Kopiere rval nach this
			new_node_type_ = rval->node_type_;
			if (rval->node_type_ == et_variable)
				value_.var_name_ = strdup_alloc(rval->value_.var_name_);
			else
			{
				// tiefe Kopie der union
				value_ = rval->value_;
				// damit der Destructor nicht den Unterbaum löscht ...
				if (deleteChilds)
				{
					rval->Lval(0);
					rval->Rval(0);
				}
			}
			if (deleteChilds)
			{
				delete lval; // 0
				delete rval; // a
			}
			node_type_ = new_node_type_;
			break;
		case et_op_sub:
			if (Lval()->isLiteral()) // 0-a = -a
			{
				if (deleteChilds)
					delete Lval();
				Lval(Rval());
				Rval(0);
				node_type_ = et_op_uminus;
			}
			else // a-0 = a
			{
				new_node_type_ = lval->node_type_;
				if (lval->node_type_ == et_variable)
					value_.var_name_ = strdup_alloc(lval->value_.var_name_);
				else
				{
					// s.o.
					value_ = lval->value_;
					if (deleteChilds)
					{
						lval->Lval(0);
						lval->Rval(0);
					}
				}
				if (deleteChilds)
				{
					delete lval;
					delete rval;
				}
				node_type_ = new_node_type_;
			}
			break;
		case et_op_mul:
			// irgendwas mal 0 ergibt 0
			if (deleteChilds)
			{
				delete lval;
				delete rval;
			}
			node_type_ = et_literal;
			value_.lit_value_ = 0.;
			break;
		case et_op_div:
			// 0/a = 0
			if (Lval()->isLiteral())
			{
				if (deleteChilds)
				{
					delete lval;
					delete rval;
				}
				node_type_ = et_literal;
				value_.lit_value_ = 0.;
			}
			else // a/0 = inf :-(
			{
				if (deleteChilds)
				{
					delete lval;
					delete rval;
				}
				node_type_ = et_literal;
				value_.lit_value_ = (double)INFINITY;
			}
			break;
		case et_op_pow:
			// 0^a = 0
			if (Lval()->isLiteral())
			{
				if (deleteChilds)
				{
					delete lval;
					delete rval;
				}
				node_type_ = et_literal;
				value_.lit_value_ = 0.;
			}
			else // a^0 == 1
			{
				if (deleteChilds)
				{
					delete lval;
					delete rval;
				}
				node_type_ = et_literal;
				value_.lit_value_ = 1.;
			}
			break;
		default:
			break;
		} // switch (node_type_)

		rebuildHashValue();
		return this;
	} // Ein Literal mit Wert 0

	// Ein Literal mit Wert 1
	if ((Lval()->isLiteral() and Lval()->value_.lit_value_ == 1.)
		or (Rval()->isLiteral() and Rval()->value_.lit_value_ == 1.))
	{
		ExprType new_node_type_;
		ExprTree * lval = Lval(), * rval = Rval();
		
		// wenn im Folgenden eine linke oder rechte Seite ein Literal
		// ist, muß es sich dabei auch um die 0 handeln, da der Fall mit
		// zwei Literalen oben schon abgefangen wurde:
		switch (node_type_)
		{
		case et_op_mul:
			// irgendwas mal 1 ergibt irgendwas
			// a*1 -> 1*a
			if (Rval()->isLiteral())
			{
				swapLvalRval();
				lval = Lval();
				rval = Rval();
			}

			// 1*a = a
			// Kopiere rval nach this
			new_node_type_ = rval->node_type_;
			if (rval->node_type_ == et_variable)
				value_.var_name_ = strdup_alloc(rval->value_.var_name_);
			else
			{
				// tiefe Kopie der union
				value_ = rval->value_;
				// damit der Destructor nicht den Unterbaum löscht ...
				if (deleteChilds)
				{
					rval->Lval(0);
					rval->Rval(0);
				}
			}
			if (deleteChilds)
			{
				delete lval;
				delete rval;
			}
			node_type_ = new_node_type_;
			break;
		case et_op_div:
			if (Rval()->isLiteral()) // a/1 = a
			{
				new_node_type_ = lval->node_type_;
				if (lval->node_type_ == et_variable)
					value_.var_name_ = strdup_alloc(lval->value_.var_name_);
				else
				{
					// s.o.
					value_ = lval->value_;
					if (deleteChilds)
					{
						lval->Lval(0);
						lval->Rval(0);
					}
				}
				if (deleteChilds)
				{
					delete lval;
					delete rval;
				}
				node_type_ = new_node_type_;
			}
			break;
		case et_op_pow:
			// 1^a = 1
			if (Lval()->isLiteral())
			{
				if (deleteChilds)
				{
					delete lval;
					delete rval;
				}
				node_type_ = et_literal;
				value_.lit_value_ = 1.;
			}
			else // a^1 == a
			{
				// Kopiere lval nach this
				new_node_type_ = lval->node_type_;
				if (lval->node_type_ == et_variable)
					value_.var_name_ = strdup_alloc(lval->value_.var_name_);
				else
				{
					// tiefe Kopie der union
					value_ = lval->value_;
					// damit der Destructor nicht den Unterbaum löscht ...
					if (deleteChilds)
					{
						lval->Lval(0);
						lval->Rval(0);
					}
				}
				if (deleteChilds)
				{
					delete lval;
					delete rval;
				}
				node_type_ = new_node_type_;
			}
			break;
		default:
			break;
		} // switch (node_type_)

		rebuildHashValue();
		return this;
	} // Ein Literal mit Wert 1

	// Ein Literal mit Wert -1
	if ((Lval()->isLiteral() and Lval()->value_.lit_value_ == -1.)
		or (Rval()->isLiteral() and Rval()->value_.lit_value_ == -1.))
	{
		// wenn im Folgenden eine linke oder rechte Seite ein Literal
		// ist, muß es sich dabei auch um die 0 handeln, da der Fall mit
		// zwei Literalen oben schon abgefangen wurde:
		switch (node_type_)
		{
		case et_op_mul:
			if (Lval()->isLiteral()) // -1*a = -a
			{
				if (deleteChilds)
					delete Lval();
				node_type_ = et_op_uminus;
				Lval(Rval());
				Rval(0);
			}
			else // a*(-1) = -a
			{
				if (deleteChilds)
					delete Rval();
				Rval(0);
				node_type_ = et_op_uminus;
			}
			break;
		case et_op_div:
			// a/(-1) = -a
			if (Rval()->isLiteral())
			{
				if (deleteChilds)
					delete Rval();
				Rval(0);
				node_type_ = et_op_uminus;
			}
			break;
		case et_op_pow:
			if (Rval()->isLiteral()) // a^(-1) == 1/a
			{
				node_type_ = et_op_div;
				Rval()->value_.lit_value_ = 1.;
				swapLvalRval();
			}
			break;
		default:
			break;
		} // switch (node_type_)

		rebuildHashValue();
		return this;
	} // Ein Literal mit -1

	// sonstige:
	// a / a^2 (tritt nach diff auf)
	if (node_type_ == et_op_div and Lval()->isLeaf()
		and IS_OP_SQR(Rval()) and *(Lval())==*(Rval()->Lval()))
	{
		ExprTree * L = Lval(), * R = Rval();

		if (L->isLiteral())
		{
			value_.lit_value_ = L->getDoubleValue();
			node_type_ = et_literal;
		}
		else
		{
			value_.var_name_ = strdup_alloc(L->getVarName());
			node_type_ = et_variable;
		}
		if (deleteChilds) { delete L; delete R; }
	}

	return this;
}

/**
 * Partial-Evalutator.
 * Bottom-Up-Auswertung und Zusammenfassung/Vereinfachung von Ausdrücken
 *
 * @param force fp-Auswertung von Brüchen forcieren
 * @param dsymmap Abbildung eines Symbolnamen auf eine Liste von
 * 	Variablennamen (von denen der Symbolname abhängt)
 */
void ExprTree::eval(
	bool force,
	charptr_map< charptr_array > * dsymmap
	)
{
	if (isLeaf()) return;

	// zuerst nach links, dann nach rechts absteigen ...
	Lval()->eval(force,dsymmap);
	if (Rval()) Rval()->eval(force,dsymmap);

	// Zusammenfassung von Multiplikationen und Divisionen
	compressMulDiv();
	// Zusammenfassung von Additionen und Subtraktionen
	compressAddSub();
	
	// lokale Auswertung
	evalNode(force,dsymmap,true);

	// Ableitungen auswerten / vereinfachen
	if (node_type_ == et_op_diff)
	{
		// sonst keine Vereinfachung möglich:
		if (not Lval()->isVariable())
		{
			ExprTree * dlval_dv = Lval()->deval(
					Rval()->getVarName(),
					dsymmap
					);
			dlval_dv->eval(force,dsymmap);

			if (*(Lval()) == *dlval_dv)
				// Ergebnis ist strukturell gleich mit
				// Lval() ... hat also nichts gebracht.
				delete dlval_dv;
			else
			{
				// Wert/Childs von dlval_dv übernehmen
				delete Lval();
				delete Rval();
				node_type_ = dlval_dv->node_type_;
				if (node_type_ == et_variable)
					value_.var_name_ = strdup_alloc(dlval_dv->value_.var_name_);
				else
					value_ = dlval_dv->value_;
				// Löschung der übernommenen Childs verhindern:
				if (dlval_dv->isOperator())
				{
					dlval_dv->Lval(0);
					dlval_dv->Rval(0);
				}
				delete dlval_dv;
			}
		} // if (not Lval()->isVariable())
	} // if (node_type_ == et_op_diff)
}


void ExprTree::evalUnaryMinus()
{
	if (isLeaf()) return;

	// zuerst nach links, dann nach rechts absteigen ...
	Lval()->evalUnaryMinus();
	if (Rval()) Rval()->evalUnaryMinus();

	if (IS_OP_UMINUS(this))
	{
		evalNode(false, 0, true);
	}


}

void ExprTree::smoothen(
	ExprTree const * sp
	)
{
	if (isLeaf())
		return;

	// rekursiver Abstieg
	Lval()->smoothen(sp);
	if (Rval()) Rval()->smoothen(sp);

	ExprTree * snode = 0;
	switch (node_type_)
	{
	case et_op_abs:
		snode = abs_alpha(Lval(),clone(sp));
		break;
	case et_op_min:
		snode = min_alpha(Lval(),Rval(),clone(sp));
		break;
	case et_op_max:
		snode = max_alpha(Lval(),Rval(),clone(sp));
		break;
	default:
		return;
	}

	if (snode)
	{
		value_.childs_.Lval_ = snode->value_.childs_.Lval_;
		value_.childs_.Rval_ = snode->value_.childs_.Rval_;
		node_type_ = snode->node_type_;
		value_ = snode->value_;
		snode->Lval(0);
		snode->Rval(0);
		delete snode;
	}
}

bool ExprTree::canonicalize()
{
	// Hashwert invalidieren
	clearAllHashValues();

	// Bei Blättern stoppt der rekursive Abstieg
	if (isLeaf())
		return true;

	// linken und rechten Nachfolger sooft wie möglich auswerten, bis ein
	// Fixpunkt erreicht ist
	while (not Lval()->canonicalize());
	while (not Rval()->canonicalize());

#if 0
	// Rn, n=1..20
	if (regel_anwendbar)
	{
		// wende Regel an ...
		
		// evtl. erneute Regelanwendung möglich
		return false;
	}
#endif

	// R1
	if (IS_OP_ADD(this) and Lval()->isLiteral() and Rval()->isLiteral())
	{
		printf("R1\n");
		node_type_ = et_literal;
		value_.lit_value_ = Lval()->value_.lit_value_ + Rval()->value_.lit_value_;
		delete Lval(); Lval(0);
		delete Rval(); Rval(0);
		return false;
	}

	// R2
	if (IS_OP_ADD(this) and Lval()->isOperator() and Rval()->isLeaf())
	{
		printf("R2\n");
		swapLvalRval();
		return false;
	}

	// R3
	if (IS_OP_MUL(this) and Lval()->isLiteral() and Rval()->isLiteral())
	{
		printf("R3\n");
		node_type_ = et_literal;
		value_.lit_value_ = Lval()->value_.lit_value_ * Rval()->value_.lit_value_;
		delete Lval(); Lval(0);
		delete Rval(); Rval(0);
		return false;
	}
	
	// R4
	if (IS_OP_MUL(this) and Lval()->isOperator() and Rval()->isLeaf())
	{
		printf("R4\n");
		swapLvalRval();
		return false;
	}

	// R5
	if (IS_OP_SUB(this) and Lval()->isLiteral() and Rval()->isLiteral())
	{
		printf("R5\n");
		node_type_ = et_literal;
		value_.lit_value_ = Lval()->value_.lit_value_ - Rval()->value_.lit_value_;
		delete Lval(); Lval(0);
		delete Rval(); Rval(0);
		return false;
	}

	// R6
	if (IS_OP_SUB(this) and Lval()->isOperator() and Rval()->isLeaf())
	{
		printf("R6\n");
		swapLvalRval();
		Lval()->value_.lit_value_ = - Lval()->value_.lit_value_;
		node_type_ = et_op_add;
		return false;
	}

	// R7
	if (IS_OP_ADD(this) and IS_OP_ADD(Rval())
		and Lval()->isOperator() and Rval()->Lval()->isOperator()
		and Rval()->Rval()->isOperator())
	{
		printf("R7\n");
		ExprTree * tmp;
		Rval()->swapLvalRval();
		swapLvalRval();
		tmp = Lval()->Lval();
		Lval()->Lval(Rval());
		Rval(tmp);
		return false;
	}

	// R8
	if (IS_OP_MUL(this) and IS_OP_MUL(Rval())
		and Lval()->isOperator() and Rval()->Lval()->isOperator()
		and Rval()->Rval()->isOperator())
	{
		printf("R8\n");
		ExprTree * tmp;
		Rval()->swapLvalRval();
		swapLvalRval();
		tmp = Lval()->Lval();
		Lval()->Lval(Rval());
		Rval(tmp);
		return false;
	}
	
	// R9
	if (IS_OP_ADD(this) and IS_OP_ADD(Lval())
		and Lval()->Lval()->isLiteral() and Lval()->Rval()->isOperator()
		and Rval()->isLiteral())
	{
		printf("R9\n");
		ExprTree * tmp = Lval()->Rval();
		Lval()->Rval(0);
		Rval()->value_.lit_value_ += Lval()->Lval()->value_.lit_value_;
		delete Lval();
		Lval(tmp);
		swapLvalRval();
		return false;
	}
	
	// R10
	if (IS_OP_MUL(this) and IS_OP_MUL(Lval())
		and Lval()->Lval()->isLiteral() and Lval()->Rval()->isOperator()
		and Rval()->isLiteral())
	{
		printf("R10\n");
		ExprTree * tmp = Lval()->Rval();
		Lval()->Rval(0);
		Rval()->value_.lit_value_ *= Lval()->Lval()->value_.lit_value_;
		delete Lval();
		Lval(tmp);
		swapLvalRval();
		return false;
	}

	// R11
	if (IS_OP_MUL(this) and IS_OP_ADD(Lval())
		and Lval()->Lval()->isLiteral() and Lval()->Rval()->isOperator()
		and Rval()->isLiteral())
	{
		printf("R11\n");
		double c2 = Rval()->value_.lit_value_;
		Rval()->value_.lit_value_ *= Lval()->Lval()->value_.lit_value_;
		Lval()->Lval()->value_.lit_value_ = c2;
		swapLvalRval();
		node_type_ = et_op_add;
		Rval()->node_type_ = et_op_mul;
		return false;
	}
	
	// R12
	if (IS_OP_MUL(this) and IS_OP_ADD(Rval())
		and Lval()->isLiteral() and Rval()->Lval()->isLiteral()
		and Rval()->Rval()->isOperator())
	{
		printf("R12\n");
		double c1 = Rval()->Lval()->value_.lit_value_;
		Lval()->value_.lit_value_ *= Rval()->Lval()->value_.lit_value_;
		Rval()->Lval()->value_.lit_value_ = c1;
		node_type_ = et_op_add;
		Rval()->node_type_ = et_op_mul;
		return false;
	}

	// R13
	if (IS_OP_MUL(this) and IS_OP_ADD(Lval())
		and Rval()->isLiteral() and Lval()->Lval()->isOperator()
		and Lval()->Rval()->isOperator())
	{
		printf("R13\n");
		ExprTree * c = Rval();
		Rval(new ExprTree(et_op_mul,c,Lval()->Rval()));
		Lval()->swapLvalRval();
		Lval()->Lval(c->clone());
		Lval()->node_type_ = et_op_mul;
		node_type_ = et_op_add;
		return false;
	}

	// R14
	if (IS_OP_MUL(this) and IS_OP_ADD(Rval())
		and Lval()->isLiteral() and Rval()->Lval()->isOperator()
		and Rval()->Rval()->isOperator())
	{
		printf("R14\n");
		ExprTree * c = Lval();
		Lval(new ExprTree(et_op_mul,c,Rval()->Lval()));
		Rval()->Lval(c->clone());
		Rval()->node_type_ = et_op_mul;
		node_type_ = et_op_add;
		return false;
	}

	// R15
	if (IS_OP_MUL(this) and IS_OP_SUB(Lval())
		and Lval()->Lval()->isOperator() and Lval()->Rval()->isOperator()
		and Rval()->isLiteral())
	{
		printf("R15\n");
		ExprTree * c = Rval();
		Rval(new ExprTree(et_op_mul,c,Lval()->Rval()));
		Lval()->swapLvalRval();
		Lval()->Lval(c->clone());
		Lval()->node_type_ = et_op_mul;
		node_type_ = et_op_sub;
		return false;
	}
	
	// R16
	if (IS_OP_MUL(this) and IS_OP_SUB(Rval()) and Lval()->isLiteral()
		and Rval()->Lval()->isOperator() and Rval()->Rval()->isOperator())
	{
		printf("R16\n");
		ExprTree * c = Lval();
		Lval(new ExprTree(et_op_mul,c,Rval()->Lval()));
		Rval()->Lval(c->clone());
		Rval()->node_type_ = et_op_mul;
		node_type_ = et_op_sub;
		return false;
	}

	// R17
	if (IS_OP_MUL(this) and IS_OP_ADD(Lval())
		and Lval()->Lval()->isOperator() and Lval()->Rval()->isOperator()
		and Rval()->isOperator())
	{
		printf("R17\n");
		ExprTree * t3 = Rval();
		ExprTree * t3c = t3->clone();
		Rval(new ExprTree(et_op_mul,Lval()->Rval(),t3));
		Lval()->Rval(t3c);
		Lval()->node_type_ = et_op_mul;
		node_type_ = et_op_add;
		return false;
	}
	
	// R18
	if (IS_OP_MUL(this) and IS_OP_ADD(Rval())
		and Lval()->isOperator()
		and Rval()->Lval()->isOperator() and Rval()->Rval()->isOperator())
	{
		printf("R18\n");
		ExprTree * t1 = Lval();
		ExprTree * t1c = t1->clone();
		Lval(new ExprTree(et_op_mul,t1,Rval()->Lval()));
		Rval()->Lval(t1c);
		Rval()->node_type_ = et_op_mul;
		node_type_ = et_op_add;
		return false;
	}

	// R19
	if (IS_OP_MUL(this) and IS_OP_SUB(Lval())
		and Lval()->Lval()->isOperator() and Lval()->Rval()->isOperator()
		and Rval()->isOperator())
	{
		printf("R19\n");
		ExprTree * t3 = Rval();
		ExprTree * t3c = t3->clone();
		Rval(new ExprTree(et_op_mul,Lval()->Rval(),t3));
		Lval()->Rval(t3c);
		Lval()->node_type_ = et_op_mul;
		node_type_ = et_op_sub;
		return false;
	}
	
	// R20
	if (IS_OP_MUL(this) and IS_OP_SUB(Rval())
		and Lval()->isOperator()
		and Rval()->Lval()->isOperator() and Rval()->Rval()->isOperator())
	{
		printf("R20\n");
		ExprTree * t1 = Lval();
		ExprTree * t1c = t1->clone();
		Lval(new ExprTree(et_op_mul,t1,Rval()->Lval()));
		Rval()->Lval(t1c);
		Rval()->node_type_ = et_op_mul;
		node_type_ = et_op_sub;
		return false;
	}

	return true;
}

void ExprTree::subst(char const * var, ExprTree * const & newE)
{
	fASSERT( newE );
	subst_descent(var,newE);
	rebuildHashValue();
}

void ExprTree::subst_descent(char const * var, ExprTree * const & newE)
{
	if (isOperator())
	{
		Lval()->subst_descent(var,newE);
		if (Rval()) Rval()->subst_descent(var,newE);
		rebuildHashValue();
	}
	else if (isVariable() and strcmp(var,value_.var_name_)==0)
	{
		delete[] value_.var_name_;
		node_type_ = newE->node_type_;
		
		if (newE->isVariable())
			value_.var_name_ = strdup_alloc(newE->value_.var_name_);
		else
			value_ = newE->value_;

		if (newE->Lval())
			Lval(newE->Lval()->clone());
		if (newE->Rval())
			Rval(newE->Rval()->clone());
		rebuildHashValue();
	}
}

std::string ExprTree::toPrefixString(bool makesrc) const
{
	std::string pstr = makesrc ? nodeToSource() : nodeToString();
	if (isOperator())
	{
		if (isBinaryOp())
		{
			pstr += makesrc ? '(' : ' ';
			pstr += Lval()->toPrefixString(makesrc);
			pstr += makesrc ? ',' : ' ';
			pstr += Rval()->toPrefixString(makesrc);
			if (makesrc) pstr += ')';
		}
		else
		{
			if (makesrc) pstr += '(';
			// uh oh ...
			switch (node_type_)
			{
			case et_op_uminus: // -a => - 0. a
				if (not makesrc)
				{
					pstr += " 0. " + Lval()->toPrefixString(makesrc);
					break;
				}
				break;
			default:
				pstr += Lval()->toPrefixString(makesrc);
				break; 
			}
			if (makesrc) pstr += ')';
		}
	}
	return pstr;
}

charptr_array ExprTree::getVarNames() const
{
	charptr_array vn;
	getVarNames_descent(vn);
	return vn;
}

void ExprTree::getVarNames_descent(charptr_array & vn) const
{
	if (isLeaf())
	{
		if (isVariable()) vn.addUnique(value_.var_name_);
		return;
	}
	if (Lval()) Lval()->getVarNames_descent(vn);
	if (Rval()) Rval()->getVarNames_descent(vn);
}

size_t ExprTree::semanticHashValue() const
{
	ExprTree * copy = clone(this);
	// Partial Evaluation => Ausgewertete "Normalform"
	//   => Semantischer-Hash-Wert
	copy->eval(true); // laufzeit-kritisch (klären!)
	size_t hval = copy->hashValue();
	delete copy;
	return hval;
}

size_t ExprTree::rebuildHashValue() const
{
	if (hval_ == 0)
		hval_ = new size_t;
	*hval_ = 0;

	if (isOperator())
	{
		// Rekursive Berechnung des Hashwertes:
		union { size_t s; char b[SIZEOF_SIZE_T]; } lhval, rhval;
		int i;

		lhval.s = Lval()->hashValue();
		if (Rval())
			rhval.s = Rval()->hashValue();
		else
			rhval.s = ~(lhval.s);

		for (i=0; i<SIZEOF_SIZE_T; i++)
		{
			*hval_ += (lhval.b[i]<<4) + (lhval.b[i]>>4);
			*hval_ *= 11;
		}
		for (i=0; i<SIZEOF_SIZE_T; i++)
		{
			*hval_ += (rhval.b[i]<<4) + (rhval.b[i]>>4);
			*hval_ *= 11;
		}
	}
	else
	{
		if (isLiteral())
		{
			int i;
			char c;
			union {	double d; char b[8]; } lv;
			lv.d = value_.lit_value_;
			*hval_ = 0xE8CED3A;
			for (i=0; i<8; i++)
			{
				c = lv.b[i];
				*hval_ += (c<<4) + (c>>4);
				*hval_ *= 11;
			}
		}
		else
		{
			fASSERT( isVariable() );
			char const *c = value_.var_name_;
			*hval_ = 0xE8CED3A;
			while (*c != '\0')
			{
				*hval_ += ((*c)<<4) + ((*c)>>4);
				*hval_ *= 11;
				c++;
			}
		}
	}
	return *hval_;
}

size_t ExprTree::hashValue() const
{
	// wenn der Hash-Wert schon berechnet wurde, dann wird er einfach
	// zurückgegeben:
	if (hval_ != 0)
		return *hval_;

	// Hash-Wert neu berechnen
	return rebuildHashValue();
}

void ExprTree::clearAllHashValues()
{
	if (Lval()) Lval()->clearAllHashValues();
	if (Rval()) Rval()->clearAllHashValues();
	clearHashValue();
}

void ExprTree::clearHashValue()
{
	if (hval_) { delete hval_; hval_ = 0; }
}

ExprTree * ExprTree::solve0() const
{
	// der Ausdruck muß eine Gleichung sein!
	if (not (isEquality() || isInEquality())) return 0;

	// "L=R" => "L-R=0"
	ExprTree * R = new ExprTree(0.);
	ExprTree * L = new ExprTree(et_op_sub,
			Lval()->clone(),
			Rval()->clone()
			);
	ExprTree * E = new ExprTree(node_type_, L, R);

	// algebraische Vereinfachung von L
	L->simplify();
	return E;
}

bool ExprTree::expand()
{
	// Hashwerte invalidieren
	clearAllHashValues();

	// hier werden nur Operatoren verarbeitet; Blätter werden ignoriert
	if (isLeaf()) return true;
	
	// Operatoren mit Blättern werden ignoriert
	if (Lval() != 0 && Rval() != 0 && Lval()->isLeaf() && Rval()->isLeaf())
		return true;	

	// Rekursiver Abstieg
	if (Lval() != 0)
		while (not Lval()->expand());
	if (Rval() != 0)
		while (not Rval()->expand());

	ExprType newop, op = node_type_;

	// nur Multiplikation, Division und Potenzen werden exandiert:
	if (not (node_type_ == et_op_mul || node_type_ == et_op_div
			|| node_type_ == et_op_pow)) return true;
	
	// Implementierung der Distributivität von Multiplikation und Division:
	// a * ( b +/- c ) => a*b +/- a*c
	if (op == et_op_mul && Lval()->isLeaf() && (IS_OP_ADD(Rval()) || IS_OP_SUB(Rval())))
	{
		ExprTree *a1, *a2, *b, *c;
		a1 = Lval();
		a2 = Lval()->clone();
		b = Rval()->Lval();
		c = Rval()->Rval();
		newop = Rval()->node_type_;
		Rval()->Lval(0);
		Rval()->Rval(0);
		delete Rval();
		Lval(new ExprTree(op, a1, b));
		Rval(new ExprTree(op, a2, c));
		node_type_ = newop;
		return false;
	}

	// (a +/- b) * c => a*c +/- b*c
	if ((op == et_op_mul || op == et_op_div) && Rval()->isLeaf() && (IS_OP_ADD(Lval()) || IS_OP_SUB(Lval())))
	{
		ExprTree *c1, *c2, *a, *b;
		c1 = Rval();
		c2 = Rval()->clone();
		a = Lval()->Lval();
		b = Lval()->Rval();
		newop = Lval()->node_type_;
		Lval()->Lval(0);
		Lval()->Rval(0);
		delete Lval();
		Lval(new ExprTree(op, a, c1));
		Rval(new ExprTree(op, b, c2));
		node_type_ = newop;
		return false;
	}

	// (a +/- b) * (c +/- d) => a*c + a*d +/- b*c +/- b*d
	if (op == et_op_mul &&
		(IS_OP_ADD(Lval()) || IS_OP_SUB(Lval())) &&
		(IS_OP_ADD(Rval()) || IS_OP_SUB(Rval())))
	{
		ExprTree *a1, *a2, *b1, *b2, *c1, *c2, *d1, *d2;
		ExprType op_acad = Rval()->node_type_;
		ExprType op_acadbc = Lval()->node_type_;
		// das Vorzeichen von b*d ergibt sich als Äquivalenz:
		ExprType op_acadbcbd =
			(Rval()->node_type_ == Lval()->node_type_) ? et_op_add : et_op_sub;
		
		a1 = Lval()->Lval();
		a2 = Lval()->Lval()->clone();
		b1 = Lval()->Rval();
		b2 = Lval()->Rval()->clone();
		c1 = Rval()->Lval();
		c2 = Rval()->Lval()->clone();
		d1 = Rval()->Rval();
		d2 = Rval()->Rval()->clone();

		Lval()->Lval(0); Lval()->Rval(0); delete Lval();
		Rval()->Lval(0); Rval()->Rval(0); delete Rval();

		node_type_ = op_acadbcbd;
		Lval(new ExprTree(op_acadbc,
				new ExprTree(op_acad,
					new ExprTree(et_op_mul, a1, c1),
					new ExprTree(et_op_mul, a2, d1)
				),
				new ExprTree(et_op_mul, b1, c2)
			));
		Rval(new ExprTree(et_op_mul, b2, d2));
		return false;
	}

	// (a+b)^c mit c Integer > 1
	if (op == et_op_pow
		&& Rval()->isLiteral() && IS_INT(Rval()->value_.lit_value_) && (Rval()->value_.lit_value_ > 1.)
		&& (IS_OP_ADD(Lval()) || IS_OP_SUB(Lval()))
	)
	{
		ExprTree *a1, *a2, *b1, *b2, *c;
		a1 = Lval()->Lval();
		a2 = Lval()->Lval()->clone();
		b1 = Lval()->Rval();
		b2 = Lval()->Rval()->clone();
		c = Rval();
		newop = Lval()->node_type_;
		Lval()->Lval(0); Lval()->Rval(0); delete Lval();

		c->value_.lit_value_ -= 1;
		Lval(new ExprTree(newop, a1, b1));
		if (c->value_.lit_value_ > 1)
		{
			Rval(new ExprTree(et_op_pow,
				new ExprTree(newop, a2, b2), c));
		}
		else
		{
			Rval(new ExprTree(newop, a2, b2));
			delete c;
		}
		node_type_ = et_op_mul;
		return false;
	}

	// nichts expandiert => Ausdruck ist maximal expandiert ...
	return true;
}

bool ExprTree::rationalize(int maxmag)
{
	bool lr = false, rr = false;

	if (Lval()) lr = Lval()->rationalize();
	if (Rval()) rr = Rval()->rationalize();

	if (isLiteral())
	{
		int64_t n,d;
		::rationalize(getDoubleValue(),n,d);
		if ((n<0?-n:n)<=maxmag and (d<0?-d:d)<=maxmag)
		{
			node_type_ = et_op_div;
			value_.childs_.Lval_ = val(n);
			value_.childs_.Rval_ = val(d);
			return true;
		}
		return false;
	}
	return (lr or rr);
}

size_t ExprTree::size()
{
	if (isLeaf()) return 1;
	return 1 + value_.childs_.Lval_->size()
		+ value_.childs_.Rval_->size();
}

ExprTree * ExprTree::parse(char const * s, int(*scanner)())
{
	ExprTree *root;
	et_inputstring = (char *)s;
	et_inputstring_pos = 0;
	et_lex = scanner ? scanner : et_lex_default;
	if ( (s == 0) || (*s == '\0') )
	{
		fTHROW(ExprParserException,"parse error: input string must not be empty!");
	}
	if ( et_parse() )
	{
		char err[128];
		int rv = snprintf( err,
			sizeof(err) - 1,
			"parse error: failed to evaluate '%s'",
			s );
		fASSERT(rv > 0);
		err[rv] = '\0';
		fTHROW(ExprParserException,err);
	}
	root = et_root;
	et_inputstring = 0;
	et_inputstring_pos = 0;
	et_root = 0;
	et_lex = et_lex_default;
	return root;
}

ExprTree * ExprTree::sym(char const * sfmt, ...)
{
	char buf[256]; // maximale Symbollänge sind 255 Zeichen
	va_list ap;
	va_start(ap,sfmt);
	vsnprintf(buf,sizeof(buf),sfmt,ap);
	va_end(ap);
	return new ExprTree(buf);
}

ExprTree * ExprTree::max_alpha(ExprTree * x, ExprTree * y, ExprTree * a)
{
	// max(x,y) = 0.5*(x+y+|x-y|)
	return mul(val(.5),add(add(x,y),abs_alpha(sub(clone(x),clone(y)),a)));
}

ExprTree * ExprTree::min_alpha(ExprTree * x, ExprTree * y, ExprTree * a)
{
	// min(x,y) = 0.5*(x+y-|x-y|)
	return mul(val(.5),sub(add(x,y),abs_alpha(sub(clone(x),clone(y)),a)));
}

ExprTree * ExprTree::sign_alpha(ExprTree * x, ExprTree * a)
{
	// sign(x,a) = x / sqrt(a+x^2)
	return div(x,sqrt(add(a,sqr(clone(x)))));
}

ExprTree * ExprTree::frac(double fp)
{
	int64_t n,d;
	::rationalize(fp,n,d);
	return div(val(n),val(d));
}

/**
 * Eine Hash-Funktion für ExprTree-Zeiger (klassenlos)
 */
size_t exprptr_hashf(exprptr const & expr)
{
	// strukturellen Hash-Wert berechnen
	return expr->hashValue();
}

} // namespace flux::symb
} // namespace flux

