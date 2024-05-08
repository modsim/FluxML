#include <list>
#include <string>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "BitArray.h"

#include "InputPool.h"
#include "Configuration.h"

#include "UnicodeTools.h"
#include "XMLException.h"
#include "XMLElement.h"

#include "MMDocument.h"
#include "MMUnicodeConstants.h"

#include "FluxMLInput.h"
#include "FluxMLUnicodeConstants.h"
#include "FluxMLConfiguration.h"
#include "FluxMLConstraints.h"
#include "FluxMLDocument.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

namespace flux {
namespace xml {

void FluxMLConfiguration::parseConfiguration(DOMNode * node)
{
	DOMNode * child, * childchild;
	DOMNamedNodeMap * nnm;
        DOMAttr * nameAttr, * stationaryAttr;
	std::string comment;
	data::Configuration * cfg;
	bool is_stationary = true;
	int input_cnt = 0;

	if (not XMLElement::match(node,fml_configuration,fml_xmlns_uri))
		fTHROW(XMLException,node,
			"element node \"configuration\" expected.");

	// Parsen des name-Attributes des configuration-Elements
	nnm = node->getAttributes();
	if (nnm == 0)
		fTHROW(XMLException,node,
			"element configuration lacks attribute \"name\".");
	nameAttr = (DOMAttr*)(nnm->getNamedItem(fml_name));
	if (nameAttr == 0)
		fTHROW(XMLException,node,
			"element configuration lacks attribute \"name\".");
	stationaryAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_stationary));
	if (stationaryAttr == 0 or XMLString::equals(stationaryAttr->getValue(),fml_true))
		is_stationary = true;
	else if (XMLString::equals(stationaryAttr->getValue(),fml_false))
		is_stationary = false;
	else
		fTHROW(XMLException,node,
			"attribute \"stationary\": invalid value");

	U2A u2a_name(nameAttr->getValue());

	child = XMLElement::skipJunkNodes(node->getFirstChild());

	if (child == 0)
		fTHROW(XMLException,node,"cfg \"%s\": element node (comment|input) expected.",
			(char const *)u2a_name);

	if (XMLElement::match(child,fml_comment,fml_xmlns_uri))
	{
		childchild = XMLElement::skipJunkNodes(child->getFirstChild());
		if (childchild != 0)
		{
			if (childchild->getNodeType() != DOMNode::TEXT_NODE)
				fTHROW(XMLException,childchild,"cfg \"%s\"/comment: #PCDATA element expected.",
					(char const *)u2a_name);
			U2A u2a_comment = static_cast< DOMText* >(childchild)->getData();
			comment = u2a_comment;
		}
		// zum nächsten Element-Knoten vorrücken
		child = XMLElement::nextNode(child);
	}
	
	if (child == 0)
		fTHROW(XMLException,node,"cfg \"%s\": element node (input) expected.",
			(char const *)u2a_name);

	cfg = doc_->createConfiguration(u2a_name,comment.c_str());
	cfg->setStationary(is_stationary);
	do
	{
		// es muß jetzt mindestens ein input-Element folgen
		if (not XMLElement::match(child,fml_input,fml_xmlns_uri))
		{
			if (input_cnt == 0)
				fTHROW(XMLException,child,"cfg \"%s\": element node (input) expected.",
					(char const *)u2a_name);
			else break;
		}
		
		// das input-Element parsen:
		FluxMLInput input(doc_, child);
		if (input.getInputPool())
		{
			cfg->addInputPool( input.getInputPool() );
			input_cnt++;
		}
		
		// zum nächsten Element-Knoten vorrücken
		child = XMLElement::nextNode(child);
	}
	while (child != 0);
        
        // ab hier sind alle Pools identifiziert und klazifiziert (input, inner, extra)
        // hier müssen noch alle Poolgrößen mit natoms == 0  (sprich die Cofaktoren)
        // identifiziert und gelabelt werden 
        charptr_map< data::Pool* > & PMap = *(doc_->getPoolMap());
	charptr_map< data::Pool* >::iterator pmi;
        std::list< data::InputPool* > const & inputs= cfg->getInputPools();
        std::list< data::InputPool* >::const_iterator ip;
        bool exist;
        for(pmi= PMap.begin(); pmi!= PMap.end();++pmi)
        {
            if(pmi->value->getNumAtoms()==0)
            {
                exist=false;
                for(ip= inputs.begin(); ip!= inputs.end();++ip)
                {
                    if(strcmp((*ip)->getName(),pmi->key)==0)
                    {
                        exist=true;
                        break;
                    }
                }
                if(!exist)
                {
                    symb::ExprTree * eq = symb::ExprTree::eq(symb::ExprTree::sym(pmi->key),symb::ExprTree::val(0.0));
                    cfg->createConstraint("cofactor" ,eq, data::POOL);
                }
            }
        }

