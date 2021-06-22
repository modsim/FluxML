#include <cstdlib>
#include <list>
#include <string>
#include <iostream>
#include <map>
#include <regex>
#include <xercesc/dom/DOM.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "Error.h"
#include "Array.h"
#include "cstringtools.h"
#include "BitArray.h"
#include "charptr_map.h"

#include "Pool.h"
#include "InputPool.h"

#include "UnicodeTools.h"
#include "XMLException.h"
#include "XMLElement.h"

#include "FluxMLContentObject.h"
#include "FluxMLUnicodeConstants.h"
#include "FluxMLInput.h"

#include "FluxMLDocument.h"

#include "ExprTree.h"
#include "LinearExpression.h"

#include "MathML.h"
#include "MathMLDocument.h"
#include "MathMLDeclare.h"
#include "MathMLExpression.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

using namespace flux::symb;

namespace flux {
namespace xml {

    
/**
 * Parst ein Input-Element aus einem DOM-Tree eines FluxML-Dokuments.
 *
 * @param node Knoten des Input-Elements
 */
void FluxMLInput::parseInput(DOMNode * input)
{
	DOMNode * label;
	DOMNamedNodeMap * nnm;
	DOMAttr * poolAttr, * typeAttr, * cfgAttr, * purityAttr, * costAttr, * idAttr;
        /*** Neue Prfoile-Version **/
        /** List der Bedingungen des Profiles */
	std::list<double> profile_conditions_;
        DOMAttr *profileAttr;
        profile_defined=false;
        /*** Ende: Neue Prfoile-Version **/
	int natoms = 0;
	double cost;
        Array< double > purity;
	charptr_map< Array< double > > values;        
        charptr_map< Array< double > > purities;
        charptr_map< double > costs;
        charptr_map< data::InputProfile> profiles;        
	BitArray mask;
	data::InputPool::Type ip_type =
        data::InputPool::ip_isotopomer; // "isotopomer" ist default
        
	if (input_pool_)
		fTHROW(XMLException,input,"FluxMLInput object already initialized!");

	if (not XMLElement::match(input,fml_input,fml_xmlns_uri))
		fTHROW(XMLException,input,"element node (input) expected.");

	nnm = input->getAttributes();
	if (nnm == 0)
		fTHROW(XMLException,input,"element node (input) lacks attributes.");

	// Attribut pool (Pool-Bezeichnung)
	poolAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_pool));
	if (poolAttr == 0)
		fTHROW(XMLException,input,"element node (input) lacks pool attribute.");
	U2A utf_pool_name(poolAttr->getValue());

	// Gibt es den Pool?
	data::Pool * ipool = lookupPool(utf_pool_name);
	if (ipool == 0)
	{
		fTHROW(XMLException,input,
				"unknown pool %s in input substrate spec",
				(char const*)utf_pool_name);
	}                        
        data::Configuration * cfg = doc_->getRootConfiguration();
	std::list< data::Constraint > const & eqConstraints = cfg->getEqualities();
	std::list< data::Constraint >::const_iterator eqConstraints_it;

	for (eqConstraints_it=eqConstraints.begin(); eqConstraints_it!=eqConstraints.end(); eqConstraints_it++)
	{
		charptr_array constraintsVars = eqConstraints_it->getConstraint()->getVarNames();
		if(constraintsVars.find((char const*)utf_pool_name)!=constraintsVars.end())
			fTHROW(XMLException,input,
				"input pool \"%s\" specified in equality constraint",
				(char const*)utf_pool_name);
	}
	
	
	std::list< data::Constraint > const & ineqConstraints = cfg->getInEqualities();
	std::list< data::Constraint >::const_iterator ineqConstraints_it;
	
	for (ineqConstraints_it=ineqConstraints.begin(); ineqConstraints_it!=ineqConstraints.end(); ineqConstraints_it++)
	{
		charptr_array constraintsVars = ineqConstraints_it->getConstraint()->getVarNames();
		if(constraintsVars.find((char const*)utf_pool_name)!=constraintsVars.end()) 
			fTHROW(XMLException,input,
				"input pool \"%s\" specified in inequality constraint",
				(char const*)utf_pool_name);
	}
	// Anzahl der Atome / Init. der Maske
	natoms = ipool->getNumAtoms();
	mask.resize(natoms,false);

