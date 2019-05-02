#ifndef MATRIXINTERFACE_H
#define MATRIXINTERFACE_H

#include <cstddef>

namespace flux {
namespace la {

enum dump_t {
	dump_default,
	dump_full,
	dump_triplet,
	dump_matlab,
	dump_matlab_sparse
};

/**
 * Schnittstelle jeder Matrizen-Klasse
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
template< typename T > class MatrixInterface
{
public:
	virtual ~MatrixInterface() { }
	
	virtual T & operator() (size_t i, size_t j) = 0;

	virtual void set(size_t i, size_t j, T const & val) = 0;

	virtual T const & get(size_t i, size_t j) const = 0;

	virtual size_t rows() const = 0;

	virtual size_t cols() const = 0;

};

} // namespace flux::la
} // namespace flux

#endif

