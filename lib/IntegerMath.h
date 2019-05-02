#ifndef INTEGERMATH_H
#define INTEGERMATH_H

extern "C"
{
#include <stdint.h>
}

class IntegerOverflow {};

template< typename T > inline bool is_signed() { return (T)(-1)<0; }

template< typename T > inline T maximum()
{
	if (is_signed<T>())
		return ~(T(1) << (8*sizeof(T)-1));
	return ~T(0);
}

template< typename T > inline T minimum()
{
	if (is_signed<T>())
		return T(1) << (8*sizeof(T)-1);
	return 0;
}

template< typename T > inline T addI(T l, T r)
{
	// kein Overflow bei ungleichem Vorzeichen
	if ( (l^r) >= T(0) )
	{
		// gleiches Vorzeichen
		if (r < 0) // zwei negative Zahlen
		{
			// L+R<minimum entspricht L<minimum-R
			if (l < minimum<T>() - r)
				throw IntegerOverflow();
		}
		else // zwei positive Zahlen
		{
			// L+R>maximum entspricht L>maximum-R
			if (l > maximum<T>() - r)
				throw IntegerOverflow();
		}
	}
	// kein Overflow bei ungleichem Vorzeichen
	return l+r;
}

template< typename T > inline T subI(T l, T r)
{
	// Overflow nur bei ungleichem Vorzeichen
	if ( (l^r) < T(0) )
	{
		// ungleiches Vorzeichen
		if (l >= 0)
		{
			// L+R<minimum entspricht L<minimum-R
			if (l > maximum<T>() + r)
				throw IntegerOverflow();
		}
		else // r >= 0
		{
			if (l < minimum<T>() + r)
				throw IntegerOverflow();
		}
	}
	// kein Overflow bei gleichem Vorzeichen
	return l-r;
}

template< typename T > T mulI(T l, T r)
{
	if (l==T(0) or r==T(0)) return T(0);

	switch (sizeof(T))
	{
	case 1:
		if (is_signed<T>())
		{
			int16_t val = (int16_t)l * (int16_t)r;
			// die oberen 9 Bit müssen identisch sein
			if ((val&0xff80) == 0 || (val&0xff80) == 0xff80)
				return val;
			throw IntegerOverflow();
		}
		else
		{
			uint16_t val = (uint16_t)l * (uint16_t)r;
			if (val&0xff00) throw IntegerOverflow();
			return val;
		}
	case 2:
		if (is_signed<T>())
		{
			int32_t val = (int32_t)l * (int32_t)r;
			// die oberen 17 Bit müssen identisch sein
			if ((val&0xffff8000) == 0 || (val&0xffff8000) == 0xffff8000)
				return val;
			throw IntegerOverflow();
		}
		else
		{
			uint32_t val = (uint32_t)l * (uint32_t)r;
			if (val & 0xffff0000UL) throw IntegerOverflow();
			return val;
		}
	case 4:
		if (is_signed<T>())
		{
			int64_t val = (int64_t)l * (int64_t)r;
			// die oberen 33 Bit müssen identisch sein
			if( (val&0xffffffff80000000LL) == 0 ||
				(val&0xffffffff80000000LL) == 0xffffffff80000000LL)
				return val;
			throw IntegerOverflow();
		}
		else
		{
			uint64_t val = (uint64_t)l * (uint64_t)r;
			if (val & 0xffffffff00000000ULL) throw IntegerOverflow();
			return val;
		}
	case 8:
		if (is_signed<T>())
		{
			if ( (l^r) >= T(0) )
			{
				// gleiche Vorzeichen
				if (l > T(0)) // => beide positiv
				{
					if (maximum<T>() / l < r) throw IntegerOverflow();
				}
				else
				{
					// siehe SafeInt-Klasse
					if (l == minimum<T>() || r == minimum<T>())
						throw IntegerOverflow();
					if (maximum<T>() / (-l) < (-r))
						throw IntegerOverflow();
				}
			}
			else
			{
				// ungleiche Vorzeichen
				if (l < T(0))
				{
					if (l < minimum<T>() / r)
						throw IntegerOverflow();
				}
				else
				{
					if (r < minimum<T>() / l)
						throw IntegerOverflow();
				}
			}
		}
		return l*r;
	default:
		// Typ T nicht unterstützt!
		throw IntegerOverflow();
	}
}

template< typename T > inline T divI(T l, T r)
{
	// nur ein Fall zu berücksichtigen: MINIMUM/-1 => MAXIMUM+1
	if (is_signed<T>() && l==minimum<T>() && r==-1)
		throw IntegerOverflow();
	return l/r;
}

template< typename T,typename U > inline T assignI(T & l, U const & r)
{
	// Zuweisung erlaubt, falls T mehr als oder gleich viele Bits wie U hat
//	if (sizeof(T) < sizeof(U)) throw IntegerOverflow();
	// bei gleicher Länge, aber unterschiedlicher "signedness"
	if (sizeof(T) == sizeof(U) && is_signed<T>() != is_signed<U>()
		&& (r > maximum<T>() || r < minimum<T>()))
		throw IntegerOverflow();
	return l=r;
}

#endif