	// danach folgt einzelnes fluxes-Element:
	if (XMLElement::match(child,fml_constraints,fml_xmlns_uri))
	{
		// Constraints bleiben lokal auf die Konfiguration beschränkt
		// und werden unter dem Namen der Konfiguration abgelegt:
		FluxMLConstraints(doc_,child,u2a_name);
		child = XMLElement::nextNode(child);
	}

	if (XMLElement::match(child,fml_measurement,fml_xmlns_uri))
	{
		// Messmodellparser einbinden
		// hier wird im Falle eines Fehlers eine XMLException geworfen ...
		MMDocument * mmdoc = 0;
		try
		{
			mmdoc = new MMDocument(child,is_stationary);

			charptr_map< int > pools;
			charptr_array reactions;

			std::list< data::IsoReaction* > const * R
				= doc_->getReactions();
			std::list< data::IsoReaction* >::const_iterator li;
			for (li=R->begin(); li!=R->end(); li++)
				reactions.add((*li)->getName());

			charptr_map< data::Pool* > const * P
				= doc_->getPoolMap();
			charptr_map< data::Pool* >::const_iterator mi;

			for (mi=P->begin(); mi!=P->end(); mi++)
				pools[mi->value->getName()] = mi->value->getNumAtoms();

                        // Übertragung der Isotope Konfiguration bei Multi-Isotopic Tracer in die 
                        // MIMS-Messgruppen
                        xml::MGroup* ptr_mg;
                        charptr_array::const_iterator mgi;
                        charptr_array mgroup_names = mmdoc->getGroupNames();
                        
                        for (mgi=mgroup_names.begin();
                             mgi != mgroup_names.end(); mgi++)
                        {
                            ptr_mg = mmdoc->getGroupByName(*mgi);
                            if((ptr_mg->getType() != xml::MGroup::mg_MIMS) || ptr_mg==0)
                                continue;
                        
                           charptr_map< data::Pool* >::const_iterator ip= 
                                   P->find(((xml::MetaboliteMGroup*)ptr_mg)->getMetaboliteName());
                           
                           if(ip != P->end())
                               ptr_mg->setIsotopesCfg(ip->value->getIsotopesCfg());
                        }
			mmdoc->validate(pools,reactions);
		}
		catch (XMLException & e)
		{
			fTHROW(XMLException,child,"cfg \"%s\": error parsing measurement element: [%s]",
				(char const *)u2a_name, (char const*)e);
		}
		cfg->linkMMDocument(mmdoc);

		child = XMLElement::nextNode(child);
	}
        
	if (XMLElement::match(child,fml_simulation,fml_xmlns_uri))
	{
		parseSimulation(cfg,child);
		child = XMLElement::nextNode(child);
	}
	if (child != 0)
		fTHROW(XMLException,child,"cfg \"%s\": extra data in configuration element?!",
			(char const *)u2a_name);
}

