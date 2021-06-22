
#ifndef INPUTPROFILE_H
#define	INPUTPROFILE_H


extern "C"
{
#include <stdint.h>
}
#include <cstddef>
#include "Error.h"
#include <string>
#include <list>
#include "LinearExpression.h"
#include <cstring>
#include "cstringtools.h"
#include "ExprTree.h"

namespace flux {
namespace data {

/**
 * Klasse zur Abbildung eines Substrat-Pools.
 *
 * @author Salah Azzouzi <info@13cflux.net>
 */
class InputProfile
{
private:
	/** Bezeichnung des Profiles */
	std::string name_;
    /** List von Umschaltzeitpunkten des Profiles */
	std::list<double> conditions_;
        /** List von Werten des Profiles */
	std::list<symb::ExprTree> values_;
	/** true, falls Profile gültig */
	bool is_valid_;

public:
        /**
	 * Constructor.
	 * Erzeugt ein neues Profile aus einer Bezeichnung und einem
	 * Gleichungs- oder Ungleichungs-Ausdruck.
	 *
	 * @param name Bezeichnung des Profiles
	 * @param Profile Wert des Profiles
	 */

        inline InputProfile() : name_("anonymous"),is_valid_(false) {}
	inline InputProfile(
		char const * name
		) : is_valid_(false)
	{
		name_ = name;
	}

	/**
	 * Copy-Constructor
	 *
	 * @param copy zu kopierendes Profile-Objekt
	 */
	inline InputProfile(InputProfile const & copy)
		: conditions_(copy.conditions_),
		  values_(copy.values_),
		  is_valid_(copy.is_valid_)
	{
		name_ = copy.name_;
	}

	/**
	 * Destructor
	 */
	inline virtual ~InputProfile() { }
        
public:
	/**
	 * Gibt die Bezeichnung des Profiles zurück.
	 *
	 * @return Bezeichnung des Profiles;
	 */
	inline char const * getName() const { return name_.c_str(); }
        
        /**
	 * Gibt die Bezeichnung des Profiles zurück.
	 *
	 * @return Bezeichnung des Profiles;
	 */
	inline void setName(char const * name) { name_ = name; }
	
	/**
	 * Gibt die Bedingung zurück.
	 *
	 * @return Profile-Gleichung oder Ungleichung
	 */
        inline std::list<double> const & getConditions() const { return conditions_; }
        
        /**
	 * Gibt den Wert zurück.
	 *
	 * @return Profile-Wert
	 */
	inline std::list<symb::ExprTree> const & getValues() const { return values_; }

	/**
	 * Gibt true zurück, falls Profile gültig ist.
	 *
	 * @return true, falls Profile gültig ist
	 */
	inline bool isValid() const { return is_valid_; }
    
        inline void addCondition(double condition) { conditions_.push_back(condition); }
            
        inline void addValue(symb::ExprTree * value) { values_.push_back(*value); }
            
        double eval(double t, bool* status);
};

} // namespace flux::data
} // namespace flux

#endif	/* INPUTPROFILE_H */

