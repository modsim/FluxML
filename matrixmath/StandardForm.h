#ifndef STANDARDFORM_H
#define STANDARDFORM_H

#include <list>
#include "charptr_array.h"
#include "charptr_map.h"
#include "SMatrix.h"
#include "MVector.h"
#include "ExprTree.h"

namespace flux {
namespace la {

/**
 * Abbildung eines Systems von linearen Ungleichungen in der
 * Standard-Form A*x<=b inklusive zusätzlicher unterer und oberer
 * Limits für x. Die getrennte Speicherung der Variablenlimits und der
 * komplizierteren Constraints kommt den LP-Solvern entgegen.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class StandardForm
{
	friend class LPProblem;

protected:
	/** Abbildung Variablen-Name -> Index */
	mutable charptr_map< int > var_indices_;
	/** Liste der Constraints in geparster Form */
	std::list< charptr_map< double > > coeff_list_;
	/** Untere Variablenlimits */
	charptr_map< double > lvbound_;
	/** Obere Variablenlimits */
	charptr_map< double > uvbound_;
	/** Constraint-Matrix A */
	mutable SMatrix A_;
	/** Rechte Seite des Constraint-Systems */
	mutable MVector b_;
	/** Muss build() aufgerufen werden? */
	mutable bool dirty_;

public:
	/**
	 * Constructor.
	 */
	StandardForm() : dirty_(true) {}

	/**
	 * Constructor.
	 *
	 * @param A Constraint-Matrix
	 * @param b Constraint-Vektor
	 * @param vnames Variablenbezeichnungen
	 */
	StandardForm(
		MMatrix const & A,
		MVector const & b,
		char const ** vnames = 0
		);

	/**
	 * Destructor.
	 */
	virtual ~StandardForm() {}
	
protected:
	/**
	 * Fügt ein Constraint hinzu, falls es nicht schon aufgenommen war.
	 *
	 * @param coeffs Koeffizienten des Constraints
	 */
	bool addCoeffsUnique(charptr_map< double > const & coeffs);

public:
	/**
	 * Baut das Constraint-System A*x<=b.
	 */
	virtual void build() const;

	/**
	 * Einfügen eines neuen Constraints.
	 *
	 * @param E ExprTree-Objekt mit Constraint
	 * @return true, falls ein äquivalentes Constraint noch nicht
	 * 	eingetragen wurde
	 */
	bool addConstraint(symb::ExprTree * E);

	/**
	 * Shift-Operator zum Einfügen eines neuen Constraints.
	 *
	 * @param E ExprTree-Objekt mit Constraint
	 * @return true, falls ein äquivalentes Constraint noch nicht
	 * 	eingetragen wurde
	 */
	inline bool operator<<(symb::ExprTree * E) { return addConstraint(E); }

	/**
	 * Shift-Operator zum Einfügen eines neuen Constraints.
	 *
	 * @param expr String-Repräsentation des Constraints
	 * @return true, falls ein äquivalentes Constraint noch nicht
	 * 	eingetragen wurde
	 */
	bool operator<<(char const * expr);
	
	/**
	 * Setzt einer Variable eine untere Schranke.
	 *
	 * @param vn Variablenname
	 * @param lb Wert der unteren Schranke
	 * @return true, falls Schranke übernommen wurde
	 */
	bool setLowerBound(char const * vn, double lb);

	/**
	 * Setzt einer Variable eine obere Schranke.
	 *
	 * @param vn Variablenname
	 * @param ub Wert der obere Schranke
	 * @return true, falls Schranke übernommen wurde
	 */
	bool setUpperBound(char const * vn, double ub);
	
	/**
	 * Gibt die untere Schranke einer Variablen zurück.
	 *
	 * @param vn Variablenname
	 * @param lb untere Schranke
	 * @return true, falls eine untere Schranke existiert
	 */
	bool getLowerBound(char const * vn, double & lb) const;

	/**
	 * Gibt die obere Schranke einer Variablen zurück.
	 *
	 * @param vn Variablenname
	 * @param ub untere Schranke
	 * @return true, falls eine obere Schranke existiert
	 */
	bool getUpperBound(char const * vn, double & ub) const;

	/**
	 * Registiert einen Variablennamen.
	 * Mit dieser Methode ist es möglich zu garantieren, dass eine
	 * Bestimmte Variable in der StandardForm auftaucht.
	 *
	 * @param vn Variablenname
	 */
	void registerVariable(char const * vn);

	/**
	 * Ermittelt den Index einer Variable.
	 *
	 * @param vn Variablenname
	 * @return Index der Variable vn oder -1 falls vn ungültig ist
	 */
	int indexOf(char const * vn) const;

	/**
	 * Gibt den Namen der Variablen zurück, welche einer angegebenen
	 * Spalte der StandardForm zugeordnet ist.
	 *
	 * @param col Spalte
	 * @return Variablenname
	 */
	char const * getColumnVar(size_t col) const;
	
