#include <cstddef>
#include <cstring>
extern "C"
{
#include <stdint.h>
}
#include "fluxml_config.h"
#include "hash_functions.h"

size_t mxkoo_hashf(const mxkoo & ij)
{
	size_t a = 0;
	size_t i = ij.i_^0x4ed3aa62, j = ij.j_^0x363f7706;

	for (size_t byte = 0; byte < sizeof(size_t); byte++)
	{
		a += ((i>>(byte*8)) & 0xff)<<4;
		a += ((i>>(byte*8)) & 0xff)>>4;
		a *= 11;
		a += ((j>>(byte*8)) & 0xff)<<4;
		a += ((j>>(byte*8)) & 0xff)>>4;
		a *= 11;
	}
	return a;
}

size_t mxkooo_hashf(const mxkooo & ijk)
{
	size_t a = 0;
	size_t i = ijk.i_^0x4ed3aa62, j = ijk.j_^0x363f7706, k = ijk.k_^0x1bfedf72;
	for (size_t byte = 0; byte < sizeof(size_t); byte++)
	{
		a += ((i>>(byte*8)) & 0xff)<<4;
		a += ((i>>(byte*8)) & 0xff)>>4;
		a *= 11;
		a += ((j>>(byte*8)) & 0xff)<<4;
		a += ((j>>(byte*8)) & 0xff)>>4;
		a *= 11;
		a += ((k>>(byte*8)) & 0xff)<<4;
		a += ((k>>(byte*8)) & 0xff)>>4;
		a *= 11;
	}
	return a;
}

size_t double_hashf(double const & val)
{
	size_t a = 0;
	char const * str = reinterpret_cast< char const* >(&val);

	a+=str[0]<<4; a+=str[0]>>4; a*=11;
	a+=str[1]<<4; a+=str[1]>>4; a*=11;
	a+=str[2]<<4; a+=str[2]>>4; a*=11;
	a+=str[3]<<4; a+=str[3]>>4; a*=11;
	a+=str[4]<<4; a+=str[4]>>4; a*=11;
	a+=str[5]<<4; a+=str[5]>>4; a*=11;
	a+=str[6]<<4; a+=str[6]>>4; a*=11;
	a+=str[7]<<4; a+=str[7]>>4; a*=11;
	return a;
}

size_t ptr_hashf(genericptr const & val)
{
	size_t a = 0;
	char const * str = reinterpret_cast< char const* >(&val);
	for (int i=0; i<SIZEOF_VOID_P; i++)
	{
		a+=str[i]<<4;
		a+=str[i]>>4;
		a*=11;
	}
	return a;
}

size_t uint_hashf(unsigned int const & val)
{
	size_t a = 0;
	char const * str = reinterpret_cast< char const* >(&val);

	for (int i=0; i<SIZEOF_INT; i++)
	{
		a+=str[i]<<4;
		a+=str[i]>>4;
		a*=11;
	}
	return a;

}

