#include <limits>
#include "charptr_array.h"
#include "StandardForm.h"
#include "MMatrix.h"
#include "LinearExpression.h"
#include <sstream>

using namespace flux::symb;

namespace flux {
namespace la {

StandardForm::StandardForm(
	MMatrix const & A,
	MVector const & b,
	char const ** vnames
	) : dirty_(true)
{
	size_t i,j;
	charptr_array vnlist;

	fASSERT(A.rows() == b.dim());

	if (vnames == 0)
	{
		for (j=0; j<A.cols(); j++)
			vnlist.add("x%i", int(j));
		vnames = vnlist.get();
	}

	for (i=0; i<A.rows(); i++)
	{
		charptr_map< double > coeffs;
		coeffs["1"] = -b.get(i);
		
		for (j=0; j<A.cols(); j++)
			if (A.get(i,j) != 0.)
				coeffs[vnames[j]] = A.get(i,j);

		addCoeffsUnique(coeffs);
	}
}

bool StandardForm::addCoeffsUnique(charptr_map< double > const & L)
{
	bool add = true, L2R, R2L;
	charptr_map< double >::const_iterator mi;
	double * dptr;

	if (L.size() <= 1)
		return false; // kein Constraint (z.B. 0*y>=2)

	// eine simple Bound?
	if (L.size() == 2)
	{
		char const * bvn = 0;
		double bvv = 0., bvc = 0.;
		for (mi=L.begin(); mi!=L.end(); mi++)
		{
			if (strcmp(mi->key,"1") != 0)
			{
				bvn = mi->key;
				bvc = mi->value;
			}
			else
				bvv = mi->value;
		}
		if (bvc < 0.)
			return setLowerBound(bvn,-bvv/bvc);
		else
			return setUpperBound(bvn,-bvv/bvc);
	}

	std::list< charptr_map< double > >::const_iterator li;

	for (li=coeff_list_.begin(); add and li!=coeff_list_.end(); li++)
	{
		charptr_map< double > const & R = *li;

		// Vorwärtsvergleich
		L2R = true;
		for (mi=L.begin(); L2R and mi!=L.end(); mi++)
		{
			if ((dptr = R.findPtr(mi->key)) == 0)
				L2R = false;
			else if (*dptr != mi->value)
				L2R = false;
		}
		if (not L2R)
			continue;
		// Rückwärtsvergleich
		R2L = true;
		for (mi=R.begin(); R2L and mi!=R.end(); mi++)
		{
			if ((dptr = L.findPtr(mi->key)) == 0)
				R2L = false;
			else if (*dptr != mi->value)
				R2L = false;
		}
		if (not R2L)
			continue;
		add = false;
	}

	if (add)
	{
		coeff_list_.push_back(L);
		charptr_array keys = L.getKeys();
		charptr_array::const_iterator i;
		
		for (i=keys.begin(); i!=keys.end(); i++)
			if (strcmp(*i,"1") != 0)
				var_indices_[*i] = -1;
		return true;
	}
	return false;
}

bool StandardForm::addConstraint(ExprTree * E)
{
	bool status = false;

	if (E->getNodeType() != et_op_leq
		and E->getNodeType() != et_op_geq
		and E->getNodeType() != et_op_eq)
	{
		fERROR("operator '%s' not supported for ineq. constraints - use '<=','>=','='",
			E->nodeToString().c_str());
		fTHROW(ExprTreeException);
	}

	try
	{
		LinearExpression lineq(E);
		
		charptr_map< double > coeffs
			= lineq.getLinearCoeffs();
		charptr_map< double >::iterator ci;
		
		switch (E->getNodeType())
		{
		case et_op_leq:
			break;
		case et_op_geq:
			// geq wird zu leq indem das Vorzeichen der Koeffizienten
			// gewechselt wird:
			for (ci=coeffs.begin(); ci!=coeffs.end(); ci++)
				ci->value = -(ci->value);
			break;
		case et_op_eq:
			// einmal als leq:
			status = addCoeffsUnique(coeffs);
			// geq wird zu leq indem das Vorzeichen der Koeffizienten
			// gewechselt wird:
			for (ci=coeffs.begin(); ci!=coeffs.end(); ci++)
				ci->value = -(ci->value);
			break;
		default:
			// unmöglicher Fehler
			return false;
		}
		
		// falls das Constraint aufgenommen wird muss demnächst build()
		// aufgerufen werden:
		if (addCoeffsUnique(coeffs) or status)
		{
			status = true;
			dirty_ = true;
		}
	}
	catch (ExprTreeException&)
	{
		// E ist wohl nicht-linear
		fWARNING("constraint expression '%s' is possibly non-linear",
			E->toString().c_str());
		return false;
	}
	return status;
}

void StandardForm::build() const
{
	// muss build() aufgerufen werden?
	if (not dirty_)
		return;
	
	int i,j = 0;
	charptr_array keys = var_indices_.getKeys();
	keys.sort();
	for (charptr_array::const_iterator k=keys.begin(); k!=keys.end(); k++)
	{
		fASSERT( strcmp(*k,"1") != 0 );
		var_indices_[*k] = j;
		j++;
	}

	i = coeff_list_.size();

	SMatrix A(i,j);
	MVector b(i);

	std::list< charptr_map< double > >::const_iterator cli;
	charptr_map< double >::const_iterator ci;

	for (i=0,cli=coeff_list_.begin(); cli!=coeff_list_.end(); cli++,i++)
	{
		charptr_map< double > const & C = *cli;
		
		for (ci=C.begin(); ci!=C.end(); ci++)
		{
			if (strcmp(ci->key,"1")==0)
				b(i) = - ci->value;
			else
				A(i,var_indices_[ci->key]) = ci->value;
		}
	}

	A_ = A;
	b_ = b;

	dirty_ = false;
}

bool StandardForm::operator<<(char const * expr)
{
	ExprTree * E = ExprTree::parse(expr);
	bool result = addConstraint(E);
	delete E;
	return result;
}

bool StandardForm::setLowerBound(char const * vn, double lb)
{
	double * lower, * upper;
	if (var_indices_.findPtr(vn) == 0)
	{
		// eine neue Variable registrieren
		var_indices_[vn] = -1;
		// aufruf von build() ist jetzt erforderlich
		dirty_ = true;
	}
	// -0 entfernen
	if (lb == 0.)
		lb = fabs(lb);
	upper = uvbound_.findPtr(vn);
	// Die obere Grenze darf nicht kleiner als die untere Grenze sein.
	if (upper and *upper < lb)
		return false;
	// Gibt es schon einen Eintrag?
	lower = lvbound_.findPtr(vn);
	if (lower and *lower > lb)
		return true; // bisherigen Eintrag nicht ersetzen
	// ansonsten eintragen
	lvbound_.insert(vn,lb);
	return true;
}

bool StandardForm::setUpperBound(char const * vn, double ub)
{
	double * lower, * upper;
	if (var_indices_.findPtr(vn) == 0)
	{
		// eine neue Variable registrieren
		var_indices_[vn] = -1;
		// aufruf von build() ist jetzt erforderlich
		dirty_ = true;
	}
	// -0 entfernen
	if (ub == 0.)
		ub = fabs(ub);
	lower = lvbound_.findPtr(vn);
	// Die untere Grenze darf nicht größer als die obere Grenze sein.
	if (lower and *lower > ub)
		return false;
	// Gibt es schon einen Eintrag?
	upper = uvbound_.findPtr(vn);
	if (upper and *upper < ub)
		return true; // bisherigen Eintrag nicht ersetzen
	// ansonten eintragen
	uvbound_.insert(vn,ub);
	return true;
}

bool StandardForm::getLowerBound(char const * vn, double & lb) const
{
	double * lower;
	if ((lower = lvbound_.findPtr(vn)) == 0)
		return false;
	lb = *lower;
	return true;
}

bool StandardForm::getUpperBound(char const * vn, double & ub) const
{
	double * upper;
	if ((upper = uvbound_.findPtr(vn)) == 0)
		return false;
	ub = *upper;
	return true;
}

void StandardForm::registerVariable(char const * vn)
{
	if (var_indices_.findPtr(vn) == 0)
	{
		// eine neue Variable registrieren
		var_indices_[vn] = -1;
		// aufruf von build() ist jetzt erforderlich
		dirty_ = true;
	}
}

int StandardForm::indexOf(char const * vn) const
{
	build();
	int * i = var_indices_.findPtr(vn);
	if (i) return *i;
	return -1;
}

char const * StandardForm::getColumnVar(size_t col) const
{
	build();
	charptr_map< int >::const_iterator vi;
	
	for (vi = var_indices_.begin(); vi != var_indices_.end(); vi++)
		if (vi->value == int(col))
			return vi->key;
	return 0;
}

charptr_array StandardForm::getVars() const
{
	build();
	charptr_array vars;
	for (size_t i=0; i<A_.cols(); i++)
		vars.add(getColumnVar(i));
	return vars;
}

bool StandardForm::isFeasible(double const * x, double tol, bool verbose) const
{
	build();
	size_t i,j;
	double s;

	for (j=0; j<A_.cols(); ++j)
		if (std::isnan(x[j]))
		{
			if (verbose)
				fWARNING("isFeasible - some components of tested vector are NaN");
			return false;
		}

	for (i=0; i<A_.rows(); i++)
	{
		s = 0.;
		for (j=0; j<A_.cols(); j++)
			s += A_.get(i,j) * x[j];
		if (s - tol > b_.get(i))
		{
			if (verbose)
			{
				ExprTree * E = getRowExpr(i);
				fWARNING("isFeasible - [%s] is violated by %g",
					E->toString().c_str(),(s-tol)-b_.get(i));
				delete E;
			}
			return false;
		}
	}

	charptr_map< double >::const_iterator bnd;
	for (bnd=lvbound_.begin(); bnd!=lvbound_.end(); bnd++)
	{
		j = *(var_indices_.findPtr(bnd->key));
		if (x[j] + tol < bnd->value)
		{
			if (verbose)
			{
				fWARNING("isFeasible - L-bound %s(=%g) + tol(=%g) < %g is violated",
					bnd->key, x[j], tol, bnd->value);
			}
			return false;
		}
	}
	for (bnd=uvbound_.begin(); bnd!=uvbound_.end(); bnd++)
	{
		j = *(var_indices_.findPtr(bnd->key));
		if (x[j] - tol > bnd->value)
		{
			if (verbose)
			{
				fWARNING("isFeasible - U-bound %s(=%g) - tol(=%g) > %g is violated",
					bnd->key, x[j], tol, bnd->value);
			}
			return false;
		}
	}
	return true;
}

bool StandardForm::isFeasible() const
{
	build();
//	LPProblem LPP;
//	LPP.importConstraints(*this);
//	LPP.maximize(getVars()[0]);
//	LPSolver LPS(LPP);
//	return LPS.solve() != LPSolver::lps_infeasible;
	//XXX how should we check this
	return true;
}

double StandardForm::distToConstraint(size_t i, double const * x0) const
{
	build();
	size_t j;
	double norm, p, D;
	fASSERT(i < A_.rows());

	// Normalenvektor erstellen. Da die Constraints die Struktur
	// A.x <= b haben, muss hier das Vorzeichen der Koeffizienten
	// in A umgedreht werden:
	MVector n(A_.cols());
	for (j=0; j<A_.cols(); j++)
		n(j) = -A_.get(i,j);

	// n Normieren und D = n*x0+p berechnen:
	//
	// Abstand D von Punkt x0 zur Constraint-Fläche:
	// Bei negativem D wurde das Ungleichungs-Constraint
	// um den Wert |D| verletzt. Bei positivem D wird
	// das Ungleichungs-Constraint erfüllt und die Distanz
	// ist D
	norm = n.norm2();
	p = b_.get(i) / norm;
	D = p;
	for (j=0; j<A_.cols(); j++)
	{
		n(j) /= norm;
		D += n(j) * x0[j];
	}
	return D;
}

double StandardForm::distance(double const * x) const
{
	build();
	size_t i;
	double Di, D = 0.;

	for (i=0; i<A_.rows(); i++)
	{
		Di = distToConstraint(i,x);
		if (Di < D) D = Di;
	}
	// wurden lower/upper bounds verletzt?
	// Den Bounds wird der gleiche Stellenwert eingeräumt
	charptr_map< double >::const_iterator bnd;
	for (bnd=lvbound_.begin(); bnd!=lvbound_.end(); bnd++)
	{
		i = *(var_indices_.findPtr(bnd->key));
		Di = x[i] - bnd->value;
		if (Di < D) D = Di;
	}
	for (bnd=uvbound_.begin(); bnd!=uvbound_.end(); bnd++)
	{
		i = *(var_indices_.findPtr(bnd->key));
		Di = bnd->value - x[i];
		if (Di < D) D = Di;
	}

	if (D == 0.) return 0.;
	return -D;
}

ExprTree * StandardForm::getRowExpr(size_t row) const
{
	size_t j;

	build();
	if (row >= A_.rows())
		return 0;
	
	ExprTree * rval = ExprTree::val(b_.get(row));
	ExprTree * lval = ExprTree::val(0);

	for (j=0; j<A_.cols(); j++)
	{
		if (A_.get(row,j) == 0.)
			continue;
		lval = ExprTree::add(
			lval,
			ExprTree::mul(
				ExprTree::val(A_.get(row,j)),
				ExprTree::sym(getColumnVar(j))
				)
			);
	}

	ExprTree * E = ExprTree::leq(lval,rval);
	// E sortieren und "verschönen"; ExprTreeException wird
	// hier nicht auftreten ...
	LinearExpression ineq(E); 
	delete E;
	return ineq.get()->clone();
}

bool StandardForm::importConstraints(StandardForm const & SF)
{
	bool added = false;
	std::list< charptr_map< double > >::const_iterator li;

	for (li=SF.coeff_list_.begin(); li!=SF.coeff_list_.end(); li++)
		if (addCoeffsUnique(*li))
			added = true;

	charptr_map< double >::const_iterator bnd;
	for (bnd=SF.lvbound_.begin(); bnd!=SF.lvbound_.end(); bnd++)
		setLowerBound(bnd->key,bnd->value);
	for (bnd=SF.uvbound_.begin(); bnd!=SF.uvbound_.end(); bnd++)
		setUpperBound(bnd->key,bnd->value);

	charptr_map< int >::const_iterator vi;
	for (vi = SF.var_indices_.begin(); vi != SF.var_indices_.end(); vi++)
		var_indices_.insert(vi->key,-1);

	if (added)
		dirty_ = true;
	return added;
}


void StandardForm::distToConstraints(
	MVector const & x,
	MVector const & r,
	double & d_neg,
	double & d_pos
	) const
{
	size_t i,j;
	double rho, rho_n, rho_d, d_x_xcp, d_x_xcm;
	MVector xci(A_.cols()); // Schnittpunkt
	// dichteste Schnittpunkte (auskommentiert, weil keiner sie braucht)
	//MVector xcp(A_.cols()); // dichtester "positiver" Schnittpunkt
	//MVector xcm(A_.cols()); // dichtester "negativer" Schnittpunkt

	// Abstand von x zu xcp (in Richtung von r)
	d_x_xcp = std::numeric_limits< double >::infinity();
	// Abstand von x zu xcm (entgegengesetzt r)
	d_x_xcm = std::numeric_limits< double >::infinity();

	fASSERT(A_.cols() == r.dim());

	// Schnittpunkte mit den Constraints (A_.rows() darf 0 sein!)
	for (i=0; i<A_.rows(); i++)
	{
		// Schnittpunkt xci von Constraint i und Gerade r durch Punkt x
		for (j=0, rho_n=-b_.get(i), rho_d=0.; j<A_.cols(); j++)
		{
			rho_n += A_.get(i,j) * x.get(j);
			rho_d += A_.get(i,j) * r.get(j);
		}
		rho = rho_n / rho_d;
		for (j=0; j<x.dim(); j++)
			xci(j) = x.get(j) - r.get(j) * rho;

		// Abstand von x zu ci
		double d_x_xci = MVector::euklid(x,xci);

		// liegt ci in der Richtung in die auch r zeigt?
		if ((r * (xci - x)) >= 0.) // ja
		{
			// xci dichter an x als xcp?
			if (d_x_xci < d_x_xcp)
			{
				//xcp = xci;
				d_x_xcp = d_x_xci;
			}
		}
		else // nein
		{
			// xci dichter an x als xcm?
			if (d_x_xci < d_x_xcm)
			{
				//xcm = xci;
				d_x_xcm = d_x_xci;
			}
		}
	}

	// Schnittpunkte mit den unteren Bounds
	charptr_map< double >::const_iterator bnd;
	for (bnd=lvbound_.begin(); bnd!=lvbound_.end(); bnd++)
	{
		// Schnittpunkt xci von lower bound i und Gerade r durch Punkt x
		// a >= 3, -a <= -3, -a+3<=0
		j = *(var_indices_.findPtr(bnd->key));
		rho_n = bnd->value - x.get(j);
		rho_d = - r.get(j);
		rho = rho_n / rho_d;
		for (j=0; j<x.dim(); j++)
			xci(j) = x.get(j) - r.get(j) * rho;

		// Abstand von x zu ci
		double d_x_xci = MVector::euklid(x,xci);
		
		// liegt ci in der Richtung in die auch r zeigt?
		if ((r * (xci - x)) > 0.) // ja
		{
			// xci dichter an x als xcp?
			if (d_x_xci < d_x_xcp)
			{
				//xcp = xci;
				d_x_xcp = d_x_xci;
			}
		}
		else // nein
		{
			// xci dichter an x als xcm?
			if (d_x_xci < d_x_xcm)
			{
				//xcm = xci;
				d_x_xcm = d_x_xci;
			}
		}
	}

	// Schnittpunkte mit den oberen Bounds
	for (bnd=uvbound_.begin(); bnd!=uvbound_.end(); bnd++)
	{
		// Schnittpunkt xci von lower bound i und Gerade r durch Punkt x
		// a <= 3, a-3<=0
		j = *(var_indices_.findPtr(bnd->key));
		rho_n = -bnd->value + x.get(j);
		rho_d = r.get(j);
		rho = rho_n / rho_d;
		for (j=0; j<x.dim(); j++)
			xci(j) = x.get(j) - r.get(j) * rho;

		// Abstand von x zu ci
		double d_x_xci = MVector::euklid(x,xci);
		
		// liegt ci in der Richtung in die auch r zeigt?
		if ((r * (xci - x)) >= 0.) // ja
		{
			// xci dichter an x als xcp?
			if (d_x_xci < d_x_xcp)
			{
				//xcp = xci;
				d_x_xcp = d_x_xci;
			}
		}
		else // nein
		{
			// xci dichter an x als xcp?
			if (d_x_xci < d_x_xcm)
			{
				//xcm = xci;
				d_x_xcm = d_x_xci;
			}
		}
	}

	// xcm, xcp auch rausgeben?
	d_pos = d_x_xcp;
	d_neg = d_x_xcm;
}

StandardForm StandardForm::shrink(double eps) const
{
	if (dirty_) build();
	size_t i,j;
	double len;
	charptr_map< double >::const_iterator bi;

	// echte Constraints
	MVector b(b_);
	for (i=0; i<A_.rows(); ++i)
	{
		len = 0.;
		for (j=0; j<A_.cols(); ++j)
			len += A_.get(i,j)*A_.get(i,j);
		len = sqrt(len);
		b(i) -= eps*len;
	}
	StandardForm SFshr(*this);
	SFshr.b_ = b;

	// Variablenbounds
	SFshr.lvbound_.clear();
	SFshr.uvbound_.clear();
	for (bi=lvbound_.begin(); bi!=lvbound_.end(); ++bi)
		SFshr.lvbound_[bi->key] = bi->value + eps;
	for (bi=uvbound_.begin(); bi!=uvbound_.end(); ++bi)
		SFshr.uvbound_[bi->key] = bi->value - eps;
	
	return SFshr;
}


void StandardForm::dump(bool compact) const
{
	char numbuf[32];
	build();
	charptr_array keys = var_indices_.getKeys();
	keys.sort();
	if (keys.size())
		printf("variables:\n\t%s\n", keys.concat(", "));
	if (A_.rows())
	{
		if (compact)
		{
			printf("constraints:\n");
			for (size_t i=0; i<A_.rows(); i++)
			{
				ExprTree * rE = getRowExpr(i);
				printf("\t%s\n", rE->toString().c_str());
				delete rE;
			}
		}
		else
		{
			printf("A:\n");
			A_.dump(stdout,dump_full);
			printf("b:\n");
			b_.dump();
		}
	}
	keys = lvbound_.getKeys();
	keys.addUnique(uvbound_.getKeys());
	keys.sort();
	if (keys.size())
	{
		printf("variable bounds:\n");
		for (charptr_array::const_iterator i=keys.begin();
			i!=keys.end(); i++)
		{
			double * j;
			if ((j=lvbound_.findPtr(*i)) != 0)
			{
				dbl2str(numbuf,*j,sizeof(numbuf));
				printf("\t%s >= %s", *i,numbuf);
			}
			if ((j=uvbound_.findPtr(*i)) != 0)
			{
				dbl2str(numbuf,*j,sizeof(numbuf));
				printf("\t%s <= %s", *i,numbuf);
			}
			printf("\n");
		}
	}
}

bool StandardFormPenalty::intersectConstraint(
	size_t i,
	MVector const & P1,
	MVector const & P2,
	MVector & Px,
	double & dist
	) const
{
	// lege Gerade durch P1 und P2 und bestimme das Schnittverhältnis u
	//
	// in 3d: u = (Ax1+By1+Cz1+D)/(A(x1-x2)+B(y1-y2)+C(z1-z2))
	size_t j;
	SMatrix const & A = SF_.getLHS();
	MVector const & b = SF_.getRHS();
	double N = -b.get(i), D = 0., u;
	for (j=0; j<A.cols(); j++)
	{
		N += A.get(i,j)*P1.get(j);
		D += A.get(i,j)*(P1.get(j)-P2.get(j));
	}
	// Schnittverhältnis u (Mischungsverhältnis von P1 und P2)
	// es sind nur Schnittpunkt zwischen P1 und P2 gesucht
	if (D == 0. or (u=(N/D))<0. or u>1.)
		return false;
	
	// Schittpunkt um MACHEPS nach innen schieben, damit
	// er fesible wird (Kosmetik)
	//u -= MACHEPS;

	// Schnittpunkt
	Px = (1.-u)*P1+u*P2;//P1+u*(P2-P1);
	// Distanz von P1 zu Px
	dist = MVector::euklid(Px,P1);
	return true;
}

bool StandardFormPenalty::intersectBound(
	size_t var_idx,
	double * lo,
	double * up,
	MVector const & P1,
	MVector const & P2,
	MVector & Px,
	double & dist
	) const
{
	double N, D, u;
	
	N = up ? P1.get(var_idx)-(*up) : (*lo)-P1.get(var_idx);
	D = P1.get(var_idx) - P2.get(var_idx);
	if (lo) D = -D;
	u = N/D;
	if (D == 0. or (u=(N/D))<0. or u>1.)
		return false;
	Px = (1.-u)*P1+u*P2;
	dist = MVector::euklid(Px,P1);
	return true;
}

bool StandardFormPenalty::closestIntersection(
	MVector const & P,
	MVector & Px
	) const
{
	bool isect, isect_i;
	size_t i;
	size_t N = SF_.getLHS().rows();
	double d, d_min, b;
	MVector Pi;

	isect = false;
	d_min = -1.;
	for (i=0; i<N; i++)
	{
		isect_i = intersectConstraint(i,center_,P,Pi,d);

		// erster Schnittpunkt?
		if (isect_i and not isect)
		{
			d_min = d;
			Px = Pi;
		}

		if (isect_i)
		{
			isect = true;
			if (d < d_min)
			{
				d_min = d;
				Px = Pi;
			}
		}
	}

	charptr_map< double >::const_iterator bnd;
	for (bnd=SF_.getLowerBounds().begin();
		bnd!=SF_.getLowerBounds().end(); bnd++)
	{
		i = SF_.indexOf(bnd->key);
		b = bnd->value;
		isect_i = intersectBound(i,&b,0,center_,P,Pi,d);
		// erster Schnittpunkt?
		if (isect_i and not isect)
		{
			d_min = d;
			Px = Pi;
		}

		if (isect_i)
		{
			isect = true;
			if (d < d_min)
			{
				d_min = d;
				Px = Pi;
			}
		}
	}
	for (bnd=SF_.getUpperBounds().begin();
		bnd!=SF_.getUpperBounds().end(); bnd++)
	{
		i = SF_.indexOf(bnd->key);
		b = bnd->value;
		isect_i = intersectBound(i,0,&b,center_,P,Pi,d);
		// erster Schnittpunkt?
		if (isect_i and not isect)
		{
			d_min = d;
			Px = Pi;
		}

		if (isect_i)
		{
			isect = true;
			if (d < d_min)
			{
				d_min = d;
				Px = Pi;
			}
		}
	}
	return isect;
}

double StandardFormPenalty::penalty(
	MVector const & x,
	MVector & lastFeasible
	) const
{
	// falls es einen Schnittpunkt gibt berechne Distanz
	if (closestIntersection(x,lastFeasible))
		return MVector::euklid(x,lastFeasible);
	// ansonsten ist x feasible und lastFeasible wird auf x gesetzt:
	lastFeasible = x;
	return 0.;
}

double StandardFormPenalty::penalty(
	double const * x
	) const
{
	MVector x_(center_.dim());
	x_.copy(x);
	MVector lastFeasible;
	if (closestIntersection(x_,lastFeasible))
		return MVector::euklid(x_,lastFeasible);
	return 0.;
}


} // namespace flux::la
} // namespace flux

