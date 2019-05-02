#ifndef CONVERSIONS_H
#define CONVERSIONS_H

#include <typeinfo>
#include <string>
#include <gmpxx.h>
#include "cstringtools.h"

/**
 * Konvertierung von template-Typ Stype (mpq_class,double)
 * nach double.
 *
 * @param v Wert
 * @return Floating-Point Zahl mit double precision
 */
template< typename Stype > inline double toDouble(Stype const & v)
{
	if (typeid(Stype) == typeid(mpq_class))
		return reinterpret_cast< mpq_class const & >(v).get_d();
	// einfachen Typen-Cast versuchen:
	return reinterpret_cast< double const & >(v);
}

/**
 * Konvertierung von template-Typ Stype (mpq_class,double)
 * in einen String.
 *
 * @param v Wert
 * @return String-Wert
 */
template< typename Stype > inline std::string toString(Stype const & v)
{
	if (typeid(Stype) == typeid(mpq_class))
	{
		return reinterpret_cast< mpq_class const & >(v).get_str();
	}
	else if (typeid(Stype) == typeid(double))
	{
		char numbuf[32] = "\0";
		dbl2str(numbuf,reinterpret_cast< double const & >(v),32);
		return std::string(numbuf);
	}
	else return std::string();
}

#endif

