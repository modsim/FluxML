#ifndef INPUTPOOL_H
#define INPUTPOOL_H

extern "C"
{
#include <stdint.h>
}
#include <cstddef>
#include "Error.h"
#include "BitArray.h"
#include "MaskedArray.h"
#include "InputProfile.h"
#include <list>

using namespace flux::symb;

namespace flux {
namespace data {

/**
 * Klasse zur Abbildung eines Substrat-Pools.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class InputPool
{
public:
	enum Type
	{
		ip_isotopomer,
		ip_cumomer,
		ip_emu
	};

private:
	/** Eindeutiger Bezeichner (bei mehreren Pools mit gl. Namen) */
	char * id_;
	/** Pool-Bezeichnung */
	char * name_;
	/** Pool-Typ */
	Type pool_type_;
	/** Isotopomer-Repräsentation des Pools */
	mutable MaskedArray iso_values_;
	/** Cumomer-Repräsentation des Pools */
	mutable MaskedArray cumo_values_;
	/** EMU-Repräsentation des Pools */
	mutable MaskedArray2D emu_values_;
	/** Reinheiten der Substrate */
	mutable MaskedArray2D purities_;
	/** Kosten der Substrate */
	mutable MaskedArray costs_;
	/** Flag; Setzen von Werten und Konvertierung abgeschlossen? */
	bool finished_;
	/** Flag; Ist der Pool ausschließlich natürlich markiert? */
	bool natural_;
	/** Kosten der Substratmischung */
	double cost_;
        /** Container für den substrate profile **/
        mutable MaskedProfile profiles_;
        /** Flag für den substrate profile **/
        bool profile_flag;
        /*Multi-Isotopoes-Map: <Isotope,NumbAtoms> Isotope und deren Anzahl*/
        charptr_map< int > iso_cfg_;
        
public:
	/**
	 * Constructor.
	 *
	 * @param name Pool-Bezeichnung
	 * @param mask Bit-Maske aktiver Atome (damit auch Atomanzahl)
	 * @param pool_type Pool-Typ (Isotopomer, Cumomer, EMU)
	 */
	InputPool(
		char const * id,
		char const * name,
		BitArray const & mask,
		Type pool_type
		);

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes InputPool-Objekt
	 */
	InputPool(InputPool const & copy);

	/**
	 * Destructor.
	 */
	virtual ~InputPool();

private:
	void convert();

	void naturalIsotopeCorrection();

public:
	void setIsotopomerValue(
		BitArray const & i,
		double val,
		Array< double > const & purity,
		double cost
		);
        
	void setCumomerValue(
		BitArray const & i,
		double val
		);

	void setEMUValue(
		BitArray const & i,
		Array< double > const & val
		);

        void setInputProfileValue(
		BitArray const & i,
		data::InputProfile const & val,
		Array< double > const & purity,
		double cost
		);
        
        void setIsotopeCfg(
		charptr_map< int > const & iso_cfg
		);
        
	inline double getIsotopomerValue(BitArray const & i) const
	{
		fASSERT(finished_);
		return iso_values_[i];
	}

	inline double getCumomerValue(BitArray const & i) const
	{
		fASSERT(finished_);
		return cumo_values_[i];
	}

	inline Array< double > const & getEMUValue(BitArray const & i) const
	{
		fASSERT(finished_);
		return emu_values_[i];
	}

	inline size_t getSize() const { return iso_values_.getLog2Size(); }
        
	inline char const * getId() const { return id_; }

	inline char const * getName() const { return name_; }

	MaskedArray const & getIsotopomerValues() const;
	        
	MaskedArray const & getCumomerValues() const;

	MaskedArray2D const & getEMUValues() const;
        
        MaskedProfile & getInputProfiles() const;
        
        inline bool hasInputProfile() { return profile_flag;}

	bool finish();

	inline BitArray const & getMask() const { return iso_values_.getMask(); }

	inline double getCost() const
	{
		fASSERT(finished_);
		return cost_;
	}

	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;
        
};

} // namespace flux::data
} // namespace flux

#endif

