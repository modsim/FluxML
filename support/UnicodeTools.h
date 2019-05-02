#ifndef UNICODETOOLS_H
#define UNICODETOOLS_H

#include <string>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/XMLString.hpp>

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/**
 * Klasse zur Konvertierung von "Unicode" nach "ASCII"
 * (eigentlich: Konvertierung von UTF16 nach UTF-8).
 *
 * UTF2ASCII u2a(mml_apply);
 * char * blabla = u2a;
 */
class UTF2ASCII
{
private:
	/** Konvertierter String */
	char * utf8_;

public:
	/**
	 * Constructor.
	 *
	 * @param UTF-16 codierter String
	 */
	UTF2ASCII(XMLCh const * unicode);

	/**
	 * Destructor.
	 */
	inline ~UTF2ASCII() { delete[] utf8_; }

public:
	/**
	 * Cast-Operator nach "char *".
	 *
	 * @return Zeiger auf konvertierten String
	 */
	inline operator char * () { return utf8_; }
	
	/**
	 * Cast-Operator nach "char const *".
	 *
	 * @return Zeiger auf konvertierten String
	 */
	inline operator char const * () const { return utf8_; }

};

/**
 * Klasse zur Konvertierung von "ASCII" nach "Unicode".
 * (eigentlich: Konvertierung vom lokalen Charset nach UTF-16)
 *
 * Achtung! Anwendung wiefolgt:
 *   ASCII2UTF a2u("blabla");
 *   XMLCh * blabla = a2u;
 * Falsch, weil Destructor aufgerufen wird:
 *   XMLCh * blabla = ASCII2UTF("blabla")
 */
class ASCII2UTF
{
private:
	/** Konvertierter String */
	XMLCh * unicode_;

public:
	/**
	 * Constructor.
	 *
	 * @param utf8 UTF8/ASCII-String
	 */
	inline ASCII2UTF(char const * utf8)
	{
		if (utf8)
			unicode_ = XN XMLString::transcode(utf8);
		else
			unicode_ = 0;
	}

	/**
	 * Constructor.
	 *
	 * @param utf8 UTF8/ASCII-String
	 */
	inline ASCII2UTF(std::string const & utf8)
	{
		unicode_ = XN XMLString::transcode(utf8.c_str());
	}

	/**
	 * Destructor.
	 */
	inline ~ASCII2UTF() {
		if (unicode_)
			XN XMLString::release(&unicode_);
	}

public:
	/**
	 * Cast-Operator nach "XMLCh *".
	 *
	 * @return Zeiger auf konvertierten String
	 */
	inline operator XMLCh * () { return unicode_; }
	
	/**
	 * Cast-Operator nach "XMLCh const *".
	 *
	 * @return Zeiger auf konvertierten String
	 */
	inline operator XMLCh const * () const { return unicode_; }

};

} // namespace flux::xml
} // namespace xml

#define U2A UTF2ASCII
#define A2U ASCII2UTF

#endif
