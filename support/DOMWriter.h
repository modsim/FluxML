#ifndef DOMWRITER_H
#define DOMWRITER_H

#include <cstdio>
#include <xercesc/dom/DOM.hpp>
#include "XMLException.h"

namespace flux {
namespace xml {
	
/**
 * Schnittstelle für einen "Schreiber" eines DOM-Dokuments.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class DOMWriter
{
protected:
	/** Pretty-Printing */
	bool f_pretty_print_;
	/** GZIP-Kompression */
	bool f_compression_;

public:
	/**
	 * Constructor.
	 */
	DOMWriter() : f_pretty_print_(false), f_compression_(false) { }

	/**
	 * Virtueller Destructor.
	 */
	virtual ~DOMWriter() { }

	/**
	 * Schreibt ein DOM-Dokument auf die angegebene URI. In der
	 * DOM L3-Implementierung wird die LSSerializer verwendet.
	 *
	 * @param uri Ziel-URI des DOM-Dokuments
	 */
	virtual void writeToURI(char const * uri) const = 0;
	
	/**
	 * Schreibt ein DOM-Dokument auf einen Stream. In der
	 * DOM L3-Implementierung wird hier LSOutput verwendet.
	 *
	 * @param out_file Ausgabe-Stream
	 */
	virtual void writeToStream(FILE * out_file) const = 0;
	
	/**
	 * Schreibt das DOM-Dokument auf einen String. In der DOM L3-Impl.
	 * wird hier LSSerializer verwendet.
	 *
	 * @return String mit dem DOM-Dokument
	 */
	virtual char const * writeToString(size_t & bufsize) const = 0;
	
	/**
	 * Gibt einen Zeiger auf den verwalteten DOM-Tree zurück.
	 *
	 * @return Zeiger auf den internen DOM-Tree
	 */
	virtual XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *
		getDOMDocument() = 0;

	/**
	 * Beeinflussung der Ausgabe: Pretty-Print
	 */
	virtual void setPrettyPrint(bool tf) { f_pretty_print_ = tf; }

	/**
	 * Beeinflussung der Ausgabe: Kompression
	 */
	virtual void setCompression(bool tf) { f_compression_ = tf; }
};

} // namespace flux::xml
} // namespace flux

#endif

