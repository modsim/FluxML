#ifndef MATHMLDOCUMENT_H
#define MATHMLDOCUMENT_H

#include <list>
#include <string>
#include <xercesc/dom/DOM.hpp>
#include "ExprTree.h"
#include "DOMWriter.h"
#include "MathMLContentObject.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

class MathMLDeclare;
class MathMLExpression;
class MathMLMatrix;
class MathMLVector;
class MathMLLambdaExpression;

/**
 * Abbildung eines MathML-Dokuments.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class MathMLDocument : public MathMLContentObject
{
private:
	/** Liste mit Definitionen */
	std::list< MathMLDeclare * > def_list_;
	/** Liste mit Content-Objekten */
	std::list< MathMLContentObject * > cont_list_;
	/** xml::DOMWriter-Objekt */
	DOMWriter * writer_;

public:
	/** Dokumenttypen eines MathML-Dokuments */
	enum MathMLType {
		mml_content, mml_presentation,
		mml_content_with_presentation,
		mml_presentation_with_content
	};

public:
	/**
	 * Constructor.
	 * Parst ein MathML-Dokument aus einem DOM-Tree.
	 *
	 * @param doc Document-Wurzel eines MathML-Dokuments
	 */
	MathMLDocument(
		XN DOMDocument * doc
		);

	/**
	 * Constructor.
	 * Parst ein MathML-Dokument aus einem math-Element-Knoten
	 *
	 * @param math ein math-Element eines MathML-Dokuments
	 */
	MathMLDocument(
		XN DOMNode * math
		);

	/**
	 * Erzeugt ein leeres MathML-Dokument.
	 */
	MathMLDocument();
	
	/**
	 * Destructor.
	 * Zerstört das MathMLDocument-Objekt und alle enthaltenen
	 * Definitionen.
	 */
	virtual ~MathMLDocument();

public:
	/*
	 * Factory-Methoden (für die Einhaltung von RAII) für
	 * Construction & Copy-Construction.
	 */

	/**
	 * Erzeugung eines Ausdrucks aus einem ExprTree-Objekt
	 *
	 * @param expr Ausdruck
	 */
	MathMLExpression * createExpression(
		symb::ExprTree const * expr
		);
	
	/**
	 * Kopie eines Ausdrucks aus einem bestehenden
	 * MathMLExpression-Objekt.
	 *
	 * @param copy Ausdruck (MathMLExpression)
	 */
	MathMLExpression * createExpression(
		MathMLExpression const & copy
		);

	/**
	 * Erzeugung einer rows X cols-Matrix.
	 *
	 * @param rows Anzahl der Zeilen
	 * @param cols Anzahl der Spalten
	 */
	MathMLMatrix * createMatrix(
		int rows,
		int cols
		);

	/**
	 * Erzeugung einer Matrix als Kopie einer bestehenden
	 * Matrix (MathMLMatrix).
	 * 
	 * @param copy Matrix (MathMLMatrix)
	 */
	MathMLMatrix * createMatrix( // Kopie
		MathMLMatrix const & copy
		);
	
	/**
	 * Erzeugung eines Vektors der Dimension dim.
	 * Nach Erzeugung kann der Vektor als Zeilen- oder
	 * Spaltenvektor gekennzeichnet werden (siehe MathMLVector).
	 *
	 * @param dim Länge des Vektors
	 */
	MathMLVector * createVector(
		int dim
		);

	/**
	 * Erzeugung eines Vektors als Kopie eines bestehenden
	 * Vektors (MathMLVector).
	 *
	 * @param copy Vektor (MathMLVector)
	 */
	MathMLVector * createVector(
		MathMLVector const & copy
		);
	
	/**
	 * Erzeugung eines Lambda-Ausdrucks aus einer Parameterliste
	 * und einem Ausdruck.
	 *
	 * @param vars Parameterliste
	 * @param expr Ausdruck
	 */
	MathMLLambdaExpression * createLambdaExpression(
		std::list< std::string > const & vars,
		MathMLExpression * expr
		);

	/**
	 * Erzeugung eines Lambda-Ausdrucks als Kopie eines bestehenden
	 * Lambda-Ausdrucks.
	 *
	 * @param copy Lambda-Ausdruck (MathMLLambdaExpression)
	 */
	MathMLLambdaExpression * createLambdaExpression(
		MathMLLambdaExpression const & copy
		);

	/**
	 * Erzeugung einer Deklaration aus einem (optionalen) Namen und
	 * einem Wert (MathMLContentObject).
	 *
	 * @param cont_obj Wert (MathMLContentObject)
	 * @param name optionaler Name der Deklaration
	 */
	MathMLDeclare * createDeclare(
		MathMLContentObject * cont_obj,
		std::string const & name = ""
		);

	/**
	 * Erzeugung einer Deklaration als Kopie einer bestehenden
	 * Deklaration.
	 *
	 * @param copy Deklaration (MathMLDeclare)
	 */
	MathMLDeclare * createDeclare(
		MathMLDeclare const & copy
		);
	
