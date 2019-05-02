#include <cstring>
#include "Combinations.h"
#include "cstringtools.h"
#include "LinearExpression.h"
#include "Constraint.h"

using namespace flux::symb;

namespace flux {
namespace data {

void Constraint::normalize()
{
	try
	{
		LinearExpression lE(constraint_);
		delete constraint_;
		constraint_ = lE.get()->clone();
		// evtl. vorhandenes unäres Minus beseitigen
		constraint_->Rval()->eval(true);

		is_valid_ = true;
		is_simple_ = constraint_->Lval()->isVariable();
		if (is_simple_)
		{
			simple_value_ = constraint_->Rval()->getDoubleValue();
			simple_varname_ = strdup_alloc(constraint_->Lval()->getVarName());
		}
		else
		{
			simple_value_ = 0.;
			simple_varname_ = 0;
		}
	}
	catch (ExprTreeException)
	{
		is_valid_ = false;
	}
}

bool Constraint::checkLinearity(ExprTree const * expr)
{
	if (expr == 0)
		return false;
	if (not (expr->isEquality() or expr->isInEquality()))
		return false;
	try { LinearExpression lE(expr); }
	catch (ExprTreeException) { return false; }
	return true;
}

bool Constraint::checkSimplicity(
	ExprTree const * expr,
	char ** varname,
	double * value
	)
{
	if (expr == 0)
		return false;
	if (not (expr->isEquality() or expr->isInEquality()))
		return false;
	try
	{
		LinearExpression lE(expr);
		charptr_map< double > const & C = lE.getLinearCoeffs();

		// es müssen genau zwei Einträge enthalten sein:
		// Variable und "1" für den Wert
		if (C.size() != 2)
			return false;

		charptr_map< double >::const_iterator coeff_i = C.begin();
			if (strcmp(coeff_i->key,"1")==0)
				coeff_i++;

		if (varname)
			*varname = strdup_alloc(coeff_i->key);
		if (value)
			// variablenname - wert = 0 => Ergebnis negieren
			*value = - *(C.findPtr("1")) / (coeff_i->value);
	}
	catch (ExprTreeException) { return false; }
	return true;
}

bool Constraint::operator==(Constraint const & rval) const
{
	if (parameter_type_ != rval.parameter_type_)
		return false;

	exprptr_eq cmp;
	return cmp(constraint_,rval.constraint_);
}

uint32_t Constraint::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if ((crc_scope & CRC_CONSTRAINTS) == 0)
		return crc;

	char * constr = strdup_alloc(constraint_->toString().c_str());
	if (name_ != 0)
		crc = update_crc32(name_,strlen(name_),crc);
	if (constr != 0)
		crc = update_crc32(constr,strlen(constr),crc);
	uint8_t parameter_type = uint8_t(parameter_type_);
	crc = update_crc32(&parameter_type,1,crc);

	if (is_simple_)
	{
		crc = update_crc32(&simple_value_,sizeof(simple_value_),crc);
		crc = update_crc32(simple_varname_,strlen(simple_varname_),crc);
	}
	delete[] constr;
	return crc;
}

} // namespace flux::data
} // namespace flux

