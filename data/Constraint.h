#ifndef CONSTRAINT_H
#define CONSTRAINT_H

extern "C"
{
#include <stdint.h>
}
#include <string>
#include "ExprTree.h"


namespace flux {
namespace data {

enum ParameterType {NET, XCH, POOL};

/*
 * *****************************************************************************
 * Klasse zur Abbildung von Constraints.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class Constraint
{
private:
	/** Bezeichnung des Constraint */
	char * name_;
	/** "Wert" des Constraint */
	symb::ExprTree * constraint_;
	/** Typ des Constraints: "netto" oder "exchange" */
	ParameterType parameter_type_;
	/** true, falls Constraint ein einfaches Werte-Constraint ist (varname=wert) */
	bool is_simple_;
	/** falls is_simple_, steht in simple_value_ der Wert */
	double simple_value_;
	/** falls is_simple_, steht in simple_varname_ der Variablenname */
	char * simple_varname_;
	/** true, falls Constraint gültig */
	bool is_valid_;
        
private:
	void normalize();

public:
	/**
	 * Constructor.
	 * Erzeugt ein neues Constraint aus einer Bezeichnung und einem
	 * Gleichungs- oder Ungleichungs-Ausdruck.
	 *
	 * @param name Bezeichnung des Constraint
	 * @param constraint Wert des Constraint
	 */

	inline Constraint(
		char const * name,
		symb::ExprTree const * constraint,
		ParameterType parameter_type
		) : constraint_(constraint->clone()),
		    parameter_type_(parameter_type), is_simple_(false),
	            simple_value_(0.), simple_varname_(0),
		    is_valid_(false)
	{
		name_ = strdup_alloc(name);
		normalize();
	}

	/**
	 * Copy-Constructor
	 *
	 * @param copy zu kopierendes Constraint-Objekt
	 */
	inline Constraint(Constraint const & copy)
		: constraint_(copy.constraint_->clone()),
		  parameter_type_(copy.parameter_type_),
		  is_simple_(copy.is_simple_),
		  simple_value_(copy.simple_value_),
		  simple_varname_(0),
		  is_valid_(copy.is_valid_)
	{
		name_ = strdup_alloc(copy.name_);
		if (is_simple_)
			simple_varname_ = strdup_alloc(copy.simple_varname_);
	}

	/**
	 * Destructor
	 */
	~Constraint()
	{
		delete[] name_;
		delete constraint_;
		if (simple_varname_)
			delete[] simple_varname_;
	}

public:
	/**
	 * Gibt die Bezeichnung des Constraints zurück.
	 *
	 * @return Bezeichnung des Constraints;
	 */
	inline char const * getName() const { return name_; }
	
	/**
	 * Gibt das Constraint zurück.
	 *
	 * @return Constraint-Gleichung oder Ungleichung
	 */
	symb::ExprTree * getConstraint() const { return constraint_; }

	/**
	 * Gibt true zurück, falls es sich um ein Netto-Constraint handelt.
	 *
	 * @return true, falls das Constraint einen Netto-Fluß beschreibt
	 */
	ParameterType getParameterType() const { return parameter_type_; }
	/**
	 * Gibt true zurück, falls das Constraint eine Gleichung ist.
	 *
	 * @return true, falls Constraint eine Gleichung ist
	 */
	inline bool isEquality() const { return constraint_->isEquality(); }
	
	/**
	 * Gibt true zurück, falls das Constraint eine Ungleichung ist.
	 *
	 * @return true, falls Constraint eine Ungleichung ist
	 */
	inline bool isInEquality() const { return constraint_->isInEquality(); }

	/**
	 * Gibt true zurück, wenn das Constraint ein einfaches Wert-Constraint
	 * ist (Variable=Wert).
	 *
	 * @return true, falls Constraint vom Typ "Variable=Wert"
	 */
	inline bool isSimple() const { return is_simple_; }

	/**
	 * Gibt den Variablennamen zurück, falls es sich um ein einfaches
	 * Wert-Constraint handelt.
	 *
	 * @return Variablenname
	 */
	inline char const * getSimpleVarName() const { return simple_varname_; }

	/**
	 * Gibt den Variablenwert zurück, falls es sich um ein einfaches
	 * Wert-Constraint handelt.
	 *
	 * @return Variablenwert
	 */
	inline double getSimpleVarValue() const { return simple_value_; }

	/**
	 * Gibt true zurück, falls Constraint gültig ist.
	 *
	 * @return true, falls Constraint gültig ist
	 */
	inline bool isValid() const { return is_valid_; }

	/**
	 * Prüfung eines Constraints:
	 *   - Linearität
	 *   - Gleichung / Ungleichung
	 *
	 * @param expr Ausdruck
	 * @return true, falls Ausdruck expr linear und (Un)Gleichung
	 */
	static bool checkLinearity(symb::ExprTree const * expr);

	static bool checkSimplicity(
		symb::ExprTree const * expr,
		char ** varname = 0,
		double * value = 0
		);

	/**
	 * Vergleichsoperator.
	 *
	 * @param rval rechtes Argument
	 * @return true bei Gleichheit
	 */
	bool operator==(Constraint const & rval) const;

	/**
	 * Berechnet eine Prüfsumme über das Configuration-Objekt.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;
        
}; // class Constraint

} // namespace flux::data
} // namespace flux

#endif

