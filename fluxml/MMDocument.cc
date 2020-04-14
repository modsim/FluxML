#include <cstring>
#include <ctime>
#include <vector>
#include "Error.h"
#include "Combinations.h"
#include "ExprTree.h"
#include "XMLElement.h"
#include "XMLException.h"
#include "MMModel.h"
#include "MMData.h"
#include "MMUnicodeConstants.h"
#include "MGroup.h"
#include "MMDocument.h"
#include "FluxMLUnicodeConstants.h"

#include "UnicodeTools.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

using namespace flux::symb;

namespace flux {
namespace xml {

void MMDocument::parse(DOMNode * measurement)
{
	DOMNode * child;
	
	// parse() wird hier für >>> simulation/model <<< bei type="explicit"
	// missbraucht:
	if (XMLElement::match(measurement,mm_model,mm_xmlns_uri))
	{
		MMModel model(this);
		model.parse(measurement);
		// dummy-Messwerte anlegen
		charptr_map< MGroup* >::iterator gi;
		for (gi=mgroup_map_.begin(); gi!=mgroup_map_.end(); ++gi)
		{
			MGroup * G = gi->value;

			// dummy-Messwerte werden auf 0 gesetzt.
			// Standardabweichung wird auf 1 gesetzt
			// ==> alle normen
			la::MVector x_stddev(G->getDim());
			la::MVector x_meas(G->getDim());
			x_stddev.fill(1);
                        
                        if (!is_stationary_)
			{
				switch (G->getType())
				{
					case xml::MGroup::mg_MS:
					case xml::MGroup::mg_MSMS:
					case xml::MGroup::mg_1HNMR:
					case xml::MGroup::mg_13CNMR:
					case xml::MGroup::mg_CUMOMER:
					case xml::MGroup::mg_MIMS:
					case xml::MGroup::mg_GENERIC:
						{
							std::set< double > ts_set = G->getTimeStampSet();
							std::set< double >::iterator ts_set_it;

							for( ts_set_it = ts_set.begin(); ts_set_it!=ts_set.end(); ++ts_set_it )
							{
								G->setMValuesStdDev(*ts_set_it,x_meas,x_stddev);
							};
						}
						break;
					case xml::MGroup::mg_FLUX:
					case xml::MGroup::mg_POOL:
						{
							G->setMValuesStdDev(-1,x_meas,x_stddev);
						}
						break;
				}

			}
			else
                        {
				G->setMValuesStdDev(-1,x_meas,x_stddev);
                        }
			
		}
                return;
	}
        
	if (not XMLElement::match(measurement,mm_measurement,mm_xmlns_uri))
		fTHROW(XMLException,"element node \"measurement\" expected.");
	child = XMLElement::skipJunkNodes(measurement->getFirstChild());

	if (XMLElement::match(child,mm_mlabel,mm_xmlns_uri))
	{            
		parseInfos(child);
		child = XMLElement::nextNode(child);
	}

	if (not XMLElement::match(child,mm_model,mm_xmlns_uri))
		fTHROW(XMLException,child, "element node \"model\" expected");

	MMModel model(this);
	model.parse(child);
	child = XMLElement::nextNode(child);

	if (not XMLElement::match(child,mm_data,mm_xmlns_uri))
		fTHROW(XMLException,child, "element node \"data\" expected");

	MMData data(this);
	data.parse(child);
	child = XMLElement::nextNode(child);
	checkValues(child);
        
	// es dürfen keine Elemente mehr folgen!
	fASSERT(child == 0);
}

void MMDocument::parseInfos(DOMNode * infos)
{
	char * end_ptr;
	DOMNode * child, * childchild;

	child = XMLElement::skipJunkNodes(infos->getFirstChild());
	
	// date, version, comment, fluxunit, poolsizeunit, timeunit
	if (XMLElement::match(child,mm_date,mm_xmlns_uri))
	{
		childchild = child->getFirstChild();
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child, "text node expected");
		U2A asc_date(static_cast< DOMText* >(childchild)->getData());
		end_ptr = strptime(asc_date, "%Y-%m-%d %H:%M:%S", &mm_ts_);
		if (end_ptr == 0)
			fTHROW(XMLException,
				child,
				"error parsing timestamp (date) [%s]",
				(char const*)asc_date
				);
		if (*end_ptr != '\0')
			fTHROW(XMLException,
				child,
				"trailing garbage in timestamp (date) [%s]",
				(char const*)end_ptr
				);
		child = XMLElement::nextNode(child);
	}
	
