#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "Notation.h"
#include "charptr_array.h"
#include "IsoReaction.h"
#include "UnicodeTools.h"
#include "XMLException.h"
#include "XMLElement.h"
#include "FluxMLUnicodeConstants.h"
#include "FluxMLDocument.h"
#include "FluxMLReaction.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

using namespace flux::symb;

namespace flux {
namespace xml {

/*
 * Parst eine Reaktionsspezifikation:
 * <reaction bidirectional_="true"|"false">
 *   <annotation name="...">...</annotation>
 *   ...
 *   <reduct id="..." cfg="..."/>
 *   <reduct id="...">
 *     <variant cfg="..." ratio="..."/>
 *     <variant cfg="..." ratio="..."/>
 *     ...
 *   </reduct>
 *   ...
 *   <rproduct id="..." cfg="..."/>
 *   <rproduct ...>
 *     <variant cfg="..." ratio="..."/>
 *     ...
 *   </rproduct>
 *   ...
 * </reaction>
 *
 * @param node Knoten mit reaction-Element
 */
void FluxMLReaction::parseReaction(
	DOMNode * node
	)
{
	data::IsoReaction * reaction;
	DOMNamedNodeMap * nnm;
	DOMAttr * idAttr, * bidirAttr;
	DOMNode * child;
	charptr_array idList;
	charptr_array::const_iterator ili, jli;
	std::list< Reactant >::iterator li;
	int nreact, nconst;
	double p_ratio;
	ExprTree * C = 0;
        bidirectional_ = true;
	// Objekt darf noch nicht initialisiert sein
	fASSERT(in_.size() == 0);
	fASSERT(out_.size() == 0);

	// richtiges Element?
	if (not XMLElement::match(node,fml_reaction,fml_xmlns_uri))
	{
		fTHROW(XMLException,node,"element node (reaction) expected.");
	}
	
	// Attribute
	nnm = node->getAttributes();

	if (nnm == 0 or (idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_id))) == 0)
	{
		fTHROW(XMLException,node,
			"reaction needs at least one unique identifier (attribute \"id\")");
	}
	U2A asc_id(idAttr->getValue());
	idList = charptr_array::split(asc_id,", ");
	for (ili=idList.begin(); ili!=idList.end(); ++ili)
	{
		if (not data::Notation::is_varname(*ili))
		{
			fTHROW(XMLException,node,
				"invalid reaction name \"%s\": does not match [A-Za-z_]([A-Za-z0-9_])",
				*ili);
		}
		if (doc_->findReaction(*ili) != 0)
			fTHROW(XMLException,node,
				"duplicate reaction name \"%s\"", *ili);
	}

