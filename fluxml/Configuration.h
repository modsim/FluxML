#ifndef CONFIGURATION_H
#define CONFIGURATION_H

extern "C"
{
#include <stdint.h>
}
#include <string>
#include <list>
#include <set>
#include "Error.h"
#include "charptr_array.h"
#include "BitArray.h"
#include "ExprTree.h"
#include "LinearExpression.h"
#include "InputPool.h"
#include "Constraint.h"
#include "ConstraintSystem.h"
#include <sstream>

namespace flux {
namespace la { class StoichMatrixInteger; }
namespace xml { class MMDocument; }
} // namespace flux

namespace flux {
namespace data {

/*
 * *****************************************************************************
 * Eine Klasse zur Abbildung einer Konfiguration eines Stoffflußnetzwerkes.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */
    
class Configuration
{
public:
	/**
	 * Randwerte für die Optimierung von Flusswerten.
	 */
	struct FreeFluxCfg
	{
		/** Gibt es eine untere Grenze für den Flusswert? */
		bool has_lo;
		/** Untere Grenze für den Flusswert */
		double lo;
		
		/** Gibt es eine obere Grenze für den Flusswert? */
		bool has_hi;
		/** Obere Grenze für den Flusswert */
		double hi;

		/** Gibt es einen festen Increment-Wert für den Fluß? */
		bool has_inc;
		/** Fester Increment-Wert für den Fluß */
		double inc;

		/** Gibt es einen Startwert für den Fluß? */
		bool has_value;
		/** Startwert des (freien) Flusses */
		double value;

		/** Gibt es den ED Gewichtungfaktor? */
		bool has_edweight;
		/** Gewichtungsfaktor für Experimental Design */
		double edweight;
	};

	/**
	 * Randwerte für die Simulation/Optimierung mit/von Poolgrößen
	 */
	struct FreePoolsizeCfg
	{
		/** Gibt es eine untere Grenze für die Poolgröße? */
		bool has_lo;
		/** Untere Grenze für die Poolgröße */
		double lo;
		
		/** Gibt es eine obere Grenze für die Poolgröße? */
		bool has_hi;
		/** Obere Grenze für die Poolgröße */
		double hi;

		/** Gibt es einen festen Increment-Wert für die Poolgröße? */
		bool has_inc;
		/** Fester Increment-Wert für die Poolgröße */
		double inc;

		/** Gibt es einen Startwert für die Poolgröße? */
		bool has_value;
		/** Startwert der Poolgröße */
		double value;
                
                /** Gibt es den ED Gewichtungfaktor? */
		bool has_edweight;
		/** Gewichtungsfaktor für Experimental Design */
		double edweight;
	};
	
	/** Zustand/Probleme der Konfiguration */
	enum ValidationState {
		cfg_ok,			// die Konfiguration ist gültig
		cfg_unvalidated,	// die Konfiguration ist noch nicht
					//   validiert
		cfg_too_few_constr,	// es gibt zu wenige Constraints
		cfg_too_much_constr,	// es gibt zu viele Constraints
		cfg_linear_dep_constr,	// die Constraints sind anscheinend
					//   linear abhängig
		cfg_nonlinear_constr,	// ein Constraint ist anscheinend
					//   nicht-linear
		cfg_invalid_constr,	// ein Constraint ist ungültig /
					//   falscher Reakt.-Name
		cfg_invalid_substrate,	// ungültige Substratmischung
					//   (Bondomer-Netzwerke)
		cfg_too_much_free_vars,	// es wurden zu viele Variablen als
					//   frei deklariert
		cfg_invalid_free_vars,	// Auswahl freier Variablen kollidiert
					//   mit Constraints
		cfg_ineqs_violated,	// die Flusswerte verletzen die
					//   Ungleichungen
		cfg_ineqs_infeasible	// Ungleichungen besitzen keine Lösung
	};

	/** Typ der Simulation (full, auto, explicit) */
	enum SimulationType {
		simt_full,		// vollständige Simulation aller Cumos
		simt_auto,		// automatische Simulation gemäß Messmodell
		simt_explicit		// Simulation spezifizierter Pools/Cumos
	};

