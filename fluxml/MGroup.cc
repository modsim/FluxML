#include <cstring>
#include <sstream>
#include "Error.h"
#include "Combinations.h"
#include "MMatrix.h"
#include "Notation.h"
#include "MGroup.h"

using namespace flux::la;
using namespace flux::symb;

namespace flux {
namespace xml {

/*
 * --- MGroup ---
 */

MGroup::MGroup(MGroup const & copy)
: mgtype_(copy.mgtype_),
  group_id_(0),
  ts_set_(copy.ts_set_),
  scale_auto_(copy.scale_auto_),
  dim_(copy.dim_),
  spec_(0),
  error_model_(0),
  iso_cfg_(copy.iso_cfg_)
{
	if (copy.group_id_)
		group_id_ = strdup_alloc(copy.group_id_);
	if (copy.spec_)
		spec_ = strdup_alloc(copy.spec_);
	row_spec_ = new char*[dim_];
	for (size_t i=0; i<dim_; i++)
		row_spec_[i] = strdup_alloc(copy.row_spec_[i]);

	if (copy.error_model_) {
		error_model_ = new symb::ExprTree*[dim_ + 1];
		if (copy.error_model_[1] == 0) {
			for (size_t i = 0; i < dim_; i++) {
				error_model_[i] = copy.error_model_[0]->clone();
			}
		}
		else {
			for (size_t i = 0; i < dim_; i++) {
				fASSERT(copy.error_model_[i] != 0);
				error_model_[i] = copy.error_model_[i]->clone();
			}
		}
	}
}

MGroup& MGroup::operator= (MGroup const & copy)
{
	mgtype_ = copy.mgtype_;
	group_id_ = 0;
	ts_set_ = copy.ts_set_;
	scale_auto_ = copy.scale_auto_;
//        iso_cfg_ =copy.iso_cfg_;
	dim_ = copy.dim_;
	spec_ = 0;
	if (copy.group_id_)
		group_id_ = strdup_alloc(copy.group_id_);
	if (copy.spec_)
		spec_ = strdup_alloc(copy.spec_);
	row_spec_ = new char*[dim_];
	for (size_t i=0; i<dim_; i++)
		row_spec_[i] = strdup_alloc(copy.row_spec_[i]);

	/* Delete old error model */
	if (error_model_) {
		for (size_t i = 0; i < dim_; i++) {
			delete error_model_[i];
		}
	}
	delete[] error_model_;

	/* Copy error model if exists */
	if (copy.error_model_) {
		error_model_ = new symb::ExprTree*[dim_ + 1];
		if (copy.error_model_[1] == 0) {
			for (size_t i = 0; i < dim_; i++) {
				error_model_[i] = copy.error_model_[0]->clone();
			}
		}
		else {
			for (size_t i = 0; i < dim_; i++) {
				fASSERT(copy.error_model_[i] != 0);
				error_model_[i] = copy.error_model_[i]->clone();
			}
		}
	}
	return *this;
}

bool MGroup::registerTimeStamp(double value)
{
	if (ts_set_.find(value) != ts_set_.end())
		return false;
	ts_set_.insert(value);
	return true;
}

void MGroup::setErrorModel(ExprTree ** error_model)
{
	size_t i;
	fASSERT(error_model != 0 and error_model[0] != 0);
	if (error_model[1] == 0)
	{
		for (i=0; i<dim_; ++i)
		{
			delete error_model_[i];
			error_model_[i] = error_model[0]->clone();
		}
	}
	else
	{
		for (i=0; i<dim_; ++i)
		{
			fASSERT(error_model[i] != 0);
			delete error_model_[i];
			error_model_[i] = error_model[i]->clone();
		}
	}
}

uint32_t MGroup::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if (crc_scope & CRC_CFG_MEAS_MODEL)
	{
		crc = update_crc32(group_id_,strlen(group_id_),crc);
		crc = update_crc32(spec_,strlen(spec_),crc);
		// TODO: error model
	}
	return crc;
}

/*
 * --- SimpleMGroup ---
 */

SimpleMGroup::SimpleMGroup(SimpleMGroup const & copy) : MGroup(copy)
{
	std::map< double,MValue* >::const_iterator mvi;
	for (mvi = copy.mvalue_map_.begin();
		mvi != copy.mvalue_map_.end(); mvi++)
	{
		mvalue_map_[mvi->first] = mvi->second->clone();
	}
}

SimpleMGroup& SimpleMGroup::operator= (SimpleMGroup const & copy)
{
	MGroup::operator= (copy);
	std::map< double,MValue* >::const_iterator mvi;
	for (mvi = copy.mvalue_map_.begin();
		mvi != copy.mvalue_map_.end(); mvi++)
	{
		mvalue_map_[mvi->first] = mvi->second->clone();
	}
	return *this;
}

SimpleMGroup::~SimpleMGroup()
{
	std::map< double,MValue* >::iterator i;
	for (i=mvalue_map_.begin(); i!=mvalue_map_.end(); i++)
		delete i->second;
}

void SimpleMGroup::setMValuesStdDev(
	double ts,
	la::MVector const & x_meas,
	la::MVector const & x_stddev
	)
{
	fASSERT(x_meas.dim() == 1 and x_stddev.dim() == 1);
	registerMValue(MValueSimple(group_id_,ts,x_stddev.get(0),x_meas.get(0)));
}

void SimpleMGroup::removeMValuesStdDev()
{
        // lösche Messwerte
        mvalue_map_.clear();
        // lösche Zeitpunkte
        this->ts_set_.clear();
}

MValue const * SimpleMGroup::getMValue(double ts) const
{
	std::map< double,MValue* >::const_iterator i;
	i = mvalue_map_.find(ts);
	if (i == mvalue_map_.end())
		return 0;
	return i->second;
}

void SimpleMGroup::registerMValue(MValue const & mvalue)
{
	// virtuellen Constructor von MValue aufrufen:
	if (not isTimeStamp(mvalue.getTimeStamp()))
		fTHROW(XMLException,
			"invalid timestamp [%f]",
			mvalue.getTimeStamp()
			);
	if (getMValue(mvalue.getTimeStamp()) != 0)
		fTHROW(XMLException,
			"duplicate measurement value [timestamp=%f]",
			mvalue.getTimeStamp());
	mvalue_map_[mvalue.getTimeStamp()] = mvalue.clone();
}

bool SimpleMGroup::getMValuesStdDev(
	double ts,
	MVector & x_meas,
	MVector & x_stddev
	) const
{
	if (not isTimeStamp(ts))
		return false;

	MValue const * mv = getMValue(ts);
	if (mv == 0)
		fTHROW(XMLException,
			"fatal: missing measurement value detected (timestamp %f)",
			ts);

	if (x_meas.dim() != dim_)
		x_meas = MVector(dim_);
	if (x_stddev.dim() != dim_)
		x_stddev = MVector(dim_);
	
	x_meas.set(0,mv->get());
	x_stddev.set(0,mv->getStdDev());
	return true;
}

uint32_t SimpleMGroup::computeCheckSum(uint32_t crc, int crc_scope) const
{
	crc = MGroup::computeCheckSum(crc,crc_scope);
	if (crc_scope & CRC_CFG_MEAS_DATA)
	{
		std::map< double,MValue* >::const_iterator mvi;
		for (mvi=mvalue_map_.begin(); mvi!=mvalue_map_.end(); ++mvi)
			crc = mvi->second->computeCheckSum(crc,crc_scope);
	}
	return crc;
}

/*
 * --- MetaboliteMGroup ---
 */

MetaboliteMGroup * MetaboliteMGroup::parseSpec(
	char const * s,
	int * state
	)
{
	bool valid = true;
	char * scpy = strdup_alloc(s);
	int type = data::Notation::check_spec(scpy,&valid);
	delete[] scpy;

	switch (type)
	{
	case 1: // MS
		if (not valid)
			fTHROW(XMLException,"Invalid MS specification: [%s]", s);
		return MGroupMS::parseSpec(s,state);
	case 2: // MSMS
		if (not valid)
			fTHROW(XMLException,"Invalid MSMS specification: [%s]", s);
		return MGroupMSMS::parseSpec(s,state);
	case 3: // NMR1H
		if (not valid)
			fTHROW(XMLException,"Invalid 1H-NMR specification: [%s]", s);
		return MGroup1HNMR::parseSpec(s,state);
	case 4: // NMR13C
		if (not valid)
			fTHROW(XMLException,"Invalid 13C-NMR specification: [%s]", s);
		return MGroup13CNMR::parseSpec(s,state);
	case 5: // generic Cumomer
		if (not valid)
			fTHROW(XMLException,"Invalid generic cumomer specification: [%s]", s);
		return MGroupCumomer::parseSpec(s,state);
	case 6: // MI-MS
		if (not valid)
			fTHROW(XMLException,"Invalid Multi-Isotopic Tracer MS specification: [%s]", s);
		return MGroupMIMS::parseSpec(s,state);
	case -1: // Fehler
	default:
		fTHROW(XMLException,"Invalid (MS|MSMS|1H-NMR|13C-NMR|MIMS) specification: [%s]", s);
	}
	return 0;
}

uint32_t MetaboliteMGroup::computeCheckSum(uint32_t crc, int crc_scope) const
{
	crc = MGroup::computeCheckSum(crc,crc_scope);
	if (crc_scope & CRC_CFG_MEAS_MODEL)
	{
		crc = update_crc32(mname_,strlen(mname_),crc);
		crc = update_crc32(&natoms_,sizeof(natoms_),crc);
	}
	return crc;
}

/*
 * ----------------
 * --- MGroupMS ---
 * ----------------
 */

MGroupMS::MGroupMS(MGroupMS const & copy)
	: MetaboliteMGroup(copy), mask_(copy.mask_), weights_(0)
{
	int w;
	for (w=0; copy.weights_[w]!=-1; w++);
	weights_ = new int[w + 1];
	weights_[w] = -1;
	for (w=0; copy.weights_[w]!=-1; w++)
		weights_[w] = copy.weights_[w];
	
	std::map< double,std::map< int,MValue* > >::const_iterator ti;
	std::map< int,MValue* >::const_iterator vi;
	for (ti=copy.ms_mvalue_map_.begin(); ti!=copy.ms_mvalue_map_.end(); ti++)
		for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
			ms_mvalue_map_[ti->first][vi->first] = vi->second->clone();
}

MGroupMS& MGroupMS::operator= (MGroupMS const & copy)
{
	MetaboliteMGroup::operator= (copy);
	mask_ = copy.mask_;
	int w;
	for (w=0; copy.weights_[w]!=-1; w++);
	weights_ = new int[w + 1];
	weights_[w] = -1;
	for (w=0; copy.weights_[w]!=-1; w++)
		weights_[w] = copy.weights_[w];
	
	std::map< double,std::map< int,MValue* > >::const_iterator ti;
	std::map< int,MValue* >::const_iterator vi;
	for (ti=copy.ms_mvalue_map_.begin(); ti!=copy.ms_mvalue_map_.end(); ti++)
		for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
			ms_mvalue_map_[ti->first][vi->first] = vi->second->clone();
	return *this;
}

MGroupMS::~MGroupMS()
{
	std::map< double,std::map< int,MValue* > >::iterator i;
	std::map< int,MValue* >::iterator j;

	for (i=ms_mvalue_map_.begin(); i!=ms_mvalue_map_.end(); i++)
		for (j=i->second.begin(); j!=i->second.end(); j++)
			delete j->second;
	delete[] weights_;
}

void MGroupMS::setMValuesStdDev(
	double ts,
	la::MVector const & x_meas,
	la::MVector const & x_stddev	
	)
{
	for (size_t i=0; i<dim_; ++i)
            registerMValue(MValueMS(group_id_,ts,x_stddev.get(i),x_meas.get(i),weights_[i]));
}

void MGroupMS::removeMValuesStdDev()
{
        // lösche Messwerte
        ms_mvalue_map_.clear();
        // lösche Zeitpunkte
        this->ts_set_.clear();
}

MGroupMS * MGroupMS::parseSpec(char const * s, int * state)
{
	char * mname;
	int * weights;
	BitArray mask;

	char * scpy = strdup_alloc(s);
	int st = data::Notation::parse_MS_spec(scpy,&mname,&weights,mask);
	delete[] scpy;
	if (state) *state = st;
	if (st != 0)
		return 0;

	// Spezifikation neu aufbauen:
	charptr_array spec;
	spec.add(mname);
	if (mask.anyZeros())
	{
		char * r = data::Notation::mask_to_range(mask);
		spec.add("[%s]", r);
		delete[] r;
	}
	spec.add("#M");
	size_t dim;
	for (dim = 0; weights[dim] != -1; ++dim)
		spec.add("%s%i", dim>0?",":"",weights[dim]);

	return new MGroupMS(mname,mask,weights,dim,spec.concat());
}

std::set< BitArray > MGroupMS::getSimSet(MetaboliteMGroup::SimDataType sdt) const
{
	std::set< BitArray > S;

	switch (sdt)
	{
	case sdt_isotopomer:
		// per Konvention:
		S.insert(mask_);
		break;
	case sdt_cumomer:
		{
		// Alle möglichen Cumomere im Fragment:
		BitArray cfg(natoms_);
		int i, n=(1<<mask_.countOnes());
		// 0-Cumomer weglassen:
		cfg.incMask(mask_);
		for (i=1; i<n; i++)
		{
			S.insert(cfg);
			cfg.incMask(mask_);
		}
		}
		break;
	case sdt_emu:
		// EMUs sind bereits Mass-Iso-Fractions
		S.insert(mask_);
		break;
	}
	return S;
}

MValue const * MGroupMS::getMValue(double ts, int weight) const
{
	std::map< double,std::map< int,MValue* > >::const_iterator i;
	i = ms_mvalue_map_.find(ts);
	if (i == ms_mvalue_map_.end())
		// ts ist kein Timestamp
		return 0;
	std::map< int,MValue* >::const_iterator j;
	j = i->second.find(weight);
	if (j == i->second.end())
		// weight ist kein Gewicht
		return 0;
	return j->second;
}

bool MGroupMS::getMValuesStdDev(
	double ts,
	MVector & x_meas,
	MVector & x_stddev
	) const
{
	if (not isTimeStamp(ts))
            return false;
	
	int i = 0;
	int const * w = weights_;

	if (x_meas.dim() != dim_)
            x_meas = MVector(getNumWeights());
	if (x_stddev.dim() != dim_)
            x_stddev = MVector(getNumWeights());
	
	while (*w != -1)
	{
            MValue const * mv = getMValue(ts,*w);
            // Fehlende Messwerte sind nicht erlaubt!
            if (mv == 0)
                    fTHROW(XMLException,
                            "fatal: missing measurement values detected"
                            );

            x_meas.set(i,mv->get());
            x_stddev.set(i,mv->getStdDev());
            w++; i++;
	}
	return true;
}

void MGroupMS::registerMValue(MValue const & mvalue)
{
	MValueMS const & mv = static_cast< MValueMS const & >(mvalue);

	// virtuellen Constructor von MValue aufrufen:
	if (not isTimeStamp(mv.getTimeStamp()))
		fTHROW(XMLException,"invalid timestamp [%f]",
			mv.getTimeStamp());
	if (getMValue(mv.getTimeStamp(),mv.getWeight()) != 0)
		fTHROW(XMLException,
			"duplicate MS value [timestamp=%f, weight=%i]",
			mv.getTimeStamp(),mv.getWeight());
	ms_mvalue_map_[mv.getTimeStamp()][mv.getWeight()] = mv.clone();
}

char const * MGroupMS::getSpec(size_t idx) const
{
	fASSERT(idx < dim_);
	if (row_spec_[idx])
		return row_spec_[idx];

	size_t buf_len;
	char * buf;
	if (mask_.anyZeros())
	{
		char * r = data::Notation::mask_to_range(mask_);
		buf_len = snprintf(0,0,"%s[%s]#M%i",
				getMetaboliteName(),r,weights_[idx]) + 1;
		buf = new char[buf_len];
		snprintf(buf,buf_len,"%s[%s]#M%i",
			getMetaboliteName(),r,weights_[idx]);
		delete[] r;
	}
	else
	{
		buf_len = snprintf(0,0,"%s#M%i",
				getMetaboliteName(),weights_[idx]) + 1;
		buf = new char[buf_len];
		snprintf(buf,buf_len,"%s#M%i",
			getMetaboliteName(),weights_[idx]);
	}
	row_spec_[idx] = buf;
	return buf;
}

uint32_t MGroupMS::computeCheckSum(uint32_t crc, int crc_scope) const
{
	crc = MetaboliteMGroup::computeCheckSum(crc,crc_scope);
	if (crc_scope & CRC_CFG_MEAS_DATA)
	{
		std::map< double,std::map< int,MValue* > >::const_iterator vi;
		std::map< int,MValue* >::const_iterator vvi;
		for (vi=ms_mvalue_map_.begin(); vi!=ms_mvalue_map_.end(); ++vi)
			for (vvi=vi->second.begin(); vvi!=vi->second.end(); ++vvi)
				crc = vvi->second->computeCheckSum(crc,crc_scope);
	}
	return crc;
}

/*
 * --- MGroupMIMS ---
 */

MGroupMIMS::MGroupMIMS(MGroupMIMS const & copy)
	: MetaboliteMGroup(copy), mask_(copy.mask_)
{
        // Speicher anlegen
        weights_vec_.resize(copy.weights_vec_.size());
        std::vector<int *>::iterator wi_;
        std::vector<int *>::const_iterator wi;
        for(wi_=weights_vec_.begin();wi_!=weights_vec_.end();wi_++)
        {
            *wi_= new int[dim_+1];
            (*wi_)[dim_] = -1;
        }

        // Gewichte übertragen
        wi_=weights_vec_.begin();
        wi= copy.weights_vec_.begin();
        
        while(wi_!=weights_vec_.end() && wi!=copy.weights_vec_.end())
        {
            for(size_t i=0; i<dim_;++i)
                (*wi_)[i] = (*wi)[i];
            wi_++;
            wi++;    
        }
	
	std::map< double,std::map< std::vector<int>,MValue* > >::const_iterator ti;
	std::map< std::vector<int>,MValue* >::const_iterator vi;
	for (ti=copy.mims_mvalue_map_.begin(); ti!=copy.mims_mvalue_map_.end(); ti++)
            for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
                mims_mvalue_map_[ti->first][vi->first] = vi->second->clone();
}

MGroupMIMS& MGroupMIMS::operator= (MGroupMIMS const & copy)
{
	MetaboliteMGroup::operator= (copy);
	mask_ = copy.mask_;
	
        // Speicher anlegen
        weights_vec_.resize(copy.weights_vec_.size());
        std::vector<int *>::iterator wi_;
        std::vector<int *>::const_iterator wi;
        for(wi_=weights_vec_.begin();wi_!=weights_vec_.end();wi_++)
        {
            *wi_= new int[dim_+1];
            (*wi_)[dim_] = -1;
        }

        // Gewichte übertragen
        wi_=weights_vec_.begin();
        wi= copy.weights_vec_.begin();
        
        while(wi_!=weights_vec_.end() && wi!=copy.weights_vec_.end())
        {
            for(size_t i=0; i<dim_;++i)
                (*wi_)[i] = (*wi)[i];
            wi_++;
            wi++;    
        }
	
	std::map< double,std::map< std::vector<int>,MValue* > >::const_iterator ti;
	std::map< std::vector<int>,MValue* >::const_iterator vi;
	for (ti=copy.mims_mvalue_map_.begin(); ti!=copy.mims_mvalue_map_.end(); ti++)
            for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
                mims_mvalue_map_[ti->first][vi->first] = vi->second->clone();
	return *this;
}

MGroupMIMS::~MGroupMIMS()
{
	std::map< double,std::map< std::vector<int>,MValue* > >::iterator i;
	std::map< std::vector<int>,MValue* >::iterator j;
	for (i=mims_mvalue_map_.begin(); i!=mims_mvalue_map_.end(); i++)
		for (j=i->second.begin(); j!=i->second.end(); j++)
			delete j->second;
        
        std::vector<int *>::iterator wi;
        for(wi=weights_vec_.begin();wi!=weights_vec_.end();wi++)
            delete[] *wi;
}

void MGroupMIMS::setMValuesStdDev(
	double ts,
	la::MVector const & x_meas,
	la::MVector const & x_stddev	
	)
{
	for (size_t i=0; i<dim_; ++i)
        {
            std::vector<int> weigths;
            for(std::vector<int *>::iterator wi=weights_vec_.begin();
                wi!=weights_vec_.end();wi++)
                    weigths.push_back((*wi)[i]);
            registerMValue(MValueMIMS(group_id_,ts,x_stddev.get(i),x_meas.get(i),weigths));
        }
}

void MGroupMIMS::removeMValuesStdDev()
{
        // lösche Messwerte
        mims_mvalue_map_.clear();
        // lösche Zeitpunkte
        this->ts_set_.clear();
}

MGroupMIMS * MGroupMIMS::parseSpec(char const * s, int * state)
{
        int i,dim;
	char * mname;
	int * weights;
	BitArray mask;
        int numIsotopes;
        
	char * scpy = strdup_alloc(s);
	int st = data::Notation::parse_MIMS_spec(scpy,&mname,&weights,mask,&numIsotopes);
	delete[] scpy;
	if (state) *state = st;
	if (st != 0)
            return 0;
        
        i=0;
        while (weights[i] != -1) i++;
        dim = i/numIsotopes;
        // Speicher anlegen
        std::vector<int *> weights_vec(numIsotopes);
        std::vector<int *>::iterator wi;
        for(wi=weights_vec.begin();wi!=weights_vec.end();wi++)
        {
            *wi= new int[dim+1];
            (*wi)[dim] = -1;
        }
        // Gewichte übertragen
        i = 0, dim=0;
	while (weights[i] != -1)
	{
            for(wi=weights_vec.begin();wi!=weights_vec.end();wi++)
		(*wi)[dim] = weights[i++];
            dim++;
	}
	delete[] weights;
        
	// Spezifikation neu aufbauen:
	charptr_array spec;
	spec.add(mname);
	if (mask.anyZeros())
	{
		char * r = data::Notation::mask_to_range(mask);
		spec.add("["
                "%s]", r);
		delete[] r;
	}
	spec.add("#M");
        for (i=0; i<dim; ++i)
        {
            std::ostringstream ostr;
            for(wi=weights_vec.begin();wi!=weights_vec.end();wi++)
                ostr<< (wi!=weights_vec.begin()?",":"") <<(*wi)[i]; 
            spec.add("%s(%s)", i>0?";":"", ostr.str().c_str());
        }
        
        // Kann verbessert werdendurch dynamische Vektor und Übergabe der Referennz
        // darauf, statt 
        MGroupMIMS * obj = new MGroupMIMS(mname,mask,weights_vec,dim,spec.concat());
        
        // Reservierter Speicher wiederfreigeben!!
        for(wi=weights_vec.begin();wi!=weights_vec.end();wi++)
            delete[] *wi;
            
	return obj;
}

std::set< BitArray > MGroupMIMS::getSimSet(MetaboliteMGroup::SimDataType sdt) const
{
	std::set< BitArray > S;

	switch (sdt)
	{
	case sdt_isotopomer:
		// per Konvention:
		S.insert(mask_);
		break;
	case sdt_cumomer:
		{
		// Alle möglichen Cumomere im Fragment:
		BitArray cfg(natoms_);
		int i, n=(1<<mask_.countOnes());
		// 0-Cumomer weglassen:
		cfg.incMask(mask_);
		for (i=1; i<n; i++)
		{
			S.insert(cfg);
			cfg.incMask(mask_);
		}
		}
		break;
	case sdt_emu:
		// EMUs sind bereits Mass-Iso-Fractions
		S.insert(mask_);
		break;
	}
	return S;
}

MValue const * MGroupMIMS::getMValue(double ts, std::vector<int> weights) const
{
	std::map< double,std::map< std::vector<int>,MValue* > >::const_iterator i;
        i = mims_mvalue_map_.find(ts);
        if (i == mims_mvalue_map_.end())
            // ts ist kein Timestamp
            return 0;
	std::map< std::vector<int>,MValue* >::const_iterator j;
	j = i->second.find(weights);
	if (j == i->second.end())
            // weight ist kein Gewicht
            return 0;
	return j->second;
}

bool MGroupMIMS::getMValuesStdDev(
	double ts,
	MVector & x_meas,
	MVector & x_stddev
	) const
{
	if (not isTimeStamp(ts))
		return false;
	
        if (x_meas.dim() != dim_)
		x_meas = MVector(dim_);
	if (x_stddev.dim() != dim_)
		x_stddev = MVector(dim_);
//        size_t j;
        for (size_t i=0; i<dim_; ++i)
        {
            std::vector<int> weigths;
//            j=0;
//            printf("x-meas(");
            for(std::vector<int *>::const_iterator wi=weights_vec_.begin();
                wi!=weights_vec_.end();wi++)
            {
                    weigths.push_back((*wi)[i]);
//                    printf("%i, ", weigths[j++]);
            }
//            printf(") => ");
            MValue const * mv = getMValue(ts,weigths);
            // Fehlende Messwerte sind nicht erlaubt!
            if (mv == 0)
                    fTHROW(XMLException,
                            "fatal: missing multi-isotopic Tracer measurement values detected"
                            );
            x_meas.set(i,mv->get());
            x_stddev.set(i,mv->getStdDev());
            
//            printf("  val= %.6g \t",x_meas.get(i));
//            printf(" stddev= %.6g "  ,x_stddev.get(i));
//            
//            printf("\n");
	}
	return true;
}

void MGroupMIMS::registerMValue(MValue const & mvalue)
{
	MValueMIMS const & mv = static_cast< MValueMIMS const & >(mvalue);

	// virtuellen Constructor von MValue aufrufen:
	if (not isTimeStamp(mv.getTimeStamp()))
		fTHROW(XMLException,"invalid timestamp [%f]",
			mv.getTimeStamp());
        
	if (getMValue(mv.getTimeStamp(),mv.getWeight()) != 0)
        {
            std::ostringstream ostr;
            for(std::vector<int>::const_iterator i=mv.getWeight().begin();
                    i!= mv.getWeight().end(); i++)
                ostr<< (i!=mv.getWeight().begin()?",":"") <<*i;
            
            fTHROW(XMLException,
                    "duplicate MIMS value [timestamp=%f, weight=(%s)]",
                    mv.getTimeStamp(), ostr.str().c_str());
        }
	mims_mvalue_map_[mv.getTimeStamp()][mv.getWeight()] = mv.clone();
}

char const * MGroupMIMS::getSpec(size_t idx) const
{
	fASSERT(idx < dim_);
	if (row_spec_[idx])
		return row_spec_[idx];

	size_t buf_len;
	char * buf;
        std::vector<int *>::const_iterator wi;
        std::ostringstream ostr;
	if (mask_.anyZeros())
	{
		char * r = data::Notation::mask_to_range(mask_);
                for(wi=weights_vec_.begin();wi!=weights_vec_.end();wi++)
                    ostr<< (wi!=weights_vec_.begin()?",":"") <<(*wi)[idx]; 
		buf_len = snprintf(0,0,"%s[%s]#M(%s)",
				getMetaboliteName(),r,ostr.str().c_str()) + 1;
		buf = new char[buf_len];
		snprintf(buf,buf_len,"%s[%s]#M(%s)",
			getMetaboliteName(),r,ostr.str().c_str());
		delete[] r;
	}
	else
	{
                for(wi=weights_vec_.begin();wi!=weights_vec_.end();wi++)
                    ostr<< (wi!=weights_vec_.begin()?",":"") <<(*wi)[idx]; 
                    
		buf_len = snprintf(0,0,"%s#M(%s)",
				getMetaboliteName(),ostr.str().c_str()) + 1;
		buf = new char[buf_len];
		snprintf(buf,buf_len,"%s#M(%s)",
			getMetaboliteName(),ostr.str().c_str());
	}
	row_spec_[idx] = buf;
	return buf;
}

uint32_t MGroupMIMS::computeCheckSum(uint32_t crc, int crc_scope) const
{
	crc = MetaboliteMGroup::computeCheckSum(crc,crc_scope);
	if (crc_scope & CRC_CFG_MEAS_DATA)
	{
            std::map< double,std::map< std::vector<int>,MValue* > >::const_iterator vi;
            std::map< std::vector<int>,MValue* >::const_iterator vvi;
            for (vi=mims_mvalue_map_.begin(); vi!=mims_mvalue_map_.end(); ++vi)
			for (vvi=vi->second.begin(); vvi!=vi->second.end(); ++vvi)
				crc = vvi->second->computeCheckSum(crc,crc_scope);
	}
	return crc;
}
/*
 * --- MGroupMSMS ---
 */

MGroupMSMS::MGroupMSMS(MGroupMSMS const & copy)
	: MetaboliteMGroup(copy),
	  mask1_(copy.mask1_), mask2_(copy.mask2_),
	  weights1_(0), weights2_(0)
{
	int w;
	for (w=0; copy.weights1_[w]!=-1; w++);
	weights1_ = new int[w + 1];
	weights1_[w] = -1;
	for (w=0; copy.weights1_[w]!=-1; w++)
		weights1_[w] = copy.weights1_[w];

	for (w=0; copy.weights2_[w]!=-1; w++);
	weights2_ = new int[w + 1];
	weights2_[w] = -1;
	for (w=0; copy.weights2_[w]!=-1; w++)
		weights2_[w] = copy.weights2_[w];

	std::map< double,std::map< std::pair< int,int >,MValue* > >::const_iterator ti;
	std::map< std::pair< int,int >,MValue* >::const_iterator vi;
	for (ti=copy.msms_mvalue_map_.begin(); ti!=copy.msms_mvalue_map_.end(); ti++)
		for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
			msms_mvalue_map_[ti->first][vi->first] = vi->second->clone();
}

MGroupMSMS& MGroupMSMS::operator= (MGroupMSMS const & copy)
{
	MetaboliteMGroup::operator= (copy);
	mask1_ = copy.mask1_;
	mask2_ = copy.mask2_;
	int w;
	for (w=0; copy.weights1_[w]!=-1; w++);
	weights1_ = new int[w + 1];
	weights1_[w] = -1;
	for (w=0; copy.weights1_[w]!=-1; w++)
		weights1_[w] = copy.weights1_[w];

	for (w=0; copy.weights2_[w]!=-1; w++);
	weights2_ = new int[w + 1];
	weights2_[w] = -1;
	for (w=0; copy.weights2_[w]!=-1; w++)
		weights2_[w] = copy.weights2_[w];

	std::map< double,std::map< std::pair< int,int >,MValue* > >::const_iterator ti;
	std::map< std::pair< int,int >,MValue* >::const_iterator vi;
	for (ti=copy.msms_mvalue_map_.begin(); ti!=copy.msms_mvalue_map_.end(); ti++)
		for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
			msms_mvalue_map_[ti->first][vi->first] = vi->second->clone();
	return *this;
}

MGroupMSMS::~MGroupMSMS()
{
	delete[] weights1_;
	delete[] weights2_;
	std::map< double,std::map< std::pair< int,int >,MValue* > >::iterator i;
	std::map< std::pair< int,int >,MValue* >::iterator j;

	for (i=msms_mvalue_map_.begin(); i!=msms_mvalue_map_.end(); i++)
		for (j=i->second.begin(); j!=i->second.end(); j++)
			delete j->second;
}

void MGroupMSMS::setMValuesStdDev(
	double ts,
	la::MVector const & x_meas,
	la::MVector const & x_stddev	
	)
{
	for (size_t i=0; i<dim_; ++i)
		registerMValue(MValueMSMS(group_id_,ts,x_stddev.get(i),x_meas.get(i),weights1_[i],weights2_[i]));
}

void MGroupMSMS::removeMValuesStdDev()
{
        // lösche Messwerte
        msms_mvalue_map_.clear();
        // lösche Zeitpunkte
        this->ts_set_.clear();
}

MGroupMSMS * MGroupMSMS::parseSpec(char const * s, int * state)
{
	int i,j;
	char * mname = 0;
	int * weights, * weights1, * weights2;
	BitArray mask1, mask2;

	char * scpy = strdup_alloc(s);
	int st = data::Notation::parse_MSMS_spec(scpy,&mname,&weights,mask1,mask2);
	delete[] scpy;
	if (state) *state = st;
	if (st != 0)
		return 0;
	i = 0;
	while (weights[i] != -1) i++;
	i = i/2;
	weights1 = new int[i+1];
	weights2 = new int[i+1];
	weights1[i] = -1;
	weights2[i] = -1;
	i = 0; j = 0;
	while (weights[i] != -1)
	{
		weights1[j] = weights[i++];
		weights2[j] = weights[i++];
		j++;
	}
	delete[] weights;

	// Spezifikation neu aufbauen
	charptr_array spec;
	spec.add(mname);
	char * r1 = data::Notation::mask_to_range(mask1);
	char * r2 = data::Notation::mask_to_range(mask2);
	spec.add("[%s:%s]#M", r1, r2);
	delete[] r1;
	delete[] r2;
	for (i=0; i<j; ++i)
		spec.add("%s(%i,%i)", i>0?",":"", weights1[i], weights2[i]);

	return new MGroupMSMS(mname,mask1,mask2,weights1,weights2,j,spec.concat());
}

std::set< BitArray > MGroupMSMS::getSimSet(MetaboliteMGroup::SimDataType sdt) const
{
	std::set< BitArray > S;

	switch (sdt)
	{
	case sdt_isotopomer:
		// per Konvention:
		S.insert(mask1_);
		break;
	case sdt_cumomer:
	case sdt_emu:
		{
		// Alle möglichen Cumomere im Fragment:
		BitArray cfg(natoms_);
		int i, n=(1<<mask1_.countOnes());
		// 0-Cumomer weglassen:
		cfg.incMask(mask1_);
		for (i=1; i<n; i++)
		{
			S.insert(cfg);
			cfg.incMask(mask1_);
		}
		}
		break;
	}
	return S;
}

MValue const * MGroupMSMS::getMValue(double ts, int weight1, int weight2) const
{
	std::map< double,std::map< std::pair< int,int >,MValue* > >::const_iterator i;
	i = msms_mvalue_map_.find(ts);
	if (i == msms_mvalue_map_.end())
		// ts ist kein Timestamp
		return 0;
	std::map< std::pair< int,int >,MValue* >::const_iterator j;
	j = i->second.find(std::pair< int,int >(weight1,weight2));
	if (j == i->second.end())
		// weight ist kein Gewicht
		return 0;
	return j->second;
}
const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}
bool MGroupMSMS::getMValuesStdDev(
	double ts,
	MVector & x_meas,
	MVector & x_stddev
	) const
{
	if (not isTimeStamp(ts))
		return false;
	
	int i = 0;
	int const * w1 = weights1_;
	int const * w2 = weights2_;

	if (x_meas.dim() != dim_)
		x_meas = MVector(dim_);
	if (x_stddev.dim() != dim_)
		x_stddev = MVector(dim_);
	
	while (*w1 != -1)
	{
		MValue const * mv = getMValue(ts,*w1,*w2);
		// Fehlende Messwerte sind nicht erlaubt!
		if (mv == 0)
			fTHROW(XMLException,
				"fatal: missing measurement values detected"
				);

		x_meas.set(i,mv->get());
		x_stddev.set(i,mv->getStdDev());
		w1++; w2++; i++;
	}
	return true;
}



