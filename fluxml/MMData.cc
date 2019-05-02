#include <cstdio>
// für strptime
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <ctime>
#include <cerrno>
#include "Error.h"
#include "XMLElement.h"
#include "MMUnicodeConstants.h"
#include "UnicodeTools.h"
#include "MMDocument.h"
#include "MMData.h"
#include "MGroup.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

namespace flux {
namespace xml {

void MMData::parseInfo(DOMNode * info)
{
	DOMNode * child, * childchild;
	struct tm start_ts, finish_ts;
	char * end_ptr;

	if (not XMLElement::match(info,mm_dlabel,mm_xmlns_uri))
		fTHROW(XMLException,"element \"dlabel\" expected");
	child = XMLElement::skipJunkNodes(info->getFirstChild());

	if (XMLElement::match(child,mm_start,mm_xmlns_uri))
	{
		childchild = XMLElement::skipJunkNodes(child->getFirstChild());
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child,"TEXT_NODE with start timestamp expected");
		U2A asc_start_ts(static_cast< DOMText* >(childchild)->getData());
		end_ptr = strptime(asc_start_ts, "%Y-%m-%d %H:%M:%S", &start_ts);
		if (end_ptr == 0)
			fTHROW(XMLException,
					childchild,
					"error parsing timestamp (start) [%s]",
					(char const*)asc_start_ts
					);
		if (*end_ptr != '\0')
			fTHROW(XMLException,
					childchild,
					"trailing garbage in timestamp (start) [%s]",
					end_ptr
					);
		child = XMLElement::nextNode(child);
	}

	if (XMLElement::match(child,mm_finish,mm_xmlns_uri))
	{
		childchild = XMLElement::skipJunkNodes(child->getFirstChild());
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child,"TEXT_NODE with finish timestamp expected");
		U2A asc_finish_ts(static_cast< DOMText* >(childchild)->getData());
		end_ptr = strptime(asc_finish_ts, "%Y-%m-%d %H:%M:%S", &finish_ts);
		if (end_ptr == 0)
			fTHROW(XMLException,
					childchild,
					"error parsing timestamp (finish) [%s]",
					(char const*)asc_finish_ts
					);
		if (*end_ptr != '\0')
			fTHROW(XMLException,
					childchild,
					"trailing garbage in timestamp (finish): [%s]",
					end_ptr
					);
		child = XMLElement::nextNode(child);
	}

	if (XMLElement::match(child,mm_people,mm_xmlns_uri))
	{
		childchild = XMLElement::skipJunkNodes(child->getFirstChild());
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child,"TEXT_NODE with people-info expected");
		U2A asc_people(static_cast< DOMText* >(childchild)->getData());
		// TODO : asc_people
		child = XMLElement::nextNode(child);
	}
        
	if (XMLElement::match(child,mm_strain,mm_xmlns_uri))
	{
		childchild =  XMLElement::skipJunkNodes(child->getFirstChild());
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child,"TEXT_NODE with strain-info expected");
		U2A asc_strain(static_cast< DOMText* >(childchild)->getData());
		// TODO: asc_strain
		child = XMLElement::nextNode(child);
	}

	if (XMLElement::match(child,mm_comment,mm_xmlns_uri))
	{
		childchild =  XMLElement::skipJunkNodes(child->getFirstChild());
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child,"TEXT_NODE with comment expected");
		U2A asc_comment(static_cast< DOMText* >(childchild)->getData());
		// TODO: asc_comment
	}
        
        if (XMLElement::match(child,mm_experiment,mm_xmlns_uri))
	{
                childchild = XMLElement::skipJunkNodes(child->getFirstChild());
                if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
                        fTHROW(XMLException,child,"TEXT_NODE with experiment-info expected");
                U2A asc_experiment(static_cast< DOMText* >(childchild)->getData());
                child = XMLElement::nextNode(child);
	}
        if (XMLElement::match(child,mm_analytics,mm_xmlns_uri))
	{
                childchild = XMLElement::skipJunkNodes(child->getFirstChild());
                if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
                        fTHROW(XMLException,child,"TEXT_NODE with analytics-info expected");
                U2A asc_analytics(static_cast< DOMText* >(childchild)->getData());
                child = XMLElement::nextNode(child);
	}
        
        if (XMLElement::match(child,mm_analysis,mm_xmlns_uri))
	{
                childchild = XMLElement::skipJunkNodes(child->getFirstChild());
                if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
                        fTHROW(XMLException,child,"TEXT_NODE with analysis-info expected");
                U2A asc_analysis(static_cast< DOMText* >(childchild)->getData());
                child = XMLElement::nextNode(child);
	}

}

