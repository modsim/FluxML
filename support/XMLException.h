#ifndef XMLEXCEPTION_H
#define XMLEXCEPTION_H

#include <cstdarg>
#include <xercesc/dom/DOM.hpp>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

namespace flux {
namespace xml {

/**
 * Exception-Klasse für XML-Fehler.
 * Mit Hilfe des Tagging-DOM-Parser kann Zeile und Spalte zu einem
 * gegebenen DOMNode ermittelt werden (das funktioniert allerdings
 * nicht immer?!).
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class XMLException
{
protected:
	/** Fehlermeldung */
	char * msg_;

	/** Zeile der XML-Datei */
	int xml_line_;

	/** Spalte der XML-Datei */
	int xml_column_;

public:
	/**
	 * Constructor.
	 * Es wird der DOM-Knoten übergeben, in dem ein Problem erkannt wurde.
	 * Mit Hilfe der Tags kann die Zeile/Spalte der XML-Datei identifiziert
	 * werden. Weiterhin kann die Bezeichnung der Quell-Datei und die
	 * Quell-Code-Zeile übergeben werden. Es empfiehlt sich der Einsatz des
	 * Makros XMLEXCEPTION, das diese Felder automatisch füllt.
	 *
	 * @param dom_node DOM-Knoten, in dem ein Problem erkannt wurde
	 * @param fmt Format-String für Fehlermeldung
	 * @param src_file Quell-Datei (optional)
	 * @param src_func Quell-Funktion (optional)
	 * @param src_line Zeile in der Quell-Datei (optional)
	 */
	XMLException(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node,
		char const * fmt, ...
		)
#ifdef __GNUG__
		__attribute__((format(printf,3,4)))
#endif
		;

	/**
	 * Einfacher Constructor.
	 * Es wird eine Fehlermeldung als String übergeben.
	 *
	 * @param fmt Format-String für Fehlermeldung
	 */
	XMLException(
		char const * fmt, ...
		)
#ifdef __GNUG__
		__attribute__((format(printf,2,3)))
#endif
		;

	/**
	 * Copy-Constructor.
	 * Wegen den C-Strings unerlässlich.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	XMLException(XMLException const & copy);

	/**
	 * Destructor.
	 */
	~XMLException();

private:
	char const * get_node_path(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node
		);

public:
	/**
	 * Cast-Operator -> C-String
	 *
	 * @return Zeichenkette mit Fehlermeldung
	 */
	inline operator char const * () const { return msg_; }

	/**
	 * Alternativ, eine toString-Methode.
	 *
	 * @return Zeichenkette mit Fehlermeldung
	 */
	inline char const * toString() const { return msg_; }

	/**
	 * Gibt die dem Fehler zugeordnete Zeile der XML-Datei zurück.
	 *
	 * @return Zeile der XML-Datei
	 */
	inline int getXMLLine() const { return xml_line_; }

	/**
	 * Gibt die dem Fehler zugeordnete Spalte der XML-Datei zurück.
	 *
	 * @return Spalte der XML-Datei
	 */
	inline int getXMLColumn() const { return xml_column_; }

	/**
	 * Setzt die dem Fehler zugeordnete Zeilennummer der XML-Datei.
	 *
	 * @param line Zeilennummer der XML-Datei
	 */
	inline void setXMLLine(size_t line) { xml_line_ = line; }

	/**
	 * Setzt die dem Fehler zugeordnete Spaltennummer der XML-Datei.
	 *
	 * @param column Spaltennummer der XML-Datei
	 */
	inline void setXMLColumn(size_t column) { xml_column_ = column; }

};

} // namespace flux::xml
} // namespace flux

#endif

