#include <cmath>
#include "MVector.h"
#include "MMatrix.h"
#include "cstringtools.h"

namespace flux {
namespace la {

double MVector::max() const
{
	double max=get(0);
	for (size_t i=1; i<dim_; i++)
            if(max<get(i))
		max= get(i);
	return max;
}
    
double MVector::norm1(size_t lo, size_t hi) const
{
	double n=0.;
	if (hi==0) hi=dim_-1;
	for (size_t k=lo; k<=hi; k++)
		n += fabs(get(k));
	return n;
}

double MVector::norm2(size_t lo, size_t hi) const
{
	double n=0.;
	if (hi==0) hi=dim_-1;
	for (size_t k=lo; k<=hi; k++)
		n += get(k)*get(k);
	return sqrt(n);
}

double MVector::normInf(size_t lo, size_t hi) const
{
	double n=0., abs;
	if (hi==0) hi=dim_-1;
	for (size_t k=lo; k<=hi; k++)
	{
		abs = fabs(get(k));
		if (abs>n) n = abs;
	}
	return n;
}

double MVector::euklid(MVector const & L, MVector const & R)
{
	double d=0.,v;
	for (size_t i=0; i<L.dim_; i++)
	{
		v = L.get(i)-R.get(i);
		d += v*v;
	}
	return sqrt(d);
}

MVector MVector::getSlice(size_t i, size_t j) const
{
	fASSERT(j<dim_ and i<=j);
	MVector s(j-i+1);
	for (size_t k=0; k<j-i+1; k++)
		s(k) = get(k+i);
	return s;
}

void MVector::dump(FILE * outf, dump_t dt, char const * fmt) const
{
	size_t i;
	char buf[64];

	switch (dt)
	{
	case dump_default:
	case dump_full:
	case dump_matlab:
		for (i=0; i<dim_; i++)
		{
			formatdbl(buf,sizeof(buf),get(i),fmt);
			fputs(buf,outf);
			fputs((i==dim_-1)?";\n":",",outf);
		}

		break;
	case dump_triplet:
		for (i=0; i<dim_; i++)
		{
			dbl2str(buf,get(i), sizeof(buf));
			fprintf(outf, "0 %d %s\n",int(i),buf);
		}
		break;
	case dump_matlab_sparse:
		fprintf(outf, "S = zeros(1,%d);\n", int(dim_));
		for (i=0; i<dim_; i++)
		{
			if (get(i)==0.) continue;
			dbl2str(buf,get(i), sizeof(buf));
			fprintf(outf, "s(%d) = %s;\n",int(i)+1,buf);
		}
		break;
	}
}
	
MMatrix MVector::diag(size_t r, size_t c)
{
	size_t i;
	if (r+c == 0)
		r = c = dim_;
	fASSERT(r >= dim_ and c >= dim_);
	MMatrix D(r,c);
	for (i=0; i<dim_; i++)
		D(i,i) = get(i);
	return D;
}

MVector MVector::operator*(MMatrix const & Rval) const
{
	size_t i,j;
	double sum;
	fASSERT(Rval.rows() == dim_);
	MVector p(Rval.cols());
	for (j=0; j<Rval.cols(); j++)
	{
		sum = 0.;
		for (i=0; i<dim_; i++)
			sum += get(i) * Rval.get(i,j);
		p(j) = sum;
	}
	return p;
}
	
double MVector::operator*(MVector const & rval) const
{
	size_t i;
	double sp;
	fASSERT(rval.dim_ == dim_);
	for (sp=0.,i=0; i<dim_; i++)
		sp += get(i) * rval.get(i);
	return sp;
}

MVector MVector::operator*(double rval) const
{
	size_t i;
	MVector p(dim_);
	for (i=0; i<dim_; i++)
		p(i) = get(i) * rval;
	return p;
}

MVector & MVector::operator*=(double rval)
{
	size_t i;
	for (i=0; i<dim_; i++)
		set(i,get(i)*rval);
	return *this;
}

MVector MVector::operator/(double rval) const
{
	fASSERT(rval != 0.);
	size_t i;
	MVector p(dim_);
	for (i=0; i<dim_; i++)
		p(i) = get(i) / rval;
	return p;
}

MVector & MVector::operator/=(double rval)
{
	fASSERT(rval != 0.);
	size_t i;
	for (i=0; i<dim_; i++)
		set(i, get(i) / rval);
	return *this;
}
 
MVector MVector::operator+(MVector const & rval) const
{
	size_t i;
	fASSERT(rval.dim_ == dim_);
	MVector s(dim_);
	for (i=0; i<dim_; i++)
		s(i) = get(i) + rval.get(i);
	return s;
}

MVector & MVector::operator+=(MVector const & rval)
{
	size_t i;
	fASSERT(rval.dim_ == dim_);
	for (i=0; i<dim_; i++)
		set(i,get(i) + rval.get(i));
	return *this;
}

MVector MVector::operator-(MVector const & rval) const
{
	size_t i;
	fASSERT(rval.dim_ == dim_);
	MVector d(dim_);
	for (i=0; i<dim_; i++)
		d(i) = get(i) - rval.get(i);
	return d;
}

MVector & MVector::operator-=(MVector const & rval)
{
	size_t i;
	fASSERT(rval.dim_ == dim_);
	for (i=0; i<dim_; i++)
		set(i,get(i) - rval.get(i));
	return *this;
}

MVector MVector::operator-() const
{
	MVector neg(dim_);
	for (size_t i=0; i<dim_; ++i)
		neg(i) = -get(i);
	return neg;
}

MVector operator*(double lval, MVector const & rval)
{
	size_t i;
	MVector p(rval.dim());
	for (i=0; i<rval.dim(); i++)
		p(i) = lval * rval.get(i);
	return p;
}

} // namespace flux::la
} // namespace flux

