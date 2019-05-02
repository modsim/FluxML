#ifndef FLUXMLINPUT_H
#define FLUXMLINPUT_H

#include <list>
#include <xercesc/dom/DOM.hpp>
#include "Array.h"
#include "FluxMLContentObject.h"
#include "XMLException.h"
#include "Pool.h"
#include "InputPool.h"
#include "ExprTree.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zum Parsen / Serialisieren von Input-Pools
 * (Teil eines configuration-Elements).
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */ 
    
class FluxMLInput : public FluxMLContentObject
{
private:
	/** die InputPool Datenstruktur der Klasse */
	data::InputPool * input_pool_;
        /** substrate profile **/
        data::InputProfile * input_profile_;
        bool profile_defined;
public:
	/**
	 * Constructor. Parst ein einzelnes input-Element aus einem
	 * DOM-Tree eines FluxML-Dokuments.
	 *
	 * @param node Knoten des info-Elements
	 */
	FluxMLInput(
		FluxMLDocument * doc,
		XN DOMNode * node
		)
		: FluxMLContentObject(doc), input_pool_(0),input_profile_(0),
                                            profile_defined(false)
	{
		parseInput(node);
	}

	/**
	 * Constructor. Erzeugt ein FluxMLInput-Objekt aus einem
	 * InputPool-Objekt.
	 *
	 * @param input_pool das InputPool-Objekt
	 */
	FluxMLInput(
		FluxMLDocument * doc,
		data::InputPool * input_pool)
		: FluxMLContentObject(doc), input_pool_(input_pool), input_profile_(0),
		  profile_defined(false)
	{ }

	/**
	 * Destructor.
	 */
	inline virtual ~FluxMLInput() 
        {
            if(input_profile_) 
                delete input_profile_;
        }

	/**
	 * Parst ein input-Element aus einem DOM-Tree eines
	 * FluxML-Dokuments.
	 *
	 * @param node Knoten des info-Elements
	 */
	void parse(XN DOMNode * node)
	{
		parseInput(node);
	}

	inline int getType() { return FluxMLContentObject::CO_CFG_INPUT; }

	/**
	 * Gibt das interne InputPool-Objekt zurück.
	 *
	 * @retval eine Referenz auf das interne InputPool-Objekt
	 */
	data::InputPool * getInputPool() { return input_pool_; }
        
private:
	/**
	 * Parst ein info-Element aus einem DOM-Tree eines FluxML-Dokuments.
	 *
	 * @param node Knoten des info-Elements
	 */
	void parseInput(XN DOMNode * node);

	/**
	 * Sucht einen Pool über seine Bezeichnung
	 *
	 * @param pool_name Poolbezeichnung
	 */
	data::Pool * lookupPool(char const * pool_name);

	/**
	 * Parst den Inhalt eines label-Elements.
	 *
	 * @param label label-Element
	 * @return double-Array mit Fractions
	 */
	Array< double > parseLabel(
		XN DOMElement * label
		);
	
        
        /**
	 * Parst den Inhalt eines Profile-Elements.
	 *
	 * @param label label-Element
	 * @return void
	 */
	void  parseProfile(
		XN DOMNode * profile
		);
        
        /**
	 * Parst Constraint-Spezifikationen in MathML.
	 *
	 * @param node math-Elementknoten
	 * @param is_netto true, falls Netto-Constraint
	 */
	void parseProfileMathML(XN DOMNode * node);

	/**
	 * Parst textuelle Bedingungen/Werten-Spezifikationen.
	 *
	 * @param node textual-Elementknoten
	 * @param is_condition true, falls es sich um eine Bedingung handelt
	 */
	void parseProfileTextual(XN DOMNode * node);
        
        
        
}; // class FluxMLInput

} // namespace flux::xml
} // namespace flux

#endif