	// Attribut id (Experimental Design)
	idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_id));
	U2A utf_id(idAttr ? idAttr->getValue() : 0);

	// Attribut type (isotopomer/cumomer/emu)
	typeAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_type));
	if (typeAttr != 0)
	{
		if (XMLString::equals(typeAttr->getValue(), fml_isotopomer))
			ip_type = data::InputPool::ip_isotopomer;
		else if (XMLString::equals(typeAttr->getValue(),fml_cumomer))
			ip_type = data::InputPool::ip_cumomer;
		else if (XMLString::equals(typeAttr->getValue(),fml_emu))
			ip_type = data::InputPool::ip_emu;
		else
		{
			fTHROW(XMLException,input,
				"input pool %s: type is restricted to \"isotopomer\", "
				"\"cumomer\" or \"emu\"",
				ipool->getName());
		}
	}
        
        // Zweite Version der Input-Proflie definiert über profile-Attribut 
        // gilt für alle Labels
	profileAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_profile));
	if (profileAttr != 0)
	{   
            // profile definiert
            profile_defined=true;

        // füge den Startpunkt Null zu Bedinngungsliste hinzu
        profile_conditions_.push_back(0.);

        // Profile-Bedingungen parsen
        U2A utf_profile(profileAttr->getValue());
        charptr_array expr_list = charptr_array::split(utf_profile,",");
        charptr_array::const_iterator i;
        double prv_val= 0., act_val=0.;

        for (i=expr_list.begin(); i!=expr_list.end(); i++)
        {
            char const * w;
            for (w=*i; *w!='\0'; w++)
                if (*w > 32) break;
            if (*w == '\0')
                continue;

            act_val= std::atof(*i);

            // Überprüfung der INF und Null-Zeitangabe
            if(act_val==0. or std::isinf(act_val))
                fTHROW(XMLException,input,
                       "parse error in input profile attribute (%s): "
                       "The timestamp \"t=0\" and \"t=inf\" must not be explicitly stated!", *i);

            // Überprüfung von streng monoton aufsteigender kommagetrennter Zeiten
            if(act_val<= prv_val)
                fTHROW(XMLException,input,
                       "parse error in input profile attribute (%s): "
                       "The timestamps must be specified in a strictly monotonously manner", *i);

            profile_conditions_.push_back(act_val);
            prv_val= act_val;
        }
	} else // Prüfe die Input-Substrate ob es sich um eine Substrate-Profile handelt
    {
        label = XMLElement::skipJunkNodes(input->getFirstChild());
        do
        {
            if(label!= 0)
            {
                DOMNode * child = XMLElement::skipJunkNodes(label->getFirstChild());
                U2A data(static_cast< DOMText* >(child)->getData());
                std::string str= (char * )data;
                if (str.find("t") != std::string::npos)
                {
                    profile_defined=true;

                    // füge den Startpunkt Null zu Bedinngungsliste hinzu
                    profile_conditions_.push_back(0.);

                    break;
                }
            }

            // Next label-Element
            label = XMLElement::nextNode(label);
        }
        while (label != 0);
    }
	
	label = XMLElement::skipJunkNodes(input->getFirstChild());
	do
	{
		if (not XMLElement::match(label,fml_label,fml_xmlns_uri))
		{
			if (label == 0)
			{
				// keine label-Kinder
				// => pool als natürlich markierten
				// Isotopomer-Pool initialisieren
				input_pool_ = new data::InputPool(
					utf_id,
					ipool->getName(),
					mask,
					data::InputPool::ip_isotopomer
					);
				if (mask.size())
                                {
					// Registrierung von eingesetzten Isotope zur Ermöglichung von
                                        // multi-Isotopic Tracer MFA Simulation,
                                        charptr_map<int> iso_map = ipool->getIsotopesCfg();
                                        if(iso_map.size())
                                        {
                                            input_pool_->setIsotopeCfg(iso_map);
                                            charptr_map<int>::const_iterator ii =iso_map.begin(); 
                                            std::stringstream iso_elem;
                                            iso_elem<<" for [" <<ii->key;
                                            ii++;
                                            while(ii!=iso_map.end())
                                            {
                                               iso_elem<<", "<< ii->key;
                                               ii++;
                                            }
                                            iso_elem<<"]";
                                            fNOTICE("assuming natural abundance %s in pool \"%s\"", iso_elem.str().c_str(),
                                                    ipool->getName());
                                        }
                                        else
                                            fNOTICE("assuming natural abundance 13C in pool \"%s\"", 
                                                    ipool->getName());
                                }
				return;
			}
			else
			{
				fTHROW(XMLException,label,
					"input pool %s: element node (label) expected",
					ipool->getName()
					);
			}
		}

		nnm = label->getAttributes();
		if (nnm == 0)
			fTHROW(XMLException,label,"element node (label) lacks attributes.");
		
                cfgAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_cfg));
		if (cfgAttr == 0)
			fTHROW(XMLException,label,"element node (label) lacks cfg attribute.");
		U2A utf_cfg(cfgAttr->getValue());

		// das cfg-Attribut muß einem speziellen Format entsprechen
		if (strlen(utf_cfg) != (size_t)natoms)
		{
			fTHROW(XMLException,label,
				"label spec. of pool %s [%s] with wrong size; expected size %i.",
				(char const *)utf_pool_name, (char const*)utf_cfg, natoms);
		}
		for (int i=0; i<natoms; i++)
		{
			switch (ip_type)
			{
			case data::InputPool::ip_cumomer:
			case data::InputPool::ip_emu:
				if (not (((char const*)utf_cfg)[i]=='x' or ((char const *)utf_cfg)[i]=='1'))
				{
					fTHROW(XMLException,label,
						"illegal %s label spec. [%s] for pool %s",
						ip_type == data::InputPool::ip_emu
							? "EMU" : "Cumomer",
						(char const*)utf_cfg,
						(char const*)utf_pool_name
						);
				}
				break;
			case data::InputPool::ip_isotopomer:
				if (not (((char const*)utf_cfg)[i]=='0' or ((char const*)utf_cfg)[i]=='1'))
				{
					fTHROW(XMLException,label,
						"illegal Isotopomer label spec. [%s] for pool %s",
						(char const*)utf_cfg, (char const*)utf_pool_name
						);
				}
			}
		}
                
		// den Wert aus [0,1] bzw. die Werteliste parsen
		size_t fvalues_len_exp;
		char const * w;      
               
                Array< double > fvalues = parseLabel(static_cast< DOMElement * >(label)); 
               
                // Substrate profile soll zunächst nur für Isotopomer unterstützen
                if((ip_type!= data::InputPool::ip_isotopomer) && (profile_defined==true))
                    fTHROW(XMLException,label,
                            "the used substrate profiling in pool \"%s\" is only supported for isotopomer type yet!",
                            (char const*)utf_pool_name);
		switch (ip_type)
		{
                    case data::InputPool::ip_cumomer:
                    case data::InputPool::ip_isotopomer:
                            if ((fvalues.size() != 1) and not profile_defined)
                            {
                                    fTHROW(XMLException,label,
                                            "illegal label spec. (expecting exactly "
                                            "one fraction value)."
                                            );
                            }
                            break;
                    case data::InputPool::ip_emu:
                            for (w = (char const *)utf_cfg, fvalues_len_exp = 1;
                                    *w != '\0'; ++w)
                            {
                                    if (*w == '1') ++fvalues_len_exp;
                            }
                            if (fvalues.size() != fvalues_len_exp)
                            {
                                    fTHROW(XMLException,label,
                                            "invalid number of mass isotopomer fractions; "
                                            "found %i, expected %i",
                                            int(fvalues.size()), int(fvalues_len_exp));
                            }
		}

		// das purity-Attribut
		if ((purityAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_purity))) != 0)
		{
			if (ip_type != data::InputPool::ip_isotopomer)
			{	
				fTHROW(XMLException,label,
					"the \"purity\" attribute is allowed only for "
					"isotopomer substrate specifications.");
			}
                        U2A utf_purity(purityAttr->getValue());
                        charptr_array purityList= charptr_array::split(utf_purity," ");
                        size_t i=0,size = purityList.size();
                        if(size!= ipool->getNumOfActiveIsotope())
                            fTHROW(XMLException,label,
                                            "label spec. of pool %s [%s]: illegal number of values "
                                            "in attribute purity: %s",
                                            (char const *)utf_pool_name,
                                            (char const *)utf_cfg,
                                            (char const *)utf_purity
                                            );
                        purity= Array<double>(size);
                        if(size==1)
                        {
                            if (not XMLElement::parseDouble(purityAttr->getValue(),purity[0])
				or (purity[0] < 0. or purity[0] > 1.))
                            {
                                    U2A utf_purity(purityAttr->getValue());
                                    fTHROW(XMLException,label,
                                            "label spec. of pool %s [%s]: illegal value "
                                            "for attribute purity: %s",
                                            (char const *)utf_pool_name,
                                            (char const *)utf_cfg,
                                            (char const *)utf_purity
                                            );
                            }
                        }
                        else if(size>1)
                        {
                            charptr_array::const_iterator pli;
                            for (pli=purityList.begin(), i=0; pli!=purityList.end(); ++pli,++i)
                            {
                                purity[i]= atof(*pli);
                                if ((purity[i] < 0. or purity[i] > 1.))
                                {
                                        U2A utf_purity(purityAttr->getValue());
                                        fTHROW(XMLException,label,
                                                "label spec. of pool %s [%s]: illegal value "
                                                "for attribute purity: %s",
                                                (char const *)utf_pool_name,
                                                (char const *)utf_cfg,
                                                (char const *)utf_purity
                                                );
                                }
                                
                            }
                        }
                        else
                        {
                            fTHROW(XMLException,label,
					"error parsing the \"purity\" attribute.");
                        }
		}
		else 
                {
                    purity=Array<double>(1);
                    purity[0]= 2.; // Berechnung abschalten (default)
                }
		
		// das cost-Attribut
		if ((costAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_cost))) != 0)
		{
			if (ip_type != data::InputPool::ip_isotopomer)
			{	
				fTHROW(XMLException,label,
					"the \"cost\" attribute is allowed only for "
					"isotopomer substrate specifications.");
			}

			if (not XMLElement::parseDouble(costAttr->getValue(),cost))
			{
				U2A utf_cost(costAttr->getValue());
				fTHROW(XMLException,label,
					"label spec. of pool %s [%s]: illegal value "
					"for attribute cost: %s",
					(char const *)utf_pool_name,
					(char const *)utf_cfg,
					(char const *)utf_cost
				        );
			}
		}
		else cost = 0.; // kostenlos

		// Konfiguration mit Fraction-Wert registrieren
		if (values.exists(utf_cfg))
			fTHROW(XMLException,label,"duplicate specification of fraction %s",
				(char const *)utf_cfg);
		values.insert(utf_cfg,fvalues);
                purities.insert(utf_cfg,purity);
		costs.insert(utf_cfg,cost);
        if(profile_defined)
        {
            /** Check duplicate **/
            if (profiles.exists(utf_cfg))
                fTHROW(XMLException,label,"duplicate profile specification of fraction %s in pool %s",
                       (char const *)utf_cfg, (char const *)utf_pool_name);

            /* add profile conditions to parsed label values*/
            std::list<double>::iterator pc;
            for(pc=profile_conditions_.begin(); pc!=profile_conditions_.end();++pc)
                input_profile_->addCondition(*pc);

            /** Check profile consistency **/
            fWARNING("#cond= %d  vs. #val= %d", int(input_profile_->getConditions().size()), int(input_profile_->getValues().size()));
            if(input_profile_->getConditions().size()!=input_profile_->getValues().size())
                fTHROW(XMLException,label,"The specified profile values of fraction %s and profile conditions are inconsistent with each other!",
                       (char const *)utf_cfg);

            profiles.insert(utf_cfg,*input_profile_);
        }
                // Next Sep-Element
		label = XMLElement::nextNode(label);
	}
	while (label != 0);
                         
	// Konfigurationen der Fractions prüfen und Maske konfigurieren
	charptr_map< Array< double > >::const_iterator vi;
	for (vi=values.begin(); vi!=values.end(); vi++)
	{
            BitArray * vi_cfg = BitArray::parseBin(vi->key);
            fASSERT(vi_cfg);
            vi_cfg->resize(natoms,false);
            mask = mask | (*vi_cfg);
            delete vi_cfg;
	}
        
	// InputPool-Objekt anlegen
	input_pool_ = new data::InputPool(
			utf_id,
			(char const *)utf_pool_name,
			mask,
			ip_type
			);
        
	// Fractions eintragen
	for (vi=values.begin(); vi!=values.end(); vi++)
	{
		BitArray * vi_cfg = BitArray::parseBin(vi->key);
		fASSERT(vi_cfg);
		vi_cfg->resize(natoms,false);
		switch (ip_type)
		{
		case data::InputPool::ip_isotopomer:
			input_pool_->setIsotopomerValue(
					*vi_cfg,
					vi->value[0],
					purities[vi->key],
					costs[vi->key]
					);
			break;
		case data::InputPool::ip_cumomer:
			input_pool_->setCumomerValue(
					*vi_cfg,
					vi->value[0]
					);
			break;
		case data::InputPool::ip_emu:
			input_pool_->setEMUValue(
					*vi_cfg,
					vi->value
					);
			break;
		}
		delete vi_cfg;
	}
        
	// der geparste Pool wird bei der später folgenden Validierung ggfs.
	// von der Isotopomer-Form in die Cumomer-Form konvertiert
        
        // Profile eintragen
        charptr_map< data::InputProfile >::const_iterator pi;
	for (pi=profiles.begin(); pi!=profiles.end();pi++)
	{
            BitArray * vi_cfg = BitArray::parseBin(pi->key);
            fASSERT(vi_cfg);
            vi_cfg->resize(natoms,false);
                    input_pool_->setInputProfileValue(
                                    *vi_cfg,
                                    pi->value,
                                    purities[pi->key],
                                    costs[pi->key]
                                    );
            delete vi_cfg;
	}
        
        // Registrierung von Isotope-Konfigurationens zur Ermöglichung von
        // multi-Isotopic Tracer MFA Simulation,
        input_pool_->setIsotopeCfg(ipool->getIsotopesCfg());
}