void MGroupMSMS::registerMValue(MValue const & mvalue)
{
	MValueMSMS const & mv = static_cast< MValueMSMS const & >(mvalue);

	// virtuellen Constructor von MValue aufrufen:
	if (not isTimeStamp(mv.getTimeStamp()))
		fTHROW(XMLException,"invalid timestamp [%f]",
			mv.getTimeStamp());
	if (getMValue(mv.getTimeStamp(),mv.getWeight1(),mv.getWeight2()) != 0)
		fTHROW(XMLException,
			"duplicate MSMS value [timestamp=%f, weight1=%i, weight2=%i]",
			mv.getTimeStamp(),mv.getWeight1(),mv.getWeight1());

	msms_mvalue_map_[mv.getTimeStamp()][
		std::pair< int,int >(mv.getWeight1(),mv.getWeight2())
		] = mv.clone();
}

char const * MGroupMSMS::getSpec(size_t idx) const
{
	fASSERT(idx < dim_);
	if (row_spec_[idx])
		return row_spec_[idx];

	size_t buf_len;
	char * buf;
	char * r1 = data::Notation::mask_to_range(mask1_);
	char * r2 = data::Notation::mask_to_range(mask2_);

	buf_len = snprintf(0,0,"%s[%s:%s]#M(%i,%i)",
			getMetaboliteName(),r1,r2,weights1_[idx],weights2_[idx]) + 1;
	buf = new char[buf_len];
	snprintf(buf,buf_len,"%s[%s:%s]#M(%i,%i)",
		getMetaboliteName(),r1,r2,weights1_[idx],weights2_[idx]);
	delete[] r1;
	delete[] r2;
	row_spec_[idx] = buf;
	return buf;
}