	/** Simulationsmethode (Cumomer, EMU) */
	enum SimulationMethod {
		simm_Cumomer,
		simm_EMU
	};

private:
	/** Name einer Konfiguration */
	char const * name_;
	/** Kommentar zu einer Konfiguration */
	char const * comment_;
	/** Eine Liste von Input-Pool-Belegungen (Obj. der Klasse InputPool) */
	std::list< InputPool* > input_pools_;
	/** Eine Liste von Fluß-Constraints (equalities) */
	std::list< Constraint > constraint_eq_;
	/** Eine Liste von Fluß-Constraints (inequalities) */
	std::list< Constraint > constraint_ineq_;
	/** Markierungsmuster (Atom-Positionen) zu simulierender Pools */
	charptr_map< BitArray > sim_atom_patterns_;
	/** Markierungsmuster zu simulierender Pools (Konfiguration von  Unbekannten) */
	charptr_map< std::set< BitArray > > sim_unknown_patterns_;
	/** Freizuhaltende Flussvariablen (netto) */
	charptr_map< FreeFluxCfg > sim_opt_free_fluxes_net_;
	/** Freizuhaltende Flussvariablen (exchange) */
	charptr_map< FreeFluxCfg > sim_opt_free_fluxes_xch_;
	/** Freizuhaltende Poolgröße-variablen */
	charptr_map< FreePoolsizeCfg > sim_opt_free_poolsizes_;
	/** Messmodell-Dokument */
	xml::MMDocument * mmdoc_;
	/** Das System der Constraints, freien- und abhängigen Flüsse */
	ConstraintSystem * CS_;
	/** Status der Validierung */
	ValidationState validation_state_;
	/** Simulations-Typ (full, auto, explicit) */
	SimulationType sim_type_;
	/** Simulations-Methode (Cumomer, EMU) */
	SimulationMethod sim_method_;
	/** Flag; Stationarität der Konfiguration */
        bool is_stationary_;
        bool generate_graph_flag;
        

public:
	/**
	 * Constructor.
	 * Erzeugt eine Konfiguration aus einer Bezeichnung, einem Kommentar,
	 * einer Liste von Input-Pool-Belegungen und einer Liste von
	 * Fluß-Einstellungen.
	 *
	 * @param name Bezeichnung der Konfiguration
	 * @param comment Kommentar zur Konfiguration
	 * @param input_pools eine Liste von Input-Pool-Belegungen
	 * @param flux_values eine Liste von Fluß-Einstellungen
	 */
	inline Configuration(
		char const * name,
		char const * comment
		) : name_(0), comment_(0), mmdoc_(0), CS_(0),
		    validation_state_(cfg_unvalidated),
		    sim_type_(simt_full), sim_method_(simm_Cumomer),
		    is_stationary_(true), generate_graph_flag(false)
	{
		name_ = strdup_alloc(name);
		comment_ = strdup_alloc(comment);
	}

	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Configuration-Objekt
	 */
	Configuration(Configuration const & copy);

	/**
	 * Destructor.
	 * Löscht Listen der InputPools und FluxValues und alle enthaltenen
	 * InputPool-Objekte.
	 */
	~Configuration();

private:
	/**
	 * Generiert Simulationsmuster aus Messgruppenspezifikationen.
	 * Es wird hier von einer Cumomer-Simulation ausgegangen.
	 * wird von der Validierung (validate()) aufgerufen
	 */
	void generateSimPatternsFromMeasurementSpecs_Cumomer();
	
	/**
	 * Generiert Simulationsmuster aus Messgruppenspezifikationen.
	 * Es wird hier von einer EMU-Simulation ausgegangen.
	 * wird von der Validierung (validate()) aufgerufen
	 */
	void generateSimPatternsFromMeasurementSpecs_EMU();

public:
	/**
	 * Erzeugt und Registiert ein InputPool-Objekt
	 *
	 * @param name Bezeichnung des Input-Pools
	 * @param is_cumomer Flag -- true, falls der Pool ein Cumomer-Pool ist
	 * @param natoms Anzahl der Kohlenstoffatoms der Vertreter des Pools
	 * @return neu allokiertes InputPool-Objekt
	 */
	inline InputPool * createInputPool(
		char const * name,
		int natoms,
		InputPool::Type pool_type
		)
	{
		InputPool * ipool = new InputPool(0,name,natoms,pool_type);
		input_pools_.push_back(ipool);
		return ipool;
	}

	/**
	 * Schnittstelle für den Parser in FluxMLInput.
	 * Fügt ein allokiertes InputPool-Objekt in die Pool-Liste ein.
	 * Wichtig: Der Destructor darf diese Objekte löschen!
	 *
	 * @param ipool Zeiger auf ein InputPool-Objekt
	 */
	inline void addInputPool(InputPool * ipool)
	{
		input_pools_.push_back(ipool);
	}

	/**
	 * Klassifiziert einen Ausdruck in Gleichung oder Ungleichung und
	 * erzeugt ein Constraint. Ein Zeiger auf das Constraint wird
	 * zurückgegeben, soweit es sich um eine Gleichung oder Ungleichung
	 * gehandelt hat (ansonsten 0-Zeiger).
	 *
	 * @param name Bezeichnung des Constraints
	 * @param constraint Ausdruck
	 * @param is_net true, falls es sich um ein Netto-Constraint handelt
	 */
	bool createConstraint(
		char const * name,
		symb::ExprTree const * constraint,
		ParameterType parameter_type
		);

	/**
	 * Übernimmt die Constraints einer Wurzel-Konfiguration.
	 *
	 * @param root_cfg Wurzel-Konfiguration
	 */
	void mergeConstraints(
		Configuration const & root_cfg
		);

