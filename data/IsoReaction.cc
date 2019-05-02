#include "Error.h"
#include "IsoReaction.h"
#include "charptr_array.h"
#include "DataException.h"
#include "Notation.h"
#include "Combinations.h"

namespace flux {
namespace data {

// REMARK: Atom-Mapping
void IsoReaction::finish()
{
	charptr_array perm_in, perm_out;
	charptr_array::const_iterator pi;
	std::list< Isotopomer* >::iterator l_iter;
	size_t size_in = 0, size_out = 0;
	int syntax_type_in, syntax_type_out;
	// Aufbau einer Konfiguration für linke und rechte Seite der Reaktion
	for (l_iter = reducts_.begin(); l_iter != reducts_.end(); l_iter++)
	{
		Isotopomer * I = static_cast< Isotopomer* >( *l_iter );
		perm_in.add(I->atom_cfg);
		size_in += Notation::perm_spec_length(I->atom_cfg);
	}

	for (l_iter = rproducts_.begin(); l_iter != rproducts_.end(); l_iter++)
	{
		Isotopomer * I = static_cast< Isotopomer* >( *l_iter );
		perm_out.add(I->atom_cfg);
		size_out += Notation::perm_spec_length(I->atom_cfg);
	}

	// Reaktion muss balanciert bzw. eine Abflussreaktion sein
	if (size_out != size_in and size_out != 0)
		fTHROW(DataException);

	// Anzahl der transp. Atome == Anzahl der Atome auf der in-Seite
	size_ = size_in;
	permutation_ = new int32_t[size_];
	
	// Fertig, falls gar keine Atome vorkommen
	if (size_in == 0)
		return;

	// Verwendete Syntax identifizieren
	syntax_type_in = Notation::check_perm_spec(perm_in);
	syntax_type_out = Notation::check_perm_spec(perm_out);

	// Da size_in > 0, muss eine kurze oder lange Syntax vorliegen
	if (syntax_type_in == -1 or syntax_type_in == 0)
	{
		delete[] permutation_;
		size_ = 0;
		permutation_ = 0;
		fERROR("illegal permutation on educt-side of reaction %s",name_);
		fTHROW(DataException);
	}

	if (size_out != 0 and syntax_type_out != syntax_type_in)
	{
		delete[] permutation_;
		size_ = 0;
		permutation_ = 0;
		fERROR("illegal permutation on product-side of reaction %s",name_);
		fTHROW(DataException);
	}

	// Lang-Syntax konkatenieren, normalisieren, prüfen
	if (syntax_type_in == 2)
	{
		// Lang-Syntax konkatenieren, normalisieren
		perm_in = charptr_array::split(perm_in.concat(" ")," \r\n\v\t");
		perm_out = charptr_array::split(perm_out.concat(" ")," \r\n\v\t");
	}
        
	if (size_in == size_out) // Produkt vorhanden
	{
		// Aufbau der Permutation
		if (syntax_type_in == 1)
		{
			// kurze Notation
			if (not get_perm(perm_in.concat(), perm_out.concat(), &permutation_))
			{
				// ungültige Atom-Konfiguration
				// den Fehler näher beschreiben
				charptr_array unmatched;
				for (size_t p=0; p<size_in; p++)
					if (permutation_[p] == -1)
						unmatched.add(perm_in[p]);
				if (unmatched.size())
				{
					fERROR("illegal permutation -- %s unmatched in cfg of reaction %s",
						unmatched.concat(", "), name_
						);
				}
				else
				{
					fERROR("illegal permutation -- dupliate atoms in cfg of reaction %s",
						name_
						);
				}

				delete[] permutation_;
				size_ = 0;
				permutation_ = 0;
				fTHROW(DataException);
			}
		}
		else
		{
			// lange Notation
			if (not get_perm(perm_in, perm_out, &permutation_))
			{
				// ungültige Atom-Konfiguration
				// den Fehler näher beschreiben
				charptr_array unmatched;
				for (size_t p=0; p<size_in; p++)
					if (permutation_[p] == -1)
						unmatched.add(perm_in[p]);
				if (unmatched.size())
				{
					fERROR("illegal permutation -- %s unmatched in cfg of reaction %s",
						unmatched.concat(", "), name_
						);
				}
				else
				{
					fERROR("illegal permutation -- dupliate atoms in cfg of reaction %s",
						name_
						);
				}
				
				delete[] permutation_;
				size_ = 0;
				permutation_ = 0;
				fTHROW(DataException);
			}
		}
	}
	else // size_out == 0, Produkt nicht vorhanden (Abfluß)
	{
		// obwohl es kein Produkt gibt, wird perm_in auf
		// uniqueness geprüft. Wird das nicht gemacht, kann
		// das zu kaum auffindbaren Fehlern führen ...
		perm_in.sort();
		pi = perm_in.begin();
		char const * c = *pi;
		for (++pi; pi!=perm_in.end(); pi++)
		{
			if (strcmp(c,*pi)==0)
			{
				fERROR("illegal permutation -- duplicate atoms in cfg of efflux reaction %s", name_);
				delete[] permutation_;
				size_ = 0;
				permutation_ = 0;
				fTHROW(DataException);
			}
			c = *pi;
		}

		// Identität
		for (size_t k=0; k<size_; k++)
			permutation_[k] = k;
	}
}

uint32_t IsoReaction::computeCheckSum(uint32_t crc, int crc_scope) const
{
	std::list< Isotopomer* >::const_iterator l_iter;

	if (crc_scope & CRC_REACTIONNETWORK)
	{
		crc = update_crc32(name_,strlen(name_),crc);
		crc = update_crc32(permutation_,sizeof(int32_t)*size_,crc);
		
		for (l_iter=reducts_.begin(); l_iter!=reducts_.end(); ++l_iter)
			crc = (*l_iter)->computeCheckSum(crc);
		for (l_iter=rproducts_.begin(); l_iter!=rproducts_.end(); ++l_iter)
			crc = (*l_iter)->computeCheckSum(crc);
	
		int32_t size = int32_t(size_);
		crc = update_crc32(&size,4,crc);
	}

	return crc;
}

uint32_t IsoReaction::Isotopomer::computeCheckSum(uint32_t crc) const
{
	// atom_cfg wird ausgelassen, da IsoReaction::permutation_ schon
	// abgearbeitet wurde ...
	return update_crc32(name,strlen(name),crc);
}

} // namespace flux::data
} // namespace flux