uint32_t MGroupMSMS::computeCheckSum(uint32_t crc, int crc_scope) const
{
	crc = MetaboliteMGroup::computeCheckSum(crc,crc_scope);
	if (crc_scope & CRC_CFG_MEAS_DATA)
	{
		std::map< double,std::map< std::pair< int,int >,MValue* > >::const_iterator vi;
		std::map< std::pair< int,int >,MValue* >::const_iterator vvi;
		for (vi=msms_mvalue_map_.begin(); vi!=msms_mvalue_map_.end(); ++vi)
			for (vvi=vi->second.begin(); vvi!=vi->second.end(); ++vvi)
				crc = vvi->second->computeCheckSum(crc,crc_scope);
	}
	return crc;
}

/*
 * --- MGroup1HNMR ---
 */

MGroup1HNMR::MGroup1HNMR(MGroup1HNMR const & copy)
	: MetaboliteMGroup(copy), poslst_(0)
{
	int p;
	for (p=0; copy.poslst_[p]!=-1; p++);
	poslst_ = new int[p + 1];
	poslst_[p] = -1;
	for (p=0; copy.poslst_[p]!=-1; p++)
		poslst_[p] = copy.poslst_[p];

	std::map< double,std::map< int,MValue* > >::const_iterator ti;
	std::map< int,MValue* >::const_iterator vi;
	for (ti=copy.nmr1h_mvalue_map_.begin(); ti!=copy.nmr1h_mvalue_map_.end(); ti++)
		for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
			nmr1h_mvalue_map_[ti->first][vi->first] = vi->second->clone();
}

