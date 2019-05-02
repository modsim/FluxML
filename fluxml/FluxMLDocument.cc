#include <list>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "Sort.h"
#include "charptr_map.h"
#include "charptr_array.h"
#include "Notation.h"
#include "Configuration.h"
#include "Info.h"
#include "Pool.h"
#include "IsoReaction.h"
#include "DOMWriter.h"
#include "DOMWriterImpl.h"
#include "FluxMLContentObject.h"
#include "FluxMLConfiguration.h"
#include "FluxMLReactionNetwork.h"
#include "FluxMLConstraints.h"
#include "FluxMLInfo.h"
#include "FluxMLUnicodeConstants.h"
#include "MMUnicodeConstants.h"
#include "UnicodeTools.h"
#include "StoichMatrixInteger.h"
#include "XMLElement.h"
#include "FluxMLDocument.h"


#include "XMLFramework.h"


// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

using namespace flux::la;
using namespace flux::symb;

namespace flux {
namespace xml {

FluxMLDocument::FluxMLDocument(XN DOMDocument * doc)
	: FluxMLContentObject(this), info_(0), writer_(0), reader_(0)
{
	// Container-Objekte erzeugen:
	pool_map_ = new charptr_map< data::Pool* >();
	reaction_list_ = new std::list< data::IsoReaction* >();
	configuration_map_ = new charptr_map< data::Configuration* >();

	// Netzwerk-Konfiguration initialisieren
	root_cfg_ = new data::Configuration("__root__", "network root configuration");
	configuration_map_->insert("__root__", root_cfg_);

	// Initialisierung von pool_roles_ und stoich_matrix_ erfolgt
	// durch validatePoolRolesAndStoichiometry():
	stoich_matrix_ = 0;
	pool_roles_ = 0;

	if (doc->getDocumentElement() == 0)
	{
		delete pool_map_;
		delete reaction_list_;
		delete root_cfg_;
		delete configuration_map_;
		fTHROW(XMLException,"XML parser failed on root-element.");
	}

	// FluxML-Dokument parsen:
	parseDocument(doc->getDocumentElement());
}

FluxMLDocument::FluxMLDocument(const char * filename)
: FluxMLContentObject(this), info_(0), writer_(0), reader_(0)
{
        try
	{
		xml::framework::initialize();            
	}
	catch (xml::XMLException& e)
	{
		fERROR("failed to initialize the XML framework: %s",
			(char const*)e);
	}
        
        try
	{
		reader_ = new xml::DOMReaderImpl;
		reader_->mapEntity("http://www.13cflux.net/xml-schema/mm.xsd", FLUX_XML_DIR "/mm.xsd");
		reader_->mapEntity("http://www.13cflux.net/xml-schema/fluxml.xsd", FLUX_XML_DIR "/fluxml.xsd");
		reader_->mapEntity("http://www.w3.org/Math/XMLSchema/mathml2/mathml2.xsd", FLUX_XML_DIR "/mathml2/mathml2.xsd");
		reader_->setResolveXInclude(true);
                
                reader_->parseFromFile(filename);
	}
	catch (xml::XMLException& e)
	{
		fERROR("FluxML parsing error: %s", (char const*)e);
		delete reader_;
	}
        
        // Container-Objekte erzeugen:
	pool_map_ = new charptr_map< data::Pool* >();
	reaction_list_ = new std::list< data::IsoReaction* >();
	configuration_map_ = new charptr_map< data::Configuration* >();

	// Netzwerk-Konfiguration initialisieren
	root_cfg_ = new data::Configuration("__root__", "network root configuration");
	configuration_map_->insert("__root__", root_cfg_);

	// Initialisierung von pool_roles_ und stoich_matrix_ erfolgt
	// durch validatePoolRolesAndStoichiometry():
	stoich_matrix_ = 0;
	pool_roles_ = 0;

        XN DOMDocument * doc = reader_->getDOMDocument();
        
	if (doc->getDocumentElement() == 0)
	{
		delete pool_map_;
		delete reaction_list_;
		delete root_cfg_;
		delete configuration_map_;
                delete reader_;
		fTHROW(XMLException,"XML parser failed on root-element.");
	}

	// FluxML-Dokument parsen:
	parseDocument(doc->getDocumentElement());
}
	
FluxMLDocument::~FluxMLDocument()
{
	// der Info-Block:
	if (info_ != 0)  delete info_;

	// die Abbildung Pool-Namen -> Pools
	if (pool_map_ != 0)
	{
		charptr_map< data::Pool* >::iterator i;
		for (i=pool_map_->begin(); i!=pool_map_->end(); i++)
			delete i->value;
		delete pool_map_;
	}

	// die Reaktionsliste
	if (reaction_list_ != 0)
	{
		std::list< data::IsoReaction* >::iterator i;
		for (i=reaction_list_->begin(); i!=reaction_list_->end(); i++)
			delete *i;
		delete reaction_list_;
	}

	// die Liste der Konfigurationen
	if (configuration_map_ != 0)
	{
		charptr_map< data::Configuration* >::iterator i;
		for (i=configuration_map_->begin();
			i!=configuration_map_->end();
			i++) delete i->value;
		delete configuration_map_;
	}

	if (stoich_matrix_ != 0)
		delete stoich_matrix_;
	if (pool_roles_ != 0)
		delete pool_roles_;
	if (writer_ != 0)
		delete writer_;
        if (reader_ != 0)
		delete reader_;
}

/**
 * Parst die Pools einer Netzwerkspezifikation aus einem
 * pool-Element-Knoten eines DOM-Dokuments
 *
 * @param node pool-Element-Knoten eines DOM-Dokuments
 */
void FluxMLDocument::parseDocument(DOMNode * node)
{
	DOMNode * child;
	XMLCh const * xmlns;

	if (not XMLElement::match(node,fml_fluxml,fml_xmlns_uri))
		fTHROW(XMLException,node,"element node (fluxml) expected.");

	xmlns = static_cast< DOMElement* >(node)->getAttribute(fml_xmlns);
	if (xmlns == 0)
		fWARNING("element node (fluxml) lacks XML namespace attribute");
	else
	{
		if (not XMLString::equals(xmlns,fml_xmlns_uri))
			fTHROW(XMLException,node,"invalid FluxML namespace");
	}


	child = XMLElement::skipJunkNodes(node->getFirstChild());
	
	// Das info-Element ist optional
	if (XMLElement::match(child,fml_info,fml_xmlns_uri))
	{
		FluxMLInfo info(doc_,child);

		// nach erfolgreichem Parsen zum nächsten child weiterrücken
		child = XMLElement::nextNode(child);
	}

	// Das reactionnetwork-Element ist vorgeschrieben
	if (not XMLElement::match(child,fml_reactionnetwork,fml_xmlns_uri))
	{
		U2A rn_asc(XMLElement::getName(child));
		fTHROW(XMLException,child,"element node (reactionnetwork) expected; found %s", (char const*)rn_asc);
	}

	// das reactionnetwork-Element parsen
	FluxMLReactionNetwork reactionnetwork(doc_,child);
	
	// zum nächsten Element-Knoten vorrücken
	child = XMLElement::nextNode(child);

	// es folgt ein optionales constraints-Element und möglicherweise
	// mehrere (oder keine) configuration-Elemente:
	if (child == 0) return;

	if (not XMLElement::match(child,fml_constraints,fml_xmlns_uri)
		and not XMLElement::match(child,fml_configuration,fml_xmlns_uri))
		fTHROW(XMLException,child,"constraints or configuration element expected!");

	if (XMLElement::match(child,fml_constraints,fml_xmlns_uri))
	{
		// das constraints-Element parsen:
		FluxMLConstraints(doc_,child,"__root__");
		child = XMLElement::nextNode(child);
	}

	// jetzt können noch einige configuration-Elemente folgen.
	while (child != 0)
	{
		if (not XMLElement::match(child,fml_configuration,fml_xmlns_uri))
			fTHROW(XMLException,child,"element node (configuration) expected.");
	
		FluxMLConfiguration cfg(doc_,child);
		child = XMLElement::nextNode(child);
	}

	// Parsen abgeschlossen; Validierung:
	
	// Pools & Reaktionen validieren:
	validatePoolsAndReactions();

	// Pool-Rollen identifizieren und Stöchiometrische Matrix erstellen
	validatePoolRolesAndStoichiometry();

	// Konfigurationen (inkl. Input-Pool-Belegungen) validieren
	validateConfigurations();
}

/**
 * (Private) Methode zur Generierung der Stöchiometrischen Matrix.
 */
void FluxMLDocument::validatePoolRolesAndStoichiometry()
{
	charptr_array r_names;
	charptr_array p_names;
	charptr_map< data::Pool* >::iterator pi;
	charptr_map< PoolRole >::iterator qi;
	std::list< data::IsoReaction* >::iterator ri;
	std::list< data::IsoReaction::Isotopomer* >::const_iterator rm;
	
	// Klassifikation der Pools anhand der Netzwerkstruktur in
	// Input-Pools und innere Pools. Mit Hilfe dieser Klassifikation
	// können Fehler in der Reversibilität von Reaktionen erkannt werden
	
	pool_roles_ = new charptr_map< PoolRole >(pool_map_->size());

	// Da von den Pool-Rollen bislang nichts bekannt ist, erfolgt
	// hier eine Vorbelegung:
	for (pi = pool_map_->begin(); pi != pool_map_->end(); ++pi)
		pool_roles_->insert( pi->key , p_input );

	// Verarbeiten der Reaktionsliste und Berechnung der Pool-Rollen:
	for (ri = reaction_list_->begin(); ri != reaction_list_->end(); ++ri)
	{
		data::IsoReaction * R = *ri;
		
		// Reaktionsnamen hinzufügen:
		r_names = (r_names, R->getName());

		// ein Pool ist solange ein Input-Pool, bis er als
		// Produkt einer Reaktion auftaucht:
		for (rm = R->getProducts().begin();
			rm != R->getProducts().end(); ++rm)
		{
			data::IsoReaction::Isotopomer * M = *rm;
			qi = pool_roles_->find( M->name );
			qi->value = p_inner;
		} // for ( Products )
	} // for ( Reactions )

	// Validierung: werden Input- mit inneren Pools auf der Edukt-Seite
	// einer Reaktion vermischt - das würde Ärger machen
	for (ri = reaction_list_->begin(); ri != reaction_list_->end(); ++ri)
	{
		data::IsoReaction * R = *ri;
		PoolRole pr;

		rm = R->getEducts().begin();
		fASSERT(rm != R->getEducts().end());
		pr = pool_roles_->find( (*rm)->name )->value;
		for (; rm != R->getEducts().end(); ++rm)
			if (pool_roles_->find( (*rm)->name )->value != pr)
			{
				// eine schöne Fehlermeldung generieren
				charptr_array sub, nonsub;
				for (rm = R->getEducts().begin();
					rm != R->getEducts().end(); ++rm)
				{
					if (pool_roles_->find( (*rm)->name )->value == p_input)
						sub.add((*rm)->name);
					else
						nonsub.add((*rm)->name);
				}
				fTHROW(XMLException,"reaction \"%s\" mixes substrate (%s) "
					"and non-substrate pools (%s) on its educt side",
					(*ri)->getName(), sub.concat(","), nonsub.concat(","));
			}
	}

	// Erzeuge eine Namens-Liste von inneren Pools:
	for (pi = pool_map_->begin(); pi != pool_map_->end(); pi++)
		// den Pool-Namen der Liste hinzufügen, falls es sich um einen
		// INNEREN(!) Pool handelt:
		if ( (*pool_roles_)[ pi->key ] == p_inner )
			p_names = (p_names, pi->key);

	if (p_names.size() == 0)
		fTHROW(XMLException,"network contains only input pools");

	// Erzeugung der Stöchiometrischen Matrix:
	stoich_matrix_ = new StoichMatrixInteger(
		p_names.size(),	r_names.size(),		// rows, cols
		p_names.get(),				// metabolite-names
		r_names.get()				// reaction-names
		);

	// die Reaktionen in die Stöchiometrische Matrix eintragen:
	// abfließend: inc(-1), zufließend: inc(+1)
	for (ri = reaction_list_->begin(); ri != reaction_list_->end(); ri++)
	{
		int idx;
		size_t row, col;
		data::IsoReaction * R = *ri;
		
		idx = r_names.findIndex( R->getName() );
		fASSERT(idx >= 0);
		col = size_t(idx);
		
		for (rm = R->getEducts().begin();
			rm != R->getEducts().end(); rm++)
		{
			data::IsoReaction::Isotopomer * M = *rm;

			// nur INNERE Pools
			if ( (*pool_roles_)[ M->name ] != p_inner )
				continue;

			idx = p_names.findIndex( M->name );
			fASSERT(idx >= 0);
			row = size_t(idx);

			// Abfluß aus einem Edukt-Pool: inc(-1)
			(*stoich_matrix_)(row,col) -= 1;
		}

		for (rm = R->getProducts().begin();
			rm != R->getProducts().end(); rm++)
		{
			data::IsoReaction::Isotopomer * M = *rm;
			
			// nur INNERE Pools
			if ( (*pool_roles_)[ M->name ] != p_inner )
				continue;

			row = p_names.findIndex( M->name );
			// Zufluß in einen Produkt-Pool: inc(+1)
			(*stoich_matrix_)(row,col) += 1;
		}
	} // for ( Reactions )
}

/*
 * Validierung des Zusammenspiels der Bezeichnungen der Pools in der
 * Pool-Liste und deren Erwähnung innerhalb der Reaktionebeschreibungen
 */
void FluxMLDocument::validatePoolsAndReactions()
{
	std::list< data::IsoReaction::Isotopomer* >::const_iterator j;
	std::list< data::IsoReaction* >::iterator i;
	int k;
	bool invalid, network_has_efflux = false;

	i = reaction_list_->begin();
	while (i != reaction_list_->end())
	{
		data::IsoReaction * reaction = *i;

		std::list< data::IsoReaction::Isotopomer* >::const_iterator iters_begin[] = {
			reaction->getEducts().begin(),
			reaction->getProducts().begin()
			};
		std::list< data::IsoReaction::Isotopomer* >::const_iterator iters_end[] = {
			reaction->getEducts().end(),
			reaction->getProducts().end()
			};

		// Ein Netzwerk ohne irgendeinen Abfluß ist defekt:
		network_has_efflux = network_has_efflux ||
			reaction->getProducts().size()==0;

		// gibt es die in der Reaktion angegebenen Edukte/Produkte wirklich
		// und in der angegebenen Form?
		for (k=0; k<2; k++)
		{
			j = iters_begin[k];
			while (j != iters_end[k])
			{
				data::Pool ** poolptr;
				data::IsoReaction::Isotopomer * metab =
					static_cast< data::IsoReaction::Isotopomer* >( *j );

				// gibt es den genannten Pool?:
				if ((poolptr = pool_map_->findPtr(metab->name)) != 0)
				{
					int natoms = data::Notation::perm_spec_length(metab->atom_cfg);

					// Stimmt die Länge der Konfiguration mit der Länge
					// der Atom-Ketten überein?
					if ((*poolptr)->getNumAtoms() != natoms)
						fTHROW(XMLException,
							"validation (%s): pool %s shows mismatch in atom cfg.",
							reaction->getName(), metab->name);
					
					// der Pool nimmt evtl. zum ersten Mal an einer Reaktion
					// teil und wird markiert:
					(*poolptr)->setUsedInReaction();

					// jeder Pool muss einmal in der Rolle eines Edukts auftreten
					// -- ansonsten hat er keinen Abfluss
					if (k == 0)
						(*poolptr)->setHasEfflux();
				}
				else
					fTHROW(XMLException,
						"validation (%s): pool %s does not exist.",
						reaction->getName(), metab->name);

				j++;
			}  // alle Edukte/Produkte              
		} // Edukte/Produkte
		i++;
	} // alle Reaktionen

	// Fehlermeldungen zunächst sammeln, bevor eine Exception ausgelöst wird:
	if (not network_has_efflux)
		fWARNING("network has no efflux");

	// Gibt es isolierte Pools, die mit Reaktionen nicht erreicht werden?
	invalid = not network_has_efflux;
	charptr_map< data::Pool* >::iterator p = pool_map_->begin();
	while (p != pool_map_->end())
	{
		data::Pool * pool = p->value;

		if (pool->isUsedInReaction() == false)
		{
			fWARNING("pool \"%s\" is isolated", pool->getName());
			invalid = true;
		}
		if (pool->hasEfflux() == false)
		{
			fWARNING("pool \"%s\" has no efflux (set at least one incident exchange flux >0)",
				pool->getName());
		}
		p++;
	}
	if (invalid)
		fTHROW(XMLException,"validation: inconsistent network structure");
}

// sind die genannten Input-Pools wirklich Input-Pools des Netzwerkes?
// wurden Input-Pools vergessen?
// sind die freien Flüsse konform mit der Stöchiometrie?
void FluxMLDocument::validateConfigurations()
{
	bool valid_cfg, found;
	
	// Die Stöchiometrische Matrix muß bereits berechnet sein!
	fASSERT(stoich_matrix_ != 0);

	// Input-Pools für die Wurzel-Konfiguration erzeugen
	charptr_map< PoolRole >::iterator ri;
	for (ri = pool_roles_->begin(); ri != pool_roles_->end(); ri++)
	{
		if (ri->value != p_input) continue;
		data::InputPool * ip = root_cfg_->createInputPool(
			ri->key,
			(*pool_map_)[ri->key]->getNumAtoms(),
			data::InputPool::ip_cumomer
			);
		ip->setCumomerValue(0,1.); // unmarkiert
	}

	// I/O-Constraints in die Wurzel-Konfiguration eintragen
	setConfInputOutputConstraints(root_cfg_);

	charptr_map< data::Configuration* >::iterator ci;
	for (ci = configuration_map_->begin(); ci != configuration_map_->end(); ci++)
	{
		// Constraints der Wurzel-Konfiguration übernehmen
		valid_cfg = true;
		if (ci->value != root_cfg_)
		{
			ci->value->mergeConstraints(*root_cfg_);

// TODO: ist die Lazy-Validierung besser geeignet?
//			// Validierung der Constraints und Flußangaben:
//			valid_cfg = validateConfSingle( ci->value );
		}

		// Validierung der Input-Pool-Spezifikation:
		std::list< data::InputPool* > const & ipools = ci->value->getInputPools();
		std::list< data::InputPool* >::const_iterator pi;

		for (ri = pool_roles_->begin(); ri != pool_roles_->end(); ri++)
		{
			if (ri->value != p_input) continue;
			found = false;
			for (pi = ipools.begin(); not found && pi != ipools.end(); pi++)
				if ((strcmp( (*pi)->getName(), ri->key ) == 0) /*or ((*pi)->getSize()==0)*/)
					found = true;
                        
                        // Co-factor pool prüfen da sie nicht als input spezifiziert werden müssen
                        data::Pool ** ptr = pool_map_->findPtr(ri->key);
                        fASSERT(ptr != 0);
                        if((*ptr)->getNumAtoms()== 0)
                            found = true;
                        
			if (not found)
			{
				valid_cfg = false;
				fWARNING("missing input pool specification "
					"(cfg: \"%s\", pool: \"%s\")",
					ci->key,ri->key);
			}
		}

		// gehören die Input-Pool Spezifikationen wirklich zu Input-Pools,
		// oder will man uns andere Pools als Input-Pools verkaufen?
		for (pi = ipools.begin(); pi != ipools.end(); pi++)
		{
			ri = pool_roles_->find( (*pi)->getName() );
			if (ri == pool_roles_->end())
			{
				valid_cfg = false;
				fWARNING("unknown pool \"%s\" in input pool specification",
					(*pi)->getName());
			}
			else if (ri->value != p_input)
			{
				valid_cfg = false;
				fWARNING("input pool specification for inner pool \"%s\"",
					(*pi)->getName());
			}
		}

		if (not valid_cfg)
		{
			fTHROW(XMLException,"validation of configuration %s failed!",
				ci->key);
		}
	}
}

bool FluxMLDocument::validateConfSingle(data::Configuration * cfg)
{
	fASSERT(stoich_matrix_ != 0);	
		
	// die Konfiguration der Flüsse und Input-Pools validieren
	cfg->validate(
		stoich_matrix_
		);

	switch (cfg->getValidationState())
	{
	case data::Configuration::cfg_unvalidated:
		// nach cfg->validate() sollte dies nicht
		// auftreten
		fASSERT_NONREACHABLE();
		return false;
	case data::Configuration::cfg_ok:
		// die Konfiguration ist gültig
		return true;
	case data::Configuration::cfg_too_few_constr:
		// es gibt zu wenige Constraints
		if (cfg == root_cfg_)
			break;
		fWARNING("configuration \"%s\": too few constraints or free variables",
				cfg->getName());
		break;
	case data::Configuration::cfg_too_much_constr:
		// es gibt zu viele Constraints
		fWARNING("configuration \"%s\" is invalid: too much constraints",
				cfg->getName());
		break;
	case data::Configuration::cfg_linear_dep_constr:
		// die Constraints sind scheinbar linear abhängig
		fWARNING("configuration \"%s\": linear dependencies between constraints",
				cfg->getName());
		break;
	case data::Configuration::cfg_nonlinear_constr:
		// das Constraint ist nicht-linear
		fWARNING("configuration \"%s\" is invalid: probably non-linear equalities",
				cfg->getName());
		break;
	case data::Configuration::cfg_invalid_constr:
		// das Constraint ist ungültig
		fWARNING("configuration \"%s\" is invalid: invalid constraints!?",
				cfg->getName());
		break;
	case data::Configuration::cfg_invalid_substrate:
		fWARNING("configuration \"%s\" is invalid: illegal substrate mixture",
				cfg->getName());
		break;
	case data::Configuration::cfg_too_much_free_vars:
		fWARNING("configuration \"%s\" is invalid: too many free fluxes!",
				cfg->getName());
		break;
	case data::Configuration::cfg_invalid_free_vars:
		fWARNING("configuration \"%s\" is invalid: wrong combination of free fluxes",
				cfg->getName());
                break;
	case data::Configuration::cfg_ineqs_violated:
		fWARNING("configuration \"%s\": inequality constraints violated",
				cfg->getName());
		break;
	case data::Configuration::cfg_ineqs_infeasible:
		fWARNING("configuration \"%s\" is invalid: infeasible inequality constraints",
				cfg->getName());
		break;
	}

	// falls die Konfiguration im Wesentlichen validiert ist
	// und eine gültige Stöchiometrie erwartet werden kann
	// wird hier true zurückgegeben, damit der FluxML-Parser
	// späteren Auswertungsschritten nicht im Weg steht ...
	return cfg->isValid(3);
}

uint32_t FluxMLDocument::computeCheckSum(int crc_scope) const
{
	uint32_t crc = 0u;
	// implement a checksum algorithm of your choice here
	return crc;
}

char const * FluxMLDocument::getSignature() const
{
	if (info_ == 0 or info_->getSignature().size() == 0)
	{
		fDEBUG(0,"FluxML document does not contain a signature");
		return 0;
	}
	return info_->getSignature().c_str();
}

void FluxMLDocument::setConfInputOutputConstraints(data::Configuration * cfg)
{
	// Da nach zuweisung der Pool-Rollen klar ist, welche Pools und
	// Reaktionen intra-zelluläre und extra-zelluläre Pools verbinden
	// werden jetzt zusätzliche Constraints auf Exchange-Flüsse generiert,
	// die besagen, das input- und output-Reaktionen irreversibel sind
	// (der Exchange-Fluß wird auf 0 gesetzt).
	std::list< data::IsoReaction* >::iterator ri;
	std::list< data::IsoReaction::Isotopomer* >::const_iterator rm;
	ExprTree * xch_expr; // Exchange von I/O-Fluss == 0
	ExprTree * net_expr; // Netto von I/O-Fluss >= 0
	bool imp_const;

	// Schritt 1: Welche Flüsse sind mit Input-Pools verbunden? Welche
	//            Flüsse sind Abflüsse? Speichere die Liste der Reaktionen
	//            in ioReactions.
	
	for (ri = reaction_list_->begin();
		ri != reaction_list_->end(); ri++)
	{
		data::IsoReaction * R = *ri;
		imp_const = false;
		
		for (rm = R->getEducts().begin(); rm != R->getEducts().end(); rm++)
			if ((*pool_roles_)[ (*rm)->name ] == p_input)
			{
				imp_const = true;
				break;
			}
		
		// Falls die Reaktion kein Produkt hat, handelt es sich um einen
		// Abfluß. Im Folgenden werden einfache Constraints für alle
		// Abflüsse generiert
		if (imp_const or R->getProducts().size() == 0)
		{
			// ein Fluss wird NUR DANN irreversibel, wenn der xch-Wert
			// auf 0 festgelegt wird und der net-Wert positiv ist!
			net_expr = ExprTree::geq(ExprTree::sym(R->getName()),ExprTree::val(0));
                        // cfg->createConstraint("unidir I/O",net_expr,true);
                        cfg->createConstraint("unidir I/O",net_expr,data::NET);
			delete net_expr;
			
			xch_expr = ExprTree::eq(ExprTree::sym(R->getName()),ExprTree::val(0));
                        // cfg->createConstraint("unidir I/O",xch_expr,false);
                        cfg->createConstraint("unidir I/O",xch_expr,data::XCH);
			delete xch_expr;
		}
		else
		{
			// bei sonstigen Flüssen soll der Exchange-Fluß niemals
			// kleiner als 0 sein
			xch_expr = ExprTree::geq(ExprTree::sym(R->getName()),ExprTree::val(0));
                        // cfg->createConstraint("pos. xch",xch_expr,false);
                        cfg->createConstraint("pos. xch",xch_expr,data::XCH);
			delete xch_expr;
		}
	}
}

DOMWriter & FluxMLDocument::getDOMWriter()
{
	if (writer_)
		return *writer_;
	writer_ = new DOMWriterImpl(0,"fluxml",0,"fluxml.dtd");
	return *writer_;
}

DOMReader & FluxMLDocument::getDOMReader()
{
	if (reader_)
		return *reader_;
	reader_ = new xml::DOMReaderImpl;
        reader_->mapEntity("http://www.13cflux.net/xml-schema/mm.xsd", FLUX_XML_DIR "/mm.xsd");
        reader_->mapEntity("http://www.13cflux.net/xml-schema/fluxml.xsd", FLUX_XML_DIR "/fluxml.xsd");
        reader_->mapEntity("http://www.w3.org/Math/XMLSchema/mathml2/mathml2.xsd", FLUX_XML_DIR "/mathml2/mathml2.xsd");
        reader_->setResolveXInclude(true);
	return *reader_;
}

bool FluxMLDocument::patchDOM(
	DOMDocument & doc,
	data::ConstraintSystem const & CS,
	char const * cfg_name
	)
{
	DOMNodeList * configuration_list
		= doc.getElementsByTagName(fml_configuration);
	DOMElement * fluxvalue, * variables = 0, * simulation;        
	DOMNode * child, * pchild;
	char dbl_str[32];
	double net, xch;    
        bool stationary = true;
	A2U utf_cfg_name(cfg_name);
	
	// 1. aus doc "variables" mit name aus variable cfg suchen
	// <configuration>
	for (XMLSize_t i=0; i<configuration_list->getLength(); i++)
	{
		DOMElement * conf_node = static_cast< DOMElement* >(
				configuration_list->item(i)
				);

		// passende Konfiguration suchen
		if (not XMLString::equals(
			conf_node->getAttribute(fml_name),
			utf_cfg_name)) continue;
                
		// <simulation>
		DOMNodeList * sim_list
			= conf_node->getElementsByTagName(fml_simulation);
		if (sim_list->getLength() == 0)
		{
			simulation = doc.createElementNS(fml_xmlns_uri,fml_simulation);
			conf_node->appendChild(simulation);
		}
		else
			simulation = dynamic_cast< DOMElement* >(sim_list->item(0));
		
		// <variables>
		DOMNodeList * var_list
			= simulation->getElementsByTagName(fml_variables);

		if (var_list->getLength() == 0)
		{
			variables = doc.createElementNS(fml_xmlns_uri,fml_variables);
			simulation->appendChild(variables);
		}
		else
			variables = dynamic_cast< DOMElement * >(var_list->item(0));

		// fluxvalue-Elemente unterhalt von variables entfernen
		for (child = variables->getFirstChild(); child != 0;)
		{
			if (child->getNodeType() == DOMNode::COMMENT_NODE)
			{
				child = child->getNextSibling();
				continue; // Kommentare möglichst erhalten
			}
			pchild = child->getNextSibling();
			variables->removeChild(child);
			child = pchild;
		}    
                // Stationarität überprüfen
                if (XMLString::equals(
                        conf_node->getAttribute(fml_stationary),
                        A2U("false"))) stationary=false;
	}

	fASSERT(variables != 0);

	// neue fluxvalue-Elemente für net und xch anlegen
	// net:
	charptr_array fluxes_net = CS.getFluxNamesByType(
			data::ConstraintSystem::f_free,true);
	charptr_array::const_iterator ni;

	for (ni=fluxes_net.begin(); ni!=fluxes_net.end(); ni++)
	{
		CS.getFlux(*ni, net, xch);
		fluxvalue = doc.createElementNS(fml_xmlns_uri,fml_fluxvalue);
		fluxvalue->setAttribute(fml_flux, A2U(*ni));
		fluxvalue->setAttribute(fml_type, fml_net);
		dbl2str(dbl_str, net, sizeof(dbl_str));
		fluxvalue->appendChild(doc.createTextNode(A2U(dbl_str)));
		
		variables->appendChild(fluxvalue);
	}
	// xch:
	charptr_array fluxes_xch = CS.getFluxNamesByType(
			data::ConstraintSystem::f_free,false); 
	charptr_array::const_iterator xi;
	for (xi = fluxes_xch.begin(); xi != fluxes_xch.end(); xi++)
	{
		CS.getFlux(*xi, net, xch);
		fluxvalue = doc.createElementNS(fml_xmlns_uri,fml_fluxvalue);
		fluxvalue->setAttribute(fml_flux, A2U(*xi));
		fluxvalue->setAttribute(fml_type, fml_xch);
		dbl2str(dbl_str, xch, sizeof(dbl_str));
		fluxvalue->appendChild(doc.createTextNode(A2U(dbl_str)));
		
		variables->appendChild(fluxvalue);
	}
        // pool:
	if(!stationary)
        {
            double pool;
            DOMElement * poolvalue;
            charptr_array pools = CS.getPoolNamesByType(data::ConstraintSystem::p_free); 
            charptr_array::const_iterator pi;
            for (pi = pools.begin(); pi != pools.end(); pi++)
            {

                CS.getPoolSize(*pi, pool);
                poolvalue = doc.createElementNS(fml_xmlns_uri,fml_poolvalue);
                poolvalue->setAttribute(fml_pool, A2U(*pi));
                dbl2str(dbl_str, pool, sizeof(dbl_str));
                poolvalue->appendChild(doc.createTextNode(A2U(dbl_str)));

                variables->appendChild(poolvalue);
            }
        }
	return true;
}

bool FluxMLDocument::patchDOM_MS(
	DOMDocument & doc,
	MGroupMS const & mgms,
	DOMElement & data
	)
{
	size_t w;
	char buf[32];
	MVector x_meas;
	MVector x_stddev;
	DOMElement * datum;
        
        std::set<double> times = mgms.getTimeStampSet();
        std::set<double>::const_iterator t;
        for(t= times.begin(); t!= times.end(); ++t)
        {
            if (not mgms.getMValuesStdDev(*t, x_meas, x_stddev))
            {
                    fERROR("Failed to get mvalues");
                    return false;
            }

            for (w=0; w<mgms.getNumWeights(); w++)
            {
                    datum = doc.createElementNS(fml_xmlns_uri,mm_datum);
                    datum->setAttribute(fml_id, A2U(mgms.getGroupId()));

                    if(*t!=-1)
                    {
                        dbl2str(buf, *t, sizeof(buf));
                        datum->setAttribute(mm_time, A2U(buf));
                    }

                    snprintf(buf, sizeof(buf), "%d", mgms.getWeights()[w]);
                    datum->setAttribute(mm_weight, A2U(buf));

                    dbl2str(buf, x_stddev.get(w), sizeof(buf));
                    datum->setAttribute(mm_stddev, A2U(buf));

                    dbl2str(buf, x_meas.get(w), sizeof(buf));
                    datum->appendChild(doc.createTextNode(A2U(buf)));

                    data.appendChild(datum);
            }
        }

	return true;
}

bool FluxMLDocument::patchDOM_MSMS(
	DOMDocument & doc,
	MGroupMSMS const & mgmsms,
	DOMElement & data
	)
{ 
	size_t w;
	char buf[32];
	MVector x_meas;
	MVector x_stddev;
	DOMElement * datum;
        
        std::set<double> times = mgmsms.getTimeStampSet();
        std::set<double>::const_iterator t;
        for(t= times.begin(); t!= times.end(); ++t)
        {
            if (not mgmsms.getMValuesStdDev(*t, x_meas, x_stddev))
            {
                    fERROR("failed to retrieve measurement values for measurement group %s",
                            mgmsms.getGroupId());
                    return false;
            }

            for (w=0; w<mgmsms.getNumWeights(); w++)
            {
                    datum = doc.createElementNS(fml_xmlns_uri,mm_datum);
                    datum->setAttribute(fml_id, A2U(mgmsms.getGroupId()));

                    if(*t!=-1)
                    {
                        dbl2str(buf, *t, sizeof(buf));
                        datum->setAttribute(mm_time, A2U(buf));
                    }
                    
                    snprintf(buf, sizeof(buf), "%d,%d",
                            mgmsms.getWeights1()[w],
                            mgmsms.getWeights2()[w]
                            );
                    datum->setAttribute(mm_weight, A2U(buf));

                    dbl2str(buf, x_stddev.get(w), sizeof(buf));
                    datum->setAttribute(mm_stddev, A2U(buf));

                    dbl2str(buf, x_meas.get(w), sizeof(buf));
                    datum->appendChild(doc.createTextNode(A2U(buf)));

                    data.appendChild(datum);
            }
        }
	return true;
}

bool FluxMLDocument::patchDOM_MIMS(
	DOMDocument & doc,
	MGroupMIMS const & mgmims,
	DOMElement & data
	)
{ 
	size_t w;
	char buf[32];
	MVector x_meas;
	MVector x_stddev;
	DOMElement * datum;
        
        std::set<double> times = mgmims.getTimeStampSet();
        std::set<double>::const_iterator t;
        for(t= times.begin(); t!= times.end(); ++t)
        {
            if (not mgmims.getMValuesStdDev(*t, x_meas, x_stddev))
            {
                    fERROR("failed to retrieve measurement values for measurement group %s",
                            mgmims.getGroupId());
                    return false;
            }
            
            const std::vector<int *> & weights_vec = mgmims.getWeights();
            std::vector<int *>::const_iterator wi;
            
            for (w=0; w<mgmims.getDim(); w++)
            {
                    datum = doc.createElementNS(fml_xmlns_uri,mm_datum);
                    datum->setAttribute(fml_id, A2U(mgmims.getGroupId()));

                    if(*t!=-1)
                    {
                        dbl2str(buf, *t, sizeof(buf));
                        datum->setAttribute(mm_time, A2U(buf));
                    }
                    
                    std::ostringstream ostr;
                    for(wi=weights_vec.begin();wi!=weights_vec.end();wi++)
                        ostr<< (wi!=weights_vec.begin()?",":"") <<(*wi)[w]; 

                    snprintf(buf, sizeof(buf), "%s",
                            ostr.str().c_str()
                            );
                    
                    datum->setAttribute(mm_weight, A2U(buf));

                    dbl2str(buf, x_stddev.get(w), sizeof(buf));
                    datum->setAttribute(mm_stddev, A2U(buf));

                    dbl2str(buf, x_meas.get(w), sizeof(buf));
                    datum->appendChild(doc.createTextNode(A2U(buf)));

                    data.appendChild(datum);
            }
        }
	return true;
}

bool FluxMLDocument::patchDOM_Generic(
	DOMDocument & doc,
	MGroupGeneric const & mgg,
	DOMElement & data
	)
{
	size_t r;
	char buf[32];
	MVector x_meas;
	MVector x_stddev;
	DOMElement * datum;
	
	if (mgg.getNumRows() > 255)
	{
		fERROR("number of measurement values exceeds 256");
		return false;
	}
        
	std::set<double> times = mgg.getTimeStampSet();
        std::set<double>::const_iterator t;
        for(t= times.begin(); t!= times.end(); ++t)
        {
            if (not mgg.getMValuesStdDev(*t, x_meas, x_stddev))
            {
                    fERROR("failed to retrieve measurement values for measurement group %s",
                            mgg.getGroupId());
                    return false;
            }

            for (r=0; r<mgg.getNumRows(); r++)
            {
                    datum = doc.createElementNS(fml_xmlns_uri,mm_datum);
                    datum->setAttribute(fml_id, A2U(mgg.getGroupId()));

                    if(*t!=-1)
                    {
                        dbl2str(buf, *t, sizeof(buf));
                        datum->setAttribute(mm_time, A2U(buf));
                    }
                    
                    snprintf(buf, sizeof(buf), "%u", (unsigned) r + 1);
                    datum->setAttribute(mm_row, A2U(buf));

                    dbl2str(buf, x_stddev.get(r), sizeof(buf));
                    datum->setAttribute(mm_stddev, A2U(buf));

                    dbl2str(buf, x_meas.get(r), sizeof(buf));
                    datum->appendChild(doc.createTextNode(A2U(buf)));

                    data.appendChild(datum);
            }
        }
	return true;
}

bool FluxMLDocument::patchDOM_1HNMR(
	DOMDocument & doc,
	MGroup1HNMR const & mg1hnmr,
	DOMElement & data
	)
{
	char buf[32];
	size_t p;
	MVector x_meas;
	MVector x_stddev;
	DOMElement * datum;
	
	std::set<double> times = mg1hnmr.getTimeStampSet();
        std::set<double>::const_iterator t;
        for(t= times.begin(); t!= times.end(); ++t)
        {
            if (not mg1hnmr.getMValuesStdDev(*t, x_meas, x_stddev))
            {
                    fERROR("failed to retrieve measurement values for measurement group %s",
                            mg1hnmr.getGroupId());
                    return false;
            }

            for (p=0; p<mg1hnmr.getNumPositions(); p++)
            {
                    datum = doc.createElementNS(fml_xmlns_uri,mm_datum);
                    datum->setAttribute(fml_id, A2U(mg1hnmr.getGroupId()));

                    if(*t!=-1)
                    {
                        dbl2str(buf, *t, sizeof(buf));
                        datum->setAttribute(mm_time, A2U(buf));
                    }
                    
                    snprintf(buf, sizeof(buf), "%d", mg1hnmr.getPositions()[p]);
                    datum->setAttribute(mm_pos, A2U(buf));

                    dbl2str(buf, x_stddev.get(p), sizeof(buf));
                    datum->setAttribute(mm_stddev, A2U(buf));

                    dbl2str(buf, x_meas.get(p), sizeof(buf));
                    datum->appendChild(doc.createTextNode(A2U(buf)));

                    data.appendChild(datum);
            }
        }
	return true;
}


bool FluxMLDocument::patchDOM_13CNMR(
	DOMDocument & doc,
	MGroup13CNMR const & mg13cnmr,
	DOMElement & data
	)
{
	size_t p;
	char buf[32];
	MVector x_meas;
	MVector x_stddev;

        std::set<double> times = mg13cnmr.getTimeStampSet();
        std::set<double>::const_iterator t;
        for(t= times.begin(); t!= times.end(); ++t)
        {
            if (not mg13cnmr.getMValuesStdDev(*t, x_meas, x_stddev))
            {
                    fERROR("failed to retrieve measurement values for measurement group %s",
                            mg13cnmr.getGroupId());
                    return false;
            }

            for (p=0; p<mg13cnmr.getNumPositions(); p++)
            {
                    DOMElement* datum = doc.createElementNS(fml_xmlns_uri,mm_datum);
                    datum->setAttribute(fml_id, A2U(mg13cnmr.getGroupId()));
                    
                    if(*t!=-1)
                    {
                        dbl2str(buf, *t, sizeof(buf));
                        datum->setAttribute(mm_time, A2U(buf));
                    }
                    
                    snprintf(buf, sizeof(buf), "%d", mg13cnmr.getPositions()[p]);
                    datum->setAttribute(mm_pos, A2U(buf));
                    dbl2str(buf, x_stddev.get(p), sizeof(buf));
                    datum->setAttribute(mm_stddev, A2U(buf));

                    buf[1] = 0; buf[2] = 0;
                    switch (mg13cnmr.getNMRTypes()[p])
                    {
                    case MValue13CNMR::S:
                            buf[0]  = 'S';
                            break;
                    case MValue13CNMR::DL:
                            buf[0] = 'D'; buf[1] = 'L';
                            break;
                    case MValue13CNMR::DR:
                            buf[0] = 'D'; buf[1] = 'R';
                            break;
                    case MValue13CNMR::DD:
                            buf[0] = 'D'; buf[1] = 'D';
                            break;
                    case MValue13CNMR::T:
                            buf[0]  = 'T';
                            break;
                    }
                    datum->setAttribute(mm_type, A2U(buf));
                    dbl2str(buf, x_meas.get(p), sizeof(buf));
                    datum->appendChild(doc.createTextNode(A2U(buf)));

                    data.appendChild(datum);
            }
        }
	return true;
}

bool FluxMLDocument::patchDOM(
	DOMDocument & doc,
	MGroup const & MG,
	char const * cfg_name
	)
{
	XMLSize_t i,j,k,d;
	DOMNodeList * configuration_list;
	DOMElement * conf_node = 0;

	// 1. aus doc "variables" mit name aus variable cfg suchen
	// <configuration>
	configuration_list = doc.getElementsByTagName(fml_configuration);
	for (i=0; i<configuration_list->getLength(); i++)
	{
		if (configuration_list->item(i)->getNodeType() != DOMNode::ELEMENT_NODE)
			continue;

		conf_node = dynamic_cast< DOMElement* >(configuration_list->item(i));

		if (not XMLString::equals(conf_node->getAttribute(fml_name),A2U(cfg_name)))
			continue;

		// <measurement>
		DOMNodeList * meas_list =
			conf_node->getElementsByTagName(fml_measurement);

		for (j=0; j<meas_list->getLength(); j++)
		{
			if (meas_list->item(j)->getNodeType() != DOMNode::ELEMENT_NODE)
				continue;

			DOMElement * measurement = dynamic_cast< DOMElement * >(
					meas_list->item(j)
					);
			DOMNodeList * data_list = measurement->getElementsByTagName(mm_data);
			for (d=0; d<data_list->getLength(); d++)
			{
				if (data_list->item(d)->getNodeType() != DOMNode::ELEMENT_NODE)
					continue;

				// <data>
				DOMElement * data = dynamic_cast< DOMElement * >(
						data_list->item(d)
						);
				DOMNodeList * datum_list = data->getElementsByTagName(
						mm_datum
						);

				for (k=datum_list->getLength(); k>0; k--)
				{
					if (datum_list->item(k-1)->getNodeType() != DOMNode::ELEMENT_NODE)
						continue;
					
					DOMElement * datum = dynamic_cast< DOMElement* >(datum_list->item(k-1));
					if (not XMLString::equals(
						datum->getAttribute(fml_id),
						A2U(MG.getGroupId()))) continue;
					data->removeChild(datum);
				}

				// neue datum-Elemente anlegen anlegen
				switch (MG.getType())
				{
				case MGroup::mg_GENERIC:
					if (not patchDOM_Generic(
						doc,
						dynamic_cast< MGroupGeneric const & >(MG),
						*data)) return false;
					break;
				case MGroup::mg_MS:
					if (not patchDOM_MS(
						doc,
						dynamic_cast< MGroupMS const & >(MG),
						*data)) return false;
					break;
				case MGroup::mg_MSMS:
					if (not patchDOM_MSMS(
						doc,
						dynamic_cast< MGroupMSMS const & >(MG),
						*data)) return false;
					break;
                                case MGroup::mg_MIMS:
					if (not patchDOM_MIMS(
						doc,
						dynamic_cast< MGroupMIMS const & >(MG),
						*data)) return false;
					break;
				case MGroup::mg_1HNMR:
					if (not patchDOM_1HNMR(
						doc,
						dynamic_cast< MGroup1HNMR const & >(MG),
						*data)) return false;
					break;
				case MGroup::mg_13CNMR:
					if (not patchDOM_13CNMR(
						doc,
						dynamic_cast< MGroup13CNMR const & >(MG),
						*data)) return false;
					break;
				
                                default: // FLUX und POOL
					{
                                            MVector x_meas;
                                            MVector x_stddev;
                                            DOMElement * datum;
                                            char buf[32];
                                            
                                            datum = doc.createElementNS(fml_xmlns_uri,mm_datum);
                                            datum->setAttribute(fml_id, A2U(MG.getGroupId()));
                                            if (not MG.getMValuesStdDev(-1., x_meas, x_stddev))
                                            {
                                                    fERROR("failed to retrieve measurement values for measurement group %s",
                                                            MG.getGroupId());
                                                    return false;
                                            }
                                            dbl2str(buf, x_stddev.get(0), sizeof(buf));
                                            datum->setAttribute(mm_stddev, A2U(buf));
                                            dbl2str(buf, x_meas.get(0), sizeof(buf));
                                            datum->appendChild(doc.createTextNode(A2U(buf)));

                                            data->appendChild(datum);                                                                                        
					}
				} // switch (MG.getType())
                                data->appendChild(doc.createComment(xercesc::XMLString::transcode("== new datum ==")));
			}
		}
	}

	return conf_node;
}

} // namespace flux::xml
} // namespace flux

