#ifndef FLUXMLDOCUMENT_H
#define FLUXMLDOCUMENT_H

#include <list>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "charptr_map.h"
#include "Configuration.h"
#include "Info.h"
#include "Pool.h"
#include "IsoReaction.h"
#include "Constraint.h"
#include "DOMWriter.h"
#include "FluxMLContentObject.h"
#include "MGroup.h"

#include "DOMReader.h"
#include "DOMReaderImpl.h"

#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace la {
	class StoichMatrixInteger;
}}

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zur Abbildung eines FluxML-Dokuments.
 * Zugleich Factory-Klasse zur Erzeugung der Daten-Elemente eines
 * FluxML-Dokuments: Ausschließlich in Objekten dieser Klasse
 * wird Speicher allokiert und freigegeben.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class FluxMLDocument : public FluxMLContentObject
{
public:
	/** Pool-Rollen: Input oder Inner */
	enum PoolRole { p_input, p_inner };

private:
	/** Info-Block des FluxML-Dokuments */
	data::Info * info_;
	/** Pools des Reaktionsnetzwerks (Liste von Pool-Objekten) */
	charptr_map< data::Pool* > * pool_map_;
	/** Reaktionen des Reaktionsnetzwerks (Liste von Reaction-Objekten */
	std::list< data::IsoReaction* > * reaction_list_;
	/** Netzwerk-Konfiguration (Constraints,Flüsse) */
	data::Configuration * root_cfg_;
	/** Eine Liste mit configuration-Elementen/Configuration-Objekten */
	charptr_map< data::Configuration* > * configuration_map_;
	/** Die Stöchiometrie */
	la::StoichMatrixInteger * stoich_matrix_;
	/** Zustand / Rollenverteilung der Pools (input/output/intra) */
	charptr_map< PoolRole > * pool_roles_;
	/** xml::DOMWriter-Objekt */
	xml::DOMWriter * writer_;
        /** xml::DOReader-Objekt */
        xml::DOMReader * reader_;

public:
	/**
	 * Constructor. Erzeugt ein FluxMLDocument-Objekt durch Parsen aus
	 * einem DOM-Tree.
	 *
	 * @param node Knoten im DOM-Tree
	 */
	FluxMLDocument(XN DOMDocument * doc);

	/**
	 * Constructor. Erzeugt ein leeres FluxMLDocument-Objekt.
	 */
	FluxMLDocument(const char * filename);

	/**
	 * Virtueller Destructor.
	 * Gibt Container-Klassem samt Inhalt frei.
	 */
	virtual ~FluxMLDocument();
	
	/**
	 * Gibt den Typ des FluxMLContentObject zurück. 
	 * In diesem Fall immer CO_POOL.
	 *
	 * @return der Wert FluxMLContentObject.CO_POOL
	 */
	inline int getType() {
	    return FluxMLContentObject::CO_FLUXML_DOC;
	}

private:
	/**
	 * Parst die Pools einer Netzwerkspezifikation aus einem
	 * pool-Element-Knoten eines DOM-Dokuments
	 *
	 * @param node pool-Element-Knoten eines DOM-Dokuments
	 */
	void parseDocument(XN DOMNode * node);

	/**
	 * Erzeugung der Stöchiometrischen Matrix
	 */
	void validatePoolRolesAndStoichiometry();
	
	/**
	 * Validierung des Zusammenspiels der Bezeichnungen der Pools in der
	 * Pool-Liste und deren Erwähnung innerhalb der Reaktionebeschreibungen
	 */
	void validatePoolsAndReactions();

	/**
	 * Fügt zusätzliche Constraints (Exchange-Fluß := 0) für Input- und
	 * Output-Reaktionen in die Konfigurationen ein (soweit nicht schon
	 * vorhanden).
	 *
	 * @param cfg Konfiguration
	 */
	void setConfInputOutputConstraints(data::Configuration * cfg);
	
	/**
	 * Validierung der Konfigurationen:
	 *
	 * Alle Konfigurationen müssen unterschiedliche Bezeichnungen haben.
	 *
	 * Genannte Input-Pools müssen wirklich Input-Pools des Netzwerks sein.
	 *
	 * Es dürfen keine Input-Pools vergessen werden.
	 *
	 * Geht die Wahl der freien Flüsse und die Belegung der Constraint-Flüsse
	 * mit der Stöchiometrie konform?
	 */
	void validateConfigurations();
	
	/**
	 * Validierung einer einzelnen Konfiguration.
	 */
	bool validateConfSingle(data::Configuration * cfg);

public:
	/**
	 * Gibt das interne Info-Objekt des Dokuments zurück.
	 *
	 * @return Bezeichnung des Dokuments
	 */
	inline data::Info * & getInfo() { return info_; }
	
	/**
	 * Gibt eine Abbildung von Pool-Name auf Pool-Objekte zurück
	 *
	 * @return eine Abbildung von Pool-Namen auf Pool-Objekte
	 */
	inline charptr_map< data::Pool* > * & getPoolMap()
	{
		return pool_map_;
	}
	
	/**
	 * Gibt die Liste der Reaktionen zurück (Reaction-Objekte)
	 *
	 * @return eine Liste von Reaktionen (Reaction-Objekte)
	 */
	inline std::list< data::IsoReaction* > const * getReactions() const
	{
		return reaction_list_;
	}

	/**
	 * Fügt ein bereits erzeugtes Reaction-Objekt in die
	 * Reaktionsliste ein. Das erzeugte Reaktions-Objekt darf durch
	 * den Destructor dieser Klasse freigegeben werden.
	 *
	 * @param reaction allokiertes Reaktions-Objekt
	 */
	inline void addReaction(data::IsoReaction * reaction)
	{
		reaction_list_->push_back(reaction);
	}

	/**
	 * Sucht ein bereits erzeugtes Reaction-Objekt über seinen Namen.
	 * Falls die Suche erfolglos ist, wird ein 0-Pointer zurückgegeben;
	 * ansonsten ein Pointer auf das Reaction-Objekt.
	 *
	 * @param name Reaktionsbezeichnung
	 * @return Pointer auf Reaction-Objekt, bzw 0-Pointer
	 */
	inline data::IsoReaction * findReaction(char const * name)
	{
		std::list< data::IsoReaction* >::iterator rli;
		for (rli = reaction_list_->begin();
			rli != reaction_list_->end(); rli++)
			if (strcmp((*rli)->getName(),name)==0)
				return *rli;
		return 0;
	}
	
	/**
	 * Gibt die Liste der Konfigurationen des FluxML-Dokuments zurück.
	 *
	 * @return eine Liste von Konfigurationen (Configuration-Objekte)
	 */
	inline charptr_array getConfigurationNames()
	{
		charptr_array cnames = configuration_map_->getKeys();
		cnames.sort();
		return cnames;
	}

	/**
	 * Gibt einen Zeiger auf ein angefordertes Konfigurationsobjekt zurück.
	 *
	 * @param name Bezeichnung der Konfiguration
	 * @param validate Flag; Konfiguration validieren (default: true)
	 * @return Zeiger auf Konfigurationsobjekt oder Null-Zeiger
	 */
	inline data::Configuration * getConfiguration(
		char const * name,
		bool validate = true)
	{
		data::Configuration ** cptr =
			configuration_map_->findPtr(name);
		if (cptr == 0)
			return 0;
		if (validate)
			validateConfSingle(*cptr);
		return *cptr;
	}

	/**
	 * Berechnet eine CRC-32 Prüfsumme für das Netzwerk und eine
	 * dazugehörige Netzwerk-Konfiguration. Dient der schnellen Erkennung
	 * von Inkonsistenzen.
	 *
	 * @param crc_scope Scope der Prüfsummenberechnung
	 * @return CRC-32 Prüfsumme
	 */
	uint32_t computeCheckSum(int crc_scope) const;

	/**
	 * Gibt die Signatur des FluxML-Dokuments zurück.
	 *
	 * @return Signatur des FluxML-Dokuments oder 0-Zeiger
	 */
	char const * getSignature() const;

	/**
	 * Gibt die (ganzzahlige) Stöchiometrie des Reaktionsnetzwerkes
	 * Netzwerkes zurück.
	 *
	 * @return Stöchiometrische Matrix des Netzwerkes
	 */
	la::StoichMatrixInteger * getStoichiometry()
	{
		// Stöchiometrie-Generierung ohne output-Pools prüfen
		fASSERT_NONREACHABLE();
		return stoich_matrix_;
	}

public:
	/*
	 * Factory-Methoden für die Erzeugung von ...
	 *   data::Pool, data::Reaction, data::Constraint, data::Configuration
	 */

	/**
	 * Erzeugt ein Info-Objekt (Beschreibung eines FluxML-Dokuments).
	 *
	 * @param name Bezeichnung des FluxML-Dokuments
	 * @param version Version des FluxML-Dokuments
	 * @param date Datum des FluxML-Dokuments
	 * @param comment Kommentar zum FluxML-Dokument
	 * @param signature FluxML-Signatur (verschlüsselte CRC)
	 * @return allokiertes Info-Objekt
	 */
	inline data::Info * createInfo(
		std::string const & modeler,
                std::string const & strain,
		std::string const & version,
		time_t timestamp,
		std::string const & comment,
		std::string const & signature
		)
	{
		info_ = new data::Info(modeler,strain,version,timestamp,comment,signature);
		return info_;
	}

	/**
	 * Erzeugt einen Pool.
	 *
	 * @param name Pool-Bezeichnung
	 * @param natoms Anzahl der C-Atome in Vertretern des Pools
	 * @return allokiertes Pool-Objekt
	 */
	inline data::Pool * createPool(
		std::string const & name,
		int natoms,
		double poolsize,
                std::string const & cfg
		)
	{
		data::Pool * pool = new data::Pool(name,natoms,poolsize,cfg);
		if (pool_map_->findPtr(name.c_str()) != 0)
			fTHROW(XMLException,"duplicate pool id: %s",name.c_str());
		pool_map_->insert(name.c_str(),pool);
		return pool;
	}

	/**
	 * Erzeugt eine Reaktion.
	 * 
	 * @param name Bezeichnung der Reaktion
	 * @return allokiertes Reaction-Objekt
	 */
	inline data::IsoReaction * createReaction(
		char const * name, bool bidirectional = true
		)
	{
		data::IsoReaction * reaction = new data::IsoReaction(name, bidirectional);
		reaction_list_->push_back(reaction);
		return reaction;
	}

	/**
	 * Erzeugt ein Constraint (Gleichung oder Ungleichung).
	 *
	 * @param name Bezeichnung des Constraint
	 * @param constraint Wert des Constraints
	 * @param is_netto true, falls Constraint einen Netto-Fluß beschreibt
	 * @return allokiertes Constraint-Objekt
	 */
	inline bool createConstraint(
		std::string const & name,
		symb::ExprTree const * constraint,
		data::ParameterType parameter_type
		)
	{
                return root_cfg_->createConstraint(name.c_str(),constraint,parameter_type);
	}

	/**
	 * Erzeugt eine Konfiguration.
	 *
	 * @param name Bezeichnung der Konfiguration
	 * @param comment Kommentar zur Konfiguration
	 * @return allokiertes Configuration-Objekt
	 */
	inline data::Configuration * createConfiguration(
		char const * name,
		char const * comment
		)
	{
		data::Configuration * conf =
			new data::Configuration(name,comment);
		configuration_map_->insert(conf->getName(),conf);
		return conf;
	}

	/**
	 * Gibt ein DOMWriter-Objekt für FluxML zurück.
	 *
	 * @return Referenz auf DOMWriter-Objekt
	 */
	DOMWriter & getDOMWriter();
        
        /**
	 * Gibt ein DOMReader-Objekt für FluxML zurück.
	 *
	 * @return Referenz auf DOMReader-Objekt
	 */
	DOMReader & getDOMReader();
	
public:
	/**
	 * Schreibt die Einstellung der freien Flüsse in den DOM tree
	 * einer FluxML-Datei.
	 *
	 * @param doc FluxML Referenz auf DOMDocument-Objekt
	 * @param CS Referenz auf ConstraintSystem-Objekt mit freien Flüssen
	 * @param cfg_name Bezeichung der Konfiguration
	 */
	static bool patchDOM(
		XN DOMDocument & doc,
		data::ConstraintSystem const & CS,
		char const * cfg_name
		);

private:
	static bool patchDOM_Generic(
		XN DOMDocument & doc,
		MGroupGeneric const & mg,
		XN DOMElement& data
		);
	static bool patchDOM_MS(
		XN DOMDocument & doc,
		MGroupMS const & mg,
		XN DOMElement& data
		);
	static bool patchDOM_MSMS(
		XN DOMDocument & doc,
		MGroupMSMS const & mg,
		XN DOMElement& data
		);
        static bool patchDOM_MIMS(
		XN DOMDocument & doc,
		MGroupMIMS const & mg,
		XN DOMElement& data
		);
	static bool patchDOM_1HNMR(
		XN DOMDocument & doc,
		MGroup1HNMR const & mg,
		XN DOMElement& data
		);
	static bool patchDOM_13CNMR(
		XN DOMDocument & doc,
		MGroup13CNMR const & mg,
		XN DOMElement& data
		);
  
public:
	/**
	 * Schreibt neue Messwerte einer Messgruppe in den DOM tree
	 * einer FluxML-Datei.
	 *
	 * @param doc FluxML Referenz auf DOMDocument-Objekt
	 * @param MG Referenz auf Messgruppen-Objekt
	 * @param cfg_name Bezeichung der Konfiguration
	 */
	static bool patchDOM(
		XN DOMDocument & doc,
		MGroup const & MG,
		char const * cfg_name
		);
        
        inline data::Configuration * getRootConfiguration() const
	{
		return root_cfg_;
	}

}; // class FluxMLDocument

} // namespace flux::xml
} // namespace flux

#endif