MGroup1HNMR& MGroup1HNMR::operator= (MGroup1HNMR const & copy)
{
	MetaboliteMGroup::operator= (copy);
	int p;
	for (p=0; copy.poslst_[p]!=-1; p++);
	poslst_ = new int[p + 1];
	poslst_[p] = -1;
	for (p=0; copy.poslst_[p]!=-1; p++)
		poslst_[p] = copy.poslst_[p];

	std::map< double,std::map< int,MValue* > >::const_iterator ti;
	std::map< int,MValue* >::const_iterator vi;
	for (ti=copy.nmr1h_mvalue_map_.begin(); ti!=copy.nmr1h_mvalue_map_.end(); ti++)
		for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
			nmr1h_mvalue_map_[ti->first][vi->first] = vi->second->clone();
	return *this;
}



MGroup1HNMR::~MGroup1HNMR()
{
	std::map< double,std::map< int,MValue* > >::iterator i;
	std::map< int,MValue* >::iterator j;

	for (i=nmr1h_mvalue_map_.begin(); i!=nmr1h_mvalue_map_.end(); i++)
		for (j=i->second.begin(); j!=i->second.end(); j++)
			delete j->second;

	delete[] poslst_;
}

void MGroup1HNMR::setMValuesStdDev(
	double ts,
	la::MVector const & x_meas,
	la::MVector const & x_stddev
	)
{
	for (size_t i=0; i<dim_; ++i)
		registerMValue(MValue1HNMR(group_id_,ts,x_stddev.get(i),x_meas.get(i),poslst_[i]));
}

void MGroup1HNMR::removeMValuesStdDev()
{
        // lösche Messwerte
        nmr1h_mvalue_map_.clear();
        // lösche Zeitpunkte
        this->ts_set_.clear();
}

