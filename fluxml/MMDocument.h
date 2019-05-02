#ifndef MMDOCUMENT_H
#define MMDOCUMENT_H

extern "C"
{
#include <stdint.h>
}
#include <ctime>
#include <list>
#include <set>
#include <xercesc/dom/DOM.hpp>
#include "charptr_map.h"
#include "charptr_array.h"
#include "MGroup.h"
#include "MValue.h"
#include <iostream>
#include <string>
#include <sstream>

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Klasse zur Abbildung eines Messmodelldokuments.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */
    
class MMDocument
{
	friend class MMModel;
private:
	/** Abbildung id->Messgruppe (Struktur) */
	charptr_map< MGroup* > mgroup_map_;

	/** Menge aller Messzeitpunkte */
	std::set< double > ts_set_;
	
	/** Verwalteter Rückgabe-Pointer für Messzeitpunkte */
	double * ts_ptr_;

	/** Flag; Stationarität des Netzwerks */
	bool is_stationary_;
	
	/** Timestamp (infos-Header) */
	struct tm mm_ts_;

	/** Version (infos-Header) */
	char * mm_version_;

	/** Kommentar (infos-Header) */
	char * mm_comment_;

	/** Fluxunit (infos-Header) */
	char * mm_fluxunit_;

	/** Poolsizeunit (infos-Header) */
	char * mm_psunit_;
        
        /** Zeiteinheit (infos-Header) */
	char * mm_timeunit_;

public:
	/**
	 * Constructor.
	 * Parst ein Messmodell aus einem DOM-Tree.
	 *
	 * @param doc Document-Root eines XML-Dokuments
	 * @param is_stationary Flag, welches vorgibt ob es sich um Inst-MFA handelt
	 */
	MMDocument(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument * doc,
		bool is_stationary
		);

	/**
	 * Constructor.
	 * Parst ein Messmodell aus einem measurement-Element-Knoten.
	 *
	 * @param measurement ein measurement-Element eines XML-Dokuments
	 * @param is_stationary Flag, welches vorgibt ob es sich um Inst-MFA handelt
	 */
	MMDocument(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMNode * measurement,
		bool is_stationary
		);

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes MMDocument-Objekt
	 */
	MMDocument(MMDocument const & copy);

	/**
	 * Destructor.
	 */
	virtual ~MMDocument();

public:
	/**
	 * Parst das MMDocument aus einem DOM-Knoten
	 *
	 * @param measurement DOM-Knoten
	 */
	void parse(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMNode * measurement
		);

	/**
	 * Gibt die id's der Messgruppen zurück.
	 */
	inline charptr_array getGroupNames() const { return mgroup_map_.getKeys(); }

	/**
	 * Gibt eine über den Namen spezifizierte Gruppe zurück.
	 */
	MGroup * getGroupByName(char const * id);

	/**
	 * Gibt die sortierten Messzeitpunkte zurück.
	 * Das zurückgegebene Array wird automatisch deallokiert.
	 *
	 * @param size Länge des zurückgegebenen Arrays
	 * @return double-Array mit sortierten Messzeitpunkten
	 */
	double const * getTimeStamps(size_t & size);
        
	/**
	 * Gibt true zurück, falls das beschriebene Modell eine
	 * stationäre Messung beschreibt.
	 *
	 * @return true, falls die Messung stationär ist
	 */
	inline bool isStationary() const { return is_stationary_; }

	/**
	 * Setzt das Stationaritäts-Flag.
	 *
	 * @param s flag, true falls stationäres Messmodell, false ansonsten
	 */
	inline void setStationary(bool s) { is_stationary_ = s; }

	/**
	 * Gibt die Version der Modellvariante zurück.
	 *
	 * @return String mit Version der Modellvariante
	 */
	inline char const * getDocumentVersion() const { return mm_version_; }

	/**
	 * Gibt das Kommentar zum Messmodell zurück.
	 *
	 * @return Kommentar zum Messmodell
	 */
	inline char const * getDocumentComment() const { return mm_comment_; }

