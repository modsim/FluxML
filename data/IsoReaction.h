#ifndef ISOREACTION_H
#define ISOREACTION_H

#include <string>
#include <list>
extern "C" {
#include <stdint.h>
}
#include "DataException.h"
#include "cstringtools.h"

namespace flux {
namespace data {

class IsoReaction
{
public:
	/** Ein reagierender Metabolit (Isotopomer) */
	class Isotopomer
	{
	public:
		char * name;
		char * atom_cfg;
	public:
		Isotopomer(
			char const * rname,
			char const * rcfg
		) : name(strdup_alloc(rname)),
		    atom_cfg(strdup_alloc(rcfg)) { }
		
		~Isotopomer() { delete[] name; delete[] atom_cfg; }

		uint32_t computeCheckSum(uint32_t crc) const;
	};
	
private:
	/** Ein eindeutiger Bezeichner für die Reaktion */
	char * name_;
	/** Der Permutationsvektor der Reaktion */
	int32_t * permutation_;
	/** Eine Liste von Edukten */
	std::list< Isotopomer* > reducts_;
	/** Eine Liste von Produkten */
	std::list< Isotopomer* > rproducts_;
	/** Anzahl der an der Reaktion beteiligten Atome */
	size_t size_;
        /** Reaktionsrichtung **/
        bool bidirectional_;
	
public:
	/**
	 * Constructor.
	 *
	 * @param name eindeutige Bezeichnung der Reaktion
	 */
	IsoReaction(char const * name, bool bidirectional = true)
		: name_(strdup_alloc(name)), permutation_(0), size_(0),
                bidirectional_(bidirectional){ }

	/**
	 * Destructor
	 */
	~IsoReaction()
	{
		std::list< Isotopomer* >::iterator iter;
		iter = reducts_.begin();
		while (iter != reducts_.end())
		{
			delete *iter; iter++;
		}

		iter = rproducts_.begin();
		while (iter != rproducts_.end())
		{
			delete *iter; iter++;
		}

		if (permutation_) delete[] permutation_;
		delete[] name_;
	}
	
public:

	/**
	 * Fügt der Reaktion ein (weiteres) Edukt hinzu.
	 *
	 * @param name Edukt-Name
	 * @param cfg Edukt-Atom-Konfiguration
	 */
	inline void addEduct(
		char const * name,
		char const * cfg
		)
	{
		reducts_.push_back(new Isotopomer(name, cfg));
	}
	
	/**
	 * Fügt der Reaktion ein (weiteres) Produkt hinzu.
	 *
	 * @param name
	 * @param cfg Produkt-Atom-Konfiguration
	 */
	inline void addProduct(
		char const * name,
		char const * cfg
		)
	{
		rproducts_.push_back(new Isotopomer(name, cfg));
	}

	/**
	 * Berechnet die Permutation(en) und schließt die Erzeugung/Konfig. des
	 * Reaktions-Objekts ab.
	 */
	void finish();

	/**
	 
	 *
	 * @return Permutationsvektor der Isotopomer-Reaktion
	 */
	inline int32_t * getPermutation() const { return permutation_; }

	/**
	 * Gibt die Liste der Edukte zurück.
	 *
	 * @return Referenz auf die Liste der Edukte der Reaktion
	 */
	std::list< Isotopomer* > const & getEducts() { return reducts_; }

	/**
	 * Gibt die Liste der Produkte zurück.
	 *
	 * @return Referenz auf die Liste der Produkte der Reaktion
	 */
	std::list< Isotopomer* > const & getProducts() { return rproducts_; }

	/**
	 * Gibt die Anzahl der beiteiligten Atome zurück
	 *
	 * @return Anzahl der beteiligten Atome
	 */
	inline size_t getNumAtoms() const { return size_; }

	/**
	 * Gibt den Namen der Reaktion zurück.
	 *
	 * @return Bezeichnung der Reaktion
	 */
	char const * getName() const { return name_; }
        
        /**
	 * Gibt den Type der Reaktion zurück.
	 *
	 * @return Bezeichnung der Reaktion
	 */
	bool getType() const { return bidirectional_; }
	
	/**
	 * Berechnet eine Prüfsumme über das IsoReaction-Objekt.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

};

} // namespace flux::data
} // namespace flux

#endif