MGroup1HNMR * MGroup1HNMR::parseSpec(char const * s, int * state)
{
	char * mname;
	int * poslst;
	char * scpy = strdup_alloc(s);
	int st = data::Notation::parse_1HNMR_spec(scpy,&mname,&poslst);
	delete[] scpy;
	if (state) *state = st;
	if (st != 0)
		return 0;

	// Spezifikation neu aufbauen
	size_t dim;
	charptr_array spec;
	spec.add(mname);
	spec.add("#P");
	for (dim=0; poslst[dim]!=-1; ++dim)
		spec.add("%s%i", dim>0?",":"", poslst[dim]);

	return new MGroup1HNMR(mname,poslst,dim,spec.concat());
}

std::set< BitArray > MGroup1HNMR::getSimSet(MetaboliteMGroup::SimDataType sdt) const
{
	std::set< BitArray > S;
	BitArray cfg(natoms_);

	switch (sdt)
	{
	case sdt_isotopomer:
		// per Konvention:
		S.insert(getSimMask());
		break;
	case sdt_cumomer:
	case sdt_emu:
		// Einzelne Positionen:
		for (int p=0; poslst_[p]!=-1; p++)
		{
			cfg.set(poslst_[p]-1);
			S.insert(cfg);
			cfg.clear(poslst_[p]-1);
		}
		break;
	}
	return S;
}

MValue const * MGroup1HNMR::getMValue(double ts, int pos) const
{
	std::map< double,std::map< int,MValue* > >::const_iterator i;
	i = nmr1h_mvalue_map_.find(ts);
	if (i == nmr1h_mvalue_map_.end())
		// ts ist kein Timestamp
		return 0;
	std::map< int,MValue* >::const_iterator j;
	j = i->second.find(pos);
	if (j == i->second.end())
		// pos ist keine Position
		return 0;
	return j->second;
}

bool MGroup1HNMR::getMValuesStdDev(
	double ts,
	MVector & x_meas,
	MVector & x_stddev
	) const
{
	if (not isTimeStamp(ts))
		return false;
	
	int i = 0;
	int const * p = poslst_;
	if (x_meas.dim() != dim_)
		x_meas = MVector(dim_);
	if (x_stddev.dim() != dim_)
		x_stddev = MVector(dim_);
	
	while (*p != -1)
	{
		MValue const * mv = getMValue(ts,*p);
		// Fehlende Messwerte sind nicht erlaubt!
		if (mv == 0)
			fTHROW(XMLException,
				"fatal: missing measurement values detected"
				);

		x_meas.set(i,mv->get());
		x_stddev.set(i,mv->getStdDev());
		p++; i++;
	}
	return true;
}

void MGroup1HNMR::registerMValue(MValue const & mvalue)
{
	MValue1HNMR const & mv = static_cast< MValue1HNMR const & >(mvalue);

	// virtuellen Constructor von MValue aufrufen:
	if (not isTimeStamp(mv.getTimeStamp()))
		fTHROW(XMLException,"invalid timestamp [%f]",
			mv.getTimeStamp());
	if (getMValue(mv.getTimeStamp(),mv.getPos()) != 0)
		fTHROW(XMLException,
			"duplicate 1H-NMR value [timestamp=%f, pos=%i]",
			mv.getTimeStamp(),mv.getPos());
	nmr1h_mvalue_map_[mv.getTimeStamp()][mv.getPos()] = mv.clone();
}

char const * MGroup1HNMR::getSpec(size_t idx) const
{
	fASSERT(idx < dim_);
	if (row_spec_[idx])
		return row_spec_[idx];
	size_t buf_len;
	char * buf;
	buf_len = snprintf(0,0,"%s#P%i",getMetaboliteName(),poslst_[idx]) + 1;
	buf = new char[buf_len];
	snprintf(buf,buf_len,"%s#P%i",getMetaboliteName(),poslst_[idx]);
	row_spec_[idx] = buf;
	return buf;
}

uint32_t MGroup1HNMR::computeCheckSum(uint32_t crc,int crc_scope) const
{
	crc = MetaboliteMGroup::computeCheckSum(crc,crc_scope);
	if (crc_scope & CRC_CFG_MEAS_DATA)
	{
		std::map< double,std::map< int,MValue* > >::const_iterator vi;
		std::map< int,MValue* >::const_iterator vvi;
		for (vi=nmr1h_mvalue_map_.begin(); vi!=nmr1h_mvalue_map_.end(); ++vi)
			for (vvi=vi->second.begin(); vvi!=vi->second.end(); ++vvi)
				crc = vvi->second->computeCheckSum(crc,crc_scope);
	}
	return crc;
}

/*
 * --- MGroup13CNMR ---
 */

MGroup13CNMR::MGroup13CNMR(MGroup13CNMR const & copy)
	: MetaboliteMGroup(copy), poslst_(0), typelst_(0)
{
	int p;
	for (p=0; copy.poslst_[p]!=-1; p++);
	poslst_ = new int[p+1];
	typelst_ = new Type[p];
	poslst_[p] = -1;
	for (p=0; copy.poslst_[p]!=-1; p++)
	{
		poslst_[p] = copy.poslst_[p];
		typelst_[p] = copy.typelst_[p];
	}

	std::map< double,std::map< std::pair< int,Type >,MValue* > >::const_iterator ti;
	std::map< std::pair< int,Type >,MValue* >::const_iterator vi;
	for (ti=copy.nmr13c_mvalue_map_.begin(); ti!=copy.nmr13c_mvalue_map_.end(); ti++)
		for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
			nmr13c_mvalue_map_[ti->first][vi->first] = vi->second->clone();
}

MGroup13CNMR& MGroup13CNMR::operator= (MGroup13CNMR const & copy)
{
	MetaboliteMGroup::operator= (copy);
	int p;
	for (p=0; copy.poslst_[p]!=-1; p++);
	poslst_ = new int[p+1];
	typelst_ = new Type[p];
	poslst_[p] = -1;
	for (p=0; copy.poslst_[p]!=-1; p++)
	{
		poslst_[p] = copy.poslst_[p];
		typelst_[p] = copy.typelst_[p];
	}

	std::map< double,std::map< std::pair< int,Type >,MValue* > >::const_iterator ti;
	std::map< std::pair< int,Type >,MValue* >::const_iterator vi;
	for (ti=copy.nmr13c_mvalue_map_.begin(); ti!=copy.nmr13c_mvalue_map_.end(); ti++)
		for (vi=ti->second.begin(); vi!=ti->second.end(); vi++)
			nmr13c_mvalue_map_[ti->first][vi->first] = vi->second->clone();
	return *this;
}


MGroup13CNMR::~MGroup13CNMR()
{
	std::map< double,std::map< std::pair< int,Type >,MValue* > >::iterator i;
	std::map< std::pair< int,Type >,MValue* >::iterator j;

	for (i=nmr13c_mvalue_map_.begin(); i!=nmr13c_mvalue_map_.end(); i++)
		for (j=i->second.begin(); j!=i->second.end(); j++)
			delete j->second;
	delete[] poslst_;
	delete[] typelst_;
}

void MGroup13CNMR::setMValuesStdDev(
	double ts,
	la::MVector const & x_meas,
	la::MVector const & x_stddev	
	)
{
	for (size_t i=0; i<dim_; ++i)
		registerMValue(MValue13CNMR(group_id_,ts,x_stddev.get(i),x_meas.get(i),poslst_[i],typelst_[i]));
}

void MGroup13CNMR::removeMValuesStdDev()
{
        // lösche Messwerte
        nmr13c_mvalue_map_.clear();
        // lösche Zeitpunkte
        this->ts_set_.clear();
}

MGroup13CNMR * MGroup13CNMR::parseSpec(char const * s, int * state)
{
	char * mname;
	int * poslst;
	int * itypelst;
	Type * typelst;
	int i,j;
	char * scpy = strdup_alloc(s);
	int st = data::Notation::parse_13CNMR_spec(scpy,&mname,&poslst,&itypelst);
	delete[] scpy;
	if (state) *state = st;
	if (st != 0)
		return 0;

	for (i=0; itypelst[i]!=-1; i++);
	{}

	typelst = new Type[i];

	for (i=0; itypelst[i]!=-1; i++)
		typelst[i] = (Type)itypelst[i];
	delete[] itypelst;

	// Spezifikation neu aufbauen
	charptr_array spec;
	spec.add(mname);
	spec.add("#");
	for (j=0; j<i; ++j)
	{
		if (j>0) spec.add(",");
		if (j==0 or typelst[j]!=typelst[j-1])
		{
			switch (typelst[j])
			{
			case MValue13CNMR::S:
				spec.add("S");
				break;
			case MValue13CNMR::DL:
				spec.add("DL");
				break;
			case MValue13CNMR::DR:
				spec.add("DR");
				break;
			case MValue13CNMR::DD:
				spec.add("DD");
				break;
			case MValue13CNMR::T:
				spec.add("T");
				break;
			}
		}
		spec.add("%i", poslst[j]);
	}
	return new MGroup13CNMR(mname,poslst,typelst,i,spec.concat());
}

BitArray MGroup13CNMR::getSimMask() const
{
	BitArray mask(natoms_);
	for (size_t i=0; i<dim_; i++)
	{
		int p = poslst_[i]-1;
		
		// Unterscheidung nach Typ ist nicht nötig.
		// JEDE 13C-NMR Messung schaut auf die
		// linke und rechte Nachbarschaft einer Position,
		// falls diese existiert.
		if (p-1>=0) mask.set(p-1);
		mask.set(p);
		if (p+1<natoms_) mask.set(p+1);
	}
	return mask;
}