public:
	/**
	 * Gibt alle Definitionen mit einem bestimmten Typ in Form einer
	 * Liste zurück.
	 *
	 * @param type Content-Typ
	 * @return Liste der Deftinionen mit dem Typ type
	 */
	std::list< MathMLDeclare const * > getDefinitionsByType(
		MathMLContentObject::Type type
		) const;

	/**
	 * Gibt alle Definitionen zurück, deren Name auf einen regulären Ausdruck
	 * passt.
	 *
	 * @param regexp Regulärer Ausdruck
	 * @return alle Definitionen, deren name auf regexp passt
	 */
	std::list< MathMLDeclare const * > getDefinitionsByRegExp(
		std::string const & regexp
		) const;

	/**
	 * Gibt alle Definitionen zurück, deren Name auf einen regulären
	 * Ausdruck passt und die einen bestimmten Typ (MathMLContentObject)
	 * haben.
	 *
	 * @param regexp Regulärer Ausdruck für Definitionsname
	 * @param type MathMLContentObject-Typ
	 */
	std::list< MathMLDeclare const * > getDefinitionsByRegExpType(
		std::string const & regexp,
		MathMLContentObject::Type type
		);

	/**
	 * Serialisiert die Definitionen des MathML-Dokuments in einen
	 * DOM-Tree eines DOMWriter-Objekts
	 *
	 * @param writer DOMWriter-Objekt
	 * @param type zu generierender Dokumenttyp
	 */
	void serializeToDom(
		XN DOMDocument * doc,
		MathMLType type
		) const;

private:
	/**
	 * Parst das math-Element des MathML-Dokuments (das Wurzel-Element)
	 *
	 * @param node Knoten mit den Wurzel-Element math
	 */
	void parseMath(
		XN DOMNode * node
		);

	/**
	 * Parst Content-MathML-Elemente, die an der Wurzel des Content-MathML,
	 * direkt hinter math oder annotated-xml, stehen können
	 *
	 * @param node Wurzel-Knoten des Content-MathML
	 */
	void parseContent(
		XN DOMNode * node
		);

	/**
	 * Parst das semantics-Element von MathML, in das Presentation-MathML
	 * und Content-MathML eingebettet sein können.
	 *
	 * @param node Element-Knoten des semantics-Elements
	 */
	void parseSemantics(
		XN DOMNode * node
		);

public:
	/**
	 * Schreibt die Definitionen aus der Definitionsliste in den
	 * DOM-Tree des MathML-Dokuments.
	 *
	 * @param node Wurzel-Element (math) des MathML-Dokuments
	 */
	void serializeContentMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Schreibt die Definitionen aus der Definitionsliste in den
	 * DOM-Tree des Presentation-MathML-Dokuments.
	 * 
	 * @param node Wurzel-Element (math) des MathML-Dokuments
	 */
	void serializePresentationMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Schreibt die Definitionen aus der Definitionsliste in den
	 * DOM-Tree eines kombinierten Content/Presentation-MathML-Dokuments.
	 * Das Resultat is ein Content-MathML-Dokument, in dem ein Presentation-
	 * MathML-Dokument eingebettet ist.
	 * 
	 * @param node Wurzel-Element (math) des MathML-Dokuments
	 */
	void serializeContentWithPresentationMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	/**
	 * Schreibt die Definitionen aus der Definitionsliste in den
	 * DOM-Tree eines kombinierten Content/Presentation-MathML-Dokuments.
	 * Das Resultat is ein Content-MathML-Dokument, in dem ein Presentation-
	 * MathML-Dokument eingebettet ist.
	 * 
	 * @param node Wurzel-Element (math) des MathML-Dokuments
	 */
	void serializePresentationWithContentMathML(
		XN DOMDocument * doc,
		XN DOMNode * node
		) const;

	Type getType() const { return co_mathml_doc; }

	/**
	 * Dummy-Implementierung der toString-Methode.
	 *
	 * @return ein leerer String
	 */
	std::string toString() const { return std::string(); }

	/**
	 * Erzeugt ein DOMWriter-Objekt für MathML
	 */
	DOMWriter & getDOMWriter();

}; // class MathMLDocument

} // namespace flux::xml
} // namespace flux

#endif

