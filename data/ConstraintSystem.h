#ifndef CONSTRAINTSYSTEM_H
#define CONSTRAINTSYSTEM_H

#include <cstddef>
#include <list>
#include "Error.h"
#include "charptr_array.h"
#include "charptr_map.h"
#include "MMatrix.h"
#include "MVector.h"
#include "StandardForm.h"
#include "StoichMatrixInteger.h"
#include "ExprTree.h"
#include "LinearExpression.h"
#include "Constraint.h"

namespace flux {
namespace data {

/*
 * *****************************************************************************
 * Klasse zur Modellierung der linearen Constraint-Matrix.
 * Mit der linearen Constraint-Matrix lassen sich lineare
 * Gleichungs-Constraints und Constraints auf die Fluß-Werte,
 * sowie die Werte freier Flüsse festlegen / validieren.
 * Nach Lösen des Gleichungssystems stehen die Werte für alle
 * Netto- & Exchange-Flüsse fest und die eigentlichen Netzwerk-
 * Gleichungen können gelöst werden.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */
    
class ConstraintSystem
{
public:
	enum ValidationState
	{
		cm_ok,			// Validierung erfolgreich abgeschlossen
		cm_unvalidated,		// Validierung noch nicht abgeschlossen
		cm_too_few_constr,	// zu wenige Constraints / freie Variablen
		cm_too_many_constr,	// zu viele Constraints / freie Variablen
		cm_linear_dep_constr,	// linear abhängige Constraints
		cm_nonlinear_constr,	// ein Constraint ist anscheinend nicht-linear
		cm_invalid_constr,	// ein Constraint ist ungültig / falscher Reakt.-Name
		cm_too_many_free_vars,	// es wurden zu viele Variablen als frei deklariert
		cm_invalid_free_vars,	// freie Variablen kollidieren mit Constraints
		cm_ineqs_violated,	// die Flusswerte verletzen die Ungleichungen
		cm_ineqs_infeasible	// Ungleichungen haben keine Lösung
	};

	enum FluxType
	{
		f_undefined,		// noch unbekannt / undefiniert
		f_dependent,		// abhängiger Fluß / Wert
		f_quasicons,		// abhängiger Fluß mit festem Wert
		f_free,			// freier Fluß
		f_constraint		// freier Fluß mit festem Wert
	};
        
        enum PoolType
	{
		p_undefined,            // noch unbekannt / undefiniert
		p_dependent,            // abhängiger Poolgröße / Wert
		p_quasicons,            // abhängiger Poolgröße mit festem Wert
		p_free,                 // freier Poolgröße
		p_constraint
	};
        
private:
	/** Stöchiometrie */
	la::StoichMatrixInteger const & S_;
	/** Eine Liste von freien Flüssen (netto-Flüsse) */
	charptr_array fFluxes_net_;
	/** Eine Liste von freien Flüssen (exchange-Flüsse) */
	charptr_array fFluxes_xch_;
	/** Eine Liste von Gleichungs-Constraints */
	std::list< Constraint > const & cEqList_;
	/** Eine Liste von Ungleichungs-Constraints */
	std::list< Constraint > const & cInEqList_;
	/** Validierungsstatus des Constraint-Systems */
	mutable ValidationState validation_state_;