	if (XMLElement::match(child,mm_version,mm_xmlns_uri))
	{
		childchild = child->getFirstChild();
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child, "text node expected");
		U2A asc_version(static_cast< DOMText* >(childchild)->getData());
		mm_version_ = strdup_alloc(asc_version);
		child = XMLElement::nextNode(child);
	}

	if (XMLElement::match(child,mm_comment,mm_xmlns_uri))
	{
		childchild = child->getFirstChild();
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child, "text node expected");
		U2A asc_comment(static_cast< DOMText* >(childchild)->getData());
		mm_comment_ = strdup_alloc(asc_comment);
		child = XMLElement::nextNode(child);
	}

	if (XMLElement::match(child,mm_fluxunit,mm_xmlns_uri))
	{
		childchild = child->getFirstChild();
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child, "text node expected");
                char * str_fluxunit (XMLString::transcode (static_cast< DOMText* >(childchild)->getData()));
                mm_fluxunit_ = strdup_alloc(str_fluxunit);
                XMLString::release (&str_fluxunit);
		child = XMLElement::nextNode(child);
	}

	if (XMLElement::match(child,mm_poolsizeunit,mm_xmlns_uri))
	{
		childchild = child->getFirstChild();
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child, "text node expected");

                char * str_psunit (XMLString::transcode (static_cast< DOMText* >(childchild)->getData()));
                mm_psunit_ = strdup_alloc(str_psunit);
                XMLString::release (&str_psunit);
		child = XMLElement::nextNode(child);
	}

        if (XMLElement::match(child,mm_timeunit,mm_xmlns_uri))
	{
		childchild = child->getFirstChild();
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child, "text node expected");

		char * str_timeunit (XMLString::transcode (static_cast< DOMText* >(childchild)->getData()));
                mm_timeunit_ = strdup_alloc(str_timeunit);
                XMLString::release (&str_timeunit);
		child = XMLElement::nextNode(child);
	}
	// es dürfen keine Elemente mehr folgen!
	fASSERT(child == 0);
}

MMDocument::MMDocument(DOMDocument * doc, bool is_stationary)
	: ts_ptr_(0), is_stationary_(is_stationary), mm_version_(0),
	  mm_comment_(0), mm_fluxunit_(0), mm_psunit_(0), mm_timeunit_(0)
{
	memset(&mm_ts_,0,sizeof(struct tm));
	DOMNode * measurement = doc->getDocumentElement();
	if (not XMLElement::match(measurement,mm_measurement,mm_xmlns_uri))
		fTHROW(XMLException,measurement, "element node \"measurement\" expected");
	parse(measurement);
}

MMDocument::MMDocument(DOMNode * measurement, bool is_stationary)
	: ts_ptr_(0), is_stationary_(is_stationary), mm_version_(0),
	  mm_comment_(0), mm_fluxunit_(0), mm_psunit_(0), mm_timeunit_(0)
{
	parse(measurement);
}

MMDocument::MMDocument(MMDocument const & copy)
	: ts_set_(copy.ts_set_), ts_ptr_(0),
	  is_stationary_(copy.is_stationary_),
	  mm_ts_(copy.mm_ts_),
	  mm_version_(0), mm_comment_(0),
	  mm_fluxunit_(0), mm_psunit_(0), 
          mm_timeunit_(0)
{
	charptr_map< MGroup* >::const_iterator mgi;
	for (mgi=copy.mgroup_map_.begin(); mgi!=copy.mgroup_map_.end(); mgi++)
		mgroup_map_.insert(mgi->key, mgi->value->clone());

	if (copy.mm_version_)
		mm_version_ = strdup_alloc(copy.mm_version_);
	if (copy.mm_comment_)
		mm_comment_ = strdup_alloc(copy.mm_comment_);
	if (copy.mm_fluxunit_)
		mm_fluxunit_ = strdup_alloc(copy.mm_fluxunit_);
	if (copy.mm_psunit_)
		mm_psunit_ = strdup_alloc(copy.mm_psunit_);
        if (copy.mm_timeunit_)
		mm_timeunit_ = strdup_alloc(copy.mm_timeunit_);
}