void FluxMLConfiguration::parseSimulation(
	data::Configuration * cfg,
	DOMNode * simulation
	)
{
	DOMAttr * typeAttr;
	DOMNode * child;
	DOMNamedNodeMap * nnm;
	char const * cfg_name = cfg->getName();

	nnm = simulation->getAttributes();
	if (nnm == 0)
		fTHROW(XMLException,simulation,
			"cfg \"%s\": element \"simulation\" lacks attributes",
			cfg_name);

	typeAttr = (DOMAttr*)(nnm->getNamedItem(fml_type));

	if (typeAttr == 0 or XMLString::equals(typeAttr->getValue(),fml_auto))
		cfg->setSimAuto();
	else if (XMLString::equals(typeAttr->getValue(),fml_explicit))
		cfg->setSimExplicit();
	else if (XMLString::equals(typeAttr->getValue(),fml_full))
		cfg->setSimFull();
	else
		fTHROW(XMLException,simulation,
			"cfg \"%s\": illegal value for simulation attribute \"type\"",
			cfg_name);
	
        
//	if (methodAttr == 0 or XMLString::equals(methodAttr->getValue(),fml_auto))
//	{
//		switch (cfg->determineBestSimMethod())
//		{
//		case data::Configuration::simm_EMU:
//			cfg->setSimMethodEMU();
//			break;
//		case data::Configuration::simm_Cumomer:
//			cfg->setSimMethodCumomer();
//			break;
//		}
//	}
//	else if (XMLString::equals(methodAttr->getValue(),fml_cumomer))
//		cfg->setSimMethodCumomer();
//	else if (XMLString::equals(methodAttr->getValue(),fml_emu))
//		cfg->setSimMethodEMU();
//	else
//		fTHROW(XMLException,simulation,
//			"cfg \"%s\": illegal value for attribute \"method\"",
//			cfg_name);
//        if(!cfg->isStationary())
			cfg->setSimMethodCumomer();

	child = XMLElement::skipJunkNodes(simulation->getFirstChild());

	// Element "objects" muss nicht zwingend für type="subset" erscheinen.
	// Netzwerkreduktion ist auch nur dem Substrat möglich.
	if (XMLElement::match(child,mm_model,mm_xmlns_uri))
	{
		// Element "model" muss für type="auto"|"full"
		// weggelassen werden
		if (cfg->getSimType() == data::Configuration::simt_auto
			or cfg->getSimType() == data::Configuration::simt_full)
			fTHROW(XMLException,child,"cfg \"%s\": a \"full\" or \"auto\""
				" simulation must not specifiy \"model\"",
				cfg_name);

		if (cfg->getMMDocument() != 0)
			fTHROW(XMLException,child,"cfg \"%s\": when using simulation type \"explicit\""
				" the configuration must not include measurement specifications",
				cfg_name);

		parseModel(cfg,child);
		child = XMLElement::nextNode(child);
	}

	if (XMLElement::match(child,fml_variables,fml_xmlns_uri))
	{
		parseVariables(cfg,child);
		child = XMLElement::nextNode(child);
	}

	if (child != 0)
		fTHROW(XMLException,simulation,
			"cfg %s: extra data in \"simulation\" element?!",
			cfg_name);
}

void FluxMLConfiguration::parseModel(
	data::Configuration * cfg,
	DOMNode * model
	)
{
	MMDocument * mmdoc = 0;
	try
	{
		mmdoc = new MMDocument(model,cfg->isStationary());

		charptr_map< int > pools;
		charptr_array reactions;

		std::list< data::IsoReaction* > const * R
			= doc_->getReactions();
		std::list< data::IsoReaction* >::const_iterator li;
		for (li=R->begin(); li!=R->end(); li++)
			reactions.add((*li)->getName());

		charptr_map< data::Pool* > const * P
			= doc_->getPoolMap();
		charptr_map< data::Pool* >::const_iterator mi;

		for (mi=P->begin(); mi!=P->end(); mi++)
			pools[mi->value->getName()] = mi->value->getNumAtoms();

                // Übertragung der Isotope Konfiguration bei Multi-Isotopic Tracer in die 
                // MIMS-Messgruppen
                xml::MGroup* ptr_mg;
                charptr_array::const_iterator mgi;
                charptr_array mgroup_names = mmdoc->getGroupNames();

                for (mgi=mgroup_names.begin();
                     mgi != mgroup_names.end(); mgi++)
                {
                    ptr_mg = mmdoc->getGroupByName(*mgi);
                    if((ptr_mg->getType() != xml::MGroup::mg_MIMS) || ptr_mg==0)
                        continue;

                   charptr_map< data::Pool* >::const_iterator ip= 
                           P->find(((xml::MetaboliteMGroup*)ptr_mg)->getMetaboliteName());

                   if(ip != P->end())
                       ptr_mg->setIsotopesCfg(ip->value->getIsotopesCfg());
                }
                        
		mmdoc->validate(pools,reactions);

		// dummy-Messwerte eintragen
	}
	catch (XMLException & e)
	{
		fTHROW(XMLException,model,"cfg \"%s\": error parsing measurement element: [%s]",
			cfg->getName(), (char const*)e);
	}
	cfg->linkMMDocument(mmdoc);
}

