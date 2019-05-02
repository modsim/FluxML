#ifndef VECTORINTERFACE_H
#define VECTORINTERFACE_H

#include <cstddef>

namespace flux {
namespace la {

/**
 * Schnittstelle jeder Vektoren-Klasse
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
template< typename T > class VectorInterface
{
public:
	virtual ~VectorInterface() { }

	virtual T & operator() (size_t i) = 0;

	virtual void set(size_t i, T const & val) = 0;

	virtual T const & get(size_t i) const = 0;

	virtual size_t dim() const = 0;
};

} // namespace flux::la
} // namespace flux

#endif

