#ifndef FLUXMLCONTENTOBJECT_H
#define FLUXMLCONTENTOBJECT_H

#include <xercesc/dom/DOM.hpp>
#include "XMLException.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

class FluxMLDocument;

/*
 * *****************************************************************************
 * Schnittstelle für ein Objekt in FluxML.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class FluxMLContentObject
{
protected:
	/** Zeiger auf ein umgebendes FluxMLDocument-Objekt */
	FluxMLDocument * doc_;

public:
	/** unterschiedliche Content-Objekt-Typen */
	enum
	{
		CO_FLUXML_DOC       = 1,
		CO_INFO             = 2,
		CO_REACTION_NETWORK = 3,
		CO_METABOLITE_POOLS = 4,
		CO_REACTION         = 5,
		CO_POOL             = 6,
		CO_CONSTRAINTS      = 7,
		CO_CFG              = 8,
		CO_CFG_INPUT        = 9,
		CO_CFG_FLUXES       = 10
	};

	/**
	 * Constructor.
	 *
	 * @param doc umgebendes FluxMLDocument-Objekt
	 */
	FluxMLContentObject(FluxMLDocument * doc) : doc_(doc) { }

	/**
	 * Virtueller Destructor.
	 */
	virtual ~FluxMLContentObject() { }
	
	/**
	 * Gibt den Typ des Objekts zurück.
	 *
	 * @return Typ des Objekts
	 */
	virtual int getType() = 0;
	
}; // class FluxMLContentObject

} // namespace flux::xml
} // namespace flux

#endif

