#include <cstdio>
#include <cstring>
#include <cmath>

#include "fluxml_config.h" // MACHEPS
#include "MMatrix.h"
#include "PMatrix.h"
#include "MVector.h"
#include "MMatrixOps.h"
#include "LAPackWrap.h"
#include "cstringtools.h"

namespace flux {
namespace la {

void MMatrix::dump(FILE * outf, dump_t dt, char const * fmt) const
{
	size_t i,j;
	char buf[64];

	switch (dt)
	{
	case dump_default:
	case dump_full:
	case dump_matlab:
		for (i=0; i<rows_; i++)
		{
			for (j=0; j<cols_; j++)
			{
				formatdbl(buf,sizeof(buf),get(i,j),fmt);
				fputs(buf,outf);
				fputs((j==cols_-1)?";\n":",",outf);
			}
		}
		break;
	case dump_triplet:
		for (i=0; i<rows_; i++)
			for (j=0; j<cols_; j++)
			{
				dbl2str(buf,get(i,j), sizeof(buf));
				fprintf(outf, "%d %d %s\n",
					int(i), int(j), buf);
			}
		break;
	case dump_matlab_sparse:
		fprintf(outf, "S = zeros(%d,%d);\n", int(rows_), int(cols_));
		for (i=0; i<rows_; i++)
			for (j=0; j<cols_; j++)
			{
				if (get(i,j)==0.) continue;
				dbl2str(buf,get(i,j), sizeof(buf));
				fprintf(outf, "S(%d,%d) = %s;\n",
					int(i)+1, int(j)+1, buf);
			}
		break;
	}
}

// Zeigt die Besetztheit von Matrizen
void MMatrix::spy() const
{
	unsigned int i, j;

	printf("    ");
	for (j=0; j<rows_; j++)
		printf("%2X", j);
	putchar('\n');
	for (i=0; i<rows_; i++)
	{
		printf("%2X: ", i);
		for (j=0; j<cols_; j++)
		{
			if (fabs(get(i,j))>=MACHEPS)
			{
				if (get(i,j)>0.)
					putchar('+');
				else
					putchar('-');
			}
			else
				putchar(' ');
			if (j<cols_-1) putchar(' ');
		}
		puts(" :");
	}
}

// Mißt die Bandbreite der Matrix (teuer!, O(N^2))
void MMatrix::measureBandWidth(int & lb, int & ub) const
{
	int i,j;
	lb = ub = 0;
	for (i=0; i<int(rows()); i++)
	{
		for (j=i-lb-1; j>=0; j--)
			if (fabs(get(i,j)) > MACHEPS)
				lb = i-j;
		for (j=i+ub+1; j<int(cols()); j++)
			if (fabs(get(i,j)) > MACHEPS)
				ub = j-i;
	}
}

// Inf-Norm der Matrix (Maximum der 1-Normen der Zeilenvektoren).
double MMatrix::normInf() const
{
	double n=0., n1_row;
	size_t i, j;

	for (i=0; i<rows_; i++)
	{
		for (j=0,n1_row=0.; j<cols_; j++)
			n1_row += fabs(get(i,j));
		if (n1_row > n) n = n1_row;
	}
	return n;
}

// Inf-Norm der Matrix (Maximum der 1-Normen der Zeilenvektoren).
double MMatrix::normInfCwise() const
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
// 1-norm: der Matrix (Maximum der 1-Normen der Spaltenvektoren).
double MMatrix::norm1() const
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

double MMatrix::norm2() const
{
	size_t n = rows_<cols_ ? rows_ : cols_;
	MVector s(n);
	MMatrix A(*this);
	lapack::svd(A,s);
	return s(0);
}

double MMatrix::conditionKappaInf() const
{
	fASSERT( rows_ == cols_ );
	PMatrix P(rows_);
	MMatrix LU(*this);
	MMatrix I(rows_,cols_);
	MMatrixOps::_LUfactor(LU,P);
	MMatrixOps::_LUinvert(LU,P,I);
	return normInf()*I.normInf();
}

MMatrix MMatrix::operator+ (MMatrix const & Rval) const
{
	size_t i,j;
	MMatrix C(rows(), cols());

	for (i=0; i<rows_; i++)
		for (j=0; j<cols_; j++)
			C.set(i,j,get(i,j) + Rval.get(i,j));
	return C;
}

MMatrix & MMatrix::operator+= (MMatrix const & Rval)
{
	size_t i,j;

	for (i=0; i<rows_; i++)
		for (j=0; j<cols_; j++)
			set(i,j,get(i,j) + Rval.get(i,j));
	return *this;
}

MMatrix MMatrix::operator- (MMatrix const & Rval) const
{
	size_t i,j;
	MMatrix C(rows(), cols());

	for (i=0; i<rows_; i++)
		for (j=0; j<cols(); j++)
			C.set(i,j,get(i,j) - Rval.get(i,j));
	return C;
}

MMatrix & MMatrix::operator-= (MMatrix const & Rval)
{
	size_t i,j;

	for (i=0; i<rows_; i++)
		for (j=0; j<cols_; j++)
			set(i,j,get(i,j) - Rval.get(i,j));
	return *this;
}

MMatrix MMatrix::operator* (MMatrix const & Rval) const
{
	size_t i,j,k;
	double sum;
	MMatrix C(rows_,Rval.cols_);

	for (i=0; i<C.rows_; i++)
		for (j=0; j<C.cols_; j++)
		{
			for (sum=0.,k=0; k<cols_; k++)
				sum += get(i,k) * Rval.get(k,j);
			C.set(i,j,sum);
		}
	return C;
}

MVector MMatrix::operator* (MVector const & rval) const
{
	size_t i,k;
	double sum;

	MVector c(rows_);

	for (i=0; i<rows_; i++)
	{
		for (sum=0.,k=0; k<cols_; k++)
			sum += get(i,k) * rval.get(k);
		c.set(i,sum);
        }
	return c;
}

MMatrix & MMatrix::operator*= (double rval)
{
	size_t i,j;

	for (i=0; i<rows_; i++)
		for (j=0; j<cols_; j++)
			set(i,j,get(i,j) * rval);
	return *this;
}

MMatrix MMatrix::operator* (double rval) const
{
	size_t i,j;
	MMatrix M(rows_,cols_);

	for (i=0; i<rows_; i++)
		for (j=0; j < cols(); j++)
			M.set(i, j, get(i, j) * rval);
	return M;
}

MMatrix & MMatrix::operator/= (double rval)
{
	size_t i,j;
	fASSERT(rval != 0.);

	for (i=0; i<rows_; i++)
		for (j=0; j<cols(); j++)
			set(i,j,get(i,j) / rval);
	return *this;
}

MMatrix MMatrix::operator/ (double rval) const
{
	size_t i,j;
	MMatrix M(rows(), cols());
	fASSERT(rval != 0.);
	
	for (i=0; i<rows_; i++)
		for (j=0; j<cols_; j++)
			M.set(i,j,get(i,j) / rval);
	return M;
}

MMatrix MMatrix::getTranspose() const
{
	size_t i,j;
	MMatrix X(cols_,rows_);
	for (i=0; i<rows_; i++)
		for (j=0; j<cols_; j++)
			X.set(j,i,get(i,j));
	return X;
}

MMatrix MMatrix::getSlice(size_t i1, size_t j1, size_t i2, size_t j2) const
{
	size_t i,j;
	fASSERT(i2<rows_ and j2<cols_ and i1<=i2 and j1<=j2);
	MMatrix S(i2-i1+1,j2-j1+1);
	for (i=0; i<i2-i1+1; i++)
		for (j=0; j<j2-j1+1; j++)
			S(i,j) = get(i1+i,j1+j);
	return S;
}

bool MMatrix::setSlice(size_t i1, size_t j1, size_t i2, size_t j2, MMatrix S) 
{
	size_t i,j;
	fASSERT(i2<rows_ and j2<cols_ and i1<=i2 and j1<=j2);
	for (i=0; i<S.rows_; i++)
            for (j=0; j<S.cols_; j++)
                    set(i1+i,j1+j,S.get(i,j));        
	return true;
}

MMatrix & MMatrix::diag(MVector & d)
{
	fASSERT(d.dim() == (rows_<cols_?rows_:cols_));
	size_t i,j;
	for (i=0; i<rows_; i++)
		for (j=0; j<cols_; j++)
			set(i,j,i==j?d(i):0.);
	return *this;
}

MVector MMatrix::diag() const
{
	size_t dim = rows_<cols_?rows_:cols_;
	MVector d(dim);
	while (dim)
	{
		dim--;
		d(dim) = get(dim,dim);
	}
	return d;
}

} // namespace flux::la
} // namespace flux