void FluxMLConfiguration::parseVariables(
	data::Configuration * cfg,
	DOMNode * variables
	)
{
	DOMNode * var, * child;
	DOMNamedNodeMap * nnm;
	DOMAttr * idAttr, * loAttr, * incAttr, * hiAttr, * typeAttr,
		* edweightAttr;
	char const * cfg_name = cfg->getName();
	std::list< data::IsoReaction* > const * rList = doc_->getReactions();
	std::list< data::IsoReaction* >::const_iterator ri;
	bool result, is_net=true, is_flux, has_value, has_lo, has_inc,
	     has_hi, has_edweight;
	double value, lo, inc, hi, edweight;

	var = XMLElement::skipJunkNodes(variables->getFirstChild());
	if (var == 0)
		return;

	while (var != 0)
	{
		idAttr = loAttr = incAttr = hiAttr = edweightAttr = 0;
		has_value = has_lo = has_inc = has_hi = has_edweight = false;
		value = lo = inc = hi = 0.;
		edweight = 1.;
		nnm = var->getAttributes();

		if (XMLElement::match(var,fml_fluxvalue,fml_xmlns_uri))
		{
			is_flux = true;

			idAttr = (DOMAttr*)(nnm->getNamedItem(fml_flux));
			if (idAttr == 0)
				fTHROW(XMLException,var,
					"cfg \"%s\": element \"fluxvalue\" lacks required attribute \"flux\"",
					cfg_name);

			U2A asc_id(idAttr->getValue());

			for (ri=rList->begin(); ri!=rList->end(); ri++)
				if (strcmp((*ri)->getName(),asc_id) == 0)
					break;

			if (ri == rList->end())
				fTHROW(XMLException,var,
					"cfg \"%s\": element \"fluxvalue\"/attribute \"flux\" "
					"references an unknown reaction \"%s\"",
					cfg_name, (char const*)asc_id);

			typeAttr = (DOMAttr*)(nnm->getNamedItem(fml_type));
			if (typeAttr == 0)
				fTHROW(XMLException,var,
					"cfg \"%s\": element \"fluxvalue\" lacks required attribute \"type\"",
					cfg_name);

			if (XMLString::equals(typeAttr->getValue(),fml_net))
				is_net = true;
			else if (XMLString::equals(typeAttr->getValue(),fml_xch))
				is_net = false;
			else
				fTHROW(XMLException,var,
					"cfg \"%s\": element \"fluxvalue\": illegal value for attribute \"type\"",
					cfg_name);

			edweightAttr = (DOMAttr*)(nnm->getNamedItem(fml_edweight));
			if (edweightAttr != 0)
			{
				has_edweight = true;

				if (not XMLElement::parseDouble(edweightAttr->getValue(),edweight)
					or edweight<0. or edweight>1.)
				{
					U2A asc_edweight(edweightAttr->getValue());
					fTHROW(XMLException,var,
						"cfg \"%s\": illegal value for attribute \"edweight\": %s",
						cfg_name, (char const*)asc_edweight);
				}
			}
		}
                // hier beide Elemente betrachtet und die Rückwärtskompatibilität zu gewährleisten
		else if (XMLElement::match(var,fml_poolvalue,fml_xmlns_uri) ||
                         XMLElement::match(var,fml_poolsizevalue,fml_xmlns_uri))
		{
			is_flux = false;

			idAttr = (DOMAttr*)(nnm->getNamedItem(fml_pool));
			if (idAttr == 0)
				fTHROW(XMLException,var,
					"cfg \"%s\": element \"poolsizevalue\" lacks required attribute \"pool\"",
					cfg_name);

			U2A asc_id(idAttr->getValue());

			if (doc_->getPoolMap()->findPtr(asc_id) == 0)
				fTHROW(XMLException,var,
					"cfg \"%s\": element \"poolsizevalue\"/attribute \"pool\" "
					"references an unknown pool \"%s\"",
					cfg_name, (char const*)asc_id);
                        
                        std::list< data::InputPool* > const & ipList = cfg->getInputPools();
			std::list< data::InputPool* >::const_iterator ipi;
	
			for (ipi=ipList.begin(); ipi!=ipList.end(); ipi++)
			{
				if(!strcmp((*ipi)->getName(), (char const*)asc_id))
					fTHROW(XMLException,var,
						"cfg \"%s\": element \"poolsizevalue\"/attribute \"pool\" "
						"references an input pool \"%s\"",
						cfg_name, (char const*)asc_id);
			}                        
		}
		else fTHROW(XMLException,var,
			"cfg \"%s\": elements of type \"fluxvalue\" or \"poolsizevalue\" expected",
			cfg_name
			);

		if ((loAttr = (DOMAttr*)(nnm->getNamedItem(fml_lo))) != 0)
		{
			has_lo = true;
			if (not XMLElement::parseDouble(loAttr->getValue(),lo)
				or std::isnan(lo) or std::isinf(lo))
			{
				U2A asc_lo(loAttr->getValue());
				fTHROW(XMLException,var,
					"cfg \"%s\": illegal value for attribute \"lo\": %s",
					cfg_name, (char const*)asc_lo);
			}
		}

		if ((incAttr = (DOMAttr*)(nnm->getNamedItem(fml_inc))) != 0)
		{
			has_inc = true;
			if (not XMLElement::parseDouble(incAttr->getValue(),inc)
				or std::isnan(inc) or std::isinf(inc))
			{
				U2A asc_inc(incAttr->getValue());
				fTHROW(XMLException,var,
					"cfg \"%s\": illegal value for attribute \"inc\": %s",
					cfg_name, (char const*)asc_inc);
			}
		}

		if ((hiAttr = (DOMAttr*)(nnm->getNamedItem(fml_hi))) != 0)
		{
			has_hi = true;
			if (not XMLElement::parseDouble(hiAttr->getValue(),hi)
				or std::isnan(hi) or std::isinf(hi))
			{
				U2A asc_hi(hiAttr->getValue());
				fTHROW(XMLException,var,
					"cfg \"%s\": illegal value for attribute \"hi\": %s",
					cfg_name, (char const*)asc_hi);
			}
		}

		if (has_lo and has_hi and lo>hi)
			fTHROW(XMLException,var,
				"cfg \"%s\": illegal values for attributes \"lo\", \"hi\": lo > hi",
				cfg_name);

		child = XMLElement::skipJunkNodes(var->getFirstChild());
		if (not is_flux and child == 0)
			fTHROW(XMLException,var,
				"cfg \"%s\": missing pool size value in element \"poolsizevalue\"",
				cfg_name);
		if (child != 0)
		{
			has_value = true;
			if (child->getNodeType() != DOMNode::TEXT_NODE)
				fTHROW(XMLException,var,
					"cfg \"%s\": #PCDATA element expected below element \"%s\"",
					cfg_name,is_flux?"fluxvalue":"poolsizevalue");
			if (not XMLElement::parseDouble(static_cast< DOMText* >(child)->getData(),value))
			{
				U2A asc_value( static_cast< DOMText* >(child)->getData() );
				fTHROW(XMLException,var,
					"cfg \"%s\": error parsing \"%s\": \"%s\"",
					cfg_name,is_flux?"fluxvalue":"poolsizevalue",(char const*)asc_value);
			}
			if (std::isnan(value) or std::isinf(value))
				fTHROW(XMLException,var,
					"cfg \"%s\": invalid (non-finite) \"%s\": \"%g\"",
					cfg_name,is_flux?"fluxvalue":"poolsizevalue",value);
		}

		// Setzen der geparsten Werte:
		if (is_flux)
		{
			// KN: lo, inc up haben nichts mit Constraints zu tun. Die können
			// erst einmal vernachlässigt werden. (Hintergrund: die sind für
			// den ResiduumScanner gedacht, d.h. durchlaufen aller Datenpunkte
			// im Parameterraum. z.B. ppp1 = 0, 0.5, 1, 1.5;
			// gs1 = 0, 0.1, 0.2, 0.3, 0.4, 0.5 etc. Der Residuumscanner testet
			// alle gegen alle.
			U2A fname(idAttr->getValue());
			if (is_net)
			{
				result = cfg->addFreeFluxNet(
					fname,
					has_lo, lo,
					has_inc, inc,
					has_hi, hi,
					has_value, value,
					has_edweight, edweight
					);
			}
			else
			{
				result = cfg->addFreeFluxXch(
					fname,
					has_lo, lo,
					has_inc, inc,
					has_hi, hi,
					has_value, value,
					has_edweight, edweight
					);
			}

			if (not result)
				fTHROW(XMLException,var,
				"cfg \"%s\": duplicate declaration of fluxvalue for flux %s (%s)",
				cfg_name, (char const*)fname,is_net?"net":"exchange");
		}
		else
		{
			U2A pname(idAttr->getValue());
                        data::Pool ** poolptr;
                        // finde den genannten Pool:
                        if ((poolptr = doc_->getPoolMap()->findPtr(pname)) != 0)
                        {
                            // ist die Anzahl der Atome ungleich 0, sonst ist der Pool
                            // unbalanziert und wird als Co-faktor modelliert
                            if ((*poolptr)->getNumAtoms() != 0)
                            {
                                result = cfg->addFreePoolSize(
                                        pname,
                                        has_lo, lo,
                                        has_inc, inc,
                                        has_hi, hi,
                                        has_value, value,
                                        has_edweight, edweight
                                        );

                                if (not result)
                                        fTHROW(XMLException,var,
                                                "cfg \"%s\": duplicate declaration of poolsizevalue for pool %s",
                                                cfg_name, (char const*)pname);
                            }
                            else
                                fTHROW(XMLException,var,
                                                "cfg \"%s\": declaration of poolsize for isotopically not balanced pool \"%s\".",
                                                cfg_name, (char const*)pname);
                        }
		}

		var = XMLElement::nextNode(var);
	} // while (var != 0)
}

} // namespace flux::xml
} // namespace flux