	/**
	 * Gibt die Bezeichnung der Konfiguration zurück.
	 * 
	 * @return Bezeichnung der Konfiguration
	 */
	inline char const * getName() const { return name_; }

	/**
	 * Gibt das Kommentar zur Konfiguration zurück.
	 *
	 * @return Kommentar zur Konfiguration
	 */
	inline char const * getComment() const { return comment_; }

	/**
	 * Gibt die Liste mit den Input-Pool-Belegungen zurück.
	 *
	 * @return Liste mit Input-Pool-Belegungen (InputPool-Objekte)
	 */
	inline std::list< InputPool* > const & getInputPools() const
	{
		return input_pools_;
	}

	/**
	 * Gibt die Liste der Gleichungs-Constraints zurück.
	 *
	 * @return Liste mit Gleichungs-Constraints
	 */
	inline std::list< Constraint > const & getEqualities() const
	{
		return constraint_eq_;
	}

	/**
	 * Gibt die Liste der Ungleichungs-Constraints zurück.
	 *
	 * @return Liste mit Ungleichungs-Constraints
	 */
	inline std::list< Constraint > const & getInEqualities() const
	{
		return constraint_ineq_;
	}

	/**
	 * Deklariert einen Namen eines Netto-Flusses als frei.
	 * Die Gültigkeit des Flussnamens kann hier nicht geprüft werden.
	 *
	 * @param fname Flussbezeichnung
	 * @param has_lo true, falls der Fluß eine untere Grenze hat
	 * @param lo Wert der unteren Grenze
	 * @param has_inc true, falls der Fluß einen festen Increment-Wert hat
	 * @param inc Increment-Wert
	 * @param has_hi true, falls der Fluß eine obere Grenze hat
	 * @param hi Wert der oberen Grenze
	 * @param has_start true, falls es einen Startwert für den Fluß gibt
	 * @param start Startwert für den Fluß
	 * @param has_edweight true, falls es einen Gewichtungsfaktor für ED gibt
	 * @param edweight Gewichtungsfaktor für Experimental Design
	 * @return false, falls ein Eintrag für den Fluß bereits existierte
	 */
	bool addFreeFluxNet(
		char const * fname,
		bool has_lo = false, double lo = 0.,
		bool has_inc = false, double inc = 0.,
		bool has_hi = false, double hi = 0.,
		bool has_start = false, double start = 0.,
		bool has_edweight = false, double edweight = 0.
		);

	/**
	 * Deklariert einen Namen eines Exchange-Flusses als frei.
	 * Die Gültigkeit des Flussnamens kann hier nicht geprüft werden.
	 *
	 * @param fname Flussbezeichnung
	 * @param has_lo true, falls der Fluß eine untere Grenze hat
	 * @param lo Wert der unteren Grenze
	 * @param has_inc true, falls der Fluß einen festen Increment-Wert hat
	 * @param inc Increment-Wert
	 * @param has_hi true, falls der Fluß eine obere Grenze hat
	 * @param hi Wert der oberen Grenze
	 * @param has_start true, falls es einen Startwert für den Fluß gibt
	 * @param start Startwert für den Fluß
	 * @param has_edweight true, falls es einen Gewichtungsfaktor für ED gibt
	 * @param edweight Gewichtungsfaktor für Experimental Design
	 * @return false, falls ein Eintrag für den Fluß bereits existierte
	 */
	bool addFreeFluxXch(
		char const * fname,
		bool has_lo = false, double lo = 0.,
		bool has_inc = false, double inc = 0.,
		bool has_hi = false, double hi = 0.,
		bool has_start = false, double start = 0.,
		bool has_edweight = false, double edweight = 0.
		);

	/**
	 * Deklariert die Parameter für eine Poolgrößen-Optimierung.
	 *
	 * @param pname Pool-Bezeichnung
	 * @param has_lo true, falls die Poolgröße eine untere Grenze hat
	 * @param lo Wert der unteren Grenze
	 * @param has_inc true, falls die Poolgröße einen festen
	 * 	Increment-Wert hat
	 * @param inc Increment-Wert
	 * @param has_hi true, falls die Poolgröße eine obere Grenze hat
	 * @param hi Wert der oberen Grenze
	 * @param has_start true, falls es einen Startwert für den Pool gibt
	 * @param start Startwert für den Pool
         * * @param edweight Gewichtungsfaktor für Experimental Design
	 * @return false, falls ein Eintrag für den Pool bereits existierte
	 */
	bool addFreePoolSize(
		char const * pname,
		bool has_lo = false, double lo = 0.,
		bool has_inc = false, double inc = 0.,
		bool has_hi = false, double hi = 0.,
		bool has_start = false, double start = 0.,
                bool has_edweight = false, double edweight = 0.
		);
	