std::set< BitArray > MGroup13CNMR::getSimSet(MetaboliteMGroup::SimDataType sdt) const
{
	std::set< BitArray > S;
	BitArray cfg(natoms_);

	switch (sdt)
	{
	case sdt_isotopomer:
		// per Konvention:
		S.insert(getSimMask());
		break;
	case sdt_cumomer:
	case sdt_emu:
		// Einzelne Positionen:
		for (int p=0; poslst_[p]!=-1; p++)
		{
			if (poslst_[p] == 1)
			{
				// möglich: S1, DR1
				switch (typelst_[p])
				{
				case MValue13CNMR::S:
					// 100... = 1xx... - 11x... - 1x1... + 111...
					cfg.zeros();
					cfg.set(0); // +1xx...
					S.insert(cfg);
					if (natoms_ > 1)
					{
						cfg.set(1); // -11x...
						S.insert(cfg);
					}
					if (natoms_ > 2)
					{
						cfg.set(2); // +111...
						S.insert(cfg);
					}
					if (natoms_ > 1)
					{
						cfg.clear(1); // -1x1...
						S.insert(cfg);
					}
					break;
				case MValue13CNMR::DR:
					// 110... = 11x... - 111...
					// +11x...
					cfg.zeros();
					cfg.set(0); cfg.set(1);
					S.insert(cfg);
					// -111...
					if (natoms_ > 2) { cfg.set(2); S.insert(cfg); }
					break;
				default:
					// unmögliche Kombination?!
					fASSERT_NONREACHABLE();
				}
			}
			else if (poslst_[p] == natoms_)
			{
				// möglich: Sn, DLn
				// ...001 = ...xx1 - ...1x1 - ...x11 + ...111
				switch (typelst_[p])
				{
				case MValue13CNMR::S:
					// ...001 = ...xx1 - ...1x1 - ...x11 + ...111
					cfg.zeros();
					cfg.set(natoms_-1); // +...xx1
					S.insert(cfg);
					if (natoms_ > 1)
					{
						cfg.set(natoms_-2); // -...x11
						S.insert(cfg);
					}
					if (natoms_ > 2)
					{
						cfg.set(natoms_-3); // +...111
						S.insert(cfg);
					}
					if (natoms_ > 1)
					{
						cfg.clear(natoms_-2); // -...1x1
						S.insert(cfg);
					}
					break;
				case MValue13CNMR::DL:
					// ...011 = ...x11 - ...111
					// +...x11
					cfg.zeros();
					cfg.set(natoms_-2); cfg.set(natoms_-1);
					S.insert(cfg);
					// -...111
					if (natoms_ > 2) { cfg.set(natoms_-3); S.insert(cfg); }
					break;
				default:
					// unmögliche Kombination?!
					fASSERT_NONREACHABLE();
				}
			}
			else
			{
				// möglich:
				// Sp  := 010 = x1x - 11x - x11 + 111
				// DLp := 110 = 11x - 111
				// DRp := 011 = x11 - 111
				// DDp = Tp := 111
				switch (typelst_[p])
				{
				case MValue13CNMR::S:
					// ...010... = ...x1x... - ...11x... - ...x11... + ...111...
					cfg.zeros();
					cfg.set(poslst_[p]-1); // +...x1x...
					S.insert(cfg);
					cfg.set(poslst_[p]-2); // -...11x...
					S.insert(cfg);
					cfg.set(poslst_[p]);   // +...111...
					S.insert(cfg);
					cfg.clear(poslst_[p]-2); // -...x11...
					S.insert(cfg);
					break;
				case MValue13CNMR::DL:
					// ...110... = ...11x... - ...111...
					cfg.zeros();
					cfg.set(poslst_[p]-1); cfg.set(poslst_[p]-2); // +...11x...
					S.insert(cfg);
					cfg.set(poslst_[p]); // -...111...
					S.insert(cfg);
					break;
				case MValue13CNMR::DR:
					// ...011... = ...x11... - ...111...
					cfg.zeros();
					cfg.set(poslst_[p]-1); cfg.set(poslst_[p]); // +...x11...
					S.insert(cfg);
					cfg.set(poslst_[p]-2); // -...111...
					S.insert(cfg);
					break;
				case MValue13CNMR::DD:
				case MValue13CNMR::T:
					cfg.zeros();
					cfg.set(poslst_[p]-2); // ...1xx...
					cfg.set(poslst_[p]-1); // ...11x...
					cfg.set(poslst_[p]);   // ...111...
					S.insert(cfg);
					break;
				}
			}
		}
		break;
	}
	return S;
}

MValue const * MGroup13CNMR::getMValue(double ts, int pos, Type type) const
{
	std::map< double,std::map< std::pair< int,Type >,MValue* > >::const_iterator i;
	i = nmr13c_mvalue_map_.find(ts);
	if (i == nmr13c_mvalue_map_.end())
		// ts ist kein Timestamp
		return 0;
	std::map< std::pair< int,Type >,MValue* >::const_iterator j;
	j = i->second.find(std::pair< int,Type >(pos,type));
	if (j == i->second.end())
		// weight ist kein Gewicht
		return 0;
	return j->second;
}

bool MGroup13CNMR::getMValuesStdDev(
	double ts,
	MVector & x_meas,
	MVector & x_stddev
	) const
{
	if (not isTimeStamp(ts))
		return false;
	
	int i = 0;
	int const * p = poslst_;
	Type const * t = typelst_;

	if (x_meas.dim() != dim_)
		x_meas = MVector(dim_);
	if (x_stddev.dim() != dim_)
		x_stddev = MVector(dim_);
	
	while (*p != -1)
	{
		MValue const * mv = getMValue(ts,*p,*t);
		
		// Fehlende Messwerte sind nicht erlaubt!
		if (mv == 0)
			fTHROW(XMLException,
				"fatal: missing measurement values detected"
				);

		x_meas.set(i,mv->get());
		x_stddev.set(i,mv->getStdDev());
		p++; t++; i++;
	}
	return true;
}

void MGroup13CNMR::registerMValue(MValue const & mvalue)
{
	MValue13CNMR const & mv = static_cast< MValue13CNMR const & >(mvalue);

	// virtuellen Constructor von MValue aufrufen:
	if (not isTimeStamp(mv.getTimeStamp()))
		fTHROW(XMLException,"invalid timestamp [%f]",
			mv.getTimeStamp());
	if (getMValue(mv.getTimeStamp(),mv.getPos(),mv.getType()) != 0)
		fTHROW(XMLException,
			"duplicate 13C-NMR value [timestamp=%f, pos=%i, type=%i]",
			mv.getTimeStamp(),mv.getPos(),mv.getType());

	nmr13c_mvalue_map_[mv.getTimeStamp()][
		std::pair< int,Type >(mv.getPos(),mv.getType())
		] = mv.clone();
}

char const * MGroup13CNMR::getSpec(size_t idx) const
{
	fASSERT(idx < dim_);
	if (row_spec_[idx])
		return row_spec_[idx];

	char const * typestr = 0;
	size_t buf_len;
	char * buf;
	
	switch (typelst_[idx])
	{
	case MValue13CNMR::S:  typestr = "S"; break;
	case MValue13CNMR::DL: typestr = "DL"; break;
	case MValue13CNMR::DR: typestr = "DR"; break;
	case MValue13CNMR::DD: typestr = "DD"; break;
	case MValue13CNMR::T:  typestr = "T"; break;
	}
	buf_len = snprintf(0,0,"%s#%s%i",getMetaboliteName(),typestr,poslst_[idx]) + 1;
	buf = new char[buf_len];
	snprintf(buf,buf_len,"%s#%s%i",getMetaboliteName(),typestr,poslst_[idx]);
	row_spec_[idx] = buf;
	return buf;
}

uint32_t MGroup13CNMR::computeCheckSum(uint32_t crc, int crc_scope) const
{
	crc = MetaboliteMGroup::computeCheckSum(crc, crc_scope);
	if (crc_scope & CRC_CFG_MEAS_DATA)
	{
		std::map< double,std::map< std::pair< int,Type >,MValue* > >::const_iterator vi;
		std::map< std::pair< int,Type >,MValue* >::const_iterator vvi;
		for (vi=nmr13c_mvalue_map_.begin(); vi!=nmr13c_mvalue_map_.end(); ++vi)
			for (vvi=vi->second.begin(); vvi!=vi->second.end(); ++vvi)
				crc = vvi->second->computeCheckSum(crc,crc_scope);
	}
	return crc;
}

/*
 * --- MGroupCumomer ---
 */

MGroupCumomer::MGroupCumomer(MGroupCumomer const & copy)
	: MetaboliteMGroup(copy), xmask_(copy.xmask_), mask_(copy.mask_)
{
	std::map< double,MValue* >::const_iterator vi;
	for (vi=copy.mvalue_map_.begin(); vi!=copy.mvalue_map_.end(); vi++)
		mvalue_map_[vi->first] = vi->second->clone();
}

MGroupCumomer& MGroupCumomer::operator= (MGroupCumomer const & copy)
{
	MetaboliteMGroup::operator= (copy);
	xmask_ = copy.xmask_;
	mask_ = copy.mask_;
	std::map< double,MValue* >::const_iterator vi;
	for (vi=copy.mvalue_map_.begin(); vi!=copy.mvalue_map_.end(); vi++)
		mvalue_map_[vi->first] = vi->second->clone();
	return *this;
}

MGroupCumomer::~MGroupCumomer()
{
	std::map< double,MValue* >::iterator i;
	for (i=mvalue_map_.begin(); i!=mvalue_map_.end(); i++)
		delete i->second;
}

void MGroupCumomer::setMValuesStdDev(
	double ts,
	la::MVector const & x_meas,
	la::MVector const & x_stddev	
	)
{
	registerMValue(MValueSimple(group_id_,ts,x_stddev.get(0),x_meas.get(0)));
}

void MGroupCumomer::removeMValuesStdDev()
{
        // lösche Messwerte
        mvalue_map_.clear();
        // lösche Zeitpunkte
        this->ts_set_.clear();
}

MGroupCumomer * MGroupCumomer::parseSpec(char const * s, int * state)
{
	char * mname;
	BitArray xmask;
	BitArray mask;
	char * scpy = strdup_alloc(s);
	int st = data::Notation::parse_cumomer_spec(scpy,&mname,xmask,mask);
	delete[] scpy;
	if (state) *state = st;
	if (st != 0)
		return 0;
	return new MGroupCumomer(mname,xmask,mask,s);
}

bool MGroupCumomer::getMValuesStdDev(
	double ts,
	MVector & x_meas,
	MVector & x_stddev
	) const
{
	if (not isTimeStamp(ts))
		return false;
	
	if (x_meas.dim() != dim_)
		x_meas = MVector(dim_);
	if (x_stddev.dim() != dim_)
		x_stddev = MVector(dim_);

	MValue const * mv = getMValue(ts);
	
	// Fehlende Messwerte sind nicht erlaubt!
	fASSERT(mv != 0);
	if (mv == 0)
		fTHROW(XMLException,
			"fatal: missing measurement values detected"
			);

	x_meas.set(0,mv->get());
	x_stddev.set(0,mv->getStdDev());
	return true;
}