	/** Die Constraint-Matrix (Netto-Flüsse) */
	la::MMatrix Nnet_;
	/** Die Constraint-Matrix (Exchange-Flüsse) */
	la::MMatrix Nxch_;
	/** Der Konstanten-Vektor zur Constraint-Matrix (Netto-Flüsse) */
	la::MVector bnet_;
	/** Der Konstanten-Vektor zur Constraint-Matrix (Exchange-Flüsse) */
	la::MVector bxch_;
	/** Kernel-Matrix: Spezielle und homogene Lösung (Netto-Flüsse) */
	la::MMatrix Vnet_;
	/** Kernel-Matrix: Spezielle und homogene Lösung (Exchange-Flüsse) */
	la::MMatrix Vxch_;
	/** Der Vektor mit den Netto-Flußwerten */
	mutable la::MVector vnet_;
	/** Der Vektor mit den Exchange-Flußwerten */
	mutable la::MVector vxch_;
	/** Der Vektor mit den konstanten Constraint-Flußwerten (Netto-Flüsse) */
	la::MVector v_const_net_;
	/** Der Vektor mit den konstanten Constraint-Flußwerten (Exchange-Flüsse) */
	la::MVector v_const_xch_;
	/** Permutationsmatrix / Auswahl freier Netto-Flüsse */
	la::PMatrix Pcnet_;
	/** Permutationsmatrix / Auswahl freier Exchange-Flüsse */
	la::PMatrix Pcxch_;
	/** Flusstypen (Netto-Flüsse) */
	la::GVector< FluxType > v_type_net_;
	/** Flusstypen (Exchange-Flüsse) */
	la::GVector< FluxType > v_type_xch_;
	/** Abbildung von Flußnamen auf Indizes */
	charptr_map< size_t > flux2idx_;
	/** Toleranz für Constraintverletzung (default: 1e-9) */
	double cons_tol_;
	/** Müssen Flüsse vor dem Auslesen neu berechnet werden? */
	mutable bool fluxes_dirty_;
	/** Zähler für Änderungen an freien Flüssen */
	unsigned int change_count_;
        /** Flag; Stationarität der Konfiguration */
        bool is_stationary_;
        /** Eine Liste von freien Poolgrößen (netto-Flüsse) */ 
	charptr_array fPools_size_;
	/** Die Constraint-Matrix (Poolgrößen) */
	la::MMatrix Npool_;
	/** Der Konstanten-Vektor zur Constraint-Matrix (Poolgrößen) */
	la::MVector bpool_;
	/** Der Vektor mit den Poolgrößen */
	mutable la::MVector vpool_;
	/** Permutationsmatrix / Auswahl freier Poolgrößen */
	la::PMatrix Pcpool_;
	/** Pooltypen (Poolgrößen) */
	la::GVector< PoolType > v_type_pool_;
	/** Abbildung von Poolname auf Indizes */
	charptr_map< size_t > pool2idx_;
	/** Kernel-Matrix: Spezielle und homogene Lösung (Poolgrößen) */
	la::MMatrix Vpool_;
	/** Der Vektor mit den konstanten Constraint-Poolgrößen */
	la::MVector v_const_pool_;        
        
private:
	/** Validiert die feasibility der Ungleichungen */
	bool validate_ineqs_feasibility() const;
	/** Erstellt eine Vorschlagsliste zur Elimination überzähliger,
	 *  freier Variablen */
	void make_free_flux_suggestion(int dfree, bool net);
	/** Erstellt Nnet_, Nxch_, bnet_, bxch_ */
	bool prepare();
	/** Berechnet V_ mit N_.V_ = [b_, 0] */
	bool solve();
	/** Generiert statische Flusswert-Constraints v_const_ */
	void impose_constraints();
	/** Validiert die Ungleichungen mit den aktuellen Flusswerten */
	bool validate_ineqs() const;
	
public:
	/**
	 * Überträgt Werte freier Flüsse und berechnet
	 * v_ = V_.(v_free+v_constr_) und validiert die Flusswerte mit den
	 * Ungleichungen.
	 */
	void eval() const;
	
	/**
	 * Constructor.
	 * Übergeben wird die Stöchiometrie eine Liste von (linearen)
	 * Gleichungen.
	 *
	 * @param S stöchiometrische Matrix
	 * @param fFluxes_net Bezeichnungen freier Flüsse (netto)
	 * @param fFluxes_xch Bezeichnungen freier Flüsse (exchange)
	 * @param cEqList lineare Gleichungen
	 */
	ConstraintSystem(
		la::StoichMatrixInteger const & S,
		charptr_array const & fFluxes_net,
		charptr_array const & fFluxes_xch,
                charptr_array const & fPools,
		std::list< Constraint > const & cEqList,
		std::list< Constraint > const & cInEqList,
                bool stationary 
		);

	/* Es gibt hier KEINE besonderen Copy-Constructor.
	 * Der default-Copy-Constructor sollte es tun ...
	 */

	/**
	 * Destructor.
	 */
	virtual ~ConstraintSystem() { }

