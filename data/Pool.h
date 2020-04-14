#ifndef POOL_H
#define POOL_H

#include "charptr_map.h"
extern "C" {
#include <stdint.h>
}
#include <string>

namespace flux {
namespace data {

/**
 * Abbildung eines Pools.
 * Objekte dieser Klasse werden ALLEIN im FluxML-Parser benötigt.
 *
 * @author Michael Weitzel <info@13cflux.net>
 * @author Salah Azzouzi <info@13cflux.net>
 * 
 * SA:  adds new cfg attribute and further functions for manipulating it
 */
class Pool
{
private:
	/** Eindeutiger Bezeichner des Pools (erforderlich) */
	std::string name_;
        /** Konfiguration beschreibt die verwendeten Isotope sowie ihre Anzahl (erforderlich) */
        std::string cfg_;
	/** Anzahl der Kohlenstoff-Atome */
	int natoms_;
	/** Kapazität eines Knotens (optional, default ist 0) */
	double poolsize_;
	/** Verwendungs-Flag */
	bool used_in_reaction_;
	/** Abfluss vorhanden? */
	bool has_efflux_;
        /** Multi-Isotopoes-Map: <Isotope,NumbAtoms> Isotope und deren Anzahl */
        charptr_map< int > iso_cfg_;
        

private:      
    
        void parseIsotopeCfgAttribute(std::string cfg);

public:
	/**
	 * Constructor.
	 * Erzeugt einen Pool aus eindeutiger Bezeichnung, einer Anzahl von
	 * Kohlenstoffatomen und einer Kapazität.
	 *
	 * @param name die eindeutige Bezeichnung des Pools
	 * @param natoms die Anzahl der Kohlenstoff-Atome im Pool
	 * @param poolsize die Kapazität des Pools (Standard ist 0)
	 */
	inline Pool(std::string const & name, int natoms, double poolsize, std::string const & cfg)
		: name_(name), cfg_(cfg), natoms_(natoms), poolsize_(poolsize),
		  used_in_reaction_(false), has_efflux_(false) 
                {                    
                    parseIsotopeCfgAttribute(cfg_);                    
                }
	
	/**
	 * Gibt die eindeutige Bezeichnung des Pools zurück.
	 *
	 * @return eindeutige Bezeichnung des Pools
	 */
	inline char const * getName() const { return name_.c_str(); }
        
        /**
	 * Gibt die Isotope-Konfiguration des Pools zurück.
	 *
	 * @return eindeutige Bezeichnung des Pools
	 */
	inline char const * getIsotopeCfg() const { return cfg_.c_str(); }
	
	/**
	 * Gibt die Anzahl der Kohlenstoffatome im Pool zurück.
	 *
	 * @return Anzahl der Kohlenstoffatome im Pool
	 */
	inline int getNumAtoms() const { return natoms_; }
        
        /**
	 * Gibt eine List mit allen chemischen Isotope und iheren Anzahl
         * im Pool zurück.
	 *
	 * @return Zugriff auf list 
	 */
	inline charptr_map< int > const & getIsotopesCfg() const { 
            
                return iso_cfg_;
        }
        
	/**
	 * Gibt die Anzahl der verwendeten Isotope im Pool zurück.
	 *
	 * @return Anzahl der Isotope im Pool
	 */
	inline size_t getNumOfActiveIsotope() const 
        {
            if(iso_cfg_.size()==0)
                return 1;
            
            size_t counter=0;
            for(charptr_map< int >::const_iterator i = iso_cfg_.begin();
                   i!=iso_cfg_.end(); i++)
            {
                if(i->value>0)
                    counter++;
            }
            return counter; 
        }
        
        /**
	 * Gibt die Anzahl der Atome von gegebenen chemischen Element
         * im Pool zurück.
	 *
	 * @return Anzahl der Kohlenstoffatome im Pool
	 */
	inline int getNumAtomsByElement(const char * element) const { 
            
            // Rückwärtskompatibilität Anzahl der Kohlenstoffatome
            if(iso_cfg_.size()==0)
                return natoms_;
            
            charptr_map< int >::const_iterator i = iso_cfg_.find(element);
            if(i!=iso_cfg_.end())
                return i->value; 
            else
                return -1;
        }
        
        /**
	 * Prüft die Konsistenz der Anzahl von definierten Atome im Pool.
	 *
	 * @return true, falls die Anzahl der Atome stimmt
	 */
	bool atomConsistencyCheck();

	/**
	 * Gibt die Kapazität des Pools zurück.
	 *
	 * @return Kapazität des Pools
	 */
	inline double getPoolSize() const { return poolsize_; }
        
	/**
	 * Zählt die gesetzten Bits in einem 32 Bit Integer.
	 *
	 * @param n Integer
	 * @return Anzahl der in n gesetzten Bits
	 */    
	static int countOnes(int n);
	