std::set< BitArray > MGroupCumomer::getSimSet(MetaboliteMGroup::SimDataType sdt) const
{
	std::set< BitArray > S;

	switch (sdt)
	{
	case sdt_isotopomer:
		// per Konvention:
		S.insert(getSimMask());
		break;
	case sdt_cumomer:
	case sdt_emu:
		{
		BitArray cfg(natoms_);
		BitArray zmask(~(mask_|xmask_));
		BitArray smask = mask_ | zmask;
		int i, n=(1<<smask.countOnes());
		// 0-Cumomer weglassen:
		cfg.incMask(smask);
		for (i=1; i<n; i++)
		{
			S.insert(cfg);
			cfg.incMask(smask);
		}
		}
		break;
	}
	return S;
}

// Kopie von SimpleMGroup::getMValue()
MValue const * MGroupCumomer::getMValue(double ts) const
{
	std::map< double,MValue* >::const_iterator i;
	i = mvalue_map_.find(ts);
	if (i == mvalue_map_.end())
		return 0;
	return i->second;
}

// Kopie von SimpleMGroup::registerMValue()
void MGroupCumomer::registerMValue(MValue const & mvalue)
{
	// virtuellen Constructor von MValue aufrufen:
	if (not isTimeStamp(mvalue.getTimeStamp()))
		fTHROW(XMLException,
			"invalid timestamp [%f]",
			mvalue.getTimeStamp()
			);
	mvalue_map_[mvalue.getTimeStamp()] = mvalue.clone();
}

uint32_t MGroupCumomer::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if (crc_scope & CRC_CFG_MEAS_MODEL)
	{
		crc = update_crc32(xmask_.toString(),xmask_.size(),crc);
		crc = update_crc32(mask_.toString(),mask_.size(),crc);
	}
	if (crc_scope & CRC_CFG_MEAS_DATA)
	{
		std::map< double,MValue* >::const_iterator vi;
		for (vi=mvalue_map_.begin(); vi!=mvalue_map_.end(); ++vi)
			crc = vi->second->computeCheckSum(crc, crc_scope);
	}
	return crc;
}

/*
 * --- MGroupGeneric ---
 */

MGroupGeneric::MGroupGeneric(MGroupGeneric const & copy)
	: MGroup(copy), row_mvalue_map_(0), E_(0), rows_(copy.rows_),
	  sub_groups_(0)
{
	row_mvalue_map_ = new std::map< double,MValue* >[rows_];
	E_ = new ExprTree*[rows_ + 1];
	E_[rows_] = 0;
	sub_groups_ = new charptr_map< MetaboliteMGroup* >[rows_];

	for (size_t r=0; r<rows_; r++)
	{
		std::map< double,MValue* >::const_iterator vi;
		for (vi=copy.row_mvalue_map_[r].begin(); vi!=copy.row_mvalue_map_[r].end(); vi++)
			row_mvalue_map_[r][vi->first] = vi->second->clone();
		E_[r] = copy.E_[r]->clone();
		charptr_map< MetaboliteMGroup* >::const_iterator gi;
		for (gi=copy.sub_groups_[r].begin(); gi!=copy.sub_groups_[r].end(); gi++)
			sub_groups_[r][gi->key] = gi->value->clone();
	}
}

MGroupGeneric& MGroupGeneric::operator= (MGroupGeneric const & copy)
{
	MGroup::operator= (copy);
	rows_ = copy.rows_;
	row_mvalue_map_ = new std::map< double,MValue* >[rows_];
	E_ = new ExprTree*[rows_ + 1];
	E_[rows_] = 0;
	sub_groups_ = new charptr_map< MetaboliteMGroup* >[rows_];

	for (size_t r=0; r<rows_; r++)
	{
		std::map< double,MValue* >::const_iterator vi;
		for (vi=copy.row_mvalue_map_[r].begin(); vi!=copy.row_mvalue_map_[r].end(); vi++)
			row_mvalue_map_[r][vi->first] = vi->second->clone();
		E_[r] = copy.E_[r]->clone();
		charptr_map< MetaboliteMGroup* >::const_iterator gi;
		for (gi=copy.sub_groups_[r].begin(); gi!=copy.sub_groups_[r].end(); gi++)
			sub_groups_[r][gi->key] = gi->value->clone();
	}
	return *this;
}
 
MGroupGeneric::MGroupGeneric(
	symb::ExprTree ** E,
	size_t rows,
	charptr_map< MetaboliteMGroup * > * sub_groups
	)
	: MGroup(mg_GENERIC,rows), E_(E),
	  rows_(rows), sub_groups_(sub_groups)
{
	row_mvalue_map_ = new std::map< double,MValue* >[rows_];
	charptr_array spec;
	for (size_t i=0; i<rows_; ++i)
	{
		row_spec_[i] = strdup_alloc(E_[i]->toString().c_str());
		spec.add(row_spec_[i]);
	}
	spec_ = strdup_alloc(spec.concat(";"));
}

MGroupGeneric::~MGroupGeneric()
{
	ExprTree ** e = E_;
	while (*e) { delete *e; e++; }
	delete[] E_;
	
	charptr_map< MetaboliteMGroup* >::iterator sgi;
	std::map< double,MValue* >::iterator i;
	for (size_t r=0; r<rows_; r++)
	{
		for (i=row_mvalue_map_[r].begin(); i!=row_mvalue_map_[r].end(); i++)
			delete i->second;
		for (sgi=sub_groups_[r].begin(); sgi!=sub_groups_[r].end(); ++sgi)
			delete sgi->value;
	}
	delete[] row_mvalue_map_;
	delete[] sub_groups_;
}

void MGroupGeneric::setMValuesStdDev(
	double ts,
	la::MVector const & x_meas,
	la::MVector const & x_stddev
	)
{
	for (size_t i=0; i<dim_; ++i)
		registerMValue(MValueGeneric(group_id_,ts,x_stddev.get(i),x_meas.get(i),i));
}


void MGroupGeneric::removeMValuesStdDev()
{
        // lösche Messwerte
        if(row_mvalue_map_) 
            delete row_mvalue_map_;
        // lösche Zeitpunkte
        this->ts_set_.clear();
}

MGroupGeneric * MGroupGeneric::parseSpec(
	ExprTree ** spec,
	int * state
	)
{
	size_t rows = 0;
	while (spec[rows] != 0)
		rows++;

	charptr_map< MetaboliteMGroup * > * sub_groups =
		new charptr_map< MetaboliteMGroup * >[rows];

	for (size_t r=0; r<rows; r++)
	{
		// Teile des Ausdrucks auf Gültigkeit / Konformität prüfen
		// (wirft XMLException bei Problemen)
		checkExpr(spec[r],sub_groups[r]);
	}

	return new MGroupGeneric(spec,rows,sub_groups);
}

MGroupGeneric * MGroupGeneric::parseSpec(
	char const * spec,
	int * state
	)
{
	charptr_array frms = charptr_array::split(spec,";");
	charptr_array::const_iterator fi;
	size_t r;
	ExprTree ** E = new ExprTree*[frms.size()+1];
	for (r=0,fi=frms.begin(); fi!=frms.end(); fi++,r++)
	{
		try
		{
			E[r] = ExprTree::parse(*fi,et_lex_mm);
		}
		catch (ExprParserException &)
		{
			if (state) *state = 1;
			for (int i=r-1; i>=0; i--)
				delete E[i];
			delete[] E;
			return 0;
		}
	}
	E[r] = 0;
	// Achtung: E darf nicht gelöscht werden!
	MGroupGeneric * G = parseSpec(E,state);
	return G;
}

void MGroupGeneric::checkExpr(
	ExprTree * R,
	charptr_map< MetaboliteMGroup * > & sub_groups
	)
{
	if (R == 0 or R->isLiteral())
		// nichts zu tun bei Literalen
		return;
	else if (R->isVariable())
	{
		// der Parser läßt nur syntaktisch richtige Kurz-
		// Notationen durch. Es folgt nun noch ein
		// Semantischer Check.
		bool valid;
		int vdim;
		char * vn = const_cast< char* >(R->getVarName());
		int det_type = data::Notation::check_spec(vn,&valid,&vdim);
		MGroupGenericType mgg_type;
		
		if (not valid)
			fTHROW(XMLException,"group spec is invalid [%s]", vn);
		
		if (det_type == -1)
			// das sollte eigentlich nicht passieren:
			fTHROW(XMLException,"group spec is not well-formed: [%s]", vn);

		if (vdim != 1)
			fTHROW(XMLException,"only subexpressions with dim=1 allowed in group spec: [%s,dim=%i]",
				vn, vdim);

		switch (det_type)
		{
		case 1: mgg_type = mgg_MS; break;
		case 2: mgg_type = mgg_MSMS; break;
		case 3: mgg_type = mgg_1HNMR; break;
		case 4: mgg_type = mgg_13CNMR; break;
		case 5: mgg_type = mgg_CUMOMER; break; // generische Notation aus 0,1,x
		case 6: mgg_type = mgg_MI_MS; break; // multi-isotopic Tracer MS
		default:
			// sollte nicht passieren:
			fTHROW(XMLException,"group spec has illegal type: [%s; determined type %i]",
				vn, det_type);
		}

		// Unter-Messgruppe anlegen, soweit sie noch nicht existiert:
		if (not sub_groups.find(R->getVarName()))
		{
			MetaboliteMGroup * mg;
			switch (mgg_type)
			{
			case mgg_MS:
				mg = MGroupMS::parseSpec(R->getVarName());
				break;
			case mgg_MSMS:
				mg = MGroupMSMS::parseSpec(R->getVarName());
				break;
			case mgg_1HNMR:
				mg = MGroup1HNMR::parseSpec(R->getVarName());
				break;
			case mgg_13CNMR:
				mg = MGroup13CNMR::parseSpec(R->getVarName());
				break;
			case mgg_CUMOMER:
				mg = MGroupCumomer::parseSpec(R->getVarName());
				break;
                        case mgg_MI_MS:
				mg = MGroupMIMS::parseSpec(R->getVarName());
				break;
			}
			// das Parsen sollte jetzt nicht mehr schiefgehen
			fASSERT(mg != 0);
			sub_groups.insert(R->getVarName(),mg);
		}
	}
	checkExpr(R->Lval(),sub_groups);
	checkExpr(R->Rval(),sub_groups);
}

