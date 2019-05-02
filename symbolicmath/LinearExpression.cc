#include <cmath>
#include "Error.h"
#include "ExprTree.h"
#include "charptr_map.h"
#include "LinearExpression.h"

namespace flux {
namespace symb {

void LinearExpression::flipSigns()
{
	charptr_map< double >::iterator ci;
	for (ci=C_.begin(); ci!=C_.end(); ci++)
	{
		if (ci->value != 0.)
			ci->value = - ci->value;
		else
			ci->value = fabs(ci->value);
	}
}

bool LinearExpression::normalizeSigns()
{
	size_t pos=0, neg=0;
	charptr_map< double >::iterator ci;

	for (ci=C_.begin(); ci!=C_.end(); ci++)
	{
		// Vorzeichen der Konstanten nicht mitzählen
		if (strcmp(ci->key,"1") == 0)
			continue;

		if (ci->value < 0.)
			neg++;
		else
			pos++;
	}

	// mehrheit negativ?
	if (neg > pos)
		flipSigns();

	// -0 zu 0 normalisieren
	ci = C_.find("1");
	if (ci->value == 0.)
		ci->value = fabs(ci->value);

	return neg > pos; // wurde geflippt? -> Operator flippen
}

void LinearExpression::rebuildExpression(bool flip_comp_op)
{
	charptr_array vnames = C_.getKeys();
	charptr_array::const_iterator vni;
	ExprTree * sop = 0, * prod = 0, * newEq = 0; // sum of products
	double constval;
	
	// alphanumerische Sortierung der Variablen
	vnames.sort();

	// Wert der Konstanten
	constval = C_["1"];
	if (not is_solvable_ and flip_comp_op)
		constval = -constval;

	for (vni=vnames.begin(); vni!=vnames.end(); ++vni)
	{
		// die Konstante wurde schon berücksichtigt:
		if (strcmp(*vni,"1")==0)
			continue;

		// Koeffizient
		double coeff = C_[*vni];

		if (not is_solvable_ and flip_comp_op)
			coeff = -coeff;

		fASSERT(coeff != 0.);

		prod = ExprTree::sym(*vni);
		if (fabs(coeff) != 1.)
		{
			prod = ExprTree::mul(
				ExprTree::val(fabs(coeff)),
				prod
				);
		}

		if (sop == 0)
		{
			sop = prod;
			if (coeff < 0.)
				sop = ExprTree::minus(sop);
		}
		else
		{
			if (coeff < 0.)
				sop = ExprTree::sub(sop,prod);
			else
				sop = ExprTree::add(sop,prod);
		}
	}

	if (not is_solvable_) // gar keine Gleichung?
	{
		if (sop == 0)
		{
			newEq = ExprTree::val(constval);
		}
		else
		{
			if (constval < 0.)
				newEq = ExprTree::sub(sop,ExprTree::val(-constval));
			else if (constval > 0.)
				newEq = ExprTree::add(sop,ExprTree::val(constval));
			else
				newEq = sop;
		}
		delete Eq_;
		Eq_ = newEq;
		return;
	}

	// Es liegt eine Gleichung vor.
	// die Konstante wird wieder auf die rechte Seite gebracht ...
	ExprTree * Eq_const = ExprTree::val(-constval);
	
	// Muss nach Multiplikation mit -1 der Vergleichsoperator
	// geflippt werden?
	if (flip_comp_op)
	{
		switch (Eq_->getNodeType())
		{
		case et_op_eq:
			newEq = ExprTree::eq(sop,Eq_const);
			break;
		case et_op_neq:
			newEq = ExprTree::neq(sop,Eq_const);
			break;
		case et_op_leq:
			newEq = ExprTree::geq(sop,Eq_const);
			break;
		case et_op_geq:
			newEq = ExprTree::leq(sop,Eq_const);
			break;
		case et_op_lt:
			newEq = ExprTree::gt(sop,Eq_const);
			break;
		case et_op_gt:
			newEq = ExprTree::lt(sop,Eq_const);
			break;
		default:
			fASSERT_NONREACHABLE();
			break;
		}
	}
	else
	{
		if (sop == 0)
			sop = ExprTree::val(0);
		newEq = new ExprTree(Eq_->getNodeType(),sop,Eq_const);
	}
	delete Eq_;
	Eq_ = newEq;
}

void LinearExpression::extractLinearCoeffs(
	ExprTree * E,
	bool positive
	)
{
	charptr_map< double >::iterator ci;

	if ( IS_OP_ADD(E) )
	{
		// Rekursiver Abstieg; die Addition ändert die Vorzeichen
		// nicht.
		extractLinearCoeffs(E->Lval(), positive);
		extractLinearCoeffs(E->Rval(), positive);
	}
	else if ( IS_OP_SUB(E) )
	{
		// Rekursiver Abstieg; auf der rechten Seite der Subtraktion
		// wird das Vorzeichen invertiert.
		extractLinearCoeffs(E->Lval(), positive);
		extractLinearCoeffs(E->Rval(), not positive);
	}
	else if ( IS_OP_UMINUS(E) )
	{
		// Rekursiver Abstieg; das unäre Minus invertiert das
		// Vorzeichen.
		extractLinearCoeffs(E->Lval(), not positive);
	}
	else if ( E->isLeaf() )
	{
		if ( E->isVariable() )
		{
			if ( (ci = C_.find(E->getVarName())) == C_.end() )
				C_.insert( E->getVarName(), positive?1.:-1. );
			else
				ci->value += positive?1.:-1.;
		}
		else // Literal
		{
			fASSERT( E->isLiteral() );
			if ( (ci = C_.find("1")) == C_.end() )
				C_.insert( "1", (positive?1.:-1.) * E->getDoubleValue() );
			else
				ci->value += (positive?1.:-1.) * E->getDoubleValue();
		}
	}
	else if ( IS_OP_MUL(E) )
	{
		double coeff = positive ? 1. : -1.;
		char const * varname = 0;

		if ( E->Lval()->isVariable() && E->Rval()->isLiteral() ) // a*3
		{
			coeff *= E->Rval()->getDoubleValue();
			varname = E->Lval()->getVarName();
		}
		else if ( E->Lval()->isLiteral() && E->Rval()->isVariable() ) // 3*a
		{
			coeff *= E->Lval()->getDoubleValue();
			varname = E->Rval()->getVarName();
		}
		else if ( IS_OP_UMINUS(E->Lval()) && E->Lval()->Lval()->isLiteral()
				&& E->Rval()->isVariable() ) // -3*a
		{
			coeff *= - E->Lval()->Lval()->getDoubleValue();
			varname = E->Rval()->getVarName();
		}
		else if ( E->Lval()->isVariable() && IS_OP_UMINUS(E->Rval())
				&& E->Rval()->Lval()->isLiteral() ) // a*-3
		{
			coeff *= - E->Rval()->Lval()->getDoubleValue();
			varname = E->Lval()->getVarName();
		}
		else if ( IS_OP_UMINUS(E->Lval()) && E->Lval()->Lval()->isVariable()
				&& E->Rval()->isLiteral() ) // -a*3
		{
			coeff *= - E->Rval()->getDoubleValue();
			varname = E->Lval()->Lval()->getVarName();
		}
		else if ( E->Lval()->isLiteral() && IS_OP_UMINUS(E->Rval())
				&& E->Rval()->Lval()->isVariable() ) // 3*-a
		{
			coeff *= - E->Lval()->getDoubleValue();
			varname = E->Rval()->Lval()->getVarName();
		}
		else if ( IS_OP_UMINUS(E->Lval()) && E->Lval()->Lval()->isLiteral()
				&& IS_OP_UMINUS(E->Rval()) && E->Rval()->Lval()->isVariable() ) // -3*-a
		{
			coeff *= E->Lval()->Lval()->getDoubleValue();
			varname = E->Rval()->Lval()->getVarName();
		}
		else if ( IS_OP_UMINUS(E->Lval()) && E->Lval()->Lval()->isVariable()
				&& IS_OP_UMINUS(E->Rval()) && E->Rval()->Lval()->isLiteral() ) // -a*-3
		{
			coeff *= E->Rval()->Lval()->getDoubleValue();
			varname = E->Lval()->Lval()->getVarName();
		}
		else
		{
			 // keine (bekannte) lineare Beziehung
			fTHROW(NonLinearExpressionException);
		}

		if ( (ci = C_.find( varname )) == C_.end() )
			C_.insert( varname, coeff );
		else
			ci->value += coeff;
	}
	else if ( IS_OP_DIV(E) )
	{
		double coeff = positive ? 1. : -1.;
		char const * varname = 0;
		
		if ( E->Lval()->isVariable() && E->Rval()->isLiteral() ) // a/3
		{
			coeff /= E->Rval()->getDoubleValue();
			varname = E->Lval()->getVarName();
		}
		else if ( E->Lval()->isVariable() && IS_OP_UMINUS(E->Rval())
				&& E->Rval()->Lval()->isLiteral() ) // a/-3
		{
			coeff /= - E->Rval()->Lval()->getDoubleValue();
			varname = E->Lval()->getVarName();
		}
		else if ( IS_OP_UMINUS(E->Lval()) && E->Lval()->Lval()->isVariable()
				&& E->Rval()->isLiteral() ) // -a/3
		{
			coeff /= - E->Rval()->getDoubleValue();
			varname = E->Lval()->Lval()->getVarName();
		}
		else if ( IS_OP_UMINUS(E->Lval()) && E->Lval()->Lval()->isVariable()
				&& IS_OP_UMINUS(E->Rval()) && E->Rval()->Lval()->isLiteral()) // -a/-3
		{
			coeff /= E->Rval()->Lval()->getDoubleValue();
			varname = E->Lval()->Lval()->getVarName();
		}
		else
		{
			// keine (bekannte) lineare Beziehung
			fTHROW(NonLinearExpressionException);
		}

		if ( (ci = C_.find( varname )) == C_.end() )
			C_.insert( varname, coeff );
		else
			ci->value += coeff;
	}
}

LinearExpression::LinearExpression(ExprTree const * E)
{
	if (not (E->isEquality() or E->isInEquality()))
	{
		// keine Gleichung; expandieren und vereinfachen:
		Eq_ = E->clone();
		Eq_->simplify();
		is_solvable_ = false;
	}
	else
	{
		// Gleichung nach 0 auflösen, expandieren und
		// vereinfachen ...
		Eq_ = E->solve0();
		is_solvable_ = true;
	}
	
	// wirft NonLinearExpressionException, falls es sich nicht
	// um eine lineare Gleichung / linearen Ausdruck handelt:
	if (is_solvable_)
		extractLinearCoeffs(Eq_->Lval());
	else
		extractLinearCoeffs(Eq_);

	// Falls es sonst keine Literale gibt, gibt es zumindest
	// die 0:
	if (C_.findPtr("1") == 0)
		C_.insert("1",0.);
	// Vorzeichen der Koeffizienten normalisieren
	// den Ausdruck neu aufbauen
	bool signs_flipped = normalizeSigns();
	rebuildExpression(signs_flipped);
	// korrekte Vorzeichen der Koeffizienten wiederherstellen
	if (signs_flipped)
		flipSigns();
}

bool LinearExpression::operator==(LinearExpression const & rval) const
{
	return *Eq_ == *(rval.Eq_);
}

LinearExpression & LinearExpression::operator=(LinearExpression const & rval)
{
	if (Eq_) { delete Eq_; Eq_ = 0; }
	if (rval.Eq_) Eq_ = rval.Eq_->clone();
	C_ = rval.C_;
	return *this;
}

} // namespace flux::symb
} // namespace flux