data::Pool * FluxMLInput::lookupPool(char const * pool_name)
{
	charptr_map< data::Pool* > * pmap = doc_->getPoolMap();
	data::Pool ** pptr = pmap->findPtr(pool_name);
	if (pptr == 0)
		return 0;
	return *pptr;
}

Array< double > FluxMLInput::parseLabel(DOMElement * label)
{   
	struct val_list_t
	{
		double value;
		val_list_t * next;
		
		val_list_t() : value(0),next(0) { }
		~val_list_t() { delete next; }
	} head, * walk;        
	bool next_is_sep = false;
        double sum;
	size_t size = 0;
	
        DOMNode * child = XMLElement::skipJunkNodes(label->getFirstChild());  
        walk = &head;
        while (child != 0)
        {
        	if(profile_defined) // parse the profile and skip this while loop
        	{
        		parseProfile(child);
        	}
            // standard labeling specification
        	else if (next_is_sep)
        	{
        		if (not XMLElement::match(child,fml_sep,fml_xmlns_uri))
        			fTHROW(XMLException,child,"element node (sep) expected.");
        	}
        	else
        	{
        		if (child->getNodeType() != DOMNode::TEXT_NODE)
        			fTHROW(XMLException,child,"#PCDATA element expected.");

        		if (not XMLElement::parseDouble(
        				static_cast< DOMText* >(child)->getData(),
						walk->value))
        		{
        			U2A utf_value(static_cast< DOMText* >(child)->getData());
        			fTHROW(XMLException,child,
        					"error parsing fraction value: %s",
							(char const*)utf_value
        			);
        		}

        		if (walk->value < 0. or walk->value > 1.)
        		{
        			U2A utf_value(static_cast< DOMText* >(child)->getData());
        			fTHROW(XMLException,child,
        					"labeling fraction value %s is out of range [0,1]",
							(char const*)utf_value
        			);
        		}

        		++size;
        		walk->next = new val_list_t;
        		walk = walk->next;
        	}
        	next_is_sep = not next_is_sep;
        	child = XMLElement::nextNode(child);
        }

        Array< double >values(size);
        walk = &head;
        sum = 0.;
        for (size_t k=0; k<size; ++k)
        {
        	values[k] = walk->value;
        	sum += walk->value;
        	walk = walk->next;
        }
        if (size > 1 and fabs(1.-sum) > 1e-5)
        {
        	fTHROW(XMLException,label,
        			"labeling fraction values are out of range [0,1]");
        }
        return values;
}