	/**
	 * Gibt die Einheit der Flüsse zurück.
	 *
	 * @return Einheit der Flüsse
	 */
	inline char const * getDocumentFluxUnit() const { return mm_fluxunit_; }
                
	/**
	 * Gibt die Pool-Größen-Einheit zurück.
	 *
	 * @return Pool-Größen-Einheit
	 */
	inline char const * getDocumentPoolSizeUnit() const { return mm_psunit_; }

        /**
	 * Gibt die Einheit der Zeitpunkte zurück.
	 *
	 * @return Einheit der Zeitpunkte
	 */
	inline char const * getDocumentTimeUnit() const {return mm_timeunit_; }
        
	/**
	 * Gibt den Timestamp des Dokuments zurück.
	 *
	 * @return Timestamp
	 * @see time.h
	 */
	inline struct tm getDocumentTimeStamp() const { return mm_ts_; }

	/**
	 * Validierung/Finalisierung aller Messgruppen.
	 * Setzt allen Metabolit-abhängigen Messgruppen die Atomanzahlen.
	 *
	 * @param pools Abbildung Poolname -> Atomanzahl
	 * @param reactions Reaktionsliste
	 */
	void validate(
		charptr_map< int > const & pools,
		charptr_array const & reactions
		) const;

	/**
	 * Aktualisiert eine bestehende Messgruppe.
	 *
	 * @param G MGroup-Objekt
	 * @return false, falls G nicht registriert ist, sonst true
	 */
	bool updateGroup(
		MGroup const & G
		);

private:
	/**
	 * Parse den Inhalt des infos-Elements.
	 *
	 * @param infos DOMNode mit infos-Element
	 */
	void parseInfos(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMNode * infos
		);

	/**
	 * Registriert einen neuen Timestamp.
	 *
	 * @param ts Timestamp
	 * @return false, falls Timestamp schon registriert war, sonst true
	 */
	inline bool registerTimeStamp(double ts)
	{
		if (ts_set_.find(ts) != ts_set_.end())
			return false;
		ts_set_.insert(ts);
		return true;
	}

	/**
	 * Registriert eine neue Messgruppe.
	 *
	 * @param gnode DOMNode des Gruppenelements
	 * @param G Zeiger auf MGroup-Objekt
	 */
	void registerGroup(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMNode * gnode,
		MGroup * G
		);

	/**
	 * Prüft die Timestamps im data-Abschnitt und erkennt
	 * fehlende Messwerte. Wird eine Inkonsistenz erkannt,
	 * wird eine XMLException geworfen.
	 *
	 * @param data DOMNode des data-Elements
	 */
	void checkValues(
		XERCES_CPP_NAMESPACE_QUALIFIER DOMNode * data
		);

	void validate_MGroupMS(
		MGroupMS * G,
		charptr_map< int > const & pools,
		charptr_array const & reactions
		) const;

	void validate_MGroupMIMS(
		MGroupMIMS * G,
		charptr_map< int > const & pools,
		charptr_array const & reactions
		) const;
        
        void validate_MGroupMSMS(
		MGroupMSMS * G,
		charptr_map< int > const & pools,
		charptr_array const & reactions
		) const;

	void validate_MGroup1HNMR(
		MGroup1HNMR * G,
		charptr_map< int > const & pools,
		charptr_array const & reactions
		) const;

	void validate_MGroup13CNMR(
		MGroup13CNMR * G,
		charptr_map< int > const & pools,
		charptr_array const & reactions
		) const;

public:
	/**
	 * Berechnet eine Prüfsumme über das Messmodell.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;
        
        /*
         * <INST>
         * @autor:          Salah Azzouzi, info@13cflux.net
         * @description:    Erweiterungen für den instationären Fall
         * 
         * @date:           22.12.2014
         * </INST>
         */
        
        
        /**
	* returns simulations time end
	*/
	double getMeasurementTimeEnd();
        
        

}; // class MMDocument

} // namespace flux::xml
} // namespace flux

#endif