charptr_map< BitArray > MGroupGeneric::getSimMasks() const
{
	charptr_map< BitArray > masks;
	charptr_map< MetaboliteMGroup * >::const_iterator mgi; 
	for (size_t r=0; r<rows_; r++)
	{
		for (mgi=sub_groups_[r].begin();
			mgi!=sub_groups_[r].end(); mgi++)
		{
			BitArray & gmask = masks[mgi->value->getMetaboliteName()];
			gmask = gmask | mgi->value->getSimMask();
		}
	}
	return masks;
}

void MGroupGeneric::setGroupId(char const * id)
{
	MGroup::setGroupId(id);
	charptr_map< MetaboliteMGroup * >::iterator sgi;
	for (size_t r=0; r<rows_; r++)
		for (sgi=sub_groups_[r].begin();
			sgi!=sub_groups_[r].end();
			sgi++) sgi->value->setGroupId(id);
}

void MGroupGeneric::setScaleAuto()
{
	MGroup::setScaleAuto();

	// automatische Skalierung auf Untergruppen abschalten
	charptr_map< MetaboliteMGroup* >::iterator sgi;
	for (size_t r=0; r<rows_; r++)
		for (sgi=sub_groups_[r].begin();
			sgi!=sub_groups_[r].end();
			sgi++) sgi->value->setUnscaled();
}

charptr_array MGroupGeneric::getVarNames(size_t row) const
{
	charptr_array vn;
	fASSERT( row < rows_ );
	if (row < rows_)
	{
		vn = E_[row]->getVarNames();
		vn.sort();
		return vn;
	}
	return vn;
}

charptr_array MGroupGeneric::getVarNames() const
{
	charptr_array vn;
	for (size_t r=0; r<rows_; r++)
		vn.addUnique(E_[r]->getVarNames());
	vn.sort();
	return vn;
}

charptr_array MGroupGeneric::getPoolNames(size_t row) const
{
	charptr_array pn, vn = getVarNames(row);
	charptr_array::const_iterator i;
	for (i=vn.begin(); i!=vn.end(); i++)
	{
		charptr_array p = charptr_array::split(*i,"[#");
		pn.add(p[0]);
	}
	return pn;
}

charptr_array MGroupGeneric::getPoolNames() const
{
	charptr_array pn;
	for (size_t r=0; r<rows_; r++)
		pn.addUnique(getPoolNames(r));
	return pn;
}

bool MGroupGeneric::getMValuesStdDev(
	double ts,
	MVector & x_meas,
	MVector & x_stddev
	) const
{
	if (not isTimeStamp(ts))
		return false;

	if (x_meas.dim() != dim_)
		x_meas = MVector(dim_);
	if (x_stddev.dim() != dim_)
		x_stddev = MVector(dim_);

	for (size_t r=0; r<rows_; r++)
	{
		MValue const * mv = getMValue(ts,r);
		// Fehlende Messwerte sind nicht erlaubt
		if (mv == 0)
			fTHROW(XMLException,
			"fatal: missing measurement values detected");
		
		x_meas.set(r,mv->get());
		x_stddev.set(r,mv->getStdDev());
	}
	return true;
}

MValue const * MGroupGeneric::getMValue(double ts, size_t row) const
{
	if (row >= rows_)
	{
		fASSERT(row < rows_);
		return 0;
	}
	std::map< double,MValue* >::const_iterator i;
	i = row_mvalue_map_[row].find(ts);
	if (i == row_mvalue_map_[row].end())
		return 0;
	return i->second;
}

void MGroupGeneric::registerMValue(MValue const & mvalue)
{
	MValueGeneric const & mv = static_cast< MValueGeneric const & >(mvalue);

	// virtuellen Constructor von MValue aufrufen:
	if (not isTimeStamp(mv.getTimeStamp()))
		fTHROW(XMLException,"invalid timestamp [%f]",
			mv.getTimeStamp());
	if (mv.getRow() >= (int)rows_)
		fTHROW(XMLException,"invalid row [%i of %i]",
			int(mv.getRow()),int(rows_));
	if (getMValue(mv.getTimeStamp(),mv.getRow()) != 0)
		fTHROW(XMLException,
			"duplicate generic measurement value [group=%s, timestamp=%f, row=%i]",
			getGroupId(),mv.getTimeStamp(),mv.getRow()+1);
	row_mvalue_map_[mv.getRow()][mv.getTimeStamp()] = mv.clone();
}

uint32_t MGroupGeneric::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if (crc_scope & CRC_CFG_MEAS_MODEL)
	{
		char const * expr = spec_;
		crc = update_crc32(expr,strlen(expr),crc);
	}

	if (crc_scope & CRC_CFG_MEAS_DATA)
	{
		std::map< double,MValue* >::const_iterator vi;
		for (size_t r=0; r<rows_; ++r)
			for (vi=row_mvalue_map_[r].begin(); vi!=row_mvalue_map_[r].end(); ++vi)
				crc = vi->second->computeCheckSum(crc,crc_scope);
	}
	return crc;
}

double MGroupFlux::evaluateRek(
	ExprTree * E,
	charptr_map< double > const & values
	) const
{
	double L=0., R=0.;
	double * fv;

	if (E->Lval())
		L = evaluateRek(E->Lval(),values);
	if (E->Rval())
		R = evaluateRek(E->Rval(),values);

	switch (E->getNodeType())
	{
	case symb::et_op_add:
		return L+R;
	case symb::et_op_sub:
		return L-R;
	case symb::et_op_uminus:
		return -L;
	case symb::et_op_mul:
		return L*R;
	case symb::et_op_div:
		return L/R;
	case symb::et_op_min:
		return L<=R?L:R;
	case symb::et_op_max:
		return L>=R?L:R;
	case symb::et_op_sqr:
		return L*L;
	case symb::et_op_abs:
		if (L<0) return -L;
		return L;
	case symb::et_literal:
		return E->getDoubleValue();
	case symb::et_variable:
		fv = values.findPtr(E->getVarName());
		if (fv == 0)
			fTHROW(XMLException,"Error: missing flux/pool value \"%s\"",
				E->getVarName());
		return *fv;
	case symb::et_op_pow:
		return ::pow(L,R);
	case symb::et_op_sqrt:
		return ::sqrt(L);
	case symb::et_op_log:
		return ::log(L);
	case symb::et_op_log2:
		return ::log(L)/::log(2.);
	case symb::et_op_log10:
		return ::log10(L);
	case symb::et_op_exp:
		return ::exp(L);
	case symb::et_op_eq:
	case symb::et_op_neq:
	case symb::et_op_leq:
	case symb::et_op_geq:
	case symb::et_op_lt:
	case symb::et_op_gt:
	case symb::et_op_diff:
	case symb::et_op_sin:
	case symb::et_op_cos:
		fTHROW(XMLException,"Error: invalid operator (%s) found flux measurement formula",
			E->nodeToString().c_str());
	}

	return evaluateRek(E,values);
}

double MGroupFlux::devaluate(
	symb::ExprTree * dexpr,
	charptr_map< double > const & values
	) const
{
	return evaluateRek(dexpr,values);
}

uint32_t MGroupFlux::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if (crc_scope & CRC_CFG_MEAS_MODEL)
	{
		charptr_array::const_iterator fni;
		for (fni=fluxes_.begin(); fni!=fluxes_.end(); ++fni)
			crc = update_crc32(*fni,strlen(*fni),crc);
	}
	return crc;
}

double MGroupPool::evaluateRek(
	ExprTree * E,
	charptr_map< double > const & values
	) const
{
	double L=0., R=0.;
	double * fv;

	if (E->Lval())
		L = evaluateRek(E->Lval(),values);
	if (E->Rval())
		R = evaluateRek(E->Rval(),values);

	switch (E->getNodeType())
	{
	case symb::et_op_add:
		return L+R;
	case symb::et_op_sub:
		return L-R;
	case symb::et_op_uminus:
		return -L;
	case symb::et_op_mul:
		return L*R;
	case symb::et_op_div:
		return L/R;
	case symb::et_op_min:
		return L<=R?L:R;
	case symb::et_op_max:
		return L>=R?L:R;
	case symb::et_op_sqr:
		return L*L;
	case symb::et_op_abs:
		if (L<0) return -L;
		return L;
	case symb::et_literal:
		return E->getDoubleValue();
	case symb::et_variable:
		fv = values.findPtr(E->getVarName());
		if (fv == 0)
			fTHROW(XMLException,"Error: missing flux/pool value \"%s\"",
				E->getVarName());
		return *fv;
	case symb::et_op_pow:
		return ::pow(L,R);
	case symb::et_op_sqrt:
		return ::sqrt(L);
	case symb::et_op_log:
		return ::log(L);
	case symb::et_op_log2:
		return ::log(L)/::log(2.);
	case symb::et_op_log10:
		return ::log10(L);
	case symb::et_op_exp:
		return ::exp(L);
	case symb::et_op_eq:
	case symb::et_op_neq:
	case symb::et_op_leq:
	case symb::et_op_geq:
	case symb::et_op_lt:
	case symb::et_op_gt:
	case symb::et_op_diff:
	case symb::et_op_sin:
	case symb::et_op_cos:
		fTHROW(XMLException,"Error: invalid operator (%s) found flux measurement formula",
			E->nodeToString().c_str());
	}

	return evaluateRek(E,values);
}

double MGroupPool::devaluate(
	symb::ExprTree * dexpr,
	charptr_map< double > const & values
	) const
{
	return evaluateRek(dexpr,values);
}

uint32_t MGroupPool::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if (crc_scope & CRC_CFG_MEAS_MODEL)
	{
		charptr_array::const_iterator pni;
		for (pni=pools_.begin(); pni!=pools_.end(); ++pni)
			crc = update_crc32(*pni,strlen(*pni),crc);
	}
	return crc;
}

} // namespace xml
} // namespace flux

