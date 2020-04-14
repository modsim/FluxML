#include <cmath>
#include <cstring>
#include "InputPool.h"
#include "InputProfile.h"
#include "Error.h"
#include "cstringtools.h"
#include "BitArray.h"
#include "Combinations.h"
#include "Conversions.h"

using namespace flux::symb;

namespace flux {
namespace data {

InputPool::InputPool(
	char const * id,
	char const * name,
	BitArray const & mask,
	Type pool_type
	)
	: id_(strdup_alloc(id)),
	  name_(strdup_alloc(name)),
	  pool_type_(pool_type),
	  iso_values_(mask),
	  cumo_values_(mask),
	  emu_values_(mask),
	  purities_(mask),
	  finished_(false),
	  natural_(true),
	  cost_(0.), 
    profiles_(mask),
    profile_flag(false){ }

InputPool::InputPool(InputPool const & copy)
	: id_(strdup_alloc(copy.id_)),
	  name_(strdup_alloc(copy.name_)),
	  pool_type_(copy.pool_type_),
	  iso_values_(copy.iso_values_),
	  cumo_values_(copy.cumo_values_),
	  emu_values_(copy.emu_values_),
	  purities_(copy.purities_),
	  finished_(copy.finished_),
	  natural_(copy.natural_),
	  cost_(copy.cost_),
	  profiles_(copy.profiles_),
	  profile_flag(copy.profile_flag){ }

InputPool::~InputPool() { 
                delete[] id_; 
                delete[] name_; 
}

void InputPool::convert()
{
	MaskedArray2D::iterator i;
	MaskedArray::iterator j;

	switch (pool_type_)
	{
            case ip_isotopomer:
                    // Isotopomer -> Cumomer
                    if (iso_values_.getMask().size())
                    {
                            cumo_values_ = iso_values_;
                            cumulate(cumo_values_.getRawArray(),
                                    cumo_values_.getRawSize(), true);
                    }
                    else
                            cumo_values_ = MaskedArray();
                    break;
            case ip_emu:
                    // EMU -> Cumomer
                    if (emu_values_.getMask().size())
                    {
                            cumo_values_ = MaskedArray(emu_values_.getMask());
                            for (i=emu_values_.begin(); i!=emu_values_.end(); ++i)
                                    cumo_values_[i->idx] = i->value[i->value.size()-1];
                    }
                    else
                            cumo_values_ = MaskedArray();
            case ip_cumomer:
                    // Cumomer -> Isotopomer
                    if (cumo_values_.getMask().size())
                    {
                            iso_values_ = cumo_values_;                            
                            cumulate(iso_values_.getRawArray(),
                                    iso_values_.getRawSize(), false);
                    }
                    else
                            iso_values_ = MaskedArray();
                    break;
	}

	switch (pool_type_)
	{
	case ip_isotopomer:
	case ip_cumomer:
		// Isotopomer -> EMU
		if (iso_values_.getMask().size())
		{
			// Massenisotopomere!
			BitArray mask = iso_values_.getMask();
			mask.ones();
			emu_values_ = MaskedArray2D(mask);
			i = emu_values_.begin();
			i->value = Array< double >(1);
			i->value[0] = 1.; // 0-EMU
			for (++i; i!=emu_values_.end(); ++i)
			{
				Array< double > v(i->idx.countOnes()+1,0.);
				for (j=iso_values_.begin(); j!=iso_values_.end(); ++j)
					v[(i->idx & j->idx).countOnes()] += j->value;
				i->value = v;
			}
		}
		else
			emu_values_ = MaskedArray2D();
		break;
	case ip_emu:
		break;
	}
}

void InputPool::naturalIsotopeCorrection()
{
	MaskedArray::iterator i,j;
        MaskedArray2D::iterator p;
	// isotope natural abundances
	double const nc = 0.01055,  // carbon: 13C
                     nh = 0.00115,  // deuterium: 2H
                     nn = 0.00368;  // nitrogen: 15N
	double r;

	if (not natural_)
	{
		// nur für nicht ausschließlich natürlich markierte Pools:
		// Gibt es ausschließlich 100%ige Substrate? (künstlich)
		bool all_pure = true;
		for (p=purities_.begin(); p!=purities_.end(); ++p)
		{
                    Array< double > pvalues = p->value ;
                    for(size_t k= 0,len= pvalues.size(); k<len;++k)
                    {
                        if (pvalues[k] <= 1.)
                        {
                                all_pure = false;
                                break;
                        }
                    }
		}
		// bei 100% reinen Substraten gibt es nichts zu tun
		if (all_pure)
			return;
	}

	// sobald natürliche Markierung im Spiel ist, sind alle
	// Atom-Positionen markiert
	BitArray nmask = iso_values_.getMask();
	nmask.ones();
	MaskedArray nvalues(nmask);
        
	// ist der Pool ausschließlich natürlich markiert?
	if (natural_)
	{
            // ################# NEW Version: Multi-Isotopic Tracer MFA   ################ //
            if(iso_cfg_.size())
            {
                size_t pos= 0;
                
                // initilisiere alle isotopomer mit 1 (aufgrund der multiplikation unten))
                for (i=nvalues.begin(); i!=nvalues.end(); ++i)
                    i->value=1.;
                
                for(charptr_map< int >::const_iterator ic=iso_cfg_.begin(); 
                        ic!= iso_cfg_.end(); ic++)
                {
                    size_t N = ic->value;
                    if(N==0) 
                        continue;
                    
                    switch((char)ic->key[0])
                    {
                        case 'C':
                                    for (i=nvalues.begin(); i!=nvalues.end(); ++i)
                                    {
                                        size_t L,k;
                                        for(k=pos, L=0; k<(pos+N); k++)
                                            if(i->idx.get(k)) L++;
                                        i->value *= ::pow(1.-nc,N-L) * ::pow(nc,L);
                                    }
                                    break;
                        case 'N':
                                    for (i=nvalues.begin(); i!=nvalues.end(); ++i)
                                    {
                                        size_t L,k;
                                        for(k=pos, L=0; k<(pos+N); k++)
                                            if(i->idx.get(k)) L++;
                                        i->value *= ::pow(1.-nn,N-L) * ::pow(nn,L);
                                    }
                                    break;
                        case 'H':
                                    for (i=nvalues.begin(); i!=nvalues.end(); ++i)
                                    {
                                        size_t L,k;
                                        for(k=pos, L=0; k<(pos+N); k++)
                                            if(i->idx.get(k)) L++;
                                        i->value *= ::pow(1.-nh,N-L) * ::pow(nh,L);
                                    }
                                    break;
                        default:    
                                    fWARNING("input pool \"%s\": unsupported isotope [%s] found",
                                    name_, ic->key);
                    }
                    pos+= N;
                }
                fDEBUG(1,"*** determined natural abundance for pool : [%s] ***", name_);
                for (i=nvalues.begin(); i!=nvalues.end(); ++i)
                    fDEBUG(1,"\tisotope: %s  => value: %.6f", i->idx.toString('0','1'), i->value);
            }
            else
            {
                // ################# OLD Version: 13C-based MFA ################ //
                size_t N = nmask.size();
		for (i=nvalues.begin(); i!=nvalues.end(); ++i)
		{
			size_t L = i->idx.countOnes();
			i->value = ::pow(1.-nc,N-L) * ::pow(nc,L);
		}
		iso_values_ = nvalues;
            }
            iso_values_ = nvalues;
            return;
	}

	// allgemeiner Fall: Beliebige Mischung verschiedener Substrate
        size_t Lm,Um,Ln,Un;
        if(iso_cfg_.size()) /** NEW Version: Multi-Isotopic Tracer MFA **/
        {
            for (i=iso_values_.begin(); i!=iso_values_.end(); ++i)
            {
                fDEBUG(1,"cfg: %s    val: %s", i->idx.toString('0','1'), toString(i->value).c_str());
                if (i->value <= 0.)
                        continue;
                
                size_t pos= 0, purity_idx=0;
                MaskedArray nc_values(nmask);
                MaskedArray nn_values(nmask);
                MaskedArray nh_values(nmask);
                for(charptr_map< int >::const_iterator ic=iso_cfg_.begin(); 
                        ic!= iso_cfg_.end(); ic++)
                {
                    size_t N = ic->value;
                    if(N==0) 
                        continue;
                    r = purities_[i->idx][purity_idx++];
                    fDEBUG(1,"Isotope: [%s]  => cfg: %.*s   purity: %g", ic->key, int(ic->value), (i->idx.toString('0','1'))+pos,r);
                    if (r > 1.)
                    {
                            // künstliches Substrat mit 100%iger Reinheit
                            nvalues[i->idx] = i->value;
                            continue;
                    }
                    switch((char)ic->key[0])
                    {
                        case 'C':
                                    for (j=nc_values.begin(); j!=nc_values.end(); ++j)
                                    {
                                        // initilisiere alle variablen (warning C++98)
                                        Lm=0; Um=0; Ln=0; Un=0;
                                        
                                        // maskierte Bits (künstlich markiert)
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((j->idx & i->idx).get(k)) Lm++;
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((~(j->idx) & i->idx).get(k)) Um++;

                                        // nicht-maskierte Bits (natürlich markiert)
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((j->idx & ~(i->idx)).get(k)) Ln++;
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((~(j->idx) & ~(i->idx)).get(k)) Un++;

                                        j->value += (::pow(r,Lm)  * ::pow(1.-r,Um) *
                                                     ::pow(nc,Ln) * ::pow(1.-nc,Un) );
                                        fDEBUG(1,"[C]: nc>idx: %s  i>idx: %s  nc->value: %-7.7s  Lm= %i,  Um= %i,  Ln= %i,  Un= %i", 
                                                j->idx.toString('0','1'), i->idx.toString('0','1'), toString(j->value).c_str(), 
                                                int(Lm), int(Um), int(Ln), int(Un));
                                    }
                                    break;
                        case 'N':
                                    for (j=nn_values.begin(); j!=nn_values.end(); ++j)
                                    {
                                        // initilisiere alle variablen (warning C++98)
                                        Lm=0; Um=0; Ln=0; Un=0;
                                        
                                        // maskierte Bits (künstlich markiert)
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((j->idx & i->idx).get(k)) Lm++;
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((~(j->idx) & i->idx).get(k)) Um++;

                                        // nicht-maskierte Bits (natürlich markiert)
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((j->idx & ~(i->idx)).get(k)) Ln++;
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((~(j->idx) & ~(i->idx)).get(k)) Un++;

                                        j->value += ( ::pow(r,Lm)  * ::pow(1.-r,Um) *
                                                      ::pow(nn,Ln) * ::pow(1.-nn,Un) );
                                        fDEBUG(1,"[N]: nn>idx: %s  i>idx: %s  nn->value: %-7.7s  Lm= %i,  Um= %i,  Ln= %i,  Un= %i", 
                                                j->idx.toString('0','1'), i->idx.toString('0','1'), toString(j->value).c_str(), 
                                                int(Lm), int(Um), int(Ln), int(Un));
                                    }
                                    break;
                        case 'H':
                                    for (j=nh_values.begin(); j!=nh_values.end(); ++j)
                                    {
                                        // initilisiere alle variablen (warning C++98)
                                        Lm=0; Um=0; Ln=0; Un=0;
                                        
                                        // maskierte Bits (künstlich markiert)
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((j->idx & i->idx).get(k)) Lm++;
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((~(j->idx) & i->idx).get(k)) Um++;

                                        // nicht-maskierte Bits (natürlich markiert)
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((j->idx & ~(i->idx)).get(k)) Ln++;
                                        for(size_t k=pos; k<(pos+N); k++)
                                            if((~(j->idx) & ~(i->idx)).get(k)) Un++;

                                        j->value += ( ::pow(r,Lm)  * ::pow(1.-r,Um) *
                                                      ::pow(nh,Ln) * ::pow(1.-nh,Un) );
                                        fNOTICE("[H]: nh>idx: %s  i>idx: %s  nh->value: %-7.7s  Lm= %i,  Um= %i,  Ln= %i,  Un= %i", 
                                                j->idx.toString('0','1'), i->idx.toString('0','1'), toString(j->value).c_str(), 
                                                int(Lm), int(Um), int(Ln), int(Un));
                                    }
                                    break;
                        default:    
                                    fWARNING("input pool \"%s\": unsupported isotope tracer [%s]",
                                            name_, ic->key);
                    }
                    pos+= N;
                }
                
                double val=0.;
                for (j=nvalues.begin(); j!=nvalues.end(); ++j)
                {
                    // wurde aus dem Grund so implementiert, da Fälle gibts bei denen nun ein isotope
                    // gibt und den anderen nicht und bei der Multiplikation die Null rauskommt
                    // Beispiel: Pool[C] => erg = val * nc * nn,  wobei hier nn = 0 => erg = 0!!! 
                    val = i->value;
                    if(nc_values[j->idx]>0.)
                        val*= nc_values[j->idx];
                    
                    if(nn_values[j->idx]>0.)
                        val*= nn_values[j->idx];
                    
                    if(nh_values[j->idx]>0.)
                        val*= nh_values[j->idx];
                    
                    j->value += val; 
                }
            }                        
            fDEBUG(1,"*** determined natural abundance for pool : [%s] ***", name_);
            for (i=nvalues.begin(); i!=nvalues.end(); ++i)
                fDEBUG(1,"\tisotope: %s  => value: %.6f", i->idx.toString('0','1'), i->value);
        }
        // ################# OLD Version: 13C-based MFA ################ //
        else
        { 
            
            for (i=iso_values_.begin(); i!=iso_values_.end(); ++i)
            {
                    if (i->value <= 0.)
                            continue;

                    r = purities_[i->idx][0];
                    if (r > 1.)
                    {
                        // künstliches Substrat mit 100%iger Reinheit
                        nvalues[i->idx] = i->value;
                        continue;
                    }

                    for (j=nvalues.begin(); j!=nvalues.end(); ++j)
                    {
                        // maskierte Bits (künstlich markiert)
                        Lm = (j->idx & i->idx).countOnes();
                        Um = (~(j->idx) & i->idx).countOnes();
                        // nicht-maskierte Bits (natürlich markiert)
                        Ln = (j->idx & ~(i->idx)).countOnes();
                        Un = (~(j->idx) & ~(i->idx)).countOnes();

                        j->value += i->value * (
                                        ::pow(r,Lm) *
                                        ::pow(1.-r,Um) *
                                        ::pow(nc,Ln) *
                                        ::pow(1.-nc,Un)
                                        );
                    }
            }
            fDEBUG(1,"*** determined natural abundance for pool : [%s] ***", name_);
            for (i=nvalues.begin(); i!=nvalues.end(); ++i)
                fDEBUG(1,"\t%s: %s", i->idx.toString('0','1'), toString(i->value).c_str());
        }
	// korrigierte Isotopomer-Fractions übernehmen
	iso_values_ = nvalues;
}

void InputPool::setIsotopomerValue(
	BitArray const & i,
	double val,
	Array< double > const & purity,
	double cost
	)
{
	fASSERT(not finished_);
	fASSERT(pool_type_ == ip_isotopomer);
	natural_ = false;
	iso_values_[i] = val;
	purities_[i] = purity; // getestet!!!
	costs_[i] = cost;
}

void InputPool::setCumomerValue(
	BitArray const & i,
	double val
	)
{
	fASSERT(not finished_);
	fASSERT(pool_type_ == ip_cumomer);
	natural_ = false;
	cumo_values_[i] = val;
	purities_[i] = 2.; // bei Cumomer gibt es nur reine Substrate
}

void InputPool::setEMUValue(
	BitArray const & i,
	Array< double > const & val
	)
{
	fASSERT(not finished_);
	fASSERT(pool_type_ == ip_emu);
	natural_ = false;
	emu_values_[i] = val;
	purities_[i] = 2.; // bei EMU gibt es nur reine Substrate
}

void InputPool::setInputProfileValue(
	BitArray const & i,
	data::InputProfile const & val,
	Array< double > const & purity,
	double cost
	)
{
	fASSERT(not finished_);
	fASSERT(pool_type_ == ip_isotopomer);
	natural_ = false;
        profile_flag= true;
	profiles_[i] = val;
	purities_[i] = purity;
	costs_[i] = cost;
}

