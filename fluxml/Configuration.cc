#include <algorithm>
#include "Error.h"
#include "Combinations.h"
#include "MMatrix.h"
#include "MVector.h"
#include "MMatrixOps.h"
#include "StoichMatrixInteger.h"
#include "Constraint.h"
#include "ConstraintSystem.h"
#include "Configuration.h"
#include "MMDocument.h"

using namespace flux::la;
using namespace flux::symb;

namespace flux {
namespace data {

Configuration::Configuration(Configuration const & copy)
	: name_(0), comment_(0),
	  constraint_eq_(copy.constraint_eq_),
	  constraint_ineq_(copy.constraint_ineq_),
	  sim_atom_patterns_(copy.sim_atom_patterns_),
	  sim_unknown_patterns_(copy.sim_unknown_patterns_),
	  sim_opt_free_fluxes_net_(copy.sim_opt_free_fluxes_net_),
	  sim_opt_free_fluxes_xch_(copy.sim_opt_free_fluxes_xch_),
	  sim_opt_free_poolsizes_(copy.sim_opt_free_poolsizes_),
	  mmdoc_(0), CS_(0), validation_state_(copy.validation_state_),
	  sim_type_(copy.sim_type_), sim_method_(copy.sim_method_),
	  is_stationary_(copy.is_stationary_),
	  generate_graph_flag(copy.generate_graph_flag)
{
	if (copy.name_)
		name_ = strdup_alloc(copy.name_);
	if (copy.comment_)
		comment_ = strdup_alloc(copy.comment_);
	
	std::list< InputPool* >::const_iterator i;
	for (i=copy.input_pools_.begin(); i!=copy.input_pools_.end(); i++)
	{
		InputPool const * I = *i;
		if (I) input_pools_.push_back(new InputPool(*I));
	}
	
	if (copy.mmdoc_) mmdoc_ = new xml::MMDocument(*(copy.mmdoc_));
	
	if (copy.CS_) CS_ = new ConstraintSystem(*(copy.CS_));
}

Configuration::~Configuration()
{
	std::list< InputPool* >::iterator i;
	for (i=input_pools_.begin(); i!=input_pools_.end(); i++)
//            if((*i)->hasInputProfile()==false)
                delete *i;
	if (CS_) delete CS_;
	// das Messmodell gehört zur Konfiguration:
	if (mmdoc_) delete mmdoc_;
	if (name_) delete[] name_;
	if (comment_) delete[] comment_;
}

bool Configuration::addFreeFluxNet(
	char const * fname,
	bool has_lo, double lo,
	bool has_inc, double inc,
	bool has_hi, double hi,
	bool has_start, double start,
	bool has_edweight, double edweight
	)
{
	if (sim_opt_free_fluxes_net_.find(fname))
		return false;
	
	if (has_lo and has_hi and lo>hi)
		return false;
	
	FreeFluxCfg ffcfg;
	ffcfg.has_lo = has_lo;
	ffcfg.has_inc = has_inc;
	ffcfg.has_hi = has_hi;
	ffcfg.has_value = has_start;
	ffcfg.has_edweight = has_edweight;
	ffcfg.lo = lo;
	ffcfg.inc = inc;
	ffcfg.hi = hi;
	ffcfg.value = has_start?start:0.;
	ffcfg.edweight = has_edweight?edweight:1.;
	sim_opt_free_fluxes_net_[fname] = ffcfg;

	/* lo und hi erzeugen KEINE zusätzlichen Constraints!
	if (has_lo)
	{
		ExprTree * ieq = ExprTree::geq(ExprTree::sym(fname),ExprTree::val(lo));
		createConstraint("lower bound" ,ieq, true);
		delete ieq;
	}
	if (has_hi)
	{
		ExprTree * ieq = ExprTree::leq(ExprTree::sym(fname),ExprTree::val(hi));
		createConstraint("upper bound" ,ieq, true);
		delete ieq;
	}
	*/
	return true;
}

bool Configuration::addFreeFluxXch(
	char const * fname,
	bool has_lo, double lo,
	bool has_inc, double inc,
	bool has_hi, double hi,
	bool has_start, double start,
	bool has_edweight, double edweight
	)
{
	if (sim_opt_free_fluxes_xch_.find(fname))
		return false;
	
	if (has_lo and has_hi and lo>hi)
		return false;

	FreeFluxCfg ffcfg;
	ffcfg.has_lo = has_lo;
	ffcfg.has_inc = has_inc;
	ffcfg.has_hi = has_hi;
	ffcfg.has_value = has_start;
	ffcfg.has_edweight = has_edweight;
	ffcfg.lo = lo;
	ffcfg.inc = inc;
	ffcfg.hi = hi;
	ffcfg.value = has_start?start:0.;
	ffcfg.edweight = has_edweight?edweight:1.;
	sim_opt_free_fluxes_xch_[fname] = ffcfg;
	
	/* lo und hi erzeugen KEINE zusätzlichen Constraints!
	if (has_lo)
	{
		ExprTree * ieq = ExprTree::geq(ExprTree::sym(fname),ExprTree::val(lo));
		createConstraint("lower bound" ,ieq, false);
		delete ieq;
	}
	if (has_hi)
	{
		ExprTree * ieq = ExprTree::leq(ExprTree::sym(fname),ExprTree::val(hi));
		createConstraint("upper bound" ,ieq, false);
		delete ieq;
	}
	*/
	return true;
}

bool Configuration::addFreePoolSize(
	char const * pname,
	bool has_lo, double lo,
	bool has_inc, double inc,
	bool has_hi, double hi,
	bool has_start, double start,
        bool has_edweight, double edweight
	)
{
	if (sim_opt_free_poolsizes_.find(pname))
		return false;
	FreePoolsizeCfg pcfg;
	pcfg.has_lo = has_lo;
	pcfg.has_inc = has_inc;
	pcfg.has_hi = has_hi;
	pcfg.has_value = has_start;
        pcfg.has_edweight = has_edweight;
	pcfg.lo = lo;
	pcfg.inc = inc;
	pcfg.hi = hi;
	pcfg.value = has_start?start:0.;
        pcfg.edweight = has_edweight?edweight:1.;
	sim_opt_free_poolsizes_[pname] = pcfg;
        
        // Setzt die untere Grenze der Poolgrößen
        symb::ExprTree * ieq = symb::ExprTree::geq(symb::ExprTree::sym(pname),symb::ExprTree::val(1.e-5));
        createConstraint("anonymous" ,ieq, data::POOL);
        delete ieq;
	return true;
}

void Configuration::addSubsetSimAtomPattern(
	char const * pool,
	BitArray const & pattern
	)
{
	fASSERT( sim_type_ == simt_explicit or sim_type_ == simt_auto );

	// Jedes neue Atom-Muster für einen Pool wird mit dem bereits
	// vorhandenen Muster ver-oder-t.
	sim_atom_patterns_[pool] |= pattern;

	// umgekehrt reserviert jede Atom-Maske mit n aktiven Positionen
	// 2^n unbekannte für die Simulation. Das 0-Cumomer/0-EMU wird
	// ausgelassen.
	//
	// Salopp gesagt: Das das range-Attribut des obj-Elements in FluxML
	// hat einen "großen Effekt", das cfg-Attribut hat einen "kleinen
	// Effekt".
	int n = (1<<pattern.countOnes())-1;
	std::set< BitArray > & uset = sim_unknown_patterns_[pool];
	BitArray unknown(pattern.size());
	do
	{
		unknown.incMask(pattern);
		uset.insert(unknown);
	}
	while (--n);
}

void Configuration::addSubsetSimUnknownPattern(
	char const * pool,
	BitArray const & pattern
	)
{
	fASSERT( sim_type_ == simt_explicit or sim_type_ == simt_auto );

	// Muster von Unbekannten werden einfach so eingefügt
	sim_unknown_patterns_[pool].insert(pattern);
	// ein eingefügtes Muster erweitert möglicherweise die Menge
	// der Atom-Positionen:
	sim_atom_patterns_[pool] |= pattern;
}

Configuration::SimulationMethod Configuration::determineBestSimMethod() const
{
	// falls es kein Messmodell gibt: Cumomer
	if (mmdoc_ == 0)
		return simm_Cumomer;
	
	size_t r, n_unk_EMU = 0, n_unk_CUMO = 0;
	charptr_array mgnames = mmdoc_->getGroupNames();
	charptr_array::const_iterator mgi;

	for (mgi=mgnames.begin(); mgi!=mgnames.end(); ++mgi)
	{
		xml::MGroup const * G = mmdoc_->getGroupByName(*mgi);
		xml::MetaboliteMGroup const * mG
			= dynamic_cast< xml::MetaboliteMGroup const * >(G);
		xml::MGroupGeneric const * gG
			= dynamic_cast< xml::MGroupGeneric const * >(G);

		if (mG)
		{
			n_unk_EMU += mG->getSimSet(xml::MetaboliteMGroup::sdt_emu).size();
			n_unk_CUMO += mG->getSimSet(xml::MetaboliteMGroup::sdt_cumomer).size();
		}
		else if (gG)
		{
			for (r=0; r<gG->getNumRows(); ++r)
			{
				charptr_array vn = gG->getVarNames(r);
				charptr_array::const_iterator vni;
				for (vni=vn.begin(); vni!=vn.end(); ++vni)
				{
					mG = gG->getSubGroup(*vni,r);
					fASSERT(mG != 0);

					n_unk_EMU += mG->getSimSet(xml::MetaboliteMGroup::sdt_emu).size();
					n_unk_CUMO += mG->getSimSet(xml::MetaboliteMGroup::sdt_cumomer).size();
				}
			}
		}
	}

	fDEBUG(0,"simulation of meas. requires %i essential cumomers / %i essential EMUs",
		int(n_unk_CUMO), int(n_unk_EMU));

	if (n_unk_EMU < n_unk_CUMO)
		return simm_EMU;
	return simm_Cumomer;
}
	
void Configuration::linkMMDocument(xml::MMDocument * mmdoc)
{
	mmdoc_ = mmdoc;
}

void Configuration::validate(
	const la::StoichMatrixInteger * stoich_matrix
	)
{
	fINFO("validating configuration \"%s\" ...", name_);

	// Validierung der Input-Pools:
	validation_state_ = cfg_unvalidated;
	std::list< InputPool* >::iterator ipi = input_pools_.begin();
	while (ipi != input_pools_.end())
	{
		if (not (*ipi)->finish())
		{
			// Falls es einen Fehler in den Input-Pools gab,
			// macht eine Fortsetzung der Validierung keinen
			// Sinn:
			validation_state_ = cfg_invalid_substrate;
			fERROR("validation of substrate \"%s\" failed",
				(*ipi)->getName());
			return;
		}
		ipi++;
	}
        
	// Erzeugen einer linearen Constraint-Matrix aus der Stöchiometrie,
	// den (linearen) constraint-Gleichungen und den Fluß/Poolgröße-Werten
	// (Constraint-Flüsse Netto/Exchange, Freie Flüsse und freie Poolgrößen).
	fASSERT( CS_ == 0 );
	fDEBUG(0,"building and evaluating the constraint system ...");
	CS_ = new ConstraintSystem(
		*stoich_matrix,
		sim_opt_free_fluxes_net_.getKeys(),
		sim_opt_free_fluxes_xch_.getKeys(),
                sim_opt_free_poolsizes_.getKeys(),
		constraint_eq_,
		constraint_ineq_,
                is_stationary_ /*,
                this */
		);

	switch (CS_->getValidationState())
	{
	case ConstraintSystem::cm_ok:
	case ConstraintSystem::cm_too_few_constr:
		{
		charptr_map< FreeFluxCfg >::iterator ffi;
		for (ffi = sim_opt_free_fluxes_net_.begin();
			ffi != sim_opt_free_fluxes_net_.end(); ffi++)
		{
			if (ffi->value.has_value)
				CS_->setNetFlux(ffi->key,ffi->value.value);
		}
		
		for (ffi = sim_opt_free_fluxes_xch_.begin();
			ffi != sim_opt_free_fluxes_xch_.end(); ffi++)
		{
			if (ffi->value.has_value)
				CS_->setXchFlux(ffi->key,ffi->value.value);
		}

                charptr_map< FreePoolsizeCfg >::iterator psi;
		for (psi = sim_opt_free_poolsizes_.begin();
			psi != sim_opt_free_poolsizes_.end(); psi++)
		{
			if (psi->value.has_value)
				CS_->setPoolSize(psi->key,psi->value.value);
		}

		CS_->eval();
		}
		break;
	default:
		break;
	}

	switch (CS_->getValidationState())
	{
	case ConstraintSystem::cm_ok:
		// Validierung erfolgreich abgeschlossen
		validation_state_ = cfg_ok;
		fINFO("validation succeeded!");
		break;
	case ConstraintSystem::cm_unvalidated:
		// Validierung noch nicht abgeschlossen
		validation_state_ = cfg_unvalidated;
		fWARNING("still unvalidated?!");
		break;
	case ConstraintSystem::cm_too_few_constr:
		// zu wenige Constraints / freie Variablen
		validation_state_ = cfg_too_few_constr;
		fINFO("you may provide additional constraints");
		break;
	case ConstraintSystem::cm_too_many_constr:
		// zu viele Constraints / freie Variablen
		validation_state_ = cfg_too_much_constr;
		fERROR("there are too many constraints");
		break;
	case ConstraintSystem::cm_linear_dep_constr:
		// linear abhängige Constraints
		validation_state_ = cfg_linear_dep_constr;
		fWARNING("linear dependencies between constraints");
		break;
	case ConstraintSystem::cm_nonlinear_constr:
		// ein Constraint ist anscheinend nicht-linear
		validation_state_ = cfg_nonlinear_constr;
		fERROR("(probably) nonlinear constraints detected!");
		break;
	case ConstraintSystem::cm_invalid_constr:
		// ein Constraint ist ungültig / falscher Reakt.-Name
		validation_state_ = cfg_invalid_constr;
		fERROR("invalid constraints detected!");
		break;
	case ConstraintSystem::cm_too_many_free_vars:
		// es wurden zu viele Variablen als frei deklariert
		validation_state_ = cfg_too_much_free_vars;
		fERROR("too many variables are defined to be free!");
		break;
	case ConstraintSystem::cm_invalid_free_vars:
		// freie Variablen kollidieren mit Constraints
		validation_state_ = cfg_invalid_free_vars;
		fERROR("Error: invalid choice of free variables (in conflict with constraints)!");
		break;
	case ConstraintSystem::cm_ineqs_violated:
		// die Flusswerte verletzen die Ungleichungen
		validation_state_ = cfg_ineqs_violated;
		fWARNING("some inequalities are violated");
		break;
	case ConstraintSystem::cm_ineqs_infeasible:
		// Ungleichungen haben keine Lösung
		validation_state_ = cfg_ineqs_infeasible;
		fERROR("infesible inequalities!");
	}

	// soweit keine größeren Probleme auftreten erfolgt jetzt eine
	// Ausgabe:
	if (isValid(2))
	{
		fINFO("using the following stoichiometry ...");
		CS_->dump();
	}
	else
	{
		fWARNING("validation failed!");
	}

	// Für Simulationstyp "auto" (sim_type_ == simt_auto) werden die
	// Simulationsmuster aus den Messmodellspezifikationen generiert
	if (isValid(3) and (sim_type_ == simt_auto or sim_type_ == simt_explicit))
	{
		// im "Automatikmodus" ist es in FluxML unmöglich separate
		// simulationsmuster anzugeben:
		fASSERT(sim_atom_patterns_.size() == 0);
		fASSERT(sim_unknown_patterns_.size() == 0);

		fINFO("tayloring simulation for measurement specifications ...");

		switch (sim_method_)
		{
		case simm_Cumomer:
			generateSimPatternsFromMeasurementSpecs_Cumomer();
			break;
		case simm_EMU:
			generateSimPatternsFromMeasurementSpecs_EMU();
			break;
		}
	} // if (isValid(3) and sim_type_ == simt_auto)
} // validate()

void Configuration::generateSimPatternsFromMeasurementSpecs_Cumomer()
{
	// Konfiguration hat kein Messmodell
	if (mmdoc_ == 0)
		return;

	charptr_array mgnames = mmdoc_->getGroupNames();
	charptr_array::const_iterator mgni;
	for (mgni=mgnames.begin(); mgni!=mgnames.end(); ++mgni)
	{
		xml::MGroup * G = mmdoc_->getGroupByName(*mgni);
		xml::MetaboliteMGroup * MG = 0;
		xml::MGroupGeneric * GG = 0;
		std::set< BitArray > S;
		std::set< BitArray >::iterator si;
		char * shortSpec;
		
		fASSERT(G != 0);
		
		switch (G->getType())
		{
		case xml::MGroup::mg_MS:
		case xml::MGroup::mg_MIMS:
		case xml::MGroup::mg_MSMS:
		case xml::MGroup::mg_1HNMR:
		case xml::MGroup::mg_13CNMR:
		case xml::MGroup::mg_CUMOMER:
			MG = dynamic_cast< xml::MetaboliteMGroup* >(G);
			S = MG->getSimSet(
				xml::MetaboliteMGroup::sdt_cumomer);
			
			if (GETLOGLEVEL() >= logDEBUG)
			{
				shortSpec = strdup_alloc(G->getSpec());
				strtrunc_inplace(shortSpec,25);
				fDEBUG(0,"allocating CUMOMERs for meas.group \"%s\" (%s)",
					G->getGroupId(), shortSpec);
				delete[] shortSpec;
			}
                        
			for (si=S.begin(); si!=S.end(); ++si)
				addSubsetSimUnknownPattern(MG->getMetaboliteName(),*si);
			break;
		case xml::MGroup::mg_GENERIC:
			{
			GG = dynamic_cast< xml::MGroupGeneric* >(G);
			
			if (GETLOGLEVEL() >= logDEBUG)
			{
				shortSpec = strdup_alloc(G->getSpec());
				strtrunc_inplace(shortSpec,20);
				fDEBUG(0,"allocating CUMOMERs for meas.group \"%s\" (%s)",
					G->getGroupId(), shortSpec);
				delete[] shortSpec;
			}

			size_t r;
			for (r=0; r<GG->getNumRows(); ++r)
			{
				charptr_array vn = GG->getVarNames(r);
				charptr_array::const_iterator vni;
				for (vni=vn.begin(); vni!=vn.end(); ++vni)
				{
					MG = GG->getSubGroup(*vni,r);
					if (MG == 0)
						continue;
					S = MG->getSimSet(
						xml::MetaboliteMGroup::sdt_cumomer);
					for (si=S.begin(); si!=S.end(); ++si)
						addSubsetSimUnknownPattern(MG->getMetaboliteName(), *si);
				}
			}
			}
			break;
		default:
			break;
		}
	}
} // generateSimPatternsFromMeasurementSpecs_Cumomer()

void Configuration::generateSimPatternsFromMeasurementSpecs_EMU()
{
	// Konfiguration hat kein Messmodell
	if (mmdoc_ == 0)
		return;
	
	charptr_array mgnames = mmdoc_->getGroupNames();
	charptr_array::const_iterator mgni;
	for (mgni=mgnames.begin(); mgni!=mgnames.end(); ++mgni)
	{
		xml::MGroup * G = mmdoc_->getGroupByName(*mgni);
		xml::MetaboliteMGroup * MG = 0;
		xml::MGroupGeneric * GG = 0;
		std::set< BitArray > S;
		std::set< BitArray >::iterator si;
		fASSERT(G != 0);
		fDEBUG(0,"allocating EMUs for measurement group \"%s\" (%s)",
			G->getGroupId(), G->getSpec());
		switch (G->getType())
		{
		case xml::MGroup::mg_MS:
		case xml::MGroup::mg_MIMS:
		case xml::MGroup::mg_MSMS:
		case xml::MGroup::mg_1HNMR:
		case xml::MGroup::mg_13CNMR:
		case xml::MGroup::mg_CUMOMER:
			MG = dynamic_cast< xml::MetaboliteMGroup* >(G);
			S = MG->getSimSet(
				xml::MetaboliteMGroup::sdt_emu);
			for (si=S.begin(); si!=S.end(); ++si)
				addSubsetSimUnknownPattern(MG->getMetaboliteName(),*si);
			break;
		case xml::MGroup::mg_GENERIC:
			{
				GG = dynamic_cast< xml::MGroupGeneric* >(G);
				size_t r;
				for (r=0; r<GG->getNumRows(); ++r)
				{
					charptr_array vn = GG->getVarNames(r);
					charptr_array::const_iterator vni;
					for (vni=vn.begin(); vni!=vn.end(); ++vni)
					{
						MG = GG->getSubGroup(*vni,r);
						if (MG == 0)
							continue;
						S = MG->getSimSet(
							xml::MetaboliteMGroup::sdt_emu);
						for (si=S.begin(); si!=S.end(); ++si)
							addSubsetSimUnknownPattern(MG->getMetaboliteName(),*si);
					}
				}
			}
			break;
		default:
			break;
		}
	}
} // generateSimPatternsFromMeasurementSpecs_EMU()

bool Configuration::createConstraint(
	char const * name,
	ExprTree const * constraint,
        ParameterType parameter_type
	)
{
	// Constraint auf Linearität prüfen
	if (not Constraint::checkLinearity(constraint))
	{
		fWARNING("constraint %s ist not a linear (in)equality",
			constraint->toString().c_str());
		return false;
	}
        Constraint C(name,constraint,parameter_type);
	std::list< Constraint > & cList =
		constraint->isEquality() ? constraint_eq_ : constraint_ineq_;

	// Versucht der Benutzer widersprüchliche (einfache) Constraints einzugeben?
	if (C.isSimple() and IS_OP_EQ(C.getConstraint()))
	{
		std::list< Constraint >::const_iterator ccli =
			cList.begin();
		while (ccli != cList.end())
		{
                        if (ccli->isSimple() and (ccli->getParameterType()==parameter_type))
			{
				if (strcmp(C.getSimpleVarName(),ccli->getSimpleVarName())==0)
				{
					// zwei einfache Constraints, die für eine
					// Variable zwei
					if (IS_OP_EQ(ccli->getConstraint())
						and C.getSimpleVarValue() != ccli->getSimpleVarValue())
					{
						switch(parameter_type)
						{
							case NET:
								fWARNING("%s-constraint %s=%f violates previous constraint %s=%f",
									"net",
									C.getSimpleVarName(),C.getSimpleVarValue(),
									ccli->getSimpleVarName(),ccli->getSimpleVarValue()
									);
								break;
							case XCH:
								fWARNING("%s-constraint %s=%f violates previous constraint %s=%f",
									"xch",
									C.getSimpleVarName(),C.getSimpleVarValue(),
									ccli->getSimpleVarName(),ccli->getSimpleVarValue()
									);
								break;
							case POOL:
								fWARNING("%s-constraint %s=%f violates previous constraint %s=%f",
									"pool",
									C.getSimpleVarName(),C.getSimpleVarValue(),
									ccli->getSimpleVarName(),ccli->getSimpleVarValue()
									);
								break;
						}
						return false;
					}
				}
			}
			ccli++;
		}
	}

	// Constraint bereits registriert?
	std::list< Constraint >::iterator cli =
		std::find(cList.begin(),cList.end(),C);
	if (cli == cList.end())
		cList.push_back(C);
	else
        {
		switch(parameter_type)
		{
			case NET:
				fWARNING("skipping duplicate %s flux constraint \"%s\" (%s)",
					"net",
					name, constraint->toString().c_str());
				break;
			case XCH:
				fWARNING("skipping duplicate %s flux constraint \"%s\" (%s)",
					"exchange",
					name, constraint->toString().c_str());
				break;
			case POOL:
				fWARNING("skipping duplicate %s poolsize constraint \"%s\" (%s)",
					"exchange",
					name, constraint->toString().c_str());
				break;
		};
            
        }
	return true;
}

void Configuration::mergeConstraints(
	Configuration const & root_cfg
	)
{
	std::list< Constraint >::const_iterator ci,cj;
	for (ci = root_cfg.constraint_eq_.begin();
		ci != root_cfg.constraint_eq_.end(); ci++)
	{
		cj = std::find(constraint_eq_.begin(),constraint_eq_.end(),*ci);
		if (cj == constraint_eq_.end())
			constraint_eq_.push_back(*ci);
	}
	for (ci = root_cfg.constraint_ineq_.begin();
		ci != root_cfg.constraint_ineq_.end(); ci++)
	{
		cj = std::find(constraint_ineq_.begin(),constraint_ineq_.end(),*ci);
		if (cj == constraint_ineq_.end())
			constraint_ineq_.push_back(*ci);
	}
}

symb::ExprTree * Configuration::getSymbolicPoolFluxValue(
	char const * name,
	char coord_type,
	bool formula
	) const
{
	fASSERT( CS_ != 0 );
	switch (coord_type)
	{
	case 'n':
	case 'N':
		{
		symb::ExprTree * ana_net = CS_->getSymbolicPoolFlux(name,NET,formula);
		symb::LinearExpression lE(ana_net);
		delete ana_net;
		return lE.get()->clone();
		}
	case 'x':
	case 'X':
		{
		symb::ExprTree * ana_xch = CS_->getSymbolicPoolFlux(name,XCH,formula);
		symb::LinearExpression lE(ana_xch);
		delete ana_xch;
		return lE.get()->clone();
		}
	case 'f':
	case 'F':
		return CS_->getSymbolicFluxFwdBwd(name,true,formula);
	case 'b':
	case 'B':
		return CS_->getSymbolicFluxFwdBwd(name,false,formula);
	case 'p':
	case 'P':
		{
		symb::ExprTree * ana_pool = CS_->getSymbolicPoolFlux(name,POOL,formula);
		symb::LinearExpression lE(ana_pool);
		delete ana_pool;
		return lE.get()->clone();
		}
	default:
		fASSERT_NONREACHABLE();
	}
	return 0;
}


symb::ExprTree * Configuration::getSymbolicPoolFluxValue(char const * name,bool formula) const
{
	size_t len = strlen(name);
	char suffix;
	char * prefix;
	
	suffix = 'p';

	if(len > 2)
	{
		if (strEndsWith(name,".n")
			or strEndsWith(name,".x")
			or strEndsWith(name,".f")
			or strEndsWith(name,".b"))
		{
			suffix = name[len-1];
		};
	};

	switch(suffix)
	{
		case 'p':
			prefix = new char[len+1];
			strncpy(prefix,name,len);
			prefix[len] = '\0';
			break;
		case 'n':
		case 'x':
		case 'f':
		case 'b':
			prefix = new char[len-2+1];
			strncpy(prefix,name,len-2);
			prefix[len-2] = '\0';
			break;
		default:
			fASSERT(false);

	}
	
	
	ExprTree * E = getSymbolicPoolFluxValue(prefix,suffix,formula);
	delete[] prefix;

	return E;
}
uint32_t Configuration::computeCheckSum(uint32_t crc, int crc_scope) const
{
	// der Konfigurationsname ist immer in der CRC-Berechnung drin
	if (name_)
		crc = update_crc32(name_,strlen(name_),crc);
	// das Kommentar nur auf Verlangen
	if (comment_ and (crc_scope & CRC_ALL_ANNOTATIONS))
		crc = update_crc32(comment_,strlen(comment_),crc);

	if (crc_scope & CRC_CFG_SUBSTRATES)
	{
		// input_pools_ in alphabetischer Reihenfolge abarbeiten
		std::list< InputPool* >::const_iterator ipi;
		charptr_map< InputPool* > ipmap;
		for (ipi=input_pools_.begin(); ipi!=input_pools_.end(); ++ipi)
			ipmap.insert((*ipi)->getName(), *ipi);
		charptr_array ipmap_keys = ipmap.getKeys();
		charptr_array::const_iterator si;
		ipmap_keys.sort();
		for (si=ipmap_keys.begin(); si!=ipmap_keys.end(); ++si)
		{
			crc = update_crc32(*si,strlen(*si),crc);
			crc = ipmap[*si]->computeCheckSum(crc, crc_scope);
		}
	}

	if (crc_scope & CRC_CONSTRAINTS)
	{
		// constraint_eq_, constraint_ineq_
		std::list< Constraint >::const_iterator cli;
		for (cli=constraint_eq_.begin(); cli!=constraint_eq_.end(); ++cli)
			crc = cli->computeCheckSum(crc,crc_scope);
		for (cli=constraint_ineq_.begin(); cli!=constraint_ineq_.end(); ++cli)
			crc = cli->computeCheckSum(crc,crc_scope);
	}
	
	// Freie Flüsse (und Werte)
	if (crc_scope & (CRC_CFG_SIM_VARIABLES|CRC_CFG_SIM_VARIABLES_DATA))
	{
		charptr_array fnet = sim_opt_free_fluxes_net_.getKeys();
		charptr_array fxch = sim_opt_free_fluxes_xch_.getKeys();
		charptr_array::const_iterator fi;
		fnet.sort();
		fxch.sort();
		for (fi=fnet.begin(); fi!=fnet.end(); ++fi)
		{
			crc = update_crc32(*fi,strlen(*fi),crc);
			if (crc_scope & CRC_CFG_SIM_VARIABLES_DATA)
			{
				FreeFluxCfg const & c = sim_opt_free_fluxes_net_[*fi];
				if (c.has_value)
					crc = update_crc32(&(c.value),sizeof(double),crc);
			}
		}
		for (fi=fxch.begin(); fi!=fxch.end(); ++fi)
		{
			crc = update_crc32(*fi,strlen(*fi),crc);
			if (crc_scope & CRC_CFG_SIM_VARIABLES_DATA)
			{
				FreeFluxCfg const & c = sim_opt_free_fluxes_xch_[*fi];
				if (c.has_value)
					crc = update_crc32(&(c.value),sizeof(double),crc);
			}

		}
	}

	// TODO: Alles abgeleitete Infos? / keine Struktur-Infos?
	// sim_atom_patterns_
	// sim_unknown_patterns_
	// opt_poolsizes_
	
	// Messmodelldokument
	if (mmdoc_)
		crc = mmdoc_->computeCheckSum(crc, crc_scope);

	if (crc_scope & CRC_CFG_SIM_SETTINGS)
	{
		int32_t sim_type_method = int32_t(sim_type_) + (int32_t(sim_method_)<<2);
		crc = update_crc32(&sim_type_method,4,crc);
	}
	return crc;
}



} // namespace flux::data
} // namespace flux