void MMData::parse(DOMNode * data)
{
	DOMNode * child, * childchild;
	DOMNamedNodeMap * nnm;
	DOMAttr * idAttr, * timeAttr, * weightAttr, * posAttr,
		* typeAttr, * stddevAttr, * rowAttr;
	char * end_ptr;
	double ts, stddev, mvalue;
	int weight, weight1, weight2, pos, type, row;
        std::vector<int> weight_vec;
	MGroup * MG;
	MGroup::MGroupType MGtype;

	if (not XMLElement::match(data,mm_data,mm_xmlns_uri))
		fTHROW(XMLException,"element \"data\" expected.");
	child = XMLElement::skipJunkNodes(data->getFirstChild());

	if (XMLElement::match(child,mm_dlabel,mm_xmlns_uri))
	{
		parseInfo(child);
		child = XMLElement::nextNode(child);
	}

	if (child == 0)
		fTHROW(XMLException,
			data,
			"at least a single \"datum\" element is expected"
			);

	do
	{
		idAttr = timeAttr = weightAttr = posAttr
			= typeAttr = stddevAttr = 0;
		ts = stddev = -1.;
		weight = weight1 = weight2 = pos = type = row = -1;
                weight_vec.clear();
                
		if (not XMLElement::match(child,mm_datum,mm_xmlns_uri))
			fTHROW(XMLException,
				child,
				"element \"datum\" expected"
				);

		nnm = child->getAttributes();
		if (nnm == 0)
			fTHROW(XMLException,
				child,
				"element \"datum\" lacks required attributes"
				);

		// erforderliche Attibs sind id,time,stddev
		//
		// implied Attribs sind weight (MS,MSMS), pos (1H-NMR,13C-NMR),
		// type (13C-NMR), group (GENERIC)
		idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_id));
		timeAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_time));
		stddevAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_stddev));
		rowAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_row));

		if (idAttr == 0)
			fTHROW(XMLException,child,
				"\"datum\" element lacks required attribute (id)");
		U2A asc_id(idAttr->getValue());

		// Die Gruppen-Id auslesen und prüfen
		MG = doc_->getGroupByName((char const *)asc_id);
		if (MG == 0)
			fTHROW(XMLException,child,
				"id=\"%s\" is not a valid group id",
				(char const*)asc_id);
		MGtype = MG->getType();

		// Bei GENERIC die richtige Untergruppe heraussuchen
		if (MGtype == MGroup::mg_GENERIC)
		{
			if (rowAttr == 0)
				fTHROW(XMLException,child,
				"\"datum\" element lacks required attribute (row) (generic measurement group)");

			if (not XMLElement::parseInt(rowAttr->getValue(),row))
				fTHROW(XMLException,child,
				"error parsing integer value out of element \"datum\"/attribute \"row\" (generic measurement group))");

			if (row < 1)
				fTHROW(XMLException,child,
				"invalid integer value for attribute \"row\" of element \"datum\" (generic measurement group)");

			if (row > int(static_cast< MGroupGeneric* >(MG)->getNumRows()))
				fTHROW(XMLException,child,
				"cannot select row %i out of %i rows in attribute \"row\" of element \"datum\" (generic measurement group)",
				row, int(static_cast< MGroupGeneric* >(MG)->getNumRows()));

			// Indizierung beginnt intern bei 0
			row--;
		}
                if ((not doc_->isStationary()) and (MGtype != MGroup::mg_FLUX) and (MGtype != MGroup::mg_POOL))
		{
			// Den timestamp auslesen und prüfen
			if (timeAttr == 0)
				fTHROW(XMLException,child,
				"\"datum\" element lacks required attribute (time) (non-stationary measurement)");

			U2A asc_time(timeAttr->getValue());
			ts = strtod((char const*)asc_time,&end_ptr);
			if (end_ptr == asc_time)
				fTHROW(XMLException,child,
					"attribute \"time\": error parsing value [%s]",
					(char const*)asc_time);
			if (ts < 0.)
				fTHROW(XMLException,child,
					"attribute \"time\": negative timestamps are not allowed [%g]",
					ts);
			if (std::isnan(ts))
				fTHROW(XMLException,child,
					"attribute \"time\": timestamp NaN is not allowed [%g]",
					ts);
			if (not MG->isTimeStamp(ts))
				fTHROW(XMLException,child,
					"attribute \"time\": timestamp [%s] is undefined",
					(char const*)asc_time);
		}
		else
		{
			// Stationärer Eintrag; timestamp nicht erlaubt
			if (timeAttr != 0)
				fTHROW(XMLException,child,
				"element \"datum\": timestamps are forbidden for stationary measurements");
			ts = -1.; // Eintrag für stationär
		}

		// Die Standardabweichung auslesen und prüfen
		if (stddevAttr == 0)
			fTHROW(XMLException,child,
				"\"datum\" element lacks required attribute (stddev)");
		U2A asc_stddev(stddevAttr->getValue());
		stddev = strtod((char const*)asc_stddev,&end_ptr);
		if (end_ptr == asc_stddev)
			fTHROW(XMLException,child,
				"attribute \"stddev\": error parsing value [%s]",
				(char const*)asc_stddev);
		if (stddev <= 0.)
			fTHROW(XMLException,child,
				"attribute \"stddev\": negative or zero standard deviation not allowed [%g]",
				stddev);
		if (std::isinf(stddev) or std::isnan(stddev))
			fTHROW(XMLException,child,
				"attribute \"stddev\": illegal value for standard deviation [%g]",
				stddev);

		weightAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_weight));
		posAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_pos));
		typeAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_type));	

		if (weightAttr)
		{
			// nur bei MS / MSMS / MIMS erlaubt:
			if (MGtype != MGroup::mg_MS && MGtype != MGroup::mg_MSMS && MGtype != MGroup::mg_MIMS)
				fTHROW(XMLException,child,
					"attribute \"weight\" not allowed for this group type");

			if (MGtype == MGroup::mg_MS)
			{
				// MS-Messung
				U2A asc_weight(weightAttr->getValue());
				errno = 0;
				weight = strtol((char const*)asc_weight,&end_ptr,10);
				if (end_ptr == asc_weight)
					fTHROW(XMLException,child,
						"attribute \"weight\": error parsing value [%s]",
						(char const*)asc_weight);
				if (weight < 0)
					fTHROW(XMLException,child,
						"attribute \"weight\": negative weight not allowed [%i]",
						weight);
			}
			else if (MGtype == MGroup::mg_MSMS)
			{
				// MS-MS-Messung
				U2A asc_weight12(weightAttr->getValue());
				charptr_array weight12 = charptr_array::split(asc_weight12,", ");
				if (weight12.size() != 2)
					fTHROW(XMLException,child,
						"attribute \"weight\": expecting two weight values, found [%s]",
						(char const*)asc_weight12);
				errno = 0;
				weight1 = strtol(weight12[0],&end_ptr,10);
				if (end_ptr == weight12[0])
					fTHROW(XMLException,child,
						"attribute \"weight\": error parsing value [%s]",
						(char const*)weight12[0]);
				if (weight1 < 0)
					fTHROW(XMLException,child,
						"attribute \"weight\": negative weight not allowed [%i]",
						weight1);

				errno = 0;
				weight2 = strtol(weight12[1],&end_ptr,10);
				if (end_ptr == weight12[1])
					fTHROW(XMLException,child,
						"attribute \"weight\": error parsing value [%s]",
						(char const*)weight12[1]);
				if (weight2 < 0)
					fTHROW(XMLException,child,
						"attribute \"weight\": negative weight not allowed [%i]",
						weight2);
			}
                        else // MIMS (multi-isotopic Tracer)
                        {
				U2A asc_weights(weightAttr->getValue());
                                
				charptr_array weights = charptr_array::split(asc_weights,",");
                                size_t size = static_cast< MGroupMIMS* >(MG)->getNumWeights();
				if (weights.size() != size)
					fTHROW(XMLException,child,
						"attribute \"weight\": expecting %i weight values, found [%s]",
						int(size), (char const*)asc_weights);
                                    
                                errno = 0;
                                for(size_t k=0; k< weights.size(); k++)
                                {
                                    
                                    int w = strtol(weights[k],&end_ptr,10);
                                    if (end_ptr == weights[k])
                                            fTHROW(XMLException,child,
                                                    "attribute \"weight\": error parsing value [%s]",
                                                    (char const*)asc_weights);
                                    if (w < 0)
                                            fTHROW(XMLException,child,
                                                    "attribute \"weight\": negative weight not allowed [%i]",
                                                    w);
                                    weight_vec.push_back(w);
                                }
                                
                                if (weight_vec.size() != size)
					fTHROW(XMLException,child,
						"attribute \"weight\": expecting %i weight values, found [%s]",
						int(size), (char const*)asc_weights);
                        }

			// ist weight in der Liste der spezifizierten Gewichte?
			if (MGtype == MGroup::mg_MS)
			{
				MGroupMS & MGms = static_cast< MGroupMS & >(*MG);
				int const * warray = MGms.getWeights();
				while (*warray != -1)
				{
					if (*warray == weight)
						break;
					warray++;
				}
				if (*warray == -1)
					fTHROW(XMLException,child,
						"attribute \"weight\": invalid weight [%i]",
						weight);
			}
			else if (MGtype == MGroup::mg_MSMS)
			{
				MGroupMSMS & MGmsms = static_cast< MGroupMSMS & >(*MG);
				int const * warray = MGmsms.getWeights1();
				while (*warray != -1)
				{
					if (*warray == weight1)
						break;
					warray++;
				}
				if (*warray == -1)
					fTHROW(XMLException,child,
						"attribute \"weight\": invalid weight #1 [%i]",
						weight1);
				warray = MGmsms.getWeights2();
				while (*warray != -1)
				{
					if (*warray == weight2)
						break;
					warray++;
				}
				if (*warray == -1)
					fTHROW(XMLException,child,
						"attribute \"weight\": invalid weight #2 [%i]",
						weight2);
			}
                        else // MI-MS type
                        {
                            // Prüfe, ob die Gewichte schon registriert sind
                            MGroupMIMS & MGmims = static_cast< MGroupMIMS & >(*MG);
                            std::vector<int* > const & weight_vec_ = MGmims.getWeights();
                            std::vector<int* >::const_iterator wi;
                            int k,l,dim = MGmims.getDim();
                            bool found;
                            for (k=0; k<dim; ++k)
                            {
                                found=true;
                                for(wi=weight_vec_.begin(),l=0;wi!=weight_vec_.end();wi++,l++)
                                {
                                    if((*wi)[k] != weight_vec[l])
                                        found=false;
                                }
                                if(found) break;
                            }
                            if (found==false)
                            {
                                U2A asc_weights(weightAttr->getValue());
                                fTHROW(XMLException,child,
                                            "attribute \"weight\": invalid weight [%s]",
                                            (const char *)asc_weights);
                            }
                        }
		}

		if (typeAttr)
		{
			// nur bei 13C-NMR erlaubt:
			if (MGtype != MGroup::mg_13CNMR)
				fTHROW(XMLException,child,
					"attribute \"type\" not allowed for this group type");

			// type := S->1, DL->2, DR->3, DD->4, T->5
			U2A asc_type(typeAttr->getValue());
			if (asc_type[0] == 'S' && asc_type[1] == '\0')
				type = 1;
			else if (asc_type[0] == 'D')
			{
				if (asc_type[1] == 'L' && asc_type[2] == '\0')
					type = 2;
				else if (asc_type[1] == 'R' && asc_type[2] == '\0')
					type = 3;
				else if (asc_type[1] == 'D' && asc_type[2] == '\0')
					type = 4;
			}
			else if (asc_type[0] == 'T' && asc_type[1] == '\0')
				type = 5;
			
			if (type == -1)
				fTHROW(XMLException,
					child,
					"unknown 13C-NMR type: [%s]",
					(char const *)asc_type
					);
		}

		if (posAttr)
		{
			// 1H-NMR / 13C-NMR Messung

			// nur bei 1H-NMR / 13C-NMR erlaubt:
			if (MGtype != MGroup::mg_1HNMR && MGtype != MGroup::mg_13CNMR)
				fTHROW(XMLException,child,
					"attribute \"pos\" not allowed for this group type");

			U2A asc_pos(posAttr->getValue());
			errno = 0;
			pos = strtol((char const*)asc_pos,&end_ptr,10);
			if (end_ptr == asc_pos)
				fTHROW(XMLException,child,
					"attribute \"pos\": error parsing value [%s]",
					(char const*)asc_pos);
			if (pos <= 0)
				fTHROW(XMLException,child,
					"attribute \"pos\": zero or negative values are not allowed");
			
			int const * parray;
			if (MGtype == MGroup::mg_1HNMR)
				parray = static_cast< MGroup1HNMR* >(MG)->getPositions();
			else
				parray = static_cast< MGroup13CNMR* >(MG)->getPositions();
			
			while (*parray != -1)
			{
				if (*parray == pos)
					break;
				parray++;
			}
			if (*parray == -1)
				fTHROW(XMLException,child,
					"attribute \"pos\": invalid position [%i]",
					pos);

			if (MGtype == MGroup::mg_13CNMR)
			{
				if (typeAttr == 0)
					fTHROW(XMLException,child,
						"missing attribute \"type\" (13C-NMR measurement)");
				// gibt es einen Eintrag in der Typen-Liste für (pos,type)?
				parray = static_cast< MGroup13CNMR* >(MG)->getPositions();
				MGroup13CNMR::Type const * tarray
					= static_cast< MGroup13CNMR* >(MG)->getNMRTypes();
				while (*parray != -1)
				{
					if (*parray == pos && *tarray == type)
						break;
					parray++;
					tarray++;
				}
				if (*parray == -1)
					fTHROW(XMLException,child,
						"attributes \"pos\", \"type\": invalid NMR type for position");
			}
		}

