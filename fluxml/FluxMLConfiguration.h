#ifndef FLUXMLCONFIGURATION_H
#define FLUXMLCONFIGURATION_H

#include <xercesc/dom/DOM.hpp>
#include "Configuration.h"
#include "InputPool.h"
#include "FluxMLContentObject.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zum Parsen / Serialisieren einer Netzwerk-Konfiguration.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

    class FluxMLConfiguration : public FluxMLContentObject
{
private:
	/** Internes Objekt zum Speichern einer Konfiguration */
	data::Configuration * configuration_;

public:
	/**
	 * Constructor.
	 * Parst ein configuration-Element aus einem DOM-Tree.
	 * 
	 * @param doc umgebendes FluxMLDocument-Objekt
	 * @param node Knoten mit configuration-Element
	 */
	inline FluxMLConfiguration(
		FluxMLDocument * doc,
		XN DOMNode * node
		)
		: FluxMLContentObject(doc), configuration_(0)
	{
		// parsen / allokieren der Konfiguration (configuration_):
		parseConfiguration(node);
	}

	/**
	 * Constructor.
	 * Erzeugt eine FluxML-Konfiguration einem fertigen Configuration-Objekt.
	 * Dieser Constructor dient der Serialisierung.
	 *
	 * @param doc umgebendes FluxMLDocument-Objekt
	 * @param configuration bereits existierendes Configuration-Objekt
	 */
	inline FluxMLConfiguration(
		FluxMLDocument * doc,
		data::Configuration * configuration
		) : FluxMLContentObject(doc), configuration_(configuration) { }

	/**
	 * Virtueller Destructor.
	 * Das configuration_-Objekt wird im umgebenden FluxMLDocument-Objekt
	 * allokiert und registriert, weshalb es hier nicht freigegeben werden
	 * muß.
	 */
	inline virtual ~FluxMLConfiguration() { }

	/**
	 * Gibt den Typ des FluxMLContentObject zurück - in diesem Fall ist
	 * dies immer der Wert FluxMLContentObject.CO_CFG.
	 *
	 * @return der Wert FluxMLContentObject.CO_CFG
	 */
	inline int getType() { return FluxMLContentObject::CO_CFG; }

private:
	/**
	 * Methoden zum Parsen einer Konfiguration.
	 * Allokation des Configuration-Objekts erfolgt über die
	 * Methoden des umgebenden FluxMLDocument-Objekts.
	 *
	 * @param node Knoten im DOM-Tree, aus dem geparst wird.
	 */
	void parseConfiguration(XN DOMNode * node);

	void parseSimulation(
		data::Configuration * cfg,
		XN DOMNode * simulation
		);

	void parseModel(
		data::Configuration * cfg,
		XN DOMNode * model
		);

	void parseVariables(
		data::Configuration * cfg,
		XN DOMNode * variables
		);

}; // class FluxMLConfiguration

} // namespace flux::xml
} // namespace flux

#undef XN
#endif