MMDocument::~MMDocument()
{
	if (ts_ptr_ != 0)
		delete[] ts_ptr_;
	if (mm_psunit_ != 0)
		delete[] mm_psunit_;
	if (mm_fluxunit_ != 0)
		delete[] mm_fluxunit_;
	if (mm_timeunit_ != 0)
		delete[] mm_timeunit_;
	if (mm_comment_ != 0)
		delete[] mm_comment_;
	if (mm_version_ != 0)
		delete[] mm_version_;

	charptr_map< MGroup* >::iterator i;
	for (i=mgroup_map_.begin(); i!=mgroup_map_.end(); i++)
		delete i->value;
}

MGroup * MMDocument::getGroupByName(char const * id)
{
	MGroup ** gptr = mgroup_map_.findPtr(id);
	if (gptr) return *gptr;
	return 0;
}

double const * MMDocument::getTimeStamps(size_t & size)
{
	std::set< double >::const_iterator ii;
	int i;

	if (ts_ptr_)
	{
		delete[] ts_ptr_;
		ts_ptr_ = 0;
	}
	size = ts_set_.size();
	if (size == 0)
		return 0;
	ts_ptr_ = new double[size];
	for (ii=ts_set_.begin(),i=0; ii!=ts_set_.end(); ii++,i++)
		ts_ptr_[i] = *ii;
	return ts_ptr_;
}

void MMDocument::registerGroup(DOMNode * gnode, MGroup * G)
{
	if (mgroup_map_.find(G->getGroupId()) != mgroup_map_.end())
		fTHROW(XMLException,
			gnode,"group id [%s] is not unique",
			G->getGroupId());
	mgroup_map_.insert(G->getGroupId(),G);

	// timestamps der Messgrupppe in globale timestamp-Menge übernehmen
	std::set< double > const & g_ts_set = G->getTimeStampSet();
	std::insert_iterator< std::set< double > > sIter(ts_set_,ts_set_.begin());
	std::copy(g_ts_set.begin(), g_ts_set.end(), sIter);
}

bool MMDocument::updateGroup(MGroup const & G)
{
	charptr_map< MGroup* >::iterator it = mgroup_map_.find(G.getGroupId());
	if (it == mgroup_map_.end())
	{
		return false;
	}
	delete it->value;
	it->value = G.clone();
	return true;
}