void  FluxMLInput::parseProfile(DOMNode * profile)
{
	input_profile_ =  new data::InputProfile();

	if (XMLElement::match(profile,fml_math,fml_xmlns_mathml_uri))
		parseProfileMathML(profile);
	else if (XMLElement::match(profile,fml_textual,fml_xmlns_uri))
		parseProfileTextual(profile);
	else
		fTHROW(XMLException,profile,
				"values in textual or MathML notation expected");
}


void FluxMLInput::parseProfileMathML(
	DOMNode * node
	)
{
	MathMLDocument * mathml;
	MathMLDeclare const * mathml_def;
	MathMLContentObject const * mathml_cont;
	MathMLExpression const * mathml_expr;
	std::string mathml_cname;
	ExprTree * expr;

	// den MathML-Parser anwerfen:
	try
	{
		mathml = new MathMLDocument( node );
	}
	catch (XMLException & e)
	{
		// MathML-Fehler in FluxML-Fehler konvertieren:
		fTHROW(XMLException,node,"MathML-error in profile: %s", e.toString());
	}

	// handelt es sich bei den Profile aus dem MathML-Dokument wirklich
	// um Profile?
	std::list< MathMLDeclare const * > mml_constr = mathml->getDefinitionsByRegExp(".*");
	
	// Sinnüberprüfung der Profile
	std::list< MathMLDeclare const * >::iterator l_iter = mml_constr.begin();
	while (l_iter != mml_constr.end())
	{
		mathml_def = *l_iter;
		mathml_cont = mathml_def->getValue();
		mathml_cname = mathml_def->getName();

		// Profile sind einfache MathMLExpressions:
		if (mathml_cont->getType() != MathMLContentObject::co_expression)
			fTHROW(XMLException,node,"Profile %s: simple MathML-expression expected.",
					mathml_cname.c_str());

		mathml_expr = static_cast<MathMLExpression const*>(mathml_cont);
		expr = mathml_expr->get();
                // Überprüfung der Variablennamen
                charptr_array vnames = expr->getVarNames();
                charptr_array::const_iterator vni;

                for (vni = vnames.begin(); vni != vnames.end(); vni++)
                {
                    if((std::strcmp(*vni,"t") != 0) and (std::strcmp(*vni,"otherwise") != 0) )
                            fTHROW(XMLException,node,
                                    "invalid variable name \"%s\" in "
                                    "profile specification \"%s\" (%s)",
                                    *vni, input_profile_->getName(), expr->toString().c_str());
                };
                
                input_profile_->addValue(expr);
                l_iter++;
	}

	// MathML-Dokument freigeben:
	delete mathml;
}

