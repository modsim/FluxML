#include "cstringtools.h"
#include "MVector.h"
#include "MMatrix.h"
#include "PMatrix.h"
#include "SMatrix.h"
#include "PMatrix.h"

#include <limits>

namespace flux {
namespace la {

double SMatrix::zero_ = 0.;

SMatrix::SMatrix(MMatrix const & copy)
	: rows_(copy.rows()), cols_(copy.cols())
{
	size_t i, j;
	double expct_fill = 0.05;
	
	hash_ = new fhash_map< mxkoo,double,mxkoo_hashf >(
			size_t(ceil(double(rows_) * double(cols_) * expct_fill))
		);
	
	for (i=0; i<rows_; i++)
		for (j=0; j<cols_; j++)
			if (fabs(copy.get(i,j)) > std::numeric_limits<double>::epsilon())
				set(i,j,copy.get(i,j));
}

SMatrix::operator MMatrix () const
{
	MMatrix M(rows_,cols_);
	for (const_iterator a_ij=begin(); a_ij!=end(); a_ij++)
		M.set(a_ij->row,a_ij->col,a_ij->value);
	return M;
}

MVector SMatrix::operator*(MVector const & rval) const
{
	fASSERT(rval.dim() == cols_);
	MVector p(rows_);
	for (const_iterator a_ij=begin(); a_ij!=end(); a_ij++)
		p(a_ij->row) += a_ij->value * rval.get(a_ij->col);
	return p;
}

MMatrix SMatrix::operator*(MMatrix const & Rval) const
{
	size_t i,j,k;
	double sum;
	MMatrix C(rows_,Rval.cols());

	for (i=0; i<C.rows(); i++)
		for (j=0; j<C.cols(); j++)
		{
			for (sum=0.,k=0; k<cols_; k++)
				sum += get(i,k) * Rval.get(k,j);
			C(i,j) = sum;
		}
	return C;
}

SMatrix SMatrix::operator*=(double rval) 
{
        for (const_iterator a_ij=begin(); a_ij!=end(); a_ij++)
		set(a_ij->row,a_ij->col, a_ij->value* rval);
        
	return *this;
}

MVector SMatrix::diag() const
{
	MVector d(rows_);
	for (size_t i= 0; i< rows_; i++)
		d(i) = get(i,i);
	return d;
}

double SMatrix::trace() const
{
    double trace=0.;
    for (size_t i= 0; i< rows_; i++)
		trace += get(i,i);
    return trace;
}
void SMatrix::spy() const
{
	MMatrix(*this).spy();
}

double SMatrix::norm1() const
{
    double n=0., n1_col;
    size_t i, j;

    for (j=0; j<cols_; j++)
    {
        for (i=0,n1_col=0.; i<rows_; i++)
            n1_col += fabs(get(i,j));
        if (n1_col > n) n = n1_col;
    }
    return n;
}

void SMatrix::dump(FILE * outf, dump_t dt) const
{
	char dbl[32];
	size_t i,j;

	switch (dt)
	{
	case dump_full:
		MMatrix(*this).dump(outf,dump_default);
		break;
	case dump_default:
	case dump_triplet:
		for (i=0; i<rows_; i++)
			for (j=0; j<cols_; j++)
			{
				if (get(i,j) == 0.)
					continue;
				dbl2str(dbl,get(i,j), sizeof(dbl));
				fprintf(outf, "%d %d %s\n",
					int(i), int(j), dbl);

			}
		break;
	case dump_matlab:
		MMatrix(*this).dump(outf,dump_matlab);
		break;
	case dump_matlab_sparse:
		fprintf(outf, "S = zeros(%d,%d);\n", int(rows_), int(cols_));
		for (const_iterator a_ij=begin(); a_ij!=end(); a_ij++)
		{
			dbl2str(dbl,a_ij->value, sizeof(dbl));
			fprintf(outf, "S(%d,%d) = %s;\n",
				int(a_ij->row)+1, int(a_ij->col)+1, dbl);
		}
		break;
	}
}

// stimmt noch nicht! Auf Permutationsmatrizen für P.A.P^T anpassen!!!
void SMatrix::symmPerm(PMatrix const & P)
{
	fASSERT(rows_ == cols_ and P.dim() == rows_);
	
	SMatrix tmp(*this);
	PMatrix Pi = P.inverse();

	// diese SMatrix säubern:
	eraseAll();
	
	// mit den permutierten Werten aus tmp neu belegen
	for (iterator k=tmp.begin(); k!=tmp.end(); k++)
		set(Pi(k->row),Pi(k->col),k->value);
	// für P^T.A.P ist es einfacher:
	//// Zeilenpermutation:
	//for (k=begin(); k!=end(); k++)
	//	tmp.set(P(k->row), k->col, k->value);
	//// diese SMatrix säubern:
	//eraseAll();
	//// Spaltenpermutation:
	//for (k=tmp.begin(); k!=tmp.end(); k++)
	//	this->set(k->row, P(k->col), k->value);
}
	
void SMatrix::measureBandWidth(
	int & lb,
	int & ub
	) const
{
	int i,j;
	lb = ub = 0;
	for (i=0; i<int(rows()); i++)
	{
		for (j=i-lb-1; j>=0; j--)
			if (fabs(get(i,j)) > std::numeric_limits<double>::epsilon())
				lb = i-j;
		for (j=i+ub+1; j<int(cols()); j++)
			if (fabs(get(i,j)) > std::numeric_limits<double>::epsilon())
				ub = j-i;
	}
}

void SMatrix::compress()
{
	iterator e, a_ij;
	for (a_ij=begin(); a_ij!=end(); )
	{
		e = a_ij++;
		if (e->value == 0.)
			hash_->erase(e.pos_);
	}
}

} // namespace flux::la
} // namespace flux