	/**
	 * Trägt ein neues Muster für ein Simulationsziel ein.
	 *
	 * @param pool Poolbezeichnung
	 * @param pattern Atom-Positions-Muster (Maske für Cumomer oder EMU)
	 */
	void addSubsetSimAtomPattern(
		char const * pool,
		BitArray const & pattern
		);
	
	/**
	 * Trägt ein Markierungsmuster (eine Unbekannte) als Simulationsziel
	 * ein.
	 *
	 * @param pool Poolbezeichnung
	 * @param pattern Markierungsmuster/Konfiguration einer Unbekannten
	 * 	(Cumomer oder EMU)
	 */
	void addSubsetSimUnknownPattern(
		char const * pool,
		BitArray const & pattern
		);

	/**
	 * Gibt die Markierungsmuster zu simulierender Pools zurück.
	 *
	 * @return Markierungsmuster zu simulierender Pools
	 */
	inline charptr_map< BitArray > const & getSubsetSimAtomPatterns() const
	{
		return sim_atom_patterns_;
	}
	
	/**
	 * Gibt die Konfiguration der Unbekannten zu simulierender Pools zurück.
	 *
	 * @return Konfigurationen von Unbekannten zu simulierender Pools
	 */
	inline charptr_map< std::set< BitArray > > const &
		getSubsetSimUnknownPatterns() const
	{
		return sim_unknown_patterns_;
	}

	/**
	 * Setzt den Simulations-Typ auf "explicit",
	 * d.h. die Muster in sim_pattern_ werden expandiert und simuliert.
	 */
	inline void setSimExplicit() { sim_type_ = simt_explicit; }

	/**
	 * Setzt den Simulations-Typ auf "full", d.h. alle Cumomere
	 * im Netzwerk werden simuliert (default-Wert).
	 */
	inline void setSimFull() { sim_type_ = simt_full; }

	/**
	 * Setzt den Simulations-Typ auf "auto", d.h. das Messmodell
	 * wird untersucht und die Menge der zu simulierenden Cumomere
	 * wird aus den dort abgelegten Spezifikationen gewonnen.
	 */
	inline void setSimAuto() { sim_type_ = simt_auto; }

	/**
	 * Gibt den Typ der Simulation zurück (full, auto, explicit).
	 *
	 * @return Typ der Simulation (full, auto, explicit)
	 */
	inline SimulationType getSimType() const { return sim_type_; }

	/**
	 * Setzt die Simulationsmethode auf 'Cumomer'.
	 */
	inline void setSimMethodCumomer() { sim_method_ = simm_Cumomer; }

	/**
	 * Setzt die Simulationsmethode auf 'EMU'
	 */
	inline void setSimMethodEMU() { sim_method_ = simm_EMU; }
	
	/**
	 * Gibt die Simulationsmethode zurück (Cumomer oder EMU)
	 *
	 * @return geforderte Simulationsmethode (Cumomer oder EMU)
	 */
	inline SimulationMethod getSimMethod() const { return sim_method_; }

	/**
	 * Versucht die geeignetste Simulationsmethode zu ermitteln.
	 * Zur Bestimmung der optimalen Simulationsmethode wird die
	 * Anzahl der für Cumomer/EMU jeweilig essenziellen Unbekannten
	 * verglichen. Die Methode mit den wenigsten Unbekannten ist
	 * am geeignetsten.
	 *
	 * @return zur Simulation der Markierungsmessungen geeignetste
	 * 	Simulationsmethode
	 */
	SimulationMethod determineBestSimMethod() const;

	/**
	 * Belegt den Verweis auf das geparste Messmodell-Dokument.
	 *
	 * @param mmdoc Zeiger auf das MMDocument-Objekt
	 */
	void linkMMDocument(xml::MMDocument * mmdoc);

	/**
	 * Gibt true zurück, falls das beschriebene Messmodell eine
         * stationäre Messung beschreibt (wodurch die ganze Konfiguration
	 * stationär / instationär wird)
	 *
	 * @return true, falls Konfiguration stationär ist
	 */
	inline bool isStationary() const { return is_stationary_; }
	
        
        /**
	 * Gibt true zurück, falls den Graph für (S)CC System generiert werden soll
	 * @return true, falls Konfiguration stationär ist
	 */
        inline bool generateGraphSystem() const { return generate_graph_flag; }
        inline void generateGraphSystem(bool state) { generate_graph_flag=state; }

	/**
	 * Setzt die Konfiguration auf stationär / instationär
	 *
	 * @param is_stationary Flag
	 */
	inline void setStationary(bool is_stationary)
	{
		is_stationary_ = is_stationary;
	}

	/**
	 * Gibt den Verweis auf das geparste Messmodell-Dokument zurück.
	 *
	 * @return Zeiger auf das MMDocument-Objekt
	 */
	inline xml::MMDocument * getMMDocument() const { return mmdoc_; }