void MMDocument::checkValues(DOMNode * data)
{
	// Gibt es für jeden Timestamp einen Messwert?
	charptr_map< MGroup* >::const_iterator gi;
	for (gi = mgroup_map_.begin(); gi != mgroup_map_.end(); gi++)
	{
		MGroup * G = gi->value;
		std::set< double > ts_set = G->getTimeStampSet();
		std::set< double >::const_iterator si;

		if (is_stationary_)
		{
			ts_set.clear();
			ts_set.insert(-1);
		}

		for (si = ts_set.begin(); si != ts_set.end(); si++)
		{
			switch (G->getType())
			{
			case MGroup::mg_MS:
				{
				MGroupMS const & G_ = static_cast< MGroupMS & >(*G);
				int const * weights = G_.getWeights();
				int w;
				for (w=0; weights[w] != -1; w++)
					if (G_.getMValue(*si,weights[w]) == 0)
						fTHROW(XMLException,data,
							"MS measurement value missing: group=%s timestamp=%g, weight=%i",
							G_.getGroupId(),*si,weights[w]);
				}
				break;
			case MGroup::mg_MIMS:
				{
				MGroupMIMS const & G_ = static_cast< MGroupMIMS & >(*G);
                                std::vector<int *>::const_iterator wi;
				std::vector<int *> const & weights_vec = G_.getWeights();
                                size_t dim = G_.getDim();
                                for (size_t i=0; i<dim; ++i)
                                {
                                    std::vector<int> weigths;
                                    for(wi=weights_vec.begin();wi!=weights_vec.end();wi++)
                                            weigths.push_back((*wi)[i]);

                                    if (G_.getMValue(*si,weigths) == 0)
                                    {
                                        std::ostringstream ostr;
                                        for(wi=weights_vec.begin();wi!=weights_vec.end();wi++)
                                            ostr<< (wi!=weights_vec.begin()?",":"") <<(*wi)[i]; 
                                        fTHROW(XMLException,data,
                                                "MIMS measurement value missing: group=%s timestamp=%g, weight=(%s)",
                                                G_.getGroupId(),*si,ostr.str().c_str());
                                    }
                                }
                                }
				break;
			case MGroup::mg_MSMS:
				{
				MGroupMSMS const & G_ = static_cast< MGroupMSMS & >(*G);
				int const * weights1 = G_.getWeights1();
				int const * weights2 = G_.getWeights2();
				int w;
				for (w=0; weights1[w] != -1; w++)
					if (G_.getMValue(*si,weights1[w],weights2[w]) == 0)
						fTHROW(XMLException,data,
							"MS-MS measurement value missing: group=%s timestamp=%g, weight=(%i,%i)",
							G_.getGroupId(),*si,weights1[w],weights2[w]);
				}
				break;
			case MGroup::mg_1HNMR:
				{
				MGroup1HNMR const & G_ = static_cast< MGroup1HNMR &>(*G);
				int const * posns = G_.getPositions();
				int p;
				for (p=0; posns[p] != -1; p++)
					if (G_.getMValue(*si,posns[p]) == 0)
						fTHROW(XMLException,data,
							"1H-NMR measurement value missing: group=%s timestamp=%g, position=%i",
							G_.getGroupId(),*si,posns[p]);
				}
				break;
			case MGroup::mg_13CNMR:
				{
				MGroup13CNMR const & G_ = static_cast< MGroup13CNMR &>(*G);
				int const * posns = G_.getPositions();
				MGroup13CNMR::Type const * types = G_.getNMRTypes();
				int p;
				for (p=0; posns[p] != -1; p++)
					if (G_.getMValue(*si,posns[p],types[p])==0)
						fTHROW(XMLException,data,
							"13C-NMR measurement value missing: group=%s timestamp=%g, position=%i, type=%i",
							G_.getGroupId(),*si,posns[p],types[p]);
				}
				break;
			case MGroup::mg_GENERIC:
				{
				MGroupGeneric const & G_ = static_cast< MGroupGeneric &>(*G);
				for (size_t r=0; r<G_.getNumRows(); r++)
					if (G_.getMValue(*si,r) == 0)
						fTHROW(XMLException,data,
							"Generic measurement value missing: group=%s timestamp=%g, row=%i",
							G_.getGroupId(),*si,int(r));
				}
				break;
			case MGroup::mg_FLUX:
				{
				MGroupFlux const & G_ = static_cast< MGroupFlux & >(*G);
				if (G_.getMValue(*si) == 0)
					fTHROW(XMLException,data,
						"Flux measurement value missing: group=%s timestamp=%g",
						G_.getGroupId(),*si);
				}
				break;
			case MGroup::mg_POOL:
				{
				MGroupPool const & G_ = static_cast< MGroupPool & >(*G);
				if (G_.getMValue(*si) == 0)
					fTHROW(XMLException,data,
						"Pool size measurement value missing: group=%s timestamp=%g",
						G_.getGroupId(),*si);
				}
				break;
			default:
				fASSERT_NONREACHABLE();
			}
		} // alle Timestamps
	} // alle Messgruppen
}

void MMDocument::validate_MGroupMS(
	MGroupMS * G,
	charptr_map< int > const & pools,
	charptr_array const & reactions
	) const
{
	int const * weights = G->getWeights();
	int w, *atomsPtr;

	if ((atomsPtr = pools.findPtr(G->getMetaboliteName())) == 0)
		fTHROW(XMLException,"MS measurement group \"%s\": unknown pool name \"%s\"",
			G->getGroupId(), G->getMetaboliteName()
			);

	// Dem Messgruppenobjekt die Anzahl der Atome mitteilen
	G->setNumAtoms(*atomsPtr);

	for (w=0; weights[w] != -1; w++)
		if (weights[w] > *atomsPtr or *atomsPtr < 1)
			fTHROW(XMLException,
				"MS measurement group \"%s\": "
				"weight %i exceeds number of atoms of pool \"%s\"",
				G->getGroupId(), weights[w], G->getMetaboliteName()
				);
}