	/**
	 * Abfrage des Verwendungs-Flags. Gibt true zurück, falls der Pool
	 * an einer Reaktion teilnimmt.
	 *
	 * @return true, falls der Pool an einer Reaktion teilnimmt
	 */
	inline bool isUsedInReaction() const { return used_in_reaction_; }
	
	/**
	 * Setzt das Verwendungs-Flag.
	 */
	inline void setUsedInReaction() { used_in_reaction_ = true; }

	/**
	 * Abfrage: Ist der Pool mit einer Reaktion verbunden die als ein
	 * Abfluß herhalten kann? Alle Pools im Netzwerk brauchen einen Abfluß.
	 *
	 * @return true, falls der Pool mit einem Abfluß hat
	 */
	inline bool hasEfflux() const { return has_efflux_; }

	/**
	 * Setzt das Abfluß-Flag
	 */
	inline void setHasEfflux() { has_efflux_ = true; }

	/**
	 * Wandelt ein Array der Länge 2^natoms, in dem Isotopomer-
	 * Fractions abgelegt sind, in ein Array der gleichen Länge um, in
	 * dem die entsprechenden Cumomer-Fractions gespeichert sind.
	 *
	 * @param cumu_array das Array mit den Cumomer-Fractions (out)
	 * @param iso_array das Array mit den Isotopomer-Fractions (in)
	 * @param natoms Anzahl der Kohlenstoffatome
	 * 	(d.h. 2^natoms==Array-Länge)
	 */
	static void makeCumulative(
		double * cumu_array,
		double * iso_array,
		int natoms
		);

	/**
	 * Inplace-Iso2Cumo-Transformation.
	 *
	 * @param iso2cumo Array mit Isotopomer-Fractions; später
	 * 	Cumomer-Fractions
	 * @param natoms Anzahl der (Kohlenstoff-)Atome
	 */
	static void makeCumulative(
		double * iso2cumu,
		int natoms
		);
	
	/**
	 * Wandelt ein Array der Länge 2^natoms, in dem Cumomer-Fractions
	 * abgelegt sind, in ein Array der gleichen Länge um, in dem
	 * die entsprechenden Isotopomer-Fractions gespeichert sind.
	 *
	 * @param iso_array das Array mit den Isotopomer-Fractions (out)
	 * @param cumo_array das Array mit den Cumomer-Fractions (in)
	 * @param natoms Anzahl der Kohlenstoffatome
	 * 	(d.h. 2^natoms==Array-Länge)
	 */
	static void makeNonCumulative(
		double * iso_array,
		double const * cumu_array,
		int natoms
		);

	/**
	 * Inplace-Cumo2Iso-Transformation.
	 *
	 * @param cumo2iso Array mit Cumomer-Fractions; später Isotopomer-Fractions
	 * @param natoms Anzahl der (Kohlenstoff-)Atome
	 */
	static void makeNonCumulative(
		double * cumu2iso,
		int natoms
		);

	/**
	 * Berechnet die Massenisotopomere gegebener Atompositionen aus dem
	 * Vektor der vollständigen Isotopomer-Verzeilung.
	 *
	 * @param iso Array mit vollständiger Isotopomer-Verteilung
	 * @param natoms Anzahl der (Kohlenstoff-)Atome / Markierungspositionen
	 * @param mask Bitmaske, Vorgabe der Atompositionen
	 * @param massiso Speicherplatz für Massenisotopomere (bei 0 wird allokiert)
	 */
	static double * makeMassIsotopomers(
		double * iso,
		int natoms,
		int mask,
		double * massiso
		);
	
	/**
	 * Wandelt eine in ASCII-Darstellung gespeicherte Binärzahl in einen
	 * int um. Es wird dabei davon ausgegangen, dass das MSB das letzte
	 * Bit in der ASCII-Darstellung ist und das LSB das erste Bit. Das
	 * Zeichen 'x' oder 'X' wird als 0 verstanden. Geeignet zum Parsen
	 * der Konfiguration von Isotopomeren und Cumomeren.
	 *
	 * @param s der zu konvertierende String
	 * @return Zahlenwert des Bit-Strings
	 */
	static int parseBinStringRev(std::string const & s);
	
	/**
	 * Wandelt eine int mit einer vorgegebenen Anzahl von bits in einen
	 * ASCII-String um. Die '0' wird durch ein 'x' ersetzt. Das LSB wird
	 * im ersten Zeichen der ASCII-Darstellung abgelegt; das MSB im letzten
	 * Zeichen.
	 *
	 * @param n der umzuwandelnde Integer
	 * @param bits die Anzahl der umzuwandelnden Bits des Integers
	 * @return String-Darstellung des Integers
	 */
	static std::string toBinString(int n, int bits);

	/**
	 * Berechnet eine Prüfsumme über das Pool-Objekt.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummenberechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;
	
}; // class Pool

} // namespace flux::data
} // namespace flux

#endif