	/**
	 * Gibt ein Array mit den Variablennamen zurück (in der Reihenfolge
	 * des Spalten).
	 *
	 * @return Array mit Variablennamen
	 */
	charptr_array getVars() const;

	/**
	 * Gibt die linke Seite des Constraint-Systems zurück.
	 *
	 * @return Matrix A
	 */
	inline SMatrix const & getLHS() const { return A_; }

	/**
	 * Gibt die rechte Seite des Constraint-Systems zurück.
	 *
	 * @return Vektor b
	 */
	inline MVector const & getRHS() const { return b_; }

	/**
	 * Gibt die unteren Schranken der Variablen zurück.
	 *
	 * @return untere Schranken der Variablen
	 */
	inline charptr_map< double > const & getLowerBounds() const { return lvbound_; }

	/**
	 * Gibt die oberen Schranken der Variablen zurück.
	 *
	 * @return obere Schranken der Variablen
	 */
	inline charptr_map< double > const & getUpperBounds() const { return uvbound_; }

	/**
	 * Erfüllt ein Punkt alle Constraints?
	 *
	 * @param x ein Punkt
	 * @param tol Toleranz gegenüber Constraintverletzung
	 * @param verbose Verletzte Constraints melden (default: false)
	 * @return true, falls der übergebene Punkt alle Constraints erfüllt
	 */
	bool isFeasible(double const * x, double tol, bool verbose=false) const;

	/**
	 * Gibt true zurück, falls das System von Ungleichungen feasible ist,
	 * d.h. die beschriebene Region nicht leer ist.
	 *
	 * @return true, falls das System von Ungleichungen feasible ist
	 */
	bool isFeasible() const;

	/**
	 * Distanz-Funktion.
	 * Mißt den minimalen Abstand D des Punktes x0 zur Hyperfläche, die
	 * durch das Constraint i beschrieben wird. Ist der Distanzwert D
	 * negativ, so wurde das Constraint um |D| verletzt. Ist er positiv,
	 * so ist das Constraint erfüllt. Nur nach build() aufrufen!
	 *
	 * @param i Index des Constraints
	 * @param x0 Punkt
	 * @return Distanzwert D
	 */
	double distToConstraint(size_t i, double const * x0) const;

	/**
	 * Kürzester Abstand zum am stärksten verletzten Constraint. Sind alle
	 * Constraints erfüllt, wird 0 zurückgegeben. Ansonsten wird der
	 * Abstand zurückgegeben. Damit lässt sich eine einfache
	 * Rampen-Funktion bauen. Nur nach build() aufrufen!
	 *
	 * @param x Punkt
	 * @return Kürzester Abstand zum am stärksten verletzten Constraint
	 */
	double distance(double const * x) const;

	/**
	 * Gibt die Anzahl der Variablen zurück.
	 *
	 * @return Anzahl der Variablen im Constraint-System
	 */
	inline size_t getNumVars() const
	{
		if (dirty_) build();
		return A_.cols();
	}

	/**
	 * Gibt die Anzahl der (echten) Ungleichungen zurück
	 */
	inline size_t getNumInequalities() const
	{
		if (dirty_) build();
		return A_.rows();
	}

	/**
	 * Gibt die Ungleichung einer angegebenen Zeile zurück.
	 * Der zurückgegebne Ausdruck muss vom Aufrufenden freigegeben
	 * werden.
	 *
	 * @param row Zeilenindex
	 * @return allokierters ExprTree-Objekt mit Ungleichung
	 */
	symb::ExprTree * getRowExpr(size_t row) const;

	/**
	 * Importiert die Constraints eines fremden StandardForm-Objekts
	 * ohne dabei Duplikate zu erzeugen.
	 *
	 * @param SF fremdes StandardForm-Objekt
	 * @return true, falls Constraints importiert wurden
	 */
	bool importConstraints(StandardForm const & SF);

	/**
	 * Berechnet, ausgehend vom feasible Punkt x, entlang eines
	 * Richtungsvektors r (mit Länge 1) die Distanzen zu den
	 * Constraints. Der zurückgegebene Wert d_neg ist die Distanz
	 * in die entgegengesetzte Richtung von r, Wert d_pos ist
	 * die Distanz in Richtung r.
	 *
	 * @param x Punkt (muss alle Constraints erfüllen) (in)
	 * @param r Richtungsvektor (Länge 1) (in)
	 * @param d_neg Distanz zu den Constraints in Richtung -r (out)
	 * @param d_pos Distanz zu den Constraints in Richtung +r (out)
	 */
	void distToConstraints(
		MVector const & x,
		MVector const & r,
		double & d_neg,
		double & d_pos
		) const;