	/**
	 * Sucht einen Flußwert zu einer Flußbezeichnung, falls er existiert.
	 *
	 * @param fluxname Flußbezeichnung
	 * @param net Netto-Fluß-Wert (out)
	 * @param xch Exchange-Fluß-Wert (out)
	 * @return false, falls Fluss nicht existiert
	 */
	bool getFlux(char const * fluxname, double & net, double & xch) const;

	/**
	 * Sucht einen Flußwert zu einer Flußbezeichnung, falls er existiert.
	 * Ob es sich um einen Netto oder einen Exchange-Fluß handelt wird
	 * am Namens-Suffix (.n, .x) erkannt.
	 *
	 * @param fluxname Flussbezeichnung
	 * @param val Flusswert (out)
	 * @return false, falls der Fluß nicht existiert
	 */
	bool getFlux(char const * fluxname, double & val) const;

	/**
	 * Setzt eine freien Netto-Fluss.
	 * Die Funktion gibt false zurück, falls der Netto-Fluss nicht frei
	 * ist, oder auf einen Wert festgelegt wurde.
	 *
	 * @param fluxname Flußbezeichnung
	 * @param net Wert des Netto-Flusses
	 * @return true, falls der freie Netto-Fluss erfolgreich belegt wurde
	 */
	bool setNetFlux(char const * fluxname, double net);
	
	/**
	 * Setzt eine freien Exchange-Fluss.
	 * Die Funktion gibt false zurück, falls der Exchange-Fluss nicht frei
	 * ist, oder auf einen Wert festgelegt wurde.
	 *
	 * @param fluxname Flußbezeichnung
	 * @param xch Wert des Exchange-Flusses
	 * @return true, falls der freie Exchange-Fluss erfolgreich belegt wurde
	 */
	bool setXchFlux(char const * fluxname, double xch);

	/**
	 * Setzt einen freien Fluß. Ob es sich um einen Netto- oder um einen
	 * Exchange-Fluß handelt wird am Namens-Suffix erkannt (.n, .x).
	 *
	 * @param fluxname Flußbezeichnung
	 * @param val Flußwert
	 * @return true, falls der freie Fluß erfolgreich belegt wurde
	 */
	bool setFlux(char const * fluxname, double val);

	/**
	 * Gibt den Validierungszustand zurück.
	 *
	 * @return Validierungszustand der linearen Constraints
	 */
	inline ValidationState getValidationState() const { return validation_state_; }

	/**
	 * Gibt ein Array mit den Flussbezeichnungen zurück. Es wird kein Suffix
	 * für netto / exchange angehängt.
	 *
	 * @return Sortiertes Array mit Flussbezeichnungen
	 */
	charptr_array getFluxNames() const;

	/**
	 * Gibt ein Array mit Flussbezeichnungen von Flüssen eines
	 * angeforderten Typs zurück (dependent, free, constraint,
	 * quasi-constraint mit jeweils net/xch). Werden die abhängigen
	 * Flüsse angefordert, so schliesst die Antwort die quasi-constraint
	 * Flüsse stets ein.
	 *
	 * @param ftype Fluss-Typ (f_dependent,f_free,f_constraint,f_quasicons)
	 * @param net true, falls Nettoflüsse gefragt sind, false für
	 * 	Exchange-Flüsse
	 * @return Sortiertes Array mit Flussbezeichnungen des angeforderten
	 * 	Typs
	 */
	charptr_array getFluxNamesByType(FluxType ftype, bool net) const;

	/**
	 * Gibt den Typ eines genannten Flusses zurück.
	 *
	 * @param fname Flussname
	 * @param net true, falls Netto-Fluss gefragt ist
	 * @return Typ des Flusses
	 */
	FluxType getFluxType(char const * name, bool net) const;

	/**
	 * Gibt eine "semi-analytische" Lösung für einen Flusswert zurück.
	 *
	 * @param fluxname Flussbezeichnung
	 * @param get_net true, falls fluxname sich auf einen Nettofluß bezieht
	 * @param formula Const-Flüsse nicht in Werte umsetzen, falls true
	 * @return Analytische Lösung für einen Flusswert
	 */
	symb::ExprTree * getSymbolicPoolFlux(
		char const * fluxname,
		ParameterType parameter_type,
		bool formula
		) const;
        
