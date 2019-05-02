#ifndef XMLELEMENT_H
#define XMLELEMENT_H

#include <xercesc/dom/DOM.hpp>
#include "XMLException.h"

namespace flux {
namespace xml {
namespace XMLElement {

/**
 * Navigiert zum nächsten auswertbaren Geschwister-Knoten im DOM-Tree.
 *
 * @param node ein Knoten im DOM-Tree
 * @return nächster Knoten
 */
XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *
	nextNode(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode * node);

/**
 * Navigiert zum nächsten Element-Geschwister-Knoten im DOM-Tree.
 *
 * @param node ein Knoten im DOM-Tree
 * @return ein Element-Knoten oder 0
 */
XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *
	nextElementNode(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode * node);

/**
 * Überspringt Text-Geschwister-Knoten im DOM-Tree.
 *
 * @param node ein Knoten im DOM-Tree
 * @return erster nicht-Text-Geschwister-Knoten
 */
XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *
	skipTextNodes(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode * node);

/**
 * Überspringt alle nicht auswertbaren Knoten (Kommentare, etc.).
 *
 * @param node ein Knoten im DOM-Tree
 * @return erster auswertbarer Knoten
 */
XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *
	skipJunkNodes(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode * node);

/**
 * Gibt den Typ des Knotens und weitere Informationen aus.
 *
 * @param node ein Knoten im DOM-Tree
 */
void dumpNode(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node);

/**
 * Erzeugt eine String-Repräsentation von einem DOMNode
 *
 * @param node ein Knoten im DOM-Tree
 * @return String, allokiert
 */
char const * nodeToString(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node);

/**
 * Matcht einen (Element-)Knoten.
 *
 * @param node ein Knoten im DOM-Tree (darf NULL sein)
 * @param name eine Knotenbezeichnung
 * @return false, falls Matching fehlschlägt, ansonsten true
 */
bool match(
	XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node,
	XMLCh const * name,
	XMLCh const * xmlns_uri = 0
	);

/**
 * Parst einen double-Wert.
 *
 * @param utf8str Unicode-String
 * @param value geparster double-Wert
 * @return true, falls erfolgreich
 */
bool parseDouble(XMLCh const * utf8str, double & value);

/**
 * Parst einen long int-Wert.
 *
 * @param utf8str Unicode-String
 * @param value geparster long int-Wert
 * @param base Basis (default = 10)
 * @return true, falls erfolgreich
 */
bool parseLongInt(
	XMLCh const * utf8str,
	long int & value,
	unsigned short base = 10
	);

/**
 * Parst einen int-Wert.
 *
 * @param utf8str Unicode-String
 * @param value geparster int-Wert
 * @param base Basis (default = 10)
 * @return true, falls erfolgreich
 */
bool parseInt(
	XMLCh const * utf8str,
	int & value,
	unsigned short base = 10
	);

/**
 * Auflösen von Element-Namen in Gegenwart von XML-Namespaces.
 * Bei übereinstimmenden xmlns wird der lokale Elementname zurückgegeben.
 *
 * @param node Node
 * @param xmlns_uri optionale xmlns URI
 * @return lokaler Elementname, falls ermittelbar.
 */
XMLCh const * getName(
	XERCES_CPP_NAMESPACE_QUALIFIER DOMNode const * node,
	XMLCh const * xmlns_uri = 0
	);

} // namespace flux::xml::XMLElement
} // namespace flux::xml
} // namespace flux

#endif