 void InputPool::setIsotopeCfg(
       charptr_map< int > const & iso_cfg)
 {
        fASSERT(not finished_);
        for(charptr_map< int >::const_iterator i=iso_cfg.begin(); i!= iso_cfg.end(); i++)
            iso_cfg_.insert(i->key, i->value);
            
 }
MaskedArray const & InputPool::getIsotopomerValues() const
{
	fASSERT(finished_);
	return iso_values_;
}

MaskedArray const & InputPool::getCumomerValues() const
{
	fASSERT(finished_);
	return cumo_values_;
}

MaskedArray2D const & InputPool::getEMUValues() const
{
	fASSERT(finished_);
	return emu_values_;
}

// TODO: ergänze die Rückgabe mit const MaskedProfile const &
// da MaskedProfile::const_iterator z.Z. noch nicht implementiert ist
MaskedProfile & InputPool::getInputProfiles() const
{
	fASSERT(finished_);
	return profiles_;
}

bool InputPool::finish()
{
	if (finished_)
		return true;
        
	// zunächst zwischen den Koordinatensystemen konvertieren
	convert();
	// jetzt liegt ein Isotopomer-Pool vor:
	pool_type_ = ip_isotopomer;
	
	// leeres Array?
	if (iso_values_.getMask().size() == 0)
	{
		finished_ = true;
		return true;
	}

	double fsum = 0.;
	MaskedArray::iterator mi = iso_values_.begin();
	cost_ = 0.;
	while (mi != iso_values_.end())
	{
		if (mi->value < 0. or mi->value > 1.)
		{
			fWARNING("input pool \"%s\": non-fractional abundances (%s#%s = %f)",
				name_, name_, mi->idx.toString('0','1'), mi->value);
			return false;
		}
		fsum += mi->value;
		cost_ += mi->value * costs_[mi->idx];
		mi++;
	}

        if ((iso_values_.getMask().anyOnes() or fabs(fsum) > 0.) && (profile_flag==false))
        {
            // Die Summe der Fractions sollte sehr nahe bei 1 liegen; falls
            // nicht, wird davon ausgegangen, dass ein Fehler in der Eingabe
            // vorliegt:
            if (fabs(fsum - 1.) > 1e-4)
            {
                    if(profile_flag)
                        fWARNING("input pool \"%s\": sum of isotopomer fractions specified through \"otherwise\" conditions should be 1",
                            name_);
                    else
                        fWARNING("input pool \"%s\": sum of isotopomer fractions should be 1",
                            name_);
                    return false;
            }

            // Zur Sicherheit werden die Fractions auf 1 normalisiert:
            for (mi=iso_values_.begin(); mi!=iso_values_.end(); mi++)
                    mi->value /= fsum;
        }
        
	// Korrektur für natürlich vorkommende Isotopen berechnen
	naturalIsotopeCorrection();

	// zwischen den Koordinatensystemen konvertieren
	convert();
	finished_ = true;
	return true;
}

uint32_t InputPool::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if ((crc_scope & CRC_CFG_SUBSTRATES) == 0)
		return crc;
	if (id_)
		crc = update_crc32(id_,strlen(id_),crc);
	if (name_)
		crc = update_crc32(name_,strlen(name_),crc);
	if (iso_values_.getRawArray())
		crc = update_crc32(
			iso_values_.getRawArray(),
			sizeof(double)*iso_values_.getRawSize(),
			crc
			);
	crc = update_crc32(&cost_,sizeof(cost_),crc);
	return crc;
}

} // namespace data
} // namespace flux