        /**
	 * Gibt eine "semi-analytische" Lösung für einen Flusswert zurück.
	 *
	 * @param fluxname Flussbezeichnung
	 * @param get_net true, falls fluxname sich auf einen Nettofluß bezieht
	 * @param formula Const-Flüsse nicht in Werte umsetzen, falls true
	 * @return Analytische Lösung für einen Flusswert
	 */
	symb::ExprTree * getSymbolicFluxNetXch(
		char const * fluxname,
		ParameterType parameter_type,
		bool formula
		) const;
	
	/**
	 * Gibt eine "semi-analytische" Lösung für einen Flusswert
	 * zurück.
	 *
	 * @param fluxname Flussbezeichnung
	 * @param get_fwd true, falls fluxname sich auf einen Vorwärtsfluß bezieht
	 * @param formula Const-Flüsse nicht in Werte umsetzen, falls true
	 * @return Analytische Lösung für einen Flusswert
	 */
	symb::ExprTree * getSymbolicFluxFwdBwd(
		char const * fluxname,
		bool get_fwd,
		bool formula
		) const;

	/**
	 * Gibt true zurück, falls der Vorwärtsfluss einer Reaktion
	 * prinzipiell zulässig ist.
	 *
	 * @param fluxname Bezeichnung eines Flusses
	 * @return true, falls der Vorwärtsfluss fließen darf
	 */
	bool fwdFluxAdmissible(char const * fluxname) const;
	
	/**
	 * Gibt true zurück, falls der Rückwärtsfluss einer Reaktion
	 * prinzipiell zulässig ist.
	 *
	 * @param fluxname Bezeichnung eines Flusses
	 * @return true, falls der Rückwärtsfluss fließen darf
	 */
	bool bwdFluxAdmissible(char const * fluxname) const;

	/**
	 * Substituiert abhängige Variablen eines (linearen) Ausdrucks und
	 * vereinfacht diesen soweit wie möglich.
	 *
	 * @param Eorig Ausdruck
	 * @param is_net true, falls eine netto-Beziehung beschrieben wird
	 * @return umgebauter Ausdruck
	 */
	symb::ExprTree * rebuildLinearExpression(
		symb::ExprTree const * Eorig,
                ParameterType parameter_type
		) const;

	/**
	 * Substituiert abhängige Variablen eines (linearen) Ausdrucks und
	 * vereinfacht diesen soweit wie möglich. Diese Version erwartet das
	 * Variablen-Suffix ".n" und ".x" für Netto und Exchange-Variablen.
	 *
	 * @param Eorig Ausdruck
	 * @return umgebauter Ausdruck
	 */
	symb::ExprTree * rebuildLinearExpression(
		symb::ExprTree const * Eorig
		) const;

	/**
	 * Trägt die Ungleichungs-Constraints in eine Standardform ein.
	 * Ungleichungen, die sich auf abhängige Flüsse beziehen werden mit
	 * Hilfe der semi-analytischen Lösung in äquivalente Ungleichungen
	 * für Freie flüsse umformuliert.
	 *
	 * @param SF (leeres) StandardForm-Objekt
	 * @param fill_net falls true, werden Netto-Constraints eingetragen
	 * @param fill_xch falls true, werden Exchange-Constraints eingetragen
	 * @return true bei Erfolg
	 */
	bool fillStandardForm(
		la::StandardForm & SF,
		bool fill_net = true,
		bool fill_xch = true,
                bool fill_pool = true
		) const;

	/**
	 * Gibt die Stöchiometrie des Netzwerks zurück
	 *
	 * @return Referenz auf Stöchiometrie des Netzwerks
	 */
	inline la::StoichMatrixInteger const & getStoichiometry() const
	{
		return S_;
	}

	/**
	 * Berichtet, welche abhängigen Flüsse Pseudo-Constraint sind,
	 * d.h. deren Wert vollständig durch andere Constraints
	 * festgelegt wird.
	 *
	 * @param net true, falls Nettoflüsse untersucht werden sollen
	 * @param loglevel LogLevel für die Ausgabe (default: logQUIET)
	 * @return Array von Flussbezeichnungen, die mittelbar constraint sind
	 */
	charptr_array reportQuasiConstraintFluxes(
		bool net,
		LogLevel loglevel = logQUIET
		) const;
        