void FluxMLInput::parseProfileTextual(
	DOMNode * node
	)
{
	DOMNode * child;
	char const anon[] = "anonymous";
	if (not XMLElement::match(node,fml_textual,fml_xmlns_uri))
		fTHROW(XMLException,node,"element node (textual) expected.");
	child = node->getFirstChild();

	while (child != 0)
	{
		// unter Element textual ist alles erlaubt -- ausgewertet
		// werden nur TEXT_NODEs
		while (child != 0 && child->getNodeType() != DOMNode::TEXT_NODE)
			child = child->getNextSibling();

		if (child == 0)
			break;

		U2A cblock(static_cast< DOMText* >(child)->getData());
		charptr_array expr_list = charptr_array::split((char const*)cblock,";");
		charptr_array::const_iterator i;

		for (i=expr_list.begin(); i!=expr_list.end(); i++)
		{
			ExprTree * expr;
			int ci;
			char * cname, * u;
			char const * w;
			for (w=*i; *w!='\0'; w++)
				if (*w > 32) break;
			if (*w == '\0')
				continue;
			
			charptr_array nv_pair = charptr_array::split(*i,":");
			switch (nv_pair.size())
			{
			case 1:
				ci = 0;
				cname = strdup_alloc(anon);
				break;
			case 2:
				ci = 1;
				cname = strdup_alloc(nv_pair[0]);
				for (u=cname; *u!='\0'; u++)
					if (*u < 32) *u = ' ';
				strtrim_inplace(cname);
				break;
			default:
				fTHROW(XMLException,child,
					"parse error in profile (%s): "
					"confused by too many ':'", *i);
			}
			
			try
			{
				expr = ExprTree::parse(nv_pair[ci]);
			}
			catch (ExprParserException &)
			{
				fTHROW(XMLException,child,"parse error in profile %s(%s)",
					cname, nv_pair[ci]);
			}

                        // Überprüfung der Variablennamen
                        charptr_array vnames = expr->getVarNames();
                        charptr_array::const_iterator vni;
                        
                        for (vni = vnames.begin(); vni != vnames.end(); vni++)
                        {
                            if((std::strcmp(*vni,"t") != 0) and (std::strcmp(*vni,"otherwise") != 0) )
                                    fTHROW(XMLException,child,
                                            "invalid variable name \"%s\" in "
                                            "profile specification \"%s\" (%s)",
                                            *vni, input_profile_->getName(), nv_pair[ci]);
                        };
                        
                        input_profile_->addValue(expr);
                        
			delete[] cname;
			delete expr;
		}
		child = child->getNextSibling();
	}
}
} // namespace flux::xml
} // namespace flux