	/**
	 * Erzeugt eine neue StandardForm die ein um ein epsilon
	 * geschrumpftes konvexes Gebiet beschreibt.
	 *
	 * @param eps Euklidische Distanz zwischen alten und neuen Ebenen
	 * @return eine um eps geschrumpfte StandardForm
	 */
	StandardForm shrink(double eps) const;

	/**
	 * Erzeugt eine neue StandardForm die ein um ein epsilon
	 * gewachsenes konvexes Gebiet beschreibt.
	 *
	 * @param eps Euklidische Distanz zwischen alten und neuen Ebenen
	 * @return eine um eps gewachsene StandardForm
	 */
	inline StandardForm grow(double eps) const { return shrink(-eps); }

	
	/**
	 * Debugging. Gibt das Constraint-System auf stdout aus.
	 *
	 * @param compact kompaktere, lesbarere ausgabe, falls true
	 */
	virtual void dump(bool compact = true) const;
        
        
};

/**
 * Standardform mit Penalty für verletzte Constraints.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class StandardFormPenalty
{
protected:
	/** Internes StandardForm-Objekt */
	StandardForm const & SF_;
	/** Berechnetes, analytisches Zentrum der Constraints */
	MVector center_;

public:
	/**
	 * Constructor.
	 *
	 * @param SF StandardForm-Objekt
	 * @param center Vorausberechnetes analytisches Zentrum der
	 * 	Ungleichungen in SF
	 */
	inline StandardFormPenalty(
		StandardForm const & SF,
		MVector const & center) : SF_(SF), center_(center) { }

private:
	/**
	 * Berechnet den Schnittpunkt Px auf dem Weg von center_ (P1) zum
	 * Punkt P2, der durch Schneiden von Constraint i entsteht.
	 * Gleichzeitig wird die Distanz zwischen P1 und Px berechnet.
	 *
	 * @param i Constraint (in)
	 * @param P1 Ausgangspunkt (center_) (in)
	 * @param P2 Zielpunkt (in)
	 * @param Px Schnittpunkt (out)
	 * @param dist Distanz zwischen P1 und Px
	 * @return true, falls ein Schnittpunkt zwischen P1 und P2 existiert
	 */
	bool intersectConstraint(
		size_t i,
		MVector const & P1,
		MVector const & P2,
		MVector & Px,
		double & dist
		) const;

	/**
	 * Berechnet den Schnittpunkt Px auf dem Weg von center_ (P1) zum
	 * Punkt P2, der durch Schnieden von Bound lo bzw up entsteht.
	 * Gleichzeitig wird die Distanz zwischen P1 und Px berechnet.
	 *
	 * @param var_idx Variablenindex des Bounds
	 * @param lo Zeiger auf Wert der unteren Grenze; 0 falls obere Grenze
	 * @param up Zeiger auf Wert der oberen Grenze; 0 falls untere Grenze
	 * @param P1 Ausgangspunkt (center_) (in)
	 * @param P2 Zielpunkt (in)
	 * @param Px Schnittpunkt (out)
	 * @param dist Distanz zwischen P1 und Px
	 * @return true, falls ein Schnittpunkt zwischen P1 und Px existiert
	 */
	bool intersectBound(
		size_t var_idx,
		double * lo,
		double * up,
		MVector const & P1,
		MVector const & P2,
		MVector & Px,
		double & dist
		) const;

	/**
	 * Berechnet den Schnittpunkt Px auf dem Rand des durch Ungleichungs-
	 * Constraints eingegrenzten Gebiets, der auf dem Weg zwischen
	 * center_ und dem Punkt P liegt.
	 *
	 * @param P Punkt (in)
	 * @param Px Schnittpunkt (out)
	 * @return true, falls ein Schnittpunkt existiert
	 */
	bool closestIntersection(
		MVector const & P,
		MVector & Px
		) const;
	
public:
	/**
	 * Berechnet einen Penalty-Wert und den naheliegendsten, noch
	 * gültigen Wert (innerhalb der eingeschränkten Region). Der
	 * Penalty-Wert entspricht der Euklidischen Distanz zwischen
	 * lastFeasible und x.
	 *
	 * @param x Punkt
	 * @param lastFeasible gültiger Wert in in kürzester Distanz
	 * 	auf dem Weg zum Zentrum der Ungleichungs-Constraints
	 * @return Euklidischen Distanz zwischen x und lastFeasible
	 */
	double penalty(
		MVector const & x,
		MVector & lastFeasible
		) const;

	/**
	 * Einfachere Version der Penalty-Methode.
	 *
	 * @param x Punkt
	 * @return Euklidischen Distanz zwischen x und letzten gültigen Punkt
	 */
	double penalty(double const * x) const;

	/**
	 * Gibt das Zentrum der Constraints zurück.
	 */
	inline MVector const & getCenter() const { return center_; }

};

} // namespace flux::la
} // namespace flux

#endif