        /**
	 * Berichtet, welche abhängigen Poolgrößen Pseudo-Constraint sind,
	 * d.h. deren Wert vollständig durch andere Constraints
	 * festgelegt wird.
	 *
	 * @param loglevel LogLevel für die Ausgabe (default: logQUIET)
	 * @return Array von Poolbezeichnungen, die mittelbar constraint sind
	 */
        charptr_array reportQuasiConstraintPools(
		LogLevel loglevel
		) const;

	/**
	 * Setzt den Toleranzwert für die Prüfung auf Constraint-Verletzung.
	 *
	 * @param cons_tol Toleranzwert
	 */
	inline void setConstraintViolationTolerance(double cons_tol)
	{
		cons_tol_ = cons_tol;
	}

	/**
	 * Gibt den Toleranzwert für die Prüfung auf Constraint-Verletzung
	 * zurück.
	 *
	 * @return Toleranzwert
	 */
	inline double getConstraintViolationTolerance() const
	{
		return cons_tol_;
	}

	/**
	 * Gibt den Wert des Änderungszählers zurück.
	 *
	 * @return Wert des Änderungszählers
	 */
	inline unsigned int getChangeCount() const
	{
		return change_count_;
	}

	/**
	 * Inkrementiert den Änderungszähler
	 * (und triggert somit eine erneute Auswertung).
	 */
	inline void incrementChangeCount()
	{
		change_count_++;
	}

        /** Erstellt eine Vorschlagsliste zur Elimination überzähliger,
	 *  freier Variablen */
        void make_free_pool_suggestion(int dfree);
        
        /*
	 * Gibt die Poolgröße einer Metabolite zurück.
	 *
	 * @param poolname Flußbezeichnung
	 * @param poolsize poolgröße (out)
	 * @return false, falls Pool nicht existiert
	 */
        bool getPoolSize(char const * poolname,	double & pool) const;
        
        /*
	 * Setzt die Poolgröße einer Metabolite.
	 *
	 * @param poolname Flußbezeichnung
	 * @param poolsize poolgröße (out)
	 * @return false, falls Pool nicht existiert
	 */
        bool setPoolSize(char const * poolname,	double size);
        
        /*
	 * liefert eine List aller Metabiliten zurück.
	 *
	 * @return List aller Metaboliten
	 */
        charptr_array getPoolNames() const;
        
        /*
	 * liefert eine List aller Metabiliten bei angegebenen Poottyp zurück.
	 *
         * @param ptype Pooltyp
	 * @return List aller Metaboliten
	 */
	charptr_array getPoolNamesByType(PoolType ptype) const;
        
        /*
	 * Gibt den Pooltyp einer Metabolite zurück.
	 *
	 * @param poolname Flußbezeichnung
	 * @return pooltyp, falls Pool existiert
	 */
        PoolType getPoolType(char const * poolname) const;
                        
        /**
	 * Debugging
	 */
	void dump() const;


        
	const la::MMatrix& getVnet() const
	{
		return Vnet_;
	}

	const la::MMatrix& getVxch() const
	{
		return Vxch_;
	}
	const la::PMatrix& getPcnet() const
	{
		return Pcnet_;
	}

	const la::PMatrix& getPcxch() const
	{
		return Pcxch_;
	}

	const la::MMatrix& getNnet() const
	{
		return Nnet_;
	}

	const la::MMatrix& getNxch() const
	{
		return Nxch_;
	}
	const la::MVector& getbnet() const
	{
		return bnet_;
	}
		const la::MVector& getbxch() const
	{
		return bxch_;
	}


	const la::GVector< FluxType >& getv_type_net() const
	{
		return v_type_net_;
	}
	const la::GVector< FluxType >& getv_type_xch() const
	{
		return v_type_xch_;
	}

	const la::MVector& getv_const_net() const
	{
		return v_const_net_;
	}
		const la::MVector& getv_const_xch() const
	{
		return v_const_xch_;
	}
};

} // namespace flux::data
} // namespace flux

#endif

