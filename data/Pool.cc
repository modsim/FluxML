#include <string>
#include "Pool.h"
#include "Combinations.h"
#include "fRegEx.h"
#include "Error.h"
#include <stdlib.h>

namespace flux {
namespace data {

void Pool::parseIsotopeCfgAttribute(std::string cfg) 
{
    regex_t reg;
    regmatch_t match;
    const char * p = cfg.c_str();
    char buff[64], c;
    
    if(regcomp(&reg, "[CNH][0-9]+", REG_EXTENDED|REG_ICASE) != 0) 
        return;
    
    while(regexec(&reg, p, 1, &match, 0) == 0)
    {
        sprintf(buff, "%.*s", (int)(match.rm_eo - match.rm_so), &p[match.rm_so]);
        c=buff[0];
        
        charptr_map< int >::iterator ic= iso_cfg_.find(&c);
        if(ic==iso_cfg_.end())
            iso_cfg_.insert(&c, atoi((buff+1))); // printf("%c: %i  | ", c, atoi((buff+1)));
        else
        {
            fERROR("fatal: Duplicate specification of the isotope [%c]", c);
            exit(EXIT_FAILURE);
        }
        
        p += match.rm_eo; 
    }
    regfree(&reg); // printf("\n");
    
    if(!atomConsistencyCheck())
    {
        fERROR("The specified number of atoms in pool [%s] is inconsistency with the cfg attribute!!", getName());
        exit(EXIT_FAILURE);
    }
}

bool Pool::atomConsistencyCheck() { 
            
            // Rückwärtskompatibilität Anzahl der Kohlenstoffatome
            if(iso_cfg_.size()==0)
                return true;
            
            charptr_map< int >::const_iterator i ;
            int sum = 0;
            
            for(i= iso_cfg_.begin(); i!= iso_cfg_.end(); ++i)
                sum+= i->value;
            
            if(sum!= natoms_)
                return false; 
            return true;
        }
    
int Pool::countOnes(int n)
{
	// der absolute Hacker-Divide&Conquer-Algorithmus:
	n = ((n >> 1) & 0x55555555) + (n & 0x55555555);
	n = ((n >> 2) & 0x33333333) + (n & 0x33333333);
	n = ((n >> 4) & 0x0F0F0F0F) + (n & 0x0F0F0F0F);
	n = ((n >> 8) & 0x00FF00FF) + (n & 0x00FF00FF);
	return ((n >> 16) & 0x0000FFFF) + (n & 0x0000FFFF);
}

void Pool::makeCumulative(
	double * iso2cumu,
	int natoms
	)
{
	cumulate(iso2cumu,(1<<natoms));
}

void Pool::makeCumulative(
	double * cumu_array,
	double * iso_array,
	int natoms 
	)
{
	int i;
	for (i=0; i<(1<<natoms); i++)
		cumu_array[i] = iso_array[i];
	cumulate(cumu_array,(1<<natoms));
}

void Pool::makeNonCumulative(
	double * cumu2iso,
	int natoms
	)
{
	cumulate(cumu2iso,(1<<natoms),false);
}

void Pool::makeNonCumulative(
	double * iso_array,
	double const * cumu_array,
	int natoms
	)
{
	int i;
	for (i=0; i<(1<<natoms); i++)
		iso_array[i] = cumu_array[i];
	cumulate(iso_array,(1<<natoms),false);
}

/**
 * Berechnet die Massenisotopomere gegebener Atompositionen aus dem
 * Vektor der vollständigen Isotopomer-Verzeilung.
 *
 * @param iso Array mit vollständiger Isotopomer-Verteilung
 * @param natoms Anzahl der (Kohlenstoff-)Atome / Markierungspositionen
 * @param mask Bitmaske, Vorgabe der Atompositionen
 * @param massiso Speicherplatz für Massenisotopomere (bei 0 wird allokiert)
 */
double * Pool::makeMassIsotopomers(
	double * iso,
	int natoms,
	int mask,
	double * massiso
	)
{
	return iso2mass_iso(iso,(1<<natoms),mask,massiso);
}

#if 0
// Alte Bit-Algorithmen:

void Pool::makeCumulative(
	double * cumo_array,
	double * iso_array,
	int natoms 
	)
{
	int i, j, ncumos = (1 << natoms);
	double C;

	for (i=0; i<ncumos; i++) // Cumomere
	{
		C = 0.;
		for (j=0; j<ncumos; j++) // Isotopomere
		{
			// entspricht ~((~i)|(i&j)) == 0
			if ((i&~j) == 0) C += iso_array[j];
		}
		cumo_array[i] = C;
	}
}

void Pool::makeNonCumulative(
	double * iso_array,
	double * cumo_array,
	int natoms
	)
{
	int i, j, ncumos = (1 << natoms);
	double I;

	for (i=0; i<ncumos; i++) // Isotopomere
	{
		I = 0.;
		for (j=0; j<ncumos; j++) // Cumomere
		{
			// entspricht ~((~i)|(i&j)) == 0
			if ((i&~j) == 0)
			{
				if (countOnes(~i&j) % 2 == 0) // gerade
					I += cumo_array[j];
				else
					I -= cumo_array[j];
			}
		}
		iso_array[i] = I;
	}
}
#endif

int Pool::parseBinStringRev(std::string const & s)
{
	int v, i, l=s.size();

	// MSB = erstes Bit, LSB = letztes Bit
	for (v=0,i=0; i<l; i++)
		if (s[i]=='1')
			v |= 1 << (l-i);

	// MSB = letztes Bit, LSB = erstes Bit
	for (i=0, v=0; i<l; i++)
		if (s[i]=='1')
			v |= 1 << i;
	return v;
}

std::string Pool::toBinString(int n, int bits)
{
	std::string s;
	for (int i=0; i<bits; i++)
		if ((n & (1<<i)) != 0)
			s = s + '1';
		else
			s = s + 'x';
	return s;
}

uint32_t Pool::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if (crc_scope & CRC_REACTIONNETWORK)
	{
		crc = update_crc32(name_.c_str(),name_.size(),crc);
		crc = update_crc32(&natoms_,sizeof(natoms_),crc);
		crc = update_crc32(&poolsize_,sizeof(poolsize_),crc);
		crc = update_crc32(&used_in_reaction_,sizeof(used_in_reaction_),crc);
		crc = update_crc32(&has_efflux_,sizeof(has_efflux_),crc);
	}
	return crc;
}

} // namespace flux::data
} // namespace flux

