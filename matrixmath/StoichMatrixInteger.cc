#include <list>
#include <cstring>
#include <cstdio>
#include "cstringtools.h"
#include "Combinations.h"
#include "GVector.h"
#include "MMatrix.h"
#include "MMatrixOps.h"
#include "StoichMatrixInteger.h"

namespace flux {
namespace la {

void StoichMatrixInteger::dump(FILE * outf, dump_t dt, char const * fmt) const
{
        size_t i,j;
	char buf[64];

	switch (dt)
	{
	case dump_default:
		fprintf(outf,"\t");
		for (j=0; j<cols_; j++)
			fprintf(outf," %s,", getReactionName(j));
		fprintf(outf,"\n");
		for (i=0; i<rows_; i++)
		{
			fprintf(outf,"%s\t", getMetaboliteName(i));
			for (j=0; j<cols_; j++)
				fprintf(outf,"%3lli%s", (long long int)get(i,j),
					(j==cols()-1)?";\n":",");
		}
		break;
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
				if (get(i,j) != 0ll)
					fprintf(outf, "%d %d %lli\n",
						int(i), int(j),
						(long long int)get(i,j));
		break;
	case dump_matlab_sparse:
		fprintf(outf, "S = zeros(%d,%d);\n", int(rows_), int(cols_));
		for (i=0; i<rows_; i++)
			for (j=0; j<cols_; j++)
			{
				if (get(i,j)==0.) continue;
				fprintf(outf, "S(%d,%d) = %lli;\n",
					int(i)+1, int(j)+1,
					(long long int)get(i,j));
			}
		break;
	}
}

StoichMatrixInteger::operator MMatrix () const
{
	size_t i,j;
	MMatrix N(rows_,cols_);
	for (i=0; i<rows_; i++)
		for (j=0; j<cols_; j++)
			N.set(i,j,get(i,j));
	return N;
}

} // namespace flux::la
} // namespace flux

