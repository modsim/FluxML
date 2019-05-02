/*
 * fhash_map / C++ universal template-based hash data structure
*/

#include "charptr_map.h"

size_t charptr_hashf(charptr const & str)
{
	char const * s = str;
	size_t a=0;
	char c;
	
	// This is the 'R5-Hash' algorithm used in ReiserFS.
	while ((c = *s) != '\0')
	{
		a+=c<<4; a+=c>>4; a*=11;
		s++;
	}
	return a;
}

