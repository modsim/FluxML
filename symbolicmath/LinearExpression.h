#ifndef LINEAREXPRESSION_H
#define LINEAREXPRESSION_H

#include "ExprTree.h"
#include "charptr_map.h"

namespace flux {
namespace symb {

class NonLinearExpressionException {};

/**
 * Eine Klasse zur Extraktion linearer Koeffizienten aus Gleichungen
 * (oder Ungleichungen).
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class LinearExpression
{
private:
	/** eine Gleichung f(v1, ... , vn) - g(v1, ... ,vn) = 0 */
	ExprTree * Eq_;
	/** lineare Koeffizienten der Gleichung Eq_ */
	charptr_map< double > C_;
	/** handelt es sich um eine (Un-)Gleichung? */
	bool is_solvable_;

private:

	/**
	 * Flippt die Vorzeichen der Koeffizienten.
	 */
	void flipSigns();

	/**
	 * Normalisiert das Vorzeichen der Koeffizienten und
	 * multipliziert ggfs. die Gleichung mit -1, falls
	 * die Mehrheit der Koeffizienten negativ ist.
	 * Macht die Konstante -0 zu 0.
	 */
	bool normalizeSigns();

	/**
	 * Baut der Ausdruck aus den Koeffizienten neu auf. Das Ergebnis
	 * ist ein Ausdruck der Form:
	 *
	 *  c1*v1+...+cn*vn = const.
	 */
	void rebuildExpression(bool flip_comp_op);

	/**
	 * Funktion zum Extrahieren von Koeffizienten aus linearen
	 * Gleichungen. Übergeben wird eine Gleichung E der Form
	 * f(v1, ... , vn) = g(v1, ... , vn) (der Operator op_eq(=) muß
	 * die Wurzel von E sein!). Zurückgegeben werden die Koeffizenten
	 * von v1, ... , vn der umgeformten Gleichung
	 * f(v1, ... , vn) - g(v1, ... ,vn) = 0.
	 *
	 * @param E eine (hoffentlich) lineare Gleichung
	 * @param positive aktuelles Vorzeichen im Ausdrucksbaum
	 * @throws NonLinearExpressionException, falls E nicht linear
	 */
	void extractLinearCoeffs(ExprTree * E, bool positive = true);

public:
	/**
	 * Constructor.
	 *
	 * @param E Ausdruck, lineare (Un)Gleichung
	 */
	LinearExpression(ExprTree const * E);

	/**
	 * Destructor
	 */
	inline ~LinearExpression() { delete Eq_; }

public:
	/**
	 * Gibt eine "schönere", einfachere, rekonstruierte Version des
	 * Ausdrucks zurück.
	 *
	 * @return vereinfachter, äquivalenter Ausdruck
	 */
	inline ExprTree const * get() const { return Eq_; }

	/**
	 * Gibt eine Map mit den Koeffizienzen der linearen (Un)Gleichung
	 * zurück. Die Map bezieht sich auf den Ausdruck der dem Constructor
	 * übergeben wurde. Die Koeffizienten im rekonstruierten Ausdruck
	 * haben evtl. andere Vorzeichen. Der Schlüssel "1" ist garantiert
	 * immer in der Map enthalten und zeigt auf den Wert der Konstanten
	 * (im Zweifelsfall 0, falls die (Un)Gleichung keine Konstante
	 * enthält).
	 *
	 * @return Abbildung mit Koeffizienten der linearen (Un)Gleichung
	 */
	charptr_map< double > const & getLinearCoeffs() { return C_; }

	/**
	 * Gibt die im Ausdruck vorkommenden Variablennamen
	 * (nur Variablen mit Koeffizient != 0) zurück.
	 *
	 * @return Liste von Variablennamen
	 */
	inline charptr_array getVarNames() const
	{
		charptr_map< double >::const_iterator i,j = C_.find("1");
		charptr_array vars;
		for (i=C_.begin(); i!=C_.end(); ++i)
			if (i != j) vars.add(i->key);
		return vars;
	}

	/**
	 * Arrayzugriffsoperator.
	 * Gibt den Wert eines Koeffizienten zurück.
	 */
	inline double operator[](char const * vname) const
	{
		charptr_map< double >::const_iterator i = C_.find(vname);
		if (i == C_.end())
			return 0.;
		return i->value;
	}

	/**
	 * Zuweisungsoperator
	 */
	LinearExpression & operator=(LinearExpression const & rval);
	
	/**
	 * Vergleichsoperator. Testet die normalisierten Ausdrücke auf
	 * strukturelle Gleichheit (=semantische Gleichheit).
	 */
	bool operator==(LinearExpression const & rval) const;

}; // class LinearExpression

} // namespace flux::symb
} // namespace flux

#endif

