#include <cstring>
#include <cerrno>
#include "PMatrix.h"

namespace flux {
namespace la {

unsigned int PMatrix::one_ = 1;
unsigned int PMatrix::zero_ = 0;

// Inversion / Transposition
PMatrix PMatrix::inverse() const
{
	PMatrix Pinv(dim_);
	for (size_t i=0; i<dim_; i++)
		Pinv.vector_storage_[vector_storage_[i]] = i;
	return Pinv;
}

// In-Situ-Inversion (ohne Allokation)
void PMatrix::invert()
{
	Sort< unsigned int >::invPermInSitu(vector_storage_, dim_);
}

// Inversion (ohne Allokation)
void PMatrix::inverse(PMatrix & Pinv) const
{
	fASSERT(dim_ == Pinv.dim_);
	for (size_t i=0; i<dim_; i++)
		Pinv.vector_storage_[vector_storage_[i]] = i;
}

// Initialisierung mit der Identitäts-Permutation
void PMatrix::initIdent()
{
	for (size_t i=0; i<dim_; i++)
		vector_storage_[i] = i;
}

// Überprüfung der Permutation
bool PMatrix::check() const
{
	size_t i;
	bool flag = true;
	bool * T = new bool[dim_];
	for (i=0; i<dim_; i++)
		T[i] = true;
	for (i=0; i<dim_ && flag; i++)
		if (vector_storage_[i] < dim_)
			T[vector_storage_[i]] = false;
		else flag = false;
	for (i=0; i<dim_ && flag; i++)
		if (T[i]) flag = false;
	delete[] T;
	return flag;
}

void PMatrix::dump() const
{
	size_t i;
	for (i=0; i<dim_; i++)
		printf("%d<->%d%s", int(i), int(vector_storage_[i]), (i==dim_-1)?"\n":", ");
}

bool PMatrix::dumpMFile(char const * fn, char const * mn) const
{
	FILE *f;
	size_t i;

	if ((f = fopen(fn, "a")) == 0)
	{
		fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
		return false;
	}

	fprintf(f, "%s = [", mn);
	for (i=0; i<dim_; i++)
		fprintf(f,"%i%s",vector_storage_[i]+1,(i<dim_-1)?",":"];\n");

	if (fclose(f) != 0)
	{
		fprintf(stderr, "%s: %s\n", __func__, strerror(errno));
		return false;
	}
	return true;
}

void PMatrix::sort(size_t lo, size_t hi)
{
	Sort< unsigned int >::sort( &(vector_storage_[lo]), hi-lo+1 );
}

PMatrix PMatrix::sortPerm(size_t lo, size_t hi)
{
	// Permutationsmatrix erzeugen und initialisieren
	PMatrix P(hi-lo+1, true);
	// diese Matrix sortieren, dabei Permutation aufzeichnen
	Sort< unsigned int >::sortPerm( &(vector_storage_[lo]), P.vector_storage_, hi-lo+1 );
	return P;
}

} // namespace flux::la
} // namespace flux