	/**
	 * Gibt den Status der Fluß-Auswahl zurück.
	 *
	 * @return Zustand/Probleme der Konfiguration
	 */
	inline ValidationState getValidationState() const
	{
		return validation_state_;
	}

	/**
	 * Gibt true zurück, falls Konfiguration gültig ist. Es gibt dabei
	 * verschiedene Stufen von "gültig", die über den Parameter tolerance
	 * beeinflusst werden werden können:
	 *
	 *  tolerance = 0: keine Toleranz gegenüber Problemen
	 *  tolerance = 1: zu wenige Constraints werden akzeptiert
	 *  	das ConstraintSystem wählt zusätzliche freie Flüsse und setzt
	 *  	diese auf 0
	 *  tolerance = 2: es werden zu wenige Constraints und zusätzlich
	 *  	verletzte Ungleichungen akzeptiert
	 *  tolerance = 3: es werden zu wenige Constraints, verletzte
	 *  	Ungleichungen und lineare Abhängigkeiten zwischen den
	 *  	Constraints akzeptiert
	 *  tolerance > 3: es werden alle Probleme akzeptiert; die
	 *  	Stöchiometrie ist möglicherweise unsinnig
	 */
	inline bool isValid(int tolerance) const
	{
		switch (tolerance)
		{
		case 0:
			return validation_state_ == cfg_ok;
		case 1:
			if (isValid(0))
				return true;
			if (validation_state_ == cfg_too_few_constr)
				return true;
			return false;
		case 2:
			if (isValid(1))
				return true;
			if (validation_state_ == cfg_ineqs_violated)
				return true;
			return false;
		case 3:
			if (isValid(2))
				return true;
			if (validation_state_ == cfg_linear_dep_constr)
				return true;
			return false;
		default:
			if (tolerance < 0)
				return false;
			return true;
		}
	}

	/**
	 * Validiert die Auswahl der freien und abhängigen Flüsse, sowie der
	 * Constraints auf die Flußwerte mit Hilfe der Stöchiometrie des
	 * darunterliegenden Netzwerkes.
	 *
	 * @param stoich_matrix Stöchiometrische Matrix
	 * @param constraint_equalities Gleichungs-Constraints
	 */
	void validate(
		const la::StoichMatrixInteger * stoich_matrix
		);

	/* Schnittstelle zum ConstraintSystem: */

	/**
	 * Sucht einen Flußwert zu einer Flußbezeichnung, falls er existiert.
	 *
	 * @param fluxname Flußbezeichnung
	 * @param net Netto-Fluß-Wert (out)
	 * @param xch Exchange-Fluß-Wert (out)
	 * @return false, falls Fluss nicht existiert
	 */
	inline bool getFlux(
		char const * fluxname,
		double & net,
		double & xch
		) const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFlux(fluxname,net,xch);
	}

	/**
	 * Setzt eine freien Netto-Fluss.
	 * Die Funktion gibt false zurück, falls der Netto-Fluss nicht frei
	 * ist, oder auf einen Wert festgelegt wurde.
	 *
	 * @param fluxname Flußbezeichnung
	 * @param net Wert des Netto-Flusses
	 * @return true, falls der freie Netto-Fluss erfolgreich belegt wurde
	 */
	inline bool setNetFlux(
		char const * fluxname,
		double net
		)
	{
		fASSERT( CS_ != 0 );
		return CS_->setNetFlux(fluxname,net);
	}

	/**
	 * Setzt eine freien Exchange-Fluss.
	 * Die Funktion gibt false zurück, falls der Exchange-Fluss nicht frei
	 * ist, oder auf einen Wert festgelegt wurde.
	 *
	 * @param fluxname Flußbezeichnung
	 * @param xch Wert des Exchange-Flusses
	 * @return true, falls der freie Exchange-Fluss erfolgreich belegt wurde
	 */
	inline bool setXchFlux(
		char const * fluxname,
		double xch
		)
	{
		fASSERT( CS_ != 0 );
		return CS_->setXchFlux(fluxname,xch);
	}

	/**
	 * Gibt die Bezeichnungen alles Flüsse zurück.
	 *
	 * @return sortiertes Array mit Flussbezeichnungen
	 */
	inline charptr_array getFluxNames() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxNames();
	}

