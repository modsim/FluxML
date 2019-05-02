#ifndef INFO_H
#define INFO_H

#include <ctime>
#include <string>
extern "C"
{
#include <stdint.h>
}

namespace flux {
namespace data {

/**
 * Klasse zur Abbildung des Inhalts eines info-Elements eines FluxML-Dokuments.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class Info
{
private:
/**<INST>**/     
	/** Bezeichnung des Modellierer */
	std::string modeler_;
        /** Bezeichnung des Stamms */
	std::string strain_;
/**</INST>**/         
	/** Version der Netzwerkspezifikation */
	std::string version_;
	/** Datum der Änderung/Erstellung */
	time_t timestamp_;
	/** Kommentare */
	std::string comment_;
	/** Signatur */
	std::string signature_;

public:
	/**
	 * Constructor.
	 *
	 * @param modeler Modelliererbezeichnung
         * @param strain Stammbezeichnung
	 * @param version Dokumentversion
	 * @param date Aenderungsdatum
	 * @param comment Kommentar zur FluxML-Datei
	 * @param signature Signatur der FluxML-Datei
	 */
	inline Info(
/**<INST>**/ 
		std::string const & modeler,
                std::string const & strain,
/**</INST>**/                 
		std::string const & version,
		time_t timestamp,
		std::string const & comment,
		std::string const & signature
		) : modeler_(modeler), strain_(strain), version_(version), timestamp_(timestamp),
		    comment_(comment), signature_(signature) { }

	/**
	 * Constructor. (Erzeugen eines leeren Info-Objekts)
	 */
	inline Info() {}
/**<INST>**/ 
        /**
	 * Gibt die im info-Element gespeicherte Bezeichnung des Modellierers
	 * zurück.
	 *
	 * @return Bezeichnung des Modellierers
	 */
	inline std::string & getModeler() { return modeler_; }
        
/**</INST>**/         
	
	/**
	 * Gibt die im info-Element gespeicherte Bezeichnung des FluxML-Dokuments
	 * zurück.
	 *
	 * @return Bezeichnung des FluxML-Dokuments
	 */
	inline std::string & getStrain() { return strain_; }
	
	/**
	 * Gibt die im info-Element gespeicherte Version des FluxML-Dokuments
	 * zurück.
	 *
	 * @return Version des FluxML-Dokuments
	 */
	inline std::string & getVersion() { return version_; }
	
	/**
	 * Gibt das im info-Element gespeicherte Änderungsdatum des 
	 * FluxML-Dokuments zurück.
	 *
	 * @return Änderungsdatum des FluxML-Dokuments
	 */
	inline struct tm getDate() { return *(gmtime(&timestamp_)); }

	/**
	 * Gibt den im info-Element gespeicherten Timestamp des
	 * FluxML-Dokuments zurück.
	 *
	 * @return Änderungs-Timestamp des FluxML-Dokuments
	 */
	inline time_t getTimeStamp() { return timestamp_; }
	
	/**
	 * Gibt das im info-Element gespeicherte Kommentar zum FluxML-Dokument
	 * zurück.
	 *
	 * @return Kommentar zum FluxML-Dokument
	 */
	inline std::string & getComment() { return comment_; }
	
	/**
	 * Gibt die im info-Element gespeicherte Signatur zum FluxML-Dokument
	 * zurück.
	 *
	 * @return Signatur des FluxML-Dokuments
	 */
	inline std::string & getSignature() { return signature_; }

	/**
	 * Berechnet eine Prüfsumme über das Info-Objekt.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class Info

} // namespace flux::data
} // namespace flux

#endif

