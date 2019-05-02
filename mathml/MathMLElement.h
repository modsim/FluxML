#ifndef MATHMLELEMENT_H
#define MATHMLELEMENT_H

#include <xercesc/dom/DOM.hpp>

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/**
 * "Datenbank-Klasse" zur Klassifizierung von MathML-Elementen.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
namespace MathMLElement
{

/**
 * Stellt fest, ob es sich bei einem angegebenen MathML-Element um
 * ein Content-MathML-Element handelt.
 *
 * @param name Name des Elements
 * @return true, falls das Element ein Content-MathML-Element ist
 */
bool isContentElement(char const * name);

/**
 * Stellt fest, ob es sich bei einem angegebenen MathML-Element um
 * ein Presentation-MathML-Element handelt.
 *
 * @param name Name des Elements
 * @return true, falls das Element ein Presentation-MathML-Element ist
 */
bool isPresentationElement(char const * name);

/**
 * Stellt fest, ob es sich bei einem angegebenen DOM-Knoten um
 * ein Content-MathML-Element handelt.
 *
 * @param node DOM-Knoten
 * @return true, falls das Element ein Content-MathML-Element ist
 */
bool isContentNode(XN DOMNode * node);

/**
 * Stellt fest, ob es sich bei einem angegebenen DOM-Knoten um
 * ein Presentation-MathML-Element handelt.
 *
 * @param node DOM-Knoten
 * @return true, falls das Element ein Presentation-MathML-Element ist
 */    
bool isPresentationNode(XN DOMNode * node);

/**
 * Stellt fest, ob es sich bei einem Operator um einen binären Operator
 * handelt.
 *
 * @param name Name es Operator-Elements
 * @return true, falls es sich um einen n-ären-Operator handelt
 */
bool isBinaryOperator(char const * name);

/**
 * Stellt fest, ob es sich bei einem Operator um einen unären Operator
 * handelt.
 *
 * @param name Name es Operator-Elements
 * @return true, falls es sich um einen n-ären-Operator handelt
 */
bool isUnaryOperator(char const * name);

/**
 * Stellt fest, ob es sich bei einem Operator um einen n-ären Operator
 * handelt.
 *
 * @param name Name es Operator-Elements
 * @return true, falls es sich um einen n-ären-Operator handelt
 */
bool isNAryOperator(char const * name);

/**
 * Stellt fest, ob ein Name einem Operator zugeordnet ist.
 *
 * @param name Name
 * @return true, falls name ein Operator ist
 */
bool isOperator(char const * name);

/**
 * Navigiert zum nächsten auswertbaren Geschwister-Knoten im DOM-Tree.
 *
 * @param node ein Knoten im DOM-Tree
 * @return nächster Knoten
 */
XN DOMNode * nextNode(XN DOMNode * node);

/**
 * Navigiert zum nächsten Element-Geschwister-Knoten im DOM-Tree.
 *
 * @param node ein Knoten im DOM-Tree
 * @return ein Element-Knoten oder 0
 */
XN DOMNode * nextElementNode(XN DOMNode * node);

/**
 * Überspringt Text-Geschwister-Knoten im DOM-Tree.
 *
 * @param node ein Knoten im DOM-Tree
 * @return erster nicht-Text-Geschwister-Knoten
 */
XN DOMNode * skipTextNodes(XN DOMNode * node);

/**
 * Überspringt alle nicht auswertbaren Knoten (Kommentare, etc.).
 *
 * @param node ein Knoten im DOM-Tree
 * @return erster auswertbarer Knoten
 */
XN DOMNode * skipJunkNodes(XN DOMNode * node);


} // namespace MathMLElement

} // namespace flux::xml
} // namspace flux

#endif