	if ((bidirAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_bidirectional))) != 0)
	{
		if (XMLString::equals(bidirAttr->getValue(),fml_true))
			bidirectional_ = true;
		else if (XMLString::equals(bidirAttr->getValue(),fml_false))
			bidirectional_ = false;
		else
		{
			U2A asc_value(bidirAttr->getValue());
			fTHROW(XMLException,node,
				"reaction \"%s\": illegal value for attribute \"bidirectional\": \"%s\"",
				idList.concat(","), (char const *)asc_value);
		}
	}
	
	child = XMLElement::skipJunkNodes(node->getFirstChild());	
	// annotation-Elemente überspringen
	while (XMLElement::match(child,fml_annotation,fml_xmlns_uri))
	{
		// zum nächsten annotation vorrücken:
		child = XMLElement::nextNode(child);
	}
	
	// Parsen der Reaktanden
	while (XMLElement::match(child,fml_reduct,fml_xmlns_uri)
		or XMLElement::match(child,fml_rproduct,fml_xmlns_uri))
	{
		try
		{
			Reactant R = parseReactant(dynamic_cast< DOMElement* >(child));
			if (XMLElement::match(child,fml_reduct,fml_xmlns_uri))
				in_.push_back(R);
			else
				out_.push_back(R);
		}
		catch (XMLException & e)
		{
			fTHROW(XMLException,"[%s] in reaction \"%s\"",
				(char const*)e, idList.concat(","));
		}
		// zum nächsten reduct/rproduct vorrücken:
		child = XMLElement::nextNode(child);
	}
	if (child != 0)
	{
		U2A asc_nname(child->getNodeName());
		fTHROW(XMLException,child,
			"reaction \"%s\": unexpected element \"%s\" found in specification",
			idList.concat(","), (char const *)asc_nname);
	}

	// Reaktionsobjekte erzeugen
	
	// 1.) Iteratoren initialisieren, Anzahl der Reaktionen berechnen
	nreact = 1;
	for (li=in_.begin(); li!=in_.end(); ++li)
	{
		li->iter = li->variants.begin();
		li->index = 1;
		nreact *= li->variants.size();
	}
	for (li=out_.begin(); li!=out_.end(); ++li)
	{
		li->iter = li->variants.begin();
		li->index = 1;
		nreact *= li->variants.size();
	}

	if (nreact != int(idList.size()))
	{
		fTHROW(XMLException,node,
			"reaction \"%s\": number of reaction IDs (%i) differs number of reactions (%i)",
			idList.concat(","), int(idList.size()), nreact);
	}

	nconst = 1;
	for (ili=idList.begin(); ili!=idList.end(); ++ili)
	{
		// neue Reaktion anlegen:
		reaction = doc_->createReaction(*ili, bidirectional_);
                
		p_ratio = 1.;

		// Edukte
		for (li=in_.begin(); li!=in_.end(); ++li)
		{
			reaction->addEduct(li->name.c_str(), li->iter->cfg.c_str());
			p_ratio *= li->iter->ratio;
		}
		// Produkte
		for (li=out_.begin(); li!=out_.end(); ++li)
		{
			reaction->addProduct(li->name.c_str(), li->iter->cfg.c_str());
			p_ratio *= li->iter->ratio;
		}

		// Reaktionserzeugung abschließen und Atom-Mapping (Permutationsvektor erstellen):
		try { reaction->finish(); } catch (data::DataException &)
		{
			fTHROW(XMLException,node,
				"reaction \"%s\": illegal permutation in atom transitions",
				idList.concat(","));
		}

		// Constraints erzeugen, letztes Constraint auslassen (redundant)
		if (nconst < nreact)
		{
			for (jli=idList.begin(); jli!=idList.end(); ++jli)
			{
				if (C == 0)
					C = ExprTree::sym(*jli);
				else
					C = ExprTree::add(C,ExprTree::sym(*jli));
			}
			C = ExprTree::mul(ExprTree::val(p_ratio),C);
			C = ExprTree::eq(ExprTree::sym(*ili),C);
			// das hier wird fälschlicherweise als nicht-linear erkannt:
                        doc_->createConstraint("scrambler",C,data::NET); // net
			doc_->createConstraint("scrambler",C,data::XCH); // xch
			delete C; C = 0;
			++nconst;
		}

		// Nächste Kombination der Varianten erzeugen:
		for (li=in_.begin(); li!=in_.end(); ++li)
		{
			if (li->index < int(li->variants.size()))
			{
				++(li->iter); ++(li->index); continue;
			}
		}
		for (li=out_.begin(); li!=out_.end(); ++li)
		{
			if (li->index < int(li->variants.size()))
			{
				++(li->iter); ++(li->index); continue;
			}
		}
	}

	// Constraints für unidirektionale Flüsse anlegen
	if (not bidirectional_)
	{
		for (ili=idList.begin(); ili!=idList.end(); ++ili)
		{
			// ein Fluss ist nur dann unidirektional, falls
			// der xch-Wert auf 0 festgelegt wird und der
			// net-Wert positiv ist:
			ExprTree * expr = ExprTree::eq(ExprTree::sym(*ili),ExprTree::val(0));
                        doc_->createConstraint("unidir",expr,data::XCH);
			delete expr;
			expr = ExprTree::geq(ExprTree::sym(*ili),ExprTree::val(0));
                        doc_->createConstraint("unidir",expr,data::NET);
			delete expr;
		}
	}
        }
            