	/**
	 * Gibt die Bezeichnungen aller freien Netto-Flüsse zurück.
	 *
	 * @return Array mit Bezeichnungen aller freien Netto-Flüsse
	 */
	inline charptr_array getFreeFluxesNet() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxNamesByType(
				ConstraintSystem::f_free,
				true
				);
	}

	/**
	 * Gibt die Bezeichnungen aller freien Exchange-Flüsse zurück.
	 *
	 * @return Array mit Bezeichnungen aller freien Exchange-Flüsse
	 */
	inline charptr_array getFreeFluxesXch() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxNamesByType(
				ConstraintSystem::f_free,
				false
				);
	}
	
	/**
	 * Gibt die Bezeichnungen aller abhängigen Netto-Flüsse zurück.
	 *
	 * @return Array mit Bezeichnungen aller abhängigen Netto-Flüsse
	 */
	inline charptr_array getDependentFluxesNet() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxNamesByType(
				ConstraintSystem::f_dependent,
				true
				);
	}

	/**
	 * Gibt die Bezeichnungen aller abhängigen Exchange-Flüsse zurück.
	 *
	 * @return Array mit Bezeichnungen aller abhängigen Exchange-Flüsse
	 */
	inline charptr_array getDependentFluxesXch() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxNamesByType(
				ConstraintSystem::f_dependent,
				false
				);
	}

	/**
	 * Gibt die Bezeichnungen aller Constraint-Netto-Flüsse zurück.
	 *
	 * @return Array mit Bezeichnungen aller Constraint-Netto-Flüsse
	 */
	inline charptr_array getConstraintFluxesNet() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxNamesByType(
				ConstraintSystem::f_constraint,
				true
				);
	}

	/**
	 * Gibt die Bezeichnungen aller Constraint-Exchange-Flüsse zurück.
	 *
	 * @return Array mit Bezeichnungen aller Constraint-Exchange-Flüsse
	 */
	inline charptr_array getConstraintFluxesXch() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxNamesByType(
				ConstraintSystem::f_constraint,
				false
				);
	}
	
	/**
	 * Gibt die Bezeichnungen aller Quasi-Constraint-Netto-Flüsse zurück.
	 *
	 * @return Array mit Bezeichnungen aller Quasi-Constraint-Netto-Flüsse
	 */
	inline charptr_array getQuasiConstraintFluxesNet() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxNamesByType(
				ConstraintSystem::f_quasicons,
				true
				);
	}

	/**
	 * Gibt die Bezeichnungen aller Quasi-Constraint-Xch-Flüsse zurück.
	 *
	 * @return Array mit Bezeichnungen aller Quasi-Constraint-Xch-Flüsse
	 */
	inline charptr_array getQuasiConstraintFluxesXch() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxNamesByType(
				ConstraintSystem::f_quasicons,
				false
				);
	}

	/**
	 * Gibt den Typ eines Netto oder Exchange-Flusses zurück
	 * (frei, abhängig, constraint).
	 *
	 * @param fname Flussname
	 * @param netto true, falls Netto-Fluss gefragt ist
	 * @return Typ des Flusses
	 */
	inline ConstraintSystem::FluxType getFluxType(
		char const * fname,
		bool netto
		) const
	{
		fASSERT( CS_ != 0 );
		return CS_->getFluxType(fname,netto);
	}

	/**
	 * Gibt einen symbolischen Fluss/Pool-Wert (lineare Gleichung) zurück.
	 *
	 * @param flux/Poolname Flussbezeichnung (ohne Suffix .n,.x,.f,.b)
	 * @param coord_type Koordinatentyp; Netto(n), Exchange(x), Forward(f), Backward(b)
	 */
	symb::ExprTree * getSymbolicPoolFluxValue(
		char const * name,
		char coord_type,
		bool formula
		) const;

	/**
	 * Gibt einen symbolischen Flusswert (lineare Gleichung) zurück.
	 *
	 * @param fluxname Flussbezeichnung mit Suffix (.n,.x,.f,.b)
	 */
	symb::ExprTree * getSymbolicPoolFluxValue(
		char const * name,
		bool formula
		) const;

	/**
	 * Gibt true zurück, falls der Vorwärtsfluss einer Reaktion
	 * prinzipiell zulässig ist.
	 *
	 * @param fluxname Bezeichnung eines Flusses
	 * @return true, falls der Vorwärtsfluss fließen darf
	 */
	inline bool fwdFluxAdmissible(char const * fluxname) const
	{
		fASSERT( CS_ != 0 );
		return CS_->fwdFluxAdmissible(fluxname);
	}

	/**
	 * Gibt true zurück, falls der Rückwärtsfluss einer Reaktion
	 * prinzipiell zulässig ist.
	 *
	 * @param fluxname Bezeichnung eines Flusses
	 * @return true, falls der Rückwärtsfluss fließen darf
	 */
	inline bool bwdFluxAdmissible(char const * fluxname) const
	{
		fASSERT( CS_ != 0 );
		return CS_->bwdFluxAdmissible(fluxname);
	}

	/**
	 * Gibt den Toleranzwert für die Prüfung auf Constraint-Verletzung
	 * zurück.
	 *
	 * @return Toleranzwert
	 */
	inline double getConstraintViolationTolerance() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getConstraintViolationTolerance();
	}

	/**
	 * Setzt den Toleranzwert für die Prüfung auf Constraint-Verletzung.
	 *
	 * @param cons_tol Toleranzwert
	 */
	inline void setConstraintViolationTolerance(double cons_tol)
	{
		fASSERT( CS_ != 0 );
		CS_->setConstraintViolationTolerance(cons_tol);
	}

	/**
	 * Gibt die Anzahl der Änderungen an der Stöchiometrie zurück.
	 *
	 * @return Wert des Änderungszählers für die freien Flüsse
	 */
	inline unsigned int getStoichiometryChangeCount() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getChangeCount();
	}

	/**
	 * Zählt die Anzahl der Änderungen an der Stöchiometrie
	 * künstlich hoch (zum Triggern einer Auswertung).
	 */
	inline void incrementStoichiometryChangeCount() const
	{
		fASSERT( CS_ != 0 );
		CS_->incrementChangeCount();
	}

	/**
	 * Gibt das ConstraintSystem-Objekt zurück, um komplexere
	 * Aufgaben erfüllen zu können.
	 *
	 * @return Referenz auf internes ConstraintSystem-Objekt
	 */
	inline ConstraintSystem & getConstraintSystem()
	{
		fASSERT(CS_ != 0);
		return *CS_;
	}

	/**
	 * Gibt die Konfiguration (lt. FluxML) eines freien netto-Flusses
	 * zurück.
	 *
	 * @param fluxname Bezeichnung des Flusses
	 * @return Zeiger auf Struct mit Flusskonfiguration oder 0-Zeiger
	 */
	inline FreeFluxCfg const * getFreeNetFluxCfg(
		char const * fluxname
		) const
	{
		return sim_opt_free_fluxes_net_.findPtr(fluxname);
	}
	
	/**
	 * Gibt die Konfiguration (lt. FluxML) eines freien exchange-Flusses
	 * zurück.
	 *
	 * @param fluxname Bezeichnung des Flusses
	 * @return Zeiger auf Struct mit Flusskonfiguration oder 0-Zeiger
	 */
	inline FreeFluxCfg const * getFreeXchFluxCfg(
		char const * fluxname
		) const
	{
		return sim_opt_free_fluxes_xch_.findPtr(fluxname);
	}
        
        /**
	 * Gibt die Konfiguration (lt. FluxML) einer freien Poolgröße
	 * zurück.
	 *
	 * @param poolname Bezeichnung des Pools
	 * @return Zeiger auf Struct mit Poolgröße-konfiguration oder 0-Zeiger
	 */
	inline FreePoolsizeCfg const * getFreePoolSizeCfg(
		char const * poolname
		) const
	{
		return sim_opt_free_poolsizes_.findPtr(poolname);
	}
	/**
	 * Gibt die (möglicherweise) gesetzte untere Grenze eines freien
	 * Netto-Flusses zurück.
	 *
	 * @param flux Flussbezeichnung
	 * @param lo gesetzte untere Grenze, falls vorhanden (out)
	 * @return true, falls untere Grenze gesetzt ist
	 */
	inline bool getFluxLoNet(char const * flux, double & lo) const
	{
		FreeFluxCfg const * fluxcfg =
			sim_opt_free_fluxes_net_.findPtr(flux);
		if (fluxcfg && fluxcfg->has_lo)
		{
			lo = fluxcfg->lo;
			return true;
		}
		return false;
	}

	/**
	 * Gibt die (möglicherweise) gesetzte obere Grenze eines freien
	 * Netto-Flusses zurück.
	 *
	 * @param flux Flussbezeichnung
	 * @param hi gesetzte obere Grenze, falls vorhanden (out)
	 * @return true, falls obere Grenze gesetzt ist
	 */
	inline bool getFluxHiNet(const char * flux, double & hi) const
	{
		FreeFluxCfg const * fluxcfg =
			sim_opt_free_fluxes_net_.findPtr(flux);
		if (fluxcfg && fluxcfg->has_hi)
		{
			hi = fluxcfg->hi;
			return true;
		}
		return false;
	}

	/**
	 * Gibt die (möglicherweise) gesetzte untere Grenze eines freien
	 * Exchange-Flusses zurück.
	 *
	 * @param flux Flussbezeichnung
	 * @param lo gesetzte untere Grenze, falls vorhanden (out)
	 * @return true, falls untere Grenze gesetzt ist
	 */
	inline bool getFluxLoXch(char const * flux, double & lo) const
	{
		FreeFluxCfg const * fluxcfg =
			sim_opt_free_fluxes_xch_.findPtr(flux);
		if (fluxcfg && fluxcfg->has_lo)
		{
			lo = fluxcfg->lo;
			return true;
		}
		return false;
	}

	/**
	 * Gibt die (möglicherweise) gesetzte obere Grenze eines freien
	 * Exchange-Flusses zurück.
	 *
	 * @param flux Flussbezeichnung
	 * @param hi gesetzte obere Grenze, falls vorhanden (out)
	 * @return true, falls obere Grenze gesetzt ist
	 */
	inline bool getFluxHiXch(char const * flux, double & hi) const
	{
		FreeFluxCfg const * fluxcfg
			= sim_opt_free_fluxes_xch_.findPtr(flux);
		if (fluxcfg && fluxcfg->has_hi)
		{
			hi = fluxcfg->hi;
			return true;
		}
		return false;
	}
        
        /**
	 * Gibt die (möglicherweise) gesetzte untere Grenze einer freien
	 * Poolgröße zurück.
	 *
	 * @param pool Poolbezeichnung
	 * @param lo gesetzte untere Grenze, falls vorhanden (out)
	 * @return true, falls untere Grenze gesetzt ist
	 */
	inline bool getPoolSizeLo(char const * pool, double & lo) const
	{
		FreePoolsizeCfg const * poolcfg =
			sim_opt_free_poolsizes_.findPtr(pool);
		if (poolcfg && poolcfg->has_lo)
		{
			lo = poolcfg->lo;
			return true;
		}
		return false;
	}

	/**
	 * Gibt die (möglicherweise) gesetzte obere Grenze einer freien
	 * Poolgröße zurück.
	 *
	 * @param pool Poolbezeichnung
	 * @param hi gesetzte obere Grenze, falls vorhanden (out)
	 * @return true, falls obere Grenze gesetzt ist
	 */
	inline bool getPoolSizeHi(const char * pool, double & hi) const
	{
		FreePoolsizeCfg const * poolcfg =
			sim_opt_free_poolsizes_.findPtr(pool);
		if (poolcfg && poolcfg->has_hi)
		{
			hi = poolcfg->hi;
			return true;
		}
		return false;
	}

	/**
	 * Berechnet eine Prüfsumme über das Configuration-Objekt.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;
        
public:        
        inline void setDefaultLowerBoundOfPools(double pmin= 1.e-4)
	{
		charptr_array pnames = getFreePools();
		charptr_array::const_iterator pi;
		for(pi = pnames.begin(); pi != pnames.end(); ++pi)
		{
                    symb::ExprTree * ieq = symb::ExprTree::geq(symb::ExprTree::sym(*pi),symb::ExprTree::val(pmin));
                    createConstraint("anonymous" ,ieq, data::POOL);
                    delete ieq;
		}
	}
        
        inline charptr_array getDependentPools() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getPoolNamesByType(
				ConstraintSystem::p_dependent
				);
	}
        
        inline charptr_array getConstraintPools() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getPoolNamesByType(
				ConstraintSystem::p_constraint
				);
	}
        
        inline charptr_array getQuasiConstraintPools() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getPoolNamesByType(
				ConstraintSystem::p_quasicons
				);
	}
        
        inline ConstraintSystem::PoolType getPoolType(
		char const * pname
		) const
	{
		fASSERT( CS_ != 0 );
		return CS_->getPoolType(pname);
	}
        
        inline bool getPoolSize(
		char const * poolname,
		double & size
		) const
	{
		fASSERT( CS_ != 0 );
		return CS_->getPoolSize(poolname,size);
	}
        
        inline bool setPoolSize(
        char const * poolname,
        double size
        )
        {
                fASSERT( CS_ != 0 );
                return CS_->setPoolSize(poolname,size);
        }
        
        inline charptr_array getPoolNames() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getPoolNames();
	}
	
	inline charptr_array getFreePools() const
	{
		fASSERT( CS_ != 0 );
		return CS_->getPoolNamesByType(
				ConstraintSystem::p_free
				);
	}
        
            

}; // class Configuration

inline std::ostream& operator <<(std::ostream& stream, const Configuration::FreeFluxCfg& fluxcfg)
{
	if (fluxcfg.has_lo)
		stream << "lo: " << fluxcfg.lo << " ";
	if (fluxcfg.has_hi)
		stream << "hi: " << fluxcfg.hi << " ";
	if (fluxcfg.has_inc)
		stream << "inc: " << fluxcfg.inc << " ";
	if (fluxcfg.has_value)
		stream << "value: " << fluxcfg.value << " ";
	if (fluxcfg.has_edweight)
		stream << "edw.: " << fluxcfg.edweight << " ";
	if(!fluxcfg.has_lo && ! fluxcfg.has_hi && !fluxcfg.has_inc &&
			!fluxcfg.has_value && !fluxcfg.has_edweight)
		stream << "Nothing set ";
	return stream;
}

inline std::string buildName(const std::string& plain_name, bool is_net)
{
        return plain_name + (is_net ? ".n" : ".x");
}

inline std::pair<std::string,bool> decomposeName(const std::string& name)
{
        std::pair<std::string,bool> result;
        if (name.size() > 3)
        {
                result.first = name.substr(0,name.size()-2);
                std::string suf = name.substr(name.size()-2, name.size());
                if (suf == ".n")
                        result.second = true;
                else if (suf == ".x")
                        result.second = false;
                else
                        fASSERT_NONREACHABLE();
        }
        return result;
}
    
    

} // namespace flux::data
} // namespace flux

#endif