void MMDocument::validate_MGroupMIMS(
	MGroupMIMS * G,
	charptr_map< int > const & pools,
	charptr_array const & reactions
	) const
{
	int *atomsPtr;
	if ((atomsPtr = pools.findPtr(G->getMetaboliteName())) == 0)
		fTHROW(XMLException,"MIMS measurement group \"%s\": unknown pool name \"%s\"",
			G->getGroupId(), G->getMetaboliteName()
			);
        
	// Dem Messgruppenobjekt die Anzahl der Atome mitteilen
	G->setNumAtoms(*atomsPtr);
        
	std::vector<int *>::const_iterator wi;
        charptr_map< int >::const_iterator ic;
        std::vector<int *> const & weights_vec = G->getWeights();
        charptr_map< int > const & iso_cfg = G->getIsotopesCfg();
        size_t dim = G->getDim();
        
        for (size_t i=0; i<dim; ++i)
        {
            for(wi=weights_vec.begin(), ic=iso_cfg.begin();
                    wi!=weights_vec.end(); wi++,ic++)
            {
                if (((*wi)[i] > ic->value) or *atomsPtr < 1)
                {
                    std::ostringstream ostr;
                    for(wi=weights_vec.begin();wi!=weights_vec.end();wi++)
                        ostr<< (wi!=weights_vec.begin()?",":"") <<(*wi)[i]; 

                    fTHROW(XMLException,
                                "MIMS measurement group \"%s\": "
                                "weight (%s) exceeds number of atoms of pool \"%s\"",
                                G->getGroupId(), ostr.str().c_str(), G->getMetaboliteName()
                                );
                }
            }
        }
}

void MMDocument::validate_MGroupMSMS(
	MGroupMSMS * G,
	charptr_map< int > const & pools,
	charptr_array const & reactions
	) const
{
	int const * weights1 = G->getWeights1();
	int w, *atomsPtr;

	if ((atomsPtr = pools.findPtr(G->getMetaboliteName())) == 0)
		fTHROW(XMLException,"MSMS measurement group \"%s\": unknown pool name \"%s\"",
			G->getGroupId(), G->getMetaboliteName()
			);
	
	// Dem Messgruppenobjekt die Anzahl der Atome mitteilen
	G->setNumAtoms(*atomsPtr);

	for (w=0; weights1[w] != -1; w++)
		if (weights1[w] > *atomsPtr or *atomsPtr < 1)
			fTHROW(XMLException,
				"MSMS measurement group \"%s\": "
				"weight %i exceeds number of atoms of pool \"%s\"",
				G->getGroupId(), weights1[w], G->getMetaboliteName()
				);
}

void MMDocument::validate_MGroup1HNMR(
	MGroup1HNMR * G,
	charptr_map< int > const & pools,
	charptr_array const & reactions
	) const
{
	int const * posns = G->getPositions();
	int p, *atomsPtr;

	if ((atomsPtr = pools.findPtr(G->getMetaboliteName())) == 0)
		fTHROW(XMLException,"1H-NMR measurement group \"%s\": unknown pool name \"%s\"",
			G->getGroupId(), G->getMetaboliteName()
			);
	
	// Dem Messgruppenobjekt die Anzahl der Atome mitteilen
	G->setNumAtoms(*atomsPtr);

	for (p=0; posns[p] != -1; p++)
		if (posns[p] > *atomsPtr)
			fTHROW(XMLException,
				"1H-NMR measurement group \"%s\": "
				"position %i does not exist in pool \"%s\"",
				G->getGroupId(), posns[p], G->getMetaboliteName()
				);
}