FluxMLReaction::Reactant FluxMLReaction::parseReactant(
	DOMElement * reactant
	)
{
	DOMNamedNodeMap * nnm;
	DOMNode * variant;
	DOMAttr * idAttr, * cfgAttr, * ratioAttr;
	Reactant R;
	U2A asc_name(XMLElement::getName(reactant));
	int natoms;
	int nratios;
	double ratio_sum;
	std::list< Variant >::iterator vi;

	// Attribute
	nnm = reactant->getAttributes();

	// id-Attribut
	if (nnm == 0 or (idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_id))) == 0)
	{
		fTHROW(XMLException,reactant,
			"element \"%s\" lacks required \"id\" (and \"cfg\") attributes",
			(char const *)asc_name);
	}
	U2A asc_id(idAttr->getValue());

	charptr_map< data::Pool* > & PMap = *(doc_->getPoolMap());
	charptr_map< data::Pool* >::iterator pmi = PMap.find((char const *)asc_id);
	if (pmi == PMap.end())
	{
		fTHROW(XMLException,reactant,
			"element \"%s\": metabolite \"%s\" is not specified in \"metabolitepools\"",
			(char const *)asc_name, (char const *)asc_id);
	}
	natoms = pmi->value->getNumAtoms();
	R.name = std::string(asc_id);

	// gibt es Varianten?
	variant = XMLElement::nextNode(reactant->getFirstChild());
	// falls nicht, wird ein cfg-Attribut erwartet:
	if (variant == 0)
	{
		if ((cfgAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_cfg))) == 0 and natoms > 0)
		{
			fTHROW(XMLException,reactant,
				"element \"%s\" (id=\"%s\") requires a \"cfg\" attribute (#atoms: %i)",
				(char const *)asc_name, R.name.c_str(), natoms);
		}
		
		Variant V;
		V.ratio = 1.;
		
		if (cfgAttr != 0)
		{
			U2A asc_cfg(cfgAttr->getValue());
			if (data::Notation::identify_perm_spec(asc_cfg) == -1)
			{
				fTHROW(XMLException,reactant,
					"element \"%s\" (id=\"%s\"): value of \"cfg\" attribute is invalid [%s]",
					(char const *)asc_name, R.name.c_str(), (char const *)asc_cfg);
			}
			if (data::Notation::perm_spec_length((char const *)asc_cfg) != natoms)
			{
				fTHROW(XMLException,reactant,
					"element \"%s\" (id=\"%s\"): \"cfg\" attribute shows %i labeling positions"
					" whereas pool definition specifies %i labeling postions",
					(char const *)asc_name, R.name.c_str(),
					data::Notation::perm_spec_length((char const *)asc_cfg), natoms);
			}
			V.cfg = std::string(asc_cfg);
		}

		R.variants.push_back(V);
		R.iter = R.variants.begin();
		R.index = 1;
		return R;
	}

	// Es gibt mehrere Varianten
	do
	{
		if (not XMLElement::match(variant,fml_variant,fml_xmlns_uri))
		{
			fTHROW(XMLException,variant,
				"element \"%s\" (id=\"%s\"): element \"variant\" expected",
				(char const *)asc_name, R.name.c_str());
		}
		
		// Attribute
		nnm = variant->getAttributes();

		// cfg-Attribut
		if (nnm == 0 or (cfgAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_cfg))) == 0)
		{
			fTHROW(XMLException,variant,
				"element \"variant\" lacks required \"cfg\" attribute");
		}
		U2A asc_cfg(cfgAttr->getValue());
		if (data::Notation::identify_perm_spec(asc_cfg) == -1)
		{
			fTHROW(XMLException,reactant,
				"element \"%s\" (id=\"%s\"): value of \"cfg\" attribute is invalid [%s]",
				(char const *)asc_name, R.name.c_str(), (char const *)asc_cfg);
		}
		if (data::Notation::perm_spec_length((char const *)asc_cfg) != natoms)
		{
			fTHROW(XMLException,variant,
				"element \"%s\" (id=\"%s\"): \"cfg\" attribute shows %i labeling positions"
				" whereas pool definition specifies %i labeling postions",
				(char const *)asc_name, R.name.c_str(),
				data::Notation::perm_spec_length((char const *)asc_cfg), natoms);
		}
		
		Variant V;
		V.cfg = std::string(asc_cfg);
		if ((ratioAttr = static_cast< DOMAttr* >(nnm->getNamedItem(fml_ratio))) != 0)
		{
			if (not XMLElement::parseDouble(ratioAttr->getValue(), V.ratio))
			{
				fTHROW(XMLException,variant,
					"element \"variant\" (reaction id=\"%s\"): failed to parse \"ratio\" attribute",
					R.name.c_str());
			}
			if (V.ratio < 0. or V.ratio > 1.)
			{
				fTHROW(XMLException,variant,
					"element \"variant\" (reaction id=\"%s\"): invalid value for attribute \"ratio\" (must be in [0,1])",
					R.name.c_str());
			}
		}
		R.variants.push_back(V);

		// zum nächsten variant-Element vorrücken:
		variant = XMLElement::nextNode(variant);
	}
	while (variant != 0);

	// Ist die Liste der Ratios konsistent?
	ratio_sum = 0.;
	nratios = 0;
	for (vi=R.variants.begin(); vi!=R.variants.end(); ++vi)
	{
		if (vi->ratio > 0.)
		{
			++nratios;
			ratio_sum += vi->ratio;
		}
	}
	if (nratios > 0)
	{
		if (nratios != int(R.variants.size()))
		{
			fTHROW(XMLException,reactant,
				"element \"%s\" (id=\"%s\"): inconsistent use of \"ratio\" attributes",
				(char const *)asc_name, R.name.c_str());
		}
		if (fabs(ratio_sum-1.) > 1e-6)
		{
			fTHROW(XMLException,reactant,
				"element \"%s\" (id=\"%s\"): inconsistent \"ratio\" attributes (sum should be =1)",
				(char const *)asc_name, R.name.c_str());
		}
		// bestehende Ratios normieren:
		for (vi=R.variants.begin(); vi!=R.variants.end(); ++vi)
			vi->ratio /= ratio_sum;
	}
	else
	{
		// neue Ratios erzeugen:
		double ratio = 1./double(R.variants.size());
		for (vi=R.variants.begin(); vi!=R.variants.end(); ++vi)
			vi->ratio = ratio;
	}
	R.iter = R.variants.begin();
	R.index = 1;
	return R;
}

} // namespace xml::fluxml
} // namespace xml