#define ONEOF3(a,b,c) (((a) && !(b) && !(c)) \
		|| (!(a) && (b) && !(c)) \
		|| (!(a) && !(b) && (c)))

#define ONEOF4(a,b,c,d) (((a) && !(b) && !(c) && !(d)) \
		|| (!(a) && (b) && !(c) && !(d)) \
		|| (!(a) && !(b) && (c) && !(d)) \
		|| (!(a) && !(b) && !(c) && (d)))

		// Sinnhaftigkeit der Parameterzusammenstellung prüfen!
		if (MGtype != MGroup::mg_FLUX and MGtype != MGroup::mg_POOL
			and MGtype != MGroup::mg_GENERIC)
		{
			if (((not ONEOF3(weight!=-1,(weight1!=-1 && weight2!=-1),pos!=-1)) &&  weight_vec.empty())
				|| (MGtype != MGroup::mg_13CNMR && type!=-1))
				fTHROW(XMLException,
					child,
					"datum (id=\"%s\"): nonsensical combination of attributes (weight,pos,type,row)",
					(char const*)asc_id
					);
		}

		// der eigentliche Messwert:
		childchild = XMLElement::skipJunkNodes(child->getFirstChild());
		if (childchild == 0 || childchild->getNodeType() != DOMNode::TEXT_NODE)
			fTHROW(XMLException,child,"TEXT_NODE with measurement value expected");

		U2A asc_mvalue(static_cast< DOMText* >(childchild)->getData());
		mvalue = strtod((char const*)asc_mvalue,&end_ptr);
		if (end_ptr == asc_mvalue)
			fTHROW(XMLException,child,
				"error parsing measurement value [%s]",
				(char const*)asc_mvalue);
		if (mvalue < 0. and not
			(MGtype == MGroup::mg_GENERIC
			 or (MGtype == MGroup::mg_FLUX and dynamic_cast< MGroupFlux* >(MG)->isNet())))
		{
			fTHROW(XMLException,child,
				"negative measurement values are not allowed [%s]",
				(char const*)asc_mvalue);
		}
		if (std::isinf(mvalue))
		{
			fTHROW(XMLException,child,
				"measurement values NaN are not allowed [%g]",
				mvalue);
		}
		
		// id, ts, stddev, [ weight | pos | type ] eintragen. Die Eintragung kann
		// auch schiefgehen, falls der Timestamp ungültig ist. Das wird aber oben
		// abgefangen (der Rückgabewert wird hier ignoriert).

		switch (MGtype)
		{
		case MGroup::mg_MS:
			MG->registerMValue(MValueMS(asc_id,ts,stddev,mvalue,weight));
			break;
                case MGroup::mg_MIMS:
			MG->registerMValue(MValueMIMS(asc_id,ts,stddev,mvalue,weight_vec));
			break;
		case MGroup::mg_MSMS:
			MG->registerMValue(MValueMSMS(asc_id,ts,stddev,mvalue,weight1,weight2));
			break;
		case MGroup::mg_1HNMR:
			MG->registerMValue(MValue1HNMR(asc_id,ts,stddev,mvalue,pos));
			break;
		case MGroup::mg_13CNMR:
			MG->registerMValue(MValue13CNMR(asc_id,ts,stddev,mvalue,pos,type));
			break;
		case MGroup::mg_GENERIC:
			MG->registerMValue(MValueGeneric(asc_id,ts,stddev,mvalue,row));
			break;
		case MGroup::mg_FLUX:
			MG->registerMValue(MValueFlux(asc_id,ts,stddev,mvalue));
			break;
		case MGroup::mg_POOL:
			MG->registerMValue(MValuePool(asc_id,ts,stddev,mvalue));
			break;
		case MGroup::mg_CUMOMER:
			// Kann hier nicht vorkommen
			fASSERT_NONREACHABLE();
			//MG->registerMValue(MValueCumomer(asc_id,ts,stddev,mvalue));
			break;
		}

		child = XMLElement::nextNode(child);
	}
	while (child != 0);
}

} // namespace flux::xml
} // namespace flux