void MMDocument::validate_MGroup13CNMR(
	MGroup13CNMR * G,
	charptr_map< int > const & pools,
	charptr_array const & reactions
	) const
{
	int const * posns = G->getPositions();
	MValue13CNMR::NMR13CType const * typelst = G->getNMRTypes();
	int p, *atomsPtr;

	if ((atomsPtr = pools.findPtr(G->getMetaboliteName())) == 0)
		fTHROW(XMLException,"13C-NMR measurement group \"%s\": unknown pool name \"%s\"",
			G->getGroupId(), G->getMetaboliteName()
			);
	
	// Dem Messgruppenobjekt die Anzahl der Atome mitteilen
	G->setNumAtoms(*atomsPtr);

	for (p=0; posns[p] != -1; p++)
	{
		if (posns[p] > *atomsPtr)
			fTHROW(XMLException,
				"13C-NMR measurement group \"%s\": "
				"position %i does not exist in pool \"%s\"",
				G->getGroupId(), posns[p], G->getMetaboliteName()
				);
		switch (typelst[p])
		{
		case MValue13CNMR::S:
			break;
		case MValue13CNMR::DL:
			if (posns[p] <= 1)
				fTHROW(XMLException,
					"13C-NMR measurement group \"%s\": "
					"no left doublet on position 1 allowed",
					G->getGroupId()
					);
			break;
		case MValue13CNMR::DR:
			if (posns[p] >= *atomsPtr)
				fTHROW(XMLException,
					"13C-NMR measurement group \"%s\": "
					"no right doublet on position %i allowed",
					G->getGroupId(), posns[p]
					);
			break;
		case MValue13CNMR::DD:
		case MValue13CNMR::T:
			if (posns[p] <= 1 or posns[p] >= *atomsPtr)
				fTHROW(XMLException,
					"13C-NMR measurement group \"%s\": "
					"no triplet/double doublet on position %i allowed",
					G->getGroupId(), posns[p]
					);
		}
	}
}

void MMDocument::validate(
	charptr_map< int > const & pools,
	charptr_array const & reactions
	) const
{
	charptr_map< MGroup* >::const_iterator gi;

	for (gi=mgroup_map_.begin(); gi!=mgroup_map_.end(); gi++)
	{
		switch (gi->value->getType())
		{
		case MGroup::mg_MS:
			{
			MGroupMS * G = static_cast< MGroupMS* >(gi->value);
			validate_MGroupMS(G,pools,reactions);
			}
			break;
		case MGroup::mg_MIMS:
			{
			MGroupMIMS * G = static_cast< MGroupMIMS* >(gi->value);
			validate_MGroupMIMS(G,pools,reactions);    
			}
			break;
		case MGroup::mg_MSMS:
			{
			MGroupMSMS * G = static_cast< MGroupMSMS* >(gi->value);
			validate_MGroupMSMS(G,pools,reactions);
			}
			break;
		case MGroup::mg_1HNMR:
			{
			MGroup1HNMR * G = static_cast< MGroup1HNMR* >(gi->value);
			validate_MGroup1HNMR(G,pools,reactions);
			}
			break;
		case MGroup::mg_13CNMR:
			{
			MGroup13CNMR * G = static_cast< MGroup13CNMR* >(gi->value);
			validate_MGroup13CNMR(G,pools,reactions);
			}
			break;
		case MGroup::mg_GENERIC:
			{
			MGroupGeneric * G = static_cast< MGroupGeneric* >(gi->value);
			charptr_array vn = G->getVarNames();
			charptr_array pn = G->getPoolNames();
			charptr_array::const_iterator i;
			for (i=pn.begin(); i!=pn.end(); i++)
				if (pools.findPtr(*i) == 0)
					fTHROW(XMLException,
						"generic measurement group \"%s\": "
						"unknown pool name \"%s\"",
						G->getGroupId(), *i);

			for (size_t r=0; r<G->getNumRows(); r++)
			{
				vn = G->getVarNames(r);
				for (i=vn.begin(); i!=vn.end(); i++)
				{
					MGroup * SG = G->getSubGroup(*i,r);
					fASSERT( SG != 0 );
					switch (SG->getType())
					{
					case MGroup::mg_MS:
						{
						MGroupMS * g = static_cast< MGroupMS * >(SG);
						validate_MGroupMS(g,pools,reactions);
						}
						break;
					case MGroup::mg_MSMS:
						{
						MGroupMSMS * g = static_cast< MGroupMSMS * >(SG);
						validate_MGroupMSMS(g,pools,reactions);
						}
						break;
					case MGroup::mg_1HNMR:
						{
						MGroup1HNMR * g = static_cast< MGroup1HNMR * >(SG);
						validate_MGroup1HNMR(g,pools,reactions);
						}
						break;
					case MGroup::mg_13CNMR:
						{
						MGroup13CNMR * g = static_cast< MGroup13CNMR * >(SG);
						validate_MGroup13CNMR(g,pools,reactions);
						}
						break;
					case MGroup::mg_MIMS:
						{
						MGroupMIMS * g = static_cast< MGroupMIMS * >(SG);
						validate_MGroupMIMS(g,pools,reactions);
						}
						break;
					case MGroup::mg_CUMOMER:
						{
						// verallgemeinerte Cumomer-Notation
						MGroupCumomer * g = static_cast< MGroupCumomer* >(SG);
						charptr_array ps = charptr_array::split(*i,"#");
						fASSERT( ps.size() == 2 );
						int * atomsPtr;
						if ((atomsPtr = pools.findPtr(ps[0])) == 0)
							fTHROW(XMLException,
							"generic measurement group \"%s\": "
							"unknown pool name \"%s\"",
							G->getGroupId(), ps[0]);

						if (*atomsPtr != (int)strlen(ps[1]))
							fTHROW(XMLException,
							"generic measurement group \"%s\": "
							"mismatch in atom numbers (%i vs. %i)",
							G->getGroupId(), (int)(strlen(ps[1])),
							*atomsPtr);
						g->setNumAtoms(*atomsPtr);
						}
						break;
					case MGroup::mg_GENERIC:
					case MGroup::mg_FLUX:
					case MGroup::mg_POOL:
						// sollte nicht passieren:
						fTHROW(XMLException,"invalid group type");
					} // switch
				} // Gruppennamen Zeile r
			} // alle Zeilen

			}
			break;
		case MGroup::mg_FLUX:
			{
			MGroupFlux * G = static_cast< MGroupFlux* >(gi->value);
			charptr_array const & fluxes = G->getFluxes();
			charptr_array::const_iterator i;
			for (i=fluxes.begin(); i!=fluxes.end(); i++)
				if (reactions.find(*i) == reactions.end())
					fTHROW(XMLException,"flux measurement \"%s\": "
						"unknown reaction name \"%s\"",
						G->getGroupId(), *i);
			}
			break;
		case MGroup::mg_POOL:
			{
			MGroupPool * G = static_cast< MGroupPool* >(gi->value);
			charptr_array const & p = G->getPools();
			charptr_array::const_iterator i;
			for (i=p.begin(); i!=p.end(); i++)
				if (pools.findPtr(*i) == 0)
					fTHROW(XMLException,"pool measurement \"%s\": "
						"unknown pool name \"%s\"",
						G->getGroupId(), *i);
			}
			break;
		case MGroup::mg_CUMOMER:
			// sollte hier nicht vorkommen; nur intern
			fASSERT_NONREACHABLE();
			break;
		}
	}
}

uint32_t MMDocument::computeCheckSum(uint32_t crc, int crc_scope) const
{
	if (crc_scope & (CRC_CFG_MEAS_MODEL|CRC_CFG_MEAS_DATA))
	{
		charptr_array mg_keys = mgroup_map_.getKeys();
		charptr_array::const_iterator mgki;
		mg_keys.sort();
		for (mgki=mg_keys.begin(); mgki!=mg_keys.end(); ++mgki)
		{
			MGroup ** ptr = mgroup_map_.findPtr(*mgki);
			fASSERT(ptr != 0);
			crc = (*ptr)->computeCheckSum(crc, crc_scope);
		}
	}

	crc = update_crc32(&is_stationary_,sizeof(is_stationary_),crc);

	// infos-Header
	if (crc_scope & CRC_ALL_ANNOTATIONS)
	{
		if (mm_version_)
			crc = update_crc32(mm_version_,strlen(mm_version_),crc);
		if (mm_comment_)
			crc = update_crc32(mm_comment_,strlen(mm_comment_),crc);
		if (mm_fluxunit_)
			crc = update_crc32(mm_fluxunit_,strlen(mm_fluxunit_),crc);
		if (mm_psunit_)
			crc = update_crc32(mm_psunit_,strlen(mm_psunit_),crc);
                if (mm_timeunit_)
			crc = update_crc32(mm_timeunit_,strlen(mm_timeunit_),crc);
	}
	return crc;
}

    double MMDocument::getMeasurementTimeEnd()
    {
            if(is_stationary_)
                return -1;
            
            std::set< double >::reverse_iterator ts_set_iterator;
            ts_set_iterator = ts_set_.rbegin();

            if(ts_set_iterator == ts_set_.rend())
                    return 0;
            
            return *ts_set_iterator;		
    }




} // namespace flux::xml
} // namespace flux

