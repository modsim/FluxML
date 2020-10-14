#ifndef MGROUP_H
#define MGROUP_H

extern "C"
{
#include <stdint.h>
}
#include <cmath>
#include <cstddef>
#include <list>
#include <vector>
#include <sstream>
#include <set>
#include <map>
#include "Error.h"
#include "charptr_array.h"
#include "charptr_map.h"
#include "Array.h"
#include "BitArray.h"
#include "MaskedArray.h"
#include "cstringtools.h"
#include "Notation.h"
#include "ExprTree.h"
#include "XMLException.h"
#include "MValue.h"
#include "MVector.h"
#include "MMatrix.h"
#include "Conversions.h"

#include <limits>

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Abstrakte Basisklasse für Messgruppen-Objekte.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */ 
    
class MGroup
{
public:
	/**
	 * Messgruppentypen:
	 * MS, MS-MS, 1H-NMR, 13C-NMR, generisch, Fluß, Poolgröße
	 */
	enum MGroupType {
		mg_MS, mg_MSMS, mg_1HNMR, mg_13CNMR, mg_GENERIC,
		mg_FLUX, mg_POOL, mg_CUMOMER, mg_MIMS
	};

protected:
	/** Messgruppentyp */
	MGroupType mgtype_;

	/** (eindeutige) Bezeichnung der Messgruppe */
	char * group_id_;

	/** Menge der Timestamps */
	std::set< double > ts_set_;

	/** Flag; automatische Skalierung */
	bool scale_auto_;

	/** Dimension eines (Messwert-)Vektors */
	size_t dim_;

	/** zugrundeliegende Spezifikation (Zeichenkette) */
	mutable char * spec_;

	/** Spezifikation einzelner Messwerte */
	mutable char ** row_spec_;

	/** Modell für Fehlerwachstum der einzelnen Messwerte
	 * wird für das Experimental Design benötigt.
	 * Default-Wert: max(std_real/meas_real*meas_sim,0);
	 */
	symb::ExprTree ** error_model_;
        
        /** Multi-Isotopes Konfiguration cfg=<Isotope,NumbAtoms> Isotope und deren Anzahl */
        charptr_map< int > iso_cfg_;
        
public:
    
    /**
	 * Gibt eine List mit allen chemischen Isotope und iheren Anzahl
         *  zurück.
	 *
	 * @return Zugriff auf list 
	 */
	inline charptr_map< int > const & getIsotopesCfg() const 
        { return iso_cfg_; }
        
        
        inline void setIsotopesCfg(charptr_map< int > const & iso_cfg)
        { 
            charptr_map< int >::const_iterator ic;
            for(ic=iso_cfg.begin(); ic!= iso_cfg.end(); ++ic)
                iso_cfg_.insert(ic->key, ic->value);
        }

protected:
	/**
	 * Constructor.
	 *
	 * @param mgtype Typ der Messgruppe
	 * @param dim Dimension der Messung
	 * @param spec Spezifikation der Messgruppe
	 */
	inline MGroup(MGroupType mgtype, size_t dim, char const * spec = 0)
		: mgtype_(mgtype), group_id_(0),
		  scale_auto_(false), dim_(dim), spec_(0)
	{
		if (spec)
			spec_ = strdup_alloc(spec);
		row_spec_ = new char*[dim_];
		error_model_ = new symb::ExprTree*[dim+1];
		error_model_[dim] = 0;
		for (size_t i=0; i<dim_; i++)
		{
			row_spec_[i] = 0;
			// alternativ: max(std_real/meas_real*meas_sim,0)
			// default: keine automatische Skalierung des Fehlers
			error_model_[i] = symb::ExprTree::sym("std_real");
		}
	}

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes MGroup-Objekt
	 */
	MGroup(MGroup const & copy);

	/**
	 * Assignment operator
	 *
	 * @param copy zu kopierendes MGroup-Objekt
	 */
	MGroup & operator= (MGroup const & copy);

	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	virtual MGroup * clone() const = 0;

public:
	/**
	 * Virtueller Destructor.
	 */
	virtual inline ~MGroup()
	{
		delete[] group_id_;
		delete[] spec_;
		for (size_t i=0; i<dim_; i++)
		{
			delete[] row_spec_[i];
			delete error_model_[i];
		}
		delete[] row_spec_;
		delete[] error_model_;
	}

public:
	/**
	 * Registriert alle Messwerte über eine einfache Schnittstelle.
	 *
	 * @param x_meas Messwerte
	 * @param x_stddev Standardabweichungen
	 */
	virtual void setMValuesStdDev(
		double ts,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		) = 0;
        
        /**
	 * löscht alle registrierte Messwerte über eine einfache Schnittstelle.
	 * Verwendet wird bei Experimental Design zur Registrierung von Messwerte
         * mit von optimierter bestimmten Zeitpunkte
	 */
	virtual void removeMValuesStdDev() = 0;

	/**
	 * Gibt den Typ der Messgruppe zurück.
	 *
	 * @return Messgruppentyp
	 */
	inline MGroupType getType() const { return mgtype_; }

	/**
	 * Setzt den eindeutigen Bezeichner der Messgruppe.
	 *
	 * @param id eindeutiger Bezeichner der Messgruppe
	 */
	inline virtual void setGroupId(char const * id)
	{
		fASSERT( group_id_ == 0 );
		group_id_ = strdup_alloc(id);
	}

	/**
	 * Gibt den eindeutigen Bezeichner der Messgruppe zurück.
	 *
	 * @return eindeutiger Bezeichner der Messgruppe
	 */
	inline char const * getGroupId() const { return group_id_; }

	/**
	 * Gibt die Menge der Timestamps zurück.
	 *
	 * @return Menge der Timestamps
	 */
	inline std::set< double > const & getTimeStampSet() const
	{
		return ts_set_;
	}
        
	/**
	 * Registriert einen neuen Timestamp.
	 *
	 * @param value Timestamp
	 */
	virtual bool registerTimeStamp(double value);

	/**
	 * Prüft die Gültigkeit eines Timestamp-Wertes, d.h. ob zu einem
	 * gegebenem Timestamp ein Messwert-Objekt existiert.
	 *
	 * @param value Timestamp
	 * @return true, falls ein Messwert existiert, sonst false
	 */
	inline virtual bool isTimeStamp(double value) const
	{
		if (value == -1)
			return true; // stationärer Messwert
		return ts_set_.find(value) != ts_set_.end();
	}
	
	/**
	 * Setzt das Group-Scale Flag auf "auto", d.h. automatische
	 * Skalierung.
	 */
	inline virtual void setScaleAuto() { scale_auto_ = true; }
	
	/**
	 * Löscht das Group-Scale Flag, d.h. keine automatische Skalierung.
	 */
	inline virtual void setUnscaled() { scale_auto_ = false; }

	/**
	 * Gibt true zurück, falls automatische Skalierung verwendet wird.
	 *
	 * @return true, falls automatische Skalierung verwendet wird
	 */
	inline virtual bool getScaleAuto() const { return scale_auto_; }

	/**
	 * Setzt die Gleichung(en) für das Fehlermodell (Experimental
	 * Design). Das übergebene Array von Expressions enthält entweder
	 * genau eine Gleichung, die für alle Messwerte dupliziert wird,
	 * oder genau dim Gleichungen (für jeden Messwert eine).
	 *
	 * @param error_model Fehlermodellgleichungen
	 */
	virtual void setErrorModel(symb::ExprTree ** error_model);

	/**
	 * Gibt das Array mit den dim Fehlermodellgleichungen zurück.
	 *
	 * @return Array mit Fehlermodellgleichungen
	 */
	inline virtual symb::ExprTree ** getErrorModel() const
	{
		return error_model_;
	}
	
	/**
	 * Gibt eine angegebene Fehlermodellgleichung zurück.
	 *
	 * @param idx Messwertindex
	 * @return Fehlermodellgleichung 
	 */
	inline virtual symb::ExprTree const * getErrorModel(size_t idx) const
	{
		fASSERT(idx < dim_);
		return error_model_[idx];
	}

	/**
	 * Gibt die Dimension des durch die Messgruppe beschriebenen
	 * Messwert-Vektors zurück.
	 *
	 * @return Dimension eines Messwert-Vektors
	 */
	inline size_t getDim() const { return dim_; }
                
	/**
	 * Registrierung eines Messwerts.
	 * Muss in einer abgeleiteten Klasse implementiert werden.
	 *
	 * @param mvalue Messwert-Objekt
	 */
	virtual void registerMValue(MValue const & mvalue) = 0;

	/**
	 * Gibt alle Messwerte zu einem gegebenen Timestamp als Vektor
	 * zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param x_meas MVector-Objekt mit Messwerten
	 * @param x_stddev MVector-Objekt mit Standardabweichungen
	 * @return true, falls Timestamp gültig
	 */
	virtual bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const = 0;

	/**
	 * Gibt die zugrundeliegende Spezifikation (Zeichenkette)
	 * zurück.
	 *
	 * @return Zeichenkette mit Spezifikation der Messgruppe
	 */
	inline char const * getSpec() const { return spec_; }

	/**
	 * Gibt die Spezifikation eines einzelnen Messwerts (Zeichenkette)
	 * zurück.
	 *
	 * @param idx Messwertindex
	 * @return Spezifikation eines einzelnen Messwerts
	 */
	virtual char const * getSpec(size_t idx) const = 0;
	
	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	virtual uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

protected:
	/**
	 * Berechnet die automatische Skalierung für simulierte Messwerte.
	 *
	 * @param x_sim simulierte Messwerte, zu skalieren (in/out)
	 * @param x_meas reale Messwerte (in)
	 * @param x_stddev angenommene Standardabweichungen pro Messung (in)
	 */
	template< typename Stype > Stype compute_groupscale(
		la::GVector< Stype > const & x_sim,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		) const
	{
		// Kleinste-Quadrate Schätzer (mit Varianzen)
		//
		// Sigma^(-1) = diag(1./(x_stddev.*x_stddev))
		// w = (x_sim^T*Sigma^(-1)*x_sim)^(-1)*(x_sim^T*Sigma^(-1))*x_meas
		//
		// daher:
		//
		// w = \sum_{i=1}^N x_sim(i)*x_meas(i)/(x_stddev(i)*x_stddev(i))
		//     / \sum_{i=1}^N x_sim(i)^2/(x_stddev(i)*x_stddev(i))
		//
		
		Stype vx, n_sum, d_sum;
		n_sum = d_sum = 0.;
		for (size_t i=0; i<x_sim.dim(); i++)
		{
			vx = x_stddev.get(i); vx *= vx;
			n_sum += x_sim.get(i) * x_meas.get(i) / vx;
			d_sum += x_sim.get(i) * x_sim.get(i) / vx;
		}
		// alle simulierten Werte 0?
		if (d_sum <= 10.*std::numeric_limits<double>::epsilon())
			return Stype(1.);
		return n_sum / d_sum;
	}

	/**
	 * Berechnet die automatische Skalierung für simulierte Messwerte.
	 *
	 * Diese Methode bietet zusätzlich die Möglichkeit der Kovarianzen
	 * zu verarbeiten (wird zur Zeit nicht verwendet).
	 *
	 * @param x_sim simulierte Messwerte, zu skalieren (in/out)
	 * @param x_meas reale Messwerte (in)
	 * @param Si Inverse(!) der Kovarianzmatrix
	 */
	template< typename Stype > void compute_groupscale(
		la::GVector< Stype > const & x_sim,
		la::MVector const & x_meas,
		la::MMatrix const & Si
		) const
	{
		size_t i,j;
		fASSERT( x_sim.dim() == Si.rows() );
		fASSERT( x_sim.dim() == Si.cols() );

		// Kleinste-Quadrate Schätzer (mit Kovarianzmatrix)
		//
		// w = (x_sim^T*S^(-1)*x_sim)^(-1)*(x_sim^T*S^(-1))*x_meas
		//
		// daher:
		//
		// w = \sum_{i=1}^N x_sim(i)*x_meas(i)/(x_stddev(i)*x_stddev(i))
		//     / \sum_{i=1}^N x_sim(i)^2/(x_stddev(i)*x_stddev(i))
		//

		// t1 := x_sim^T*S^-1
		la::GVector< Stype > t1(x_sim.dim());
		for (j=0; j<x_sim.dim(); j++)
			for (i=0; i<x_sim.dim(); i++)
				t1(j) += x_sim.get(i) * Si.get(i,j);
		// t2 := (x_sim^T*S^-1*x_sim)^-1
		Stype t2 = 0.;
		for (i=0; i<x_sim.dim(); i++)
			t2 += t1.get(i) * x_sim.get(i);
		t2 = 1./t2;
		// t3 := t1*x_meas
		Stype t3 = 0.;
		for (i=0; i<x_sim.dim(); i++)
			t3 += t1.get(i) * x_meas.get(i);
		// w := (x_sim^T*S^-1*x_sim)^-1 * (x_sim^T*S^-1*x_meas)
		return t2 * t3;
	}
	
	/**
	 * Ableitung des Group-Scale-Faktors nach einem freien Fluss.
	 * Als Nebenprodukt wird der Group-Scale-Faktor selbst berechnet.
	 *
	 * @param x_sim Simulierte Messwerte
	 * @param x_meas reale Messwerte
	 * @param x_stddev Standardabweichungen für reale Messwerte
	 * @param dxsim_dflux Ableitung der simulierten Messwerte nach
	 * 	dem freien Fluss
	 * @param gs ermittelter Group-Scale-Wert (out)
	 * @return Ableitung des Group-Scale-Werts nach dem freien Fluss
	 */
	template< typename Stype > Stype compute_dgroupscale_dflux(
		la::GVector< Stype > const & x_sim,
		la::MVector const & x_meas,
		la::MVector const & x_stddev,
		la::GVector< Stype > const & dx_sim_dflux,
		Stype & gs
		) const
	{
		Stype S1, S2, S3, S4, V;
		S1 = S2 = S3 = S4 = 0.;

		// Dimensionen der Vektoren müssen passen:
		fASSERT( x_meas.dim() == dim_ );
		fASSERT( x_sim.dim() == dim_ );
		fASSERT( x_stddev.dim() == dim_ );
		fASSERT( dx_sim_dflux.dim() == dim_ );

		for (size_t i=0; i<dim_; i++)
		{
			// Varianz
			V = x_stddev.get(i); V *= V;
			S1 += x_meas.get(i) * dx_sim_dflux.get(i) / V;
			S2 += x_sim.get(i) * x_meas.get(i) / V;
			S3 += x_sim.get(i) * dx_sim_dflux.get(i) / V;
			S4 += x_sim.get(i) * x_sim.get(i) / V;
		}
		
		// alle simulierten Werte 0?
		if (S4 <= 10.*std::numeric_limits<double>::epsilon())
		{
			gs = Stype(1.);
			return Stype(0.);
		}
		
		// Group-Scale-Wert:
		gs = S2/S4;
		// Ableitung des Group-Scale-Werts:
		return (S1 - 2.*S2*S3/S4)/S4;
	}

}; // class MGroup

/*
 * *****************************************************************************
 * Eine "einfacher" Messgruppentyp (Fluß, Poolgröße, generic).
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */ 

class SimpleMGroup : public MGroup
{
protected:
	/** Abbildung von Timestamp auf Messwert */
	std::map< double,MValue* > mvalue_map_;

protected:
	/**
	 * Constructor (protected!).
	 *
	 * @param mgtype Typ der Messung
	 * @param dim Dimension der Messung
	 */
	inline SimpleMGroup(
		MGroupType mgtype,
		char const * spec
		)
		: MGroup(mgtype, 1, spec) { }

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	SimpleMGroup(SimpleMGroup const & copy);

	/**
	 * Assignment operator
	 *
	 * @param copy zu kopierendes MGroup-Objekt
	 */
	SimpleMGroup & operator= (SimpleMGroup const & copy);

	/**
	 * Destructor.
	 */
	virtual ~SimpleMGroup();

	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual SimpleMGroup * clone() const
	{
		return new SimpleMGroup(*this);
	}

public:
	/**
	 * Registriert alle Messwerte über eine einfache Schnittstelle.
	 *
	 * @param x_meas Messwerte
	 * @param x_stddev Standardabweichungen
	 */	
	virtual void setMValuesStdDev(
		double ts,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		);
        
        /**
	 * löscht alle registrierten Messwerte.
	 */
        virtual void removeMValuesStdDev();

	/**
	 * Gibt zu einem gegebenen Timestamp-Wert einen Zeiger auf einen
	 * Messwert (MValue) zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @return Zeiger auf Messwert
	 */
	virtual MValue const * getMValue(double ts) const;
	
	/**
	 * Registriert einen neuen Messwert.
	 *
	 * @param mvalue Messwert-Objekt
	 */
	virtual void registerMValue(MValue const & mvalue);

	/**
	 * Gibt den Messwert zu einem gegebenen Timestamp als Vektor
	 * zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param x_meas MVector-Objekt mit Messwerten
	 * @param x_stddev MVector-Objekt mit Standardabweichungen
	 * @return true, falls Timestamp gültig
	 */
	virtual bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const;

	/**
	 * Normberechnung.
	 *
	 * @param nx simulierter Wert
	 * @param ts Timestamp
	 * @param gs berechneter Group-Scale-Wert (out), immer 1
	 * @return Norm
	 */
	template< typename Stype > Stype norm(Stype nx, double ts, Stype & gs) const
	{
		MValue const * mv = getMValue(ts);
		if (mv == 0)
			fTHROW(XMLException,
				"fatal: missing measurement value detected (timestamp %f)",
				ts);
		// (sim-meas)*Sigma^(-1)*(sim-meas)
		Stype n = nx-mv->get(); n *= n;
		Stype s = mv->getStdDev(); s *= s;
		gs = Stype(1.);
		return n/s;
	}

	/**
	 * Ableitung der Norm nach einem freien Fluss.
	 * Doku: Siehe Aufzeichnungen.
	 *
	 * @param nx simulierter Wert
	 * @param nx_dflux Ableitung des simulierten Wertes nach einem
	 * 	freien Fluss
	 * @param ts Timestamp
	 * @return Ableitung der Norm
	 */
	template< typename Stype > Stype dnorm_dflux(
		Stype nx,
		Stype nx_dflux,
		double ts,
		Stype & gs
		) const
	{
		MValue const * mv = getMValue(ts);
		if (mv == 0)
			fTHROW(XMLException,
				"fatal: missing measurement value detected (timestamp %f)",
				ts);
		Stype s(mv->getStdDev()); s *= s;
		gs = Stype(1.);
		return -2.*(mv->get() - nx) * nx_dflux / s;
	}
	
	/**
	 * Messwertspezifikation: SimpleMGroup beschreibt einen einzelnen
	 * Messwert.
	 *
	 * @param idx Index des Messwerts
	 * @return Messwertspezifikation
	 */
	inline char const * getSpec(size_t idx) const { return MGroup::getSpec(); }
	
	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	virtual uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class SimpleMGroup

/*
 * *****************************************************************************
 * Ein Messgruppentyp der einen einzelnen Metaboliten beschreibt
 * (MS, MSMS, 1H-NMR, 13C-NMR).
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class MetaboliteMGroup : public MGroup
{
public:
	/**
	 * Typ/Ausgangsdaten der Messwert-Simulation:
	 *  - Isotopomer Fractions
	 *  - Cumomer Fractions
	 *  - EMUs
	 */
	enum SimDataType { sdt_isotopomer, sdt_cumomer, sdt_emu };

protected:
	/** Bezeichnung des Metaboliten (Poolname) */
	char * mname_;
	/** Anzahl der Atome */
	int natoms_;

protected:
	/**
	 * Constructor (protected!).
	 *
	 * @param mname fremd-allokierter Metabolitname
	 */
	inline MetaboliteMGroup(
		MGroupType mgtype,
		size_t dim,
		char * mname,
		char const * spec = 0
		) : MGroup(mgtype,dim,spec), mname_(mname), natoms_(-1) { }

protected:
	/**
	 * Gemeinsam verwendete Auswertung der Markierungsnorm.
	 *
	 * @param x_sim Simulierte Messwerte (mit Groupscales multipliziert)
	 * @param ts Timestamp
	 * @return Wert der Norm
	 */
	template< typename Stype > Stype norm_common(
		la::GVector< Stype > const & x_sim,
		double ts
		) const
	{
		// reale Messwerte
		la::MVector x_meas(dim_), x_stddev(dim_);
		if (not getMValuesStdDev(ts,x_meas,x_stddev))
			fTHROW(XMLException,"invalid timestamp [%f]", ts);
                
		// Mit Standardabweichungen gewichtete L2 Norm berechnen
		size_t i;
		Stype w,ni,n(0.);
//                fWARNING("[%s] => Norm", this->getMetaboliteName());
//                fWARNING("sim:");
//                for(i=0;i<x_sim.dim();++i)
//                    printf("%g, ", toDouble<Stype>(x_sim[i]));
//                printf("\n");
//                
//                fWARNING("meas:");
//                for(i=0;i<x_meas.dim();++i)
//                    printf("%g, ", toDouble<Stype>(x_meas[i]));
//                printf("\n\n");
                
		for (i=0; i<x_sim.dim(); i++)
		{
			// (Relativen) Wichtungsfaktor aus Standardabweichung berechnen
			// entspricht (x_sim-x_meas)^T*Sigma^-1*(x_sim-x_meas)
			w = 1./x_stddev.get(i);
			w = w * w;

			// Norm
			ni = x_sim.get(i)-x_meas.get(i);
			ni = w*ni*ni;

			n += ni;
		}
		return n;
	}

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	inline MetaboliteMGroup(MetaboliteMGroup const & copy)
		: MGroup(copy), mname_(0), natoms_(copy.natoms_)
	{
		mname_ = strdup_alloc(copy.mname_);
	}
	

	inline MetaboliteMGroup & operator= (MetaboliteMGroup const & copy)
	{
		MGroup::operator= (copy);
		natoms_ = (copy.natoms_);
		mname_ = strdup_alloc(copy.mname_);
		return *this;
	}


	/**
	 * Virtueller Destructor.
	 */
	inline virtual ~MetaboliteMGroup() { if (mname_) delete[] mname_; }

	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	virtual MetaboliteMGroup * clone() const = 0;

public:
	/**
	 * Gibt die Metabolitbezeichnung zurück.
	 */
	inline virtual char const * getMetaboliteName() const { return mname_; }

	/**
	 * Gibt die Anzahl der Atome zurück.
	 */
	inline virtual int getNumAtoms() const { return natoms_; }

	/**
	 * Setzt die Anzahl der Atome (beim Parsen der Spezifikation unbekannt).
	 *
	 * @param natoms Anzahl der Atome
	 */
	inline virtual void setNumAtoms(unsigned int natoms) { natoms_ = natoms; }

	/**
	 * Gibt die Simulationsmaske zurück (Isotopomer-Simulation).
	 *
	 * @return Simulationsmaske
	 */
	virtual BitArray getSimMask() const = 0;

	/**
	 * Gibt die Menge der zu simulierenden Unbekannten zurück
	 * (Isotopomer Fractions, Cumomer Fractions, EMUs).
	 *
	 * @param sdt Datenbasis der Simulation (Iso, Cumo, EMU)
	 * @return Menge von Indizes der zu simulierenden Unbekannten
	 */
	virtual std::set< BitArray > getSimSet(SimDataType sdt) const = 0;

	/**
	 * Gibt alle Messwerte zu einem gegebenen Timestamp als Vektor
	 * zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param x_meas MVector-Objekt mit Messwerten
	 * @param x_stddev MVector-Objekt mit Standardabweichungen
	 * @return true, falls Timestamp gültig
	 */
	virtual bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const = 0;

	/**
	 * Wertet das Messmodell auf Basis von EMUs oder Cumomer-Fractions
	 * aus. Dient als Multiplexer für die evaluate()-Methoden in den
	 * abgeleiteten Klassen (virtuelle template-Members sind in C++ nicht
	 * erlaubt).
	 *
	 * @param Ftype Typ der Fraction (CumulativeVector, CumulativeScalar)
	 * @param Stype Zahlentyp (double, mpq_class)
	 * @param ts gültiger Timestamp
	 * @param values Abbildung von (EMU-/Cumomer-)Index auf Fraction(s)
	 * @param allow_scaling Flag, automatische Messgruppen-Skalierung verwenden
	 * @param gs Verwendeter Skalierungsfaktor für Messgruppen (out)
	 * @return MVector-Object mit simulierten Messwerten
	 */
	template< typename Ftype,typename Stype > la::GVector< Stype > evaluate(
		double ts,
		fhash_map< BitArray,Ftype,BitArray_hashf > const & values,
		bool allow_scaling,
		Stype & gs
		) const;

	/**
	 * Wertet die Ableitung des Messmodells auf Basis von EMUs oder
	 * Cumomer-Fractions aus. Greift auf die evaluate()-Methode zu
	 * und ist deshalb für alle von MetaboliteMGroup abgeleiteten
	 * Klassen gültig.
	 *
	 * @param ts gültiger Timestamp
	 * @param values Abbildung von Indices auf (EMU-/Cumomer-)Fractions
	 * @param dvalues Abbildung von Indices auf (EMU-/Cumomer-)Ableitungen
	 * @param allow_scaling Flag, automatische Messgruppen-Skalierung verwenden
	 * @param gs Verwendeter Skalierungsfaktor für Messgruppen (out)
	 * @param dgs Ableitung des verwendetet Skalierungsfaktors (out)
	 * @return Vektor mit Ableitungen der simulierten Messwerte
	 */
	template< typename Ftype,typename Stype > la::GVector< Stype > devaluate(
		double ts,
		fhash_map< BitArray,Ftype,BitArray_hashf > const & values,
		fhash_map< BitArray,Ftype,BitArray_hashf > const & dvalues,
		bool allow_scaling,
		Stype & gs,
		Stype & dgs
		) const
	{
		if (not (allow_scaling and scale_auto_ and dim_>1))
		{
			// Ohne Messwertskalierung wird das Messmodell einfach
			// noch einmal mit den Ableitungen ausgewertet.
			dgs = Stype(0.);
			return evaluate(ts,dvalues,false,gs);
		}

		// Bei eingeschalteter Messwertskalierung muß der Skalierungsfaktor
		// abgeleitet werden
		la::GVector< Stype > x_sim = evaluate(ts,values,false,gs);
		la::GVector< Stype > dx_sim_dflux = evaluate(ts,dvalues,false,gs);
		la::MVector x_meas(dim_), x_stddev(dim_);

		bool ts_valid = getMValuesStdDev(ts,x_meas,x_stddev);
		// ein ungültiger Timestamp fällt schon früher auf:
		fASSERT(ts_valid);

		// d omega/d flux und omega berechnen:
		dgs = compute_dgroupscale_dflux(x_sim,x_meas,x_stddev,dx_sim_dflux,gs);
		
		// Ableitung via Produktregel:
		return dgs * x_sim + gs * dx_sim_dflux;
	}

	
	/**
	 * Statische Factory-Methode.
	 * Parst alle von MetaboliteMGroup abgeleiteten Messgruppen.
	 *
	 * @param s Spezifikation der Messgruppe als String
	 * @param state Fehlercode des Parsers (optional)
	 * @return Zeiger auf generiertes Messgruppen-Objekt
	 */
	static MetaboliteMGroup * parseSpec(
		char const * s,
		int * state = 0
		);

	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	virtual uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class MetaboliteMGroup

/*
 * *****************************************************************************
 * Massenspektrometer-Messgruppe.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class MGroupMS : public MetaboliteMGroup
{
protected:
	/** Bitmaske der gemessenen Atom-Positionen (Netzwerkreduktion!) */
	BitArray mask_;

	/** Array der durch Messungen beschriebenen Massengewichte */
	int * weights_;

	/** Abbildung von Timestamp auf (Massengewicht,Messwert) */
	std::map< double,std::map< int,MValue* > > ms_mvalue_map_;

protected:
	/**
	 * Constructor.
	 * Nicht von außen aufrufbar; Factory-Methode parseSpec verwenden!
	 *
	 * @param mname Name des Metaboliten
	 * @param mask Bitmaske der aktiven Atom-Positionen (Netzwerkreduktion!)
	 * @param weights Array mit beschriebenen Massengewichten
	 * 	(Vorsicht! Wird im Destructor deallokiert!)
	 * @param dim Dimension der Messung
	 */
	inline MGroupMS(
		char * mname,
		BitArray const & mask,
		int * weights,
		size_t dim,
		char const * spec = 0
		) : MetaboliteMGroup(mg_MS,dim,mname,spec), mask_(mask),
		    weights_(weights) { }

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupMS(MGroupMS const & copy);

	/**
	 * Assignment operator.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupMS & operator= (MGroupMS const & copy);
 
	/**
	 * Destructor.
	 * Deallokiert das fremd-allokierte Array weights_ (vorsicht!).
	 */
	virtual ~MGroupMS();
	
	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual MGroupMS * clone() const
	{
		return new MGroupMS(*this);
	}

protected:
	/**
	 * Wertet das Messmodell aus.
	 * Das zurückgegebene MVector-Objekt besitzt einen Aufbau
	 * gemäß Fragment-Maske und Gewichts-Vektor.
	 *
	 * @param iso MaskedArray-Objekt mit Isotopomer-Fractions
	 * @param ts gültiger Timestamp
	 * @param allow_scaling
	 * @param gs verwendeter Group-Scale-Faktor (out)
	 * @return MVector-Objekt mit simulierten Messwerten
	 */
	template< typename Stype > la::GVector< Stype > evaluate(
		double ts,
		GenMaskedArray< Stype > & iso,
		bool allow_scaling,
		Stype & gs
		) const
	{
		// Vektor mit ALLEN Gewichten (0 bis Anzahl gesetzter Bits der Maske)
		la::GVector< Stype > x_all(mask_.countOnes()+1); // alle isotopomer
		la::GVector< Stype > x_sim(getNumWeights()); // zu simulierende Isotopomers

		// 1. Die Maske gibt vor an welchen Positionen das Gewicht
		//    "gemessen" wird. Hier wird ein bitweise & von Maske und
		//    Isotopomer-Index berechnet. Das "Gewicht" des Produkts
		//    (=Anzahl der gesetzten Bits) entspricht dem Gewicht des
		//    Fragments.
		// 2. Hier werden nur die Isotopomere des Vektors iso aufgezählt,
		//    die auch der Iterator aufzählt. Andere Isotopomere sind
		//    garantiert immer 0 und tragen nichts zu x_all bei.
		for (typename GenMaskedArray< Stype >::iterator i=iso.begin(); i!=iso.end(); i++)
			x_all( (mask_ & i->idx).countOnes() ) += i->value; 

		// 3. Gewünschte Gewichte kopieren
		for (size_t j=0; j<x_sim.dim(); j++)
			x_sim(j) = x_all(weights_[j]);

		// 4. ggfs. Messgruppe automatisch skalieren.
		//    Aber nur dann, wenn mehr als ein Messwert vorliegt
		if (allow_scaling and scale_auto_ and x_sim.dim() > 1)
		{
			la::MVector x_meas(dim_), x_stddev(dim_);
			if (getMValuesStdDev(ts,x_meas,x_stddev))
			{
				// Skalierungsfaktor berechnen und Skalieren
				gs = compute_groupscale(x_sim,x_meas,x_stddev);
				x_sim *= gs;                                
			}
			else fTHROW(XMLException,"invalid timestamp [%f]", ts);
		}
		else gs = Stype(1.);

		// 5. x_sim zurückgeben, x_all verwerfen
		return x_sim;
	}

public:
	/**
	 * Registriert alle Messwerte über eine einfache Schnittstelle.
	 *
	 * @param x_meas Messwerte
	 * @param x_stddev Standardabweichungen
	 */	
	virtual void setMValuesStdDev(
		double ts,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		);
        
        /**
	 * löscht alle registrierten Messwerte.
	 */
	virtual void removeMValuesStdDev();

	/**
	 * Statische Factory-Methode zum Parsen einer MS-Spezifikation.
	 *
	 * @param s MS-Spezifikation (Zeichenkette)
	 * @param state Fehlercode des Parsers, optional
	 * @return Zeiger auf allokiertes MGroupMS-Objekt
	 */
	static MGroupMS * parseSpec(char const * s, int * state = 0);

	/**
	 * Gibt das Array der beschriebenen Massengewichte zurück.
	 *
	 * @return Array der Massengewichte
	 */
	inline int const * getWeights() const { return weights_; }

	/**
	 * Gibt die Anzahl der Massengewichte zurück.
	 *
	 * @return Anzahl der Massengewichte
	 */
	inline size_t getNumWeights() const { return dim_; }

	/**
	 * Setzt die Anzahl der Atome und schneidet die Maske auf passende
	 * Länge zurecht. Evtl. wäre hier Fehlerprüfung noch wichtig.
	 *
	 * @param natoms Anzahl der Atome
	 */
	inline void setNumAtoms(unsigned int natoms)
	{
		natoms_ = natoms;
		mask_.resize(natoms,false);
	}

	/**
	 * Gibt die Bitmaske der aktiven Atome zurück (Netzwerkreduktion!).
	 *
	 * @return Bitmaske der aktiven Atom-Positionen
	 */
	inline BitArray const & getMask() const { return mask_; }

	/**
	 * Gibt die Simulationsmaske zurück (Schnittstellenimplementierung)
	 *
	 * @return Simulationsmaske
	 */
	inline BitArray getSimMask() const { return mask_; }

	/**
	 * Gibt die Menge der zu simulierenden Unbekannten zurück
	 * (Isotopomer Fractions, Cumomer Fractions, EMUs).
	 *
	 * @param sdt Datenbasis der Simulation (Iso, Cumo, EMU)
	 * @return Menge von Indizes der zu simulierenden Unbekannten
	 */
	std::set< BitArray > getSimSet(SimDataType sdt) const;

	/**
	 * MS-spezifische Implementierung von getMValue;
	 * gibt zu gegebenem Gewicht und Timestamp einen Messwert zurück
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param weight Massengewichts-Inkrement
	 * @return Zeiger auf Messwert-Objekt, falls existent
	 */
	MValue const * getMValue(double ts, int weight) const;

	/**
	 * Gibt alle Messwerte zu einem gegebenen Timestamp als Vektor
	 * zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param x_meas MVector-Objekt mit Messwerten
	 * @param x_stddev MVector-Objekt mit Standardabweichungen
	 * @return true, falls Timestamp gültig
	 */
	bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const;

	/**
	 * Registriert ein Messwert-Objekt.
	 *
	 * @param mvalue Messwert-Objekt
	 */
	void registerMValue(MValue const & mvalue);

	/**
	 * Gibt die Spezifikation eines einzelnen Messwerts zurück.
	 *
	 * @param idx Index
	 * @return Messwertspezifikation
	 */
	char const * getSpec(size_t idx) const;
	
	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;
	
}; // class MGroupMS

/*
 * *****************************************************************************
 * Multi-Isotopic Tracer basierte Massenspektrometer-Messgruppe.
 * 
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

    class MGroupMIMS : public MetaboliteMGroup
    {
    protected:
            /** Bitmaske der gemessenen Atom-Positionen (Netzwerkreduktion!) */
            BitArray mask_;                

            /** Array der durch Messungen beschriebenen Massengewichte */
            std::vector<int *> weights_vec_;

            /** Abbildung von Timestamp auf (Massengewichte,Messwert) */
            std::map< double,std::map< std::vector<int> ,MValue* > > mims_mvalue_map_;

protected:
	/**
	 * Constructor.
	 * Nicht von außen aufrufbar; Factory-Methode parseSpec verwenden!
	 *
	 * @param mname Name des Metaboliten
	 * @param mask Bitmaske der aktiven Atom-Positionen (Netzwerkreduktion!)
	 * @param weights Array mit beschriebenen Massengewichten
	 * 	(Vorsicht! Wird im Destructor deallokiert!)
	 * @param dim Dimension der Messung
	 */
	inline MGroupMIMS(
		char * mname,
		BitArray const & mask,
                std::vector<int *> weights_vec,
		size_t dim,
		char const * spec = 0
		) : MetaboliteMGroup(mg_MIMS,dim,mname,spec), mask_(mask)
                {
                    // Speicher anlegen
                    weights_vec_.resize(weights_vec.size());
                    std::vector<int *>::iterator wi,wi_;
                    for(wi_=weights_vec_.begin();wi_!=weights_vec_.end();wi_++)
                    {
                        *wi_= new int[dim+1];
                        (*wi_)[dim] = -1;
                    }
                    
                    // Gewichte übertragen
                    wi= weights_vec.begin();
                    wi_=weights_vec_.begin();
                    while(wi_!=weights_vec_.end() && wi!=weights_vec.end())
                    {
                        for(size_t i=0; i<dim;++i)
                            (*wi_)[i] = (*wi)[i];
                        wi_++;
                        wi++;    
                    }
                }

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupMIMS(MGroupMIMS const & copy);

	/**
	 * Assignment operator.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupMIMS & operator= (MGroupMIMS const & copy);
 
	/**
	 * Destructor.
	 * Deallokiert das fremd-allokierte Array weights_ (vorsicht!).
	 */
	virtual ~MGroupMIMS();
	
	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual MGroupMIMS * clone() const
	{
		return new MGroupMIMS(*this);
	}

protected:
	/**
	 * Wertet das Messmodell aus.
	 * Das zurückgegebene MVector-Objekt besitzt einen Aufbau
	 * gemäß Fragment-Maske und Gewichts-Vektor.
	 *
	 * @param iso MaskedArray-Objekt mit Isotopomer-Fractions
	 * @param ts gültiger Timestamp
	 * @param allow_scaling
	 * @param gs verwendeter Group-Scale-Faktor (out)
	 * @return MVector-Objekt mit simulierten Messwerten
	 */
	template< typename Stype > la::GVector< Stype > evaluate(
		double ts,
		GenMaskedArray< Stype > & iso,
		bool allow_scaling,
		Stype & gs
		) const
	{
                std::map< std::vector<int> , Stype> x_values;
                la::GVector< Stype > x_sim(dim_);
                BitArray cfgMask(iso.getMask());               
                size_t i;
                
                // Initialisere alle gemessenen Gewichtstupel
                for (i=0; i<dim_; ++i)
                {
                    std::vector<int> weigths;
                    for(std::vector<int *>::const_iterator wi=weights_vec_.begin();
                        wi!=weights_vec_.end();wi++)
                            weigths.push_back((*wi)[i]);
                    x_values[weigths]= 0.;
                }
                
                // Extrahierung der simulierten Werte entsprechend der Gewichtstupel
                typename std::map< std::vector<int> , Stype>::iterator iv;
		for (typename GenMaskedArray< Stype >::iterator i=iso.begin(); i!=iso.end(); i++)
                {
                    std::vector<int> weigths;
                    size_t pos= 0;
                    for(charptr_map< int >::const_iterator ic=this->iso_cfg_.begin(); 
                        ic!= this->iso_cfg_.end(); ic++)
                    {
                        size_t N = ic->value;
                        cfgMask.zeros();
                        for(size_t k=pos; k<(pos+N); k++)
                            cfgMask.set(k,true);
                        weigths.push_back((cfgMask & i->idx).countOnes());                    
                        pos+= N;
                    }
                    iv = x_values.find(weigths);
                    if(iv!= x_values.end())
                        iv->second += i->value;
                }
                
                // Übertragung von simulierten Werte in den SimulationsVektor
//                size_t j=0;
                for (i=0; i<dim_; ++i)
                {
                    std::vector<int> weigths;
//                    j=0;
//                    printf("x-sim(");
                    for(std::vector<int *>::const_iterator wi=weights_vec_.begin();
                        wi!=weights_vec_.end();wi++)
                    {
                            weigths.push_back((*wi)[i]);
//                            printf("%i, ", weigths[j++]);
                    }
                    
                    iv = x_values.find(weigths);
                    if(iv!= x_values.end())
                        x_sim(i)= iv->second;
                    
//                    printf(") => ");
//                    printf("  val= %.6g \t", toDouble<Stype>(x_sim(i)));
//                    printf("\n");
                }
//                getchar();
                
		if (allow_scaling and scale_auto_ and x_sim.dim() > 1)
		{
                    la::MVector x_meas(dim_), x_stddev(dim_);
                    if (getMValuesStdDev(ts,x_meas,x_stddev))
                    {
                        // Skalierungsfaktor berechnen und Skalieren
                        gs = compute_groupscale(x_sim,x_meas,x_stddev);
                        // Skalieren
                        x_sim *= gs;
                    }
                    else fTHROW(XMLException,"invalid timestamp [%f]", ts);
		}
		else gs = Stype(1.);

		return x_sim;
	}

public:
	/**
	 * Registriert alle Messwerte über eine einfache Schnittstelle.
	 *
	 * @param x_meas Messwerte
	 * @param x_stddev Standardabweichungen
	 */	
	virtual void setMValuesStdDev(
		double ts,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		);

        /**
	 * löscht alle registrierten Messwerte.
	 */
        virtual void removeMValuesStdDev();
        
	/**
	 * Statische Factory-Methode zum Parsen einer MS-Spezifikation.
	 *
	 * @param s MS-Spezifikation (Zeichenkette)
	 * @param state Fehlercode des Parsers, optional
	 * @return Zeiger auf allokiertes MGroupMS-Objekt
	 */
	static MGroupMIMS * parseSpec(char const * s, int * state = 0);

	/**
	 * Gibt das Array der beschriebenen Massengewichte zurück.
	 *
	 * @return Array der Massengewichte
	 */
	inline std::vector<int *> const & getWeights() const { return weights_vec_; }

	/**
	 * Gibt die Anzahl der Massengewichte zurück.
	 *
	 * @return Anzahl der Massengewichte
	 */
	inline size_t getNumWeights() const { return weights_vec_.size(); }
        
	/**
	 * Setzt die Anzahl der Atome und schneidet die Maske auf passende
	 * Länge zurecht. Evtl. wäre hier Fehlerprüfung noch wichtig.
	 *
	 * @param natoms Anzahl der Atome
	 */
	inline void setNumAtoms(unsigned int natoms)
	{
		natoms_ = natoms;
		mask_.resize(natoms,false);
	}

	/**
	 * Gibt die Bitmaske der aktiven Atome zurück (Netzwerkreduktion!).
	 *
	 * @return Bitmaske der aktiven Atom-Positionen
	 */
	inline BitArray const & getMask() const { return mask_; }

	/**
	 * Gibt die Simulationsmaske zurück (Schnittstellenimplementierung)
	 *
	 * @return Simulationsmaske
	 */
	inline BitArray getSimMask() const { return mask_; }

	/**
	 * Gibt die Menge der zu simulierenden Unbekannten zurück
	 * (Isotopomer Fractions, Cumomer Fractions, EMUs).
	 *
	 * @param sdt Datenbasis der Simulation (Iso, Cumo, EMU)
	 * @return Menge von Indizes der zu simulierenden Unbekannten
	 */
	std::set< BitArray > getSimSet(SimDataType sdt) const;

	/**
	 * MS-spezifische Implementierung von getMValue;
	 * gibt zu gegebenem Gewicht und Timestamp einen Messwert zurück
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param weight Massengewichts-Inkrement
	 * @return Zeiger auf Messwert-Objekt, falls existent
	 */
	MValue const * getMValue(double ts, std::vector<int> weight) const;

	/**
	 * Gibt alle Messwerte zu einem gegebenen Timestamp als Vektor
	 * zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param x_meas MVector-Objekt mit Messwerten
	 * @param x_stddev MVector-Objekt mit Standardabweichungen
	 * @return true, falls Timestamp gültig
	 */
	bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const;

	/**
	 * Registriert ein Messwert-Objekt.
	 *
	 * @param mvalue Messwert-Objekt
	 */
	void registerMValue(MValue const & mvalue);


	/**
	 * Gibt die Spezifikation eines einzelnen Messwerts zurück.
	 *
	 * @param idx Index
	 * @return Messwertspezifikation
	 */
	char const * getSpec(size_t idx) const;
	
	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;
	
}; // class MGroupMIMS

/*
 * *****************************************************************************
 * Tandem-Massenspektrometer-Messgruppe.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class MGroupMSMS : public MetaboliteMGroup
{
protected:
	/** Bitmaske der gemessenen Atom-Positionen (Netzwerkreduktion!) */
	BitArray mask1_;
	/** Bitmaske der gemessenen Atom-Positionen (Netzwerkreduktion!) */
	BitArray mask2_;

	/** Array der durch Messungen beschriebenen Massengewichte */
	int * weights1_;
	/** Array der durch Messungen beschriebenen Massengewichte */
	int * weights2_;

	/** Abbildung von Timestamp auf Abbildung (weight,weight)->Messwert */
	std::map< double,std::map< std::pair< int,int >,MValue* > >
		msms_mvalue_map_;

protected:
	/**
	 * Constructor.
	 * Nicht von außen aufrufbar; Factory-Methode parseSpec verwenden!
	 *
	 * @param mname Name des Metaboliten
	 * @param mask1 erste Bitmaske der aktiven Atom-Positionen (Netzwerkreduktion!)
	 * @param mask2 zweite Bitmaske der aktiven Atom-Positionen (Netzwerkreduktion!)
	 * @param weights1 erstes Array mit beschriebenen Massengewichten
	 * 	(Vorsicht! Wird im Destructor deallokiert!)
	 * @param weights2 zweites Array mit beschriebenen Massengewichten
	 * @param dim Dimension der Messung
	 */
	inline MGroupMSMS(
		char * mname,
		BitArray const & mask1,
		BitArray const & mask2,
		int * weights1,
		int * weights2,
		size_t dim,
		char const * spec = 0
		) : MetaboliteMGroup(mg_MSMS,dim,mname,spec),
		    mask1_(mask1), mask2_(mask2),
		    weights1_(weights1), weights2_(weights2) {}

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupMSMS(MGroupMSMS const & copy);

	/**
	 * Assignment operator.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupMSMS& operator= (MGroupMSMS const & copy);
 
	/**
	 * Virtueller Destructor.
	 * Gibt die fremd-allokierten Arrays frei.
	 */
	virtual ~MGroupMSMS();

	/**
	 * Virteller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual MGroupMSMS * clone() const
	{
		return new MGroupMSMS(*this);
	}

protected:
	/**
	 * Wertet das Messmodell aus.
	 * Das zurückgegebene MVector-Objekt besitzt einen Aufbau
	 * gemäß Fragment-Masken und Gewichts-Vektoren.
	 *
	 * @param iso MaskedArray-Objekt mit Isotopomer-Fractions
	 * @param ts gültiger Timestamp
	 * @return MVector-Objekt mit simulierten Messwerten
	 */
	template< typename Stype > la::GVector< Stype > evaluate(
		double ts,
		GenMaskedArray< Stype > & iso,
		bool allow_scaling,
		Stype & gs
		) const
	{
		// Matrix mit ALLEN Gewichtspaaren; wird nicht komplett genutzt
		// ist aber so schön unkompliziert ...
		la::GMatrix< Stype > X_all(mask1_.countOnes()+1,mask2_.countOnes()+1);
		la::GVector< Stype > x_sim(getNumWeights());

		for (typename GenMaskedArray< Stype >::iterator i=iso.begin(); i!=iso.end(); i++)
			X_all( (mask1_ & i->idx).countOnes(), (mask2_ & i->idx).countOnes() )
				+= i->value;

		for (size_t j=0; j<x_sim.dim(); j++)
			x_sim(j) = X_all(weights1_[j],weights2_[j]);

		if (allow_scaling and scale_auto_ and x_sim.dim() > 1)
		{
			la::MVector x_meas(dim_), x_stddev(dim_);
			if (getMValuesStdDev(ts,x_meas,x_stddev))
			{
				// Skalierungsfaktor berechnen und Skalieren
				gs = compute_groupscale(x_sim,x_meas,x_stddev);
				// Skalieren
				x_sim *= gs;
			}
			else fTHROW(XMLException,"invalid timestamp [%f]", ts);
		}
		else gs = Stype(1.);

		return x_sim;
	}

public:
	/**
	 * Registriert alle Messwerte über eine einfache Schnittstelle.
	 *
	 * @param x_meas Messwerte
	 * @param x_stddev Standardabweichungen
	 */	
	virtual void setMValuesStdDev(
		double ts,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		);

        /**
	 * löscht alle registrierten Messwerte.
	 */
        virtual void removeMValuesStdDev();
        
	/**
	 * Statische Factory-Methode.
	 * Parst eine Zeichenkette mit MSMS-Spezifikation, welche die
	 * Messung beschreibt.
	 *
	 * @param s Zeichenkette mit MSMS-Spezifikation
	 * @param state Fehlercode des Parsers (optional)
	 */
	static MGroupMSMS * parseSpec(char const * s, int * state = 0);
	
	/**
	 * Gibt das Array der beschriebenen Massengewichte zurück.
	 *
	 * @return Array der Massengewichte
	 */
	inline int const * getWeights1() const { return weights1_; }

	/**
	 * Gibt das Array der beschriebenen Massengewichte zurück.
	 *
	 * @return Array der Massengewichte
	 */
	inline int const * getWeights2() const { return weights2_; }

	/**
	 * Gibt die Anzahl der Massengewichte zurück.
	 *
	 * @return Anzahl der Massengewichte
	 */
	inline size_t getNumWeights() const { return dim_; }

	/**
	 * Setzt die Anzahl der Atome und schneidet die Masken auf passende
	 * Länge zurecht. Evtl. wäre hier Fehlerprüfung noch wichtig.
	 *
	 * @param natoms Anzahl der Atome
	 */
	inline void setNumAtoms(unsigned int natoms)
	{
		natoms_ = natoms;
		mask1_.resize(natoms,false);
		mask2_.resize(natoms,false);
	}

	/**
	 * Gibt die Bitmaske der aktiven Atome zurück (Netzwerkreduktion!).
	 *
	 * @return Bitmaske der aktiven Atom-Positionen
	 */
	inline BitArray const & getMask1() const { return mask1_; }

	/**
	 * Gibt die Bitmaske der aktiven Atome zurück (Netzwerkreduktion!).
	 *
	 * @return Bitmaske der aktiven Atom-Positionen
	 */
	inline BitArray const & getMask2() const { return mask2_; }
	
	/**
	 * Gibt die Simulationsmaske zurück (Schnittstellenimplementierung).
	 * Bei MSMS ist die zweite Maske eine Teilmenge der ersten!
	 *
	 * @return Simulationsmaske
	 */
	inline BitArray getSimMask() const { return mask1_; }

	/**
	 * Gibt die Menge der zu simulierenden Unbekannten zurück
	 * (Isotopomer Fractions, Cumomer Fractions, EMUs).
	 *
	 * @param sdt Datenbasis der Simulation (Iso, Cumo, EMU)
	 * @return Menge von Indizes der zu simulierenden Unbekannten
	 */
	std::set< BitArray > getSimSet(SimDataType sdt) const;

	/**
	 * MSMS-spezifische Implementierung von getMValue;
	 * gibt zu gegebenen Gewichten und Timestamp einen Messwert zurück
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param weight1 erstes Massengewichts-Inkrement
	 * @þaram weight2 zweites Massengewichts-Inkrement
	 * @return Zeiger auf Messwert-Objekt, falls existent
	 */
	MValue const * getMValue(double ts, int weight1, int weight2) const;
	
	/**
	 * Gibt alle Messwerte zu einem gegebenen Timestamp als Vektor
	 * zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param x_meas MVector-Objekt mit Messwerten
	 * @param x_stddev MVector-Objekt mit Standardabweichungen
	 * @return true, falls Timestamp gültig
	 */
	bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const;
	
	/**
	 * Registriert ein Messwert-Objekt.
	 *
	 * @param mvalue Messwert-Objekt
	 */
	void registerMValue(MValue const & mvalue);
	/**
	 * Gibt die Spezifikation eines einzelnen Messwerts zurück.
	 *
	 * @param idx Index des Messwerts
	 * @return Messwertspezifikation
	 */
	char const * getSpec(size_t idx) const;

	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class MGroupMSMS

/*
 * *****************************************************************************
 * 1H-NMR-Messgruppe.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class MGroup1HNMR : public MetaboliteMGroup
{
protected:
	/** 1H-NMR Atompositionsliste */
	int * poslst_;
	/** Abbildung von Timestamp auf (Atomposition,Messwert) */
	std::map< double,std::map< int,MValue* > > nmr1h_mvalue_map_;

protected:
	/**
	 * Constructor (protected!).
	 *
	 * @param mname Metabolitname (fremd-allokiert!)
	 * @param poslst Positionsliste (fremd-allokiert!)
	 * @param dim Dimension der Messung
	 */
	inline MGroup1HNMR(
		char * mname,
		int * poslst,
		size_t dim,
		char const * spec = 0
		)
		: MetaboliteMGroup(mg_1HNMR,dim,mname,spec),
		  poslst_(poslst) {}

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroup1HNMR(MGroup1HNMR const & copy);

	/**
	 * Assignment operator.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroup1HNMR& operator= (MGroup1HNMR const & copy);
 
	/**
	 * Virtueller Destructor.
	 * Gibt die fremd-allokierten Array poslst_ frei.
	 */
	virtual ~MGroup1HNMR();

	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual MGroup1HNMR * clone() const
	{
		return new MGroup1HNMR(*this);
	}

protected:
	/**
	 * Wertet das Messmodell aus.
	 * Die Länge des zurückgegebenen MVector-Objekts entspricht der
	 * Anzahl der Positionen.
	 *
	 * @param iso MaskedArray-Objekt mit Isotopomer-Fractions
	 * @param ts gültiger Timestamp
	 * @return MVector-Objekt mit simulierten Messwerten
	 */
	template< typename Stype > la::GVector< Stype > evaluate(
		double ts,
		GenMaskedArray< Stype > & iso,
		bool allow_scaling,
		Stype & gs
		) const
	{
		la::GVector< Stype > x_sim(getNumPositions());

		for (typename GenMaskedArray< Stype >::iterator i=iso.begin(); i!=iso.end(); i++)
		{
			// in jedem non-zero Isotopomer jede Position:
			for (int p=0; poslst_[p]!=-1; p++)
				if (i->idx.get(poslst_[p]-1))
					x_sim(p) += i->value; // markiert
		}

		if (allow_scaling and scale_auto_ and x_sim.dim() > 1)
		{
			la::MVector x_meas(dim_), x_stddev(dim_);
			if (getMValuesStdDev(ts,x_meas,x_stddev))
			{
				// Skalierungsfaktor berechnen und Skalieren
				gs = compute_groupscale(x_sim,x_meas,x_stddev);
				// Skalieren
				x_sim *= gs;
			}
			else fTHROW(XMLException,"invalid timestamp [%f]", ts);
		}
		else gs = Stype(1.);

		return x_sim;
	}

public:
	/**
	 * Registriert alle Messwerte über eine einfache Schnittstelle.
	 *
	 * @param x_meas Messwerte
	 * @param x_stddev Standardabweichungen
	 */	
	virtual void setMValuesStdDev(
		double ts,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		);

        /**
	 * löscht alle registrierten Messwerte.
	 */
        virtual void removeMValuesStdDev();
        
	/**
	 * Statische Factory-Methode.
	 * Parst eine Zeichenkette mit 1H-NMR-Spezifikation, welche die
	 * Messung beschreibt.
	 *
	 * @param s Zeichenkette mit 13C-NMR-Spezifikation
	 * @param state Fehlercode des Parsers (optional)
	 */
	static MGroup1HNMR * parseSpec(char const * s, int * state = 0);

	/**
	 * Gibt ein Array mit den Positionen der der 1H-NMR-Messung zurück.
	 *
	 * @return Array mit 1H-NMR-Atompositionen
	 */
	inline int const * getPositions() const { return poslst_; }

	/**
	 * Gibt die Anzahl der Positonen zurück, an denen gemessen wird.
	 *
	 * @return Anzahl der Positionen
	 */
	inline size_t getNumPositions() const { return dim_; }
	
	/**
	 * Gibt die Simulationsmaske zurück (Schnittstellenimplementierung).
	 *
	 * @return Simulationsmaske
	 */
	inline BitArray getSimMask() const
	{
		BitArray mask(natoms_);
		for (size_t i=0; i<dim_; i++)
			mask.set(poslst_[i]-1); // poslst_ zählt ab 1!
		return mask;
	}

	/**
	 * Gibt die Menge der zu simulierenden Unbekannten zurück
	 * (Isotopomer Fractions, Cumomer Fractions, EMUs).
	 *
	 * @param sdt Datenbasis der Simulation (Iso, Cumo, EMU)
	 * @return Menge von Indizes der zu simulierenden Unbekannten
	 */
	std::set< BitArray > getSimSet(SimDataType sdt) const;

	/**
	 * 1H-NMR-spezifische Implementierung von getMValue;
	 * gibt zu einer gegebenen Atomposition und einem Timestamp einen
	 * Messwert zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param pos Atomposition
	 * @return Zeiger auf Messwert-Objekt, falls existent
	 */
	MValue const * getMValue(double ts, int pos) const;

	/**
	 * Gibt alle Messwerte zu einem gegebenen Timestamp als Vektor
	 * zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param x_meas MVector-Objekt mit Messwerten
	 * @param x_stddev MVector-Objekt mit Standardabweichungen
	 * @return true, falls Timestamp gültig
	 */
	bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const;

	/**
	 * Registriert ein Messwert-Objekt.
	 *
	 * @param mvalue Messwert-Objekt
	 */
	void registerMValue(MValue const & mvalue);


	/**
	 * Spezifikation eines einzelnen Messwerts.
	 *
	 * @param idx Index des Messwerts
	 * @return Messwertspezifikation
	 */
	char const * getSpec(size_t idx) const;

	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class MGroup1HNMR

/*
 * *****************************************************************************
 * 13C-NMR-Messgruppe.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class MGroup13CNMR : public MetaboliteMGroup
{
public:
	/** 13C-NMR Typen */
	typedef MValue13CNMR::NMR13CType Type;

protected:
	/** 13C-NMR Atompositionsliste */
	int * poslst_;
	/** 13C-NMR Typenliste (Atompositionen zugeordnet) */
	Type * typelst_;
	/** Abbildung Timestamp auf Abbildung (Position,Typ)->Messwert */
	std::map< double,std::map< std::pair< int,Type >,MValue* > >
		nmr13c_mvalue_map_;

protected:
	/**
	 * Constructor (protected!).
	 *
	 * @param mname Metabolitname (fremd-allokiert!)
	 * @param poslst Positionsliste (fremd-allokiert!)
	 * @param typelst Typenliste (fremd-allokiert!)
	 * @param dim Dimension der Messung
	 */
	inline MGroup13CNMR(
		char * mname,
		int * poslst,
		Type * typelst,
		size_t dim,
		char const * spec = 0
		) : MetaboliteMGroup(mg_13CNMR,dim,mname,spec),
		    poslst_(poslst), typelst_(typelst) {}

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroup13CNMR(MGroup13CNMR const & copy);

	MGroup13CNMR& operator= (MGroup13CNMR const & copy);


	/**
	 * Virtueller Destructor.
	 * Gibt die fremd-allokierten Array poslst_ und typelst_ frei.
	 */
	virtual ~MGroup13CNMR();

	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual MGroup13CNMR * clone() const
	{
		return new MGroup13CNMR(*this);
	}

protected:
	/**
	 * Wertet das Messmodell aus.
	 *
	 * @param iso MaskedArray-Objekt mit Isotopomer-Fractions
	 * @param ts gültiger Timestamp
	 * @return MVector-Objekt mit simulierten Messwerten
	 */
	template< typename Stype > la::GVector< Stype > evaluate(
		double ts,
		GenMaskedArray< Stype > & iso,
		bool allow_scaling,
		Stype & gs
		) const
	{
		la::GVector< Stype > x_sim(getNumPositions());

		for (typename GenMaskedArray< Stype >::iterator i=iso.begin(); i!=iso.end(); i++)
		{
			// in jedem non-zero Isotopomer jede Position:
			for (int p=0; poslst_[p]!=-1; p++)
			{
				int tri = 0;
				// linkes Bit (falls vorhanden)
				if (poslst_[p] > 1 and i->idx.get(poslst_[p]-2))
					tri = 1;
				// mittleres Bit
				if (i->idx.get(poslst_[p]-1))
					tri = tri | 2;
				// rechtes Bit (falls vorhanden)
				if (poslst_[p] < natoms_ and i->idx.get(poslst_[p]))
					tri = tri | 4;

				switch (typelst_[p])
				{
				case MValue13CNMR::S:
					// nur mittleres Bit gesetzt; tri = 010b = 2
					if (tri == 2)
						x_sim(p) += i->value;
					break;
				case MValue13CNMR::DL:
					// mittleres und linkes Bit; tri = 110b = 3
					if (tri == 3)
						x_sim(p) += i->value;
					break;
				case MValue13CNMR::DR:
					// mittleres und rechtes Bit; tri = 011b = 6
					if (tri == 6)
						x_sim(p) += i->value;
					break;
				case MValue13CNMR::DD:
				case MValue13CNMR::T:
					// DD und T bezieht sich auf Symmetrie und macht
					// hier keinen Unterschied ...
					// linkes, mittleres und rechtes Bit; tri = 111b = 7
					if (tri == 7)
						x_sim(p) += i->value;
					break;
				}
			}
		}

		if (allow_scaling and scale_auto_ and x_sim.dim() > 1)
		{
			la::MVector x_meas(dim_), x_stddev(dim_);
			if (getMValuesStdDev(ts,x_meas,x_stddev))
			{
				// Skalierungsfaktor bestimmen und Skalieren
				gs = compute_groupscale(x_sim,x_meas,x_stddev);
				// Skalieren
				x_sim *= gs;
			}
			else fTHROW(XMLException,"invalid timestamp [%f]", ts);
		}
		else gs = Stype(1.);

		return x_sim;
	}

public:
	/**
	 * Registriert alle Messwerte über eine einfache Schnittstelle.
	 *
	 * @param x_meas Messwerte
	 * @param x_stddev Standardabweichungen
	 */	
	virtual void setMValuesStdDev(
		double ts,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		);

        /**
	 * löscht alle registrierten Messwerte.
	 */
        virtual void removeMValuesStdDev();
        
	/**
	 * Statische Factory-Methode.
	 * Parst eine Zeichenkette mit 13C-NMR-Spezifikation, welche die
	 * Messung beschreibt.
	 *
	 * @param s Zeichenkette mit 13C-NMR-Spezifikation
	 * @param state Fehlercode des Parsers (optional)
	 */
	static MGroup13CNMR * parseSpec(char const * s, int * state = 0);

	/**
	 * Gibt ein Array mit den Positionen der der 13C-NMR-Messung zurück.
	 *
	 * @return Array mit 13C-NMR-Atompositionen
	 */
	inline int const * getPositions() const { return poslst_; }

	/**
	 * Gibt die Anzahl der Positonen zurück, an denen gemessen wird.
	 *
	 * @return Anzahl der Positionen
	 */
	inline size_t getNumPositions() const { return dim_; }

	/**
	 * Gibt ein Array mit den Typen der 13C-NMR Messungen zurück
	 * (welche den Positionen zugeordnet sind).
	 *
	 * @return Array mit 13C-NMR-Typen
	 */
	inline Type const * getNMRTypes() const { return typelst_; }
	
	/**
	 * Gibt die Simulationsmaske zurück (Schnittstellenimplementierung).
	 *
	 * @return Simulationsmaske
	 */
	BitArray getSimMask() const;

	/**
	 * Gibt die Menge der zu simulierenden Unbekannten zurück
	 * (Isotopomer Fractions, Cumomer Fractions, EMUs).
	 *
	 * @param sdt Datenbasis der Simulation (Iso, Cumo, EMU)
	 * @return Menge von Indizes der zu simulierenden Unbekannten
	 */
	std::set< BitArray > getSimSet(SimDataType sdt) const;

	/**
	 * 13C-NMR-spezifische Implementierung von getMValue;
	 * gibt zu einer gegebenen Atomposition, einem 13C-NMR-Typen und
	 * einem Timestamp einen Messwert zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param pos Atomposition
	 * @param type 13C-NMR Typ
	 * @return Zeiger auf Messwert-Objekt, falls existent
	 */
	MValue const * getMValue(double ts, int pos, Type type) const;

	/**
	 * Gibt alle Messwerte zu einem gegebenen Timestamp als Vektor
	 * zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param x_meas MVector-Objekt mit Messwerten
	 * @param x_stddev MVector-Objekt mit Standardabweichungen
	 * @return true, falls Timestamp gültig
	 */
	bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const;
	
	/**
	 * Registriert ein Messwert-Objekt.
	 *
	 * @param mvalue Messwert-Objekt
	 */
	void registerMValue(MValue const & mvalue);


	/**
	 * Spezifikation eines einzelnen Messwerts.
	 *
	 * @param idx Index des Messwerts
	 * @return Messwertspezifikation
	 */
	char const * getSpec(size_t idx) const;
	
	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class MGroup13CNMR

/*
 * *****************************************************************************
 * Messgruppen-Klasse für allgemeine Cumomer-Notation (0,1,x).
 * Von außen nur über die generische Messgruppe (MGroupGeneric)
 * zugänglich.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class MGroupCumomer : public MetaboliteMGroup
{
protected:
	/** Abbildung von Timestamp auf Messwert */
	std::map< double,MValue* > mvalue_map_;
	/** 1-Maske (xen der allg. Cumomer-Notation) */
	BitArray xmask_;
	/** 1-Maske (1en der allg. Cumomer-Notation) */
	BitArray mask_;
	
protected:
	/**
	 * Constructor (protected!).
	 *
	 * @param pools Pool-Liste
	 */
	inline MGroupCumomer(
		char * mname,
		BitArray const & xmask,
		BitArray const & mask,
		char const * spec = 0
		)
		: MetaboliteMGroup(mg_CUMOMER,1,mname,spec),
		  xmask_(xmask), mask_(mask) { }

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupCumomer(MGroupCumomer const & copy);

	/**
	 * Assignment operator.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupCumomer& operator= (MGroupCumomer const & copy);
 
	/**
	 * Virtueller Destructor.
	 */
	virtual ~MGroupCumomer();

	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual MGroupCumomer * clone() const
	{
		return new MGroupCumomer(*this);
	}

protected:
	/**
	 * Wertet das Modell aus.
	 *
	 * @param iso Vektor mit Isotopomer-Fraction-Werten
	 * @param ts Timestamp (wird ignoriert)
	 * @return Vektor der Länge 1
	 */
	template< typename Stype > la::GVector< Stype > evaluate(
		double ts,
		GenMaskedArray< Stype > & iso,
		bool allow_scaling,
		Stype & gs
		) const
	{
		la::GVector< Stype > x_sim(1);

		// ggfs. die Länge von mask_ korrigieren?
		fASSERT( int(mask_.size()) == natoms_ );
		fASSERT( mask_.size() == iso.getLog2Size() );

		// 0-Maske vorbereiten; 0en sind überall da wo keine 1en oder xen sind
		BitArray zmask(~(mask_|xmask_));

		for (typename GenMaskedArray< Stype >::iterator i=iso.begin(); i!=iso.end(); i++)
		{
			BitArray const & idx = i->idx;
			// 0-Bits prüfen, 1-Bits prüfen (equiv: (idx==mask_) or (idx & xmask_))
			if ((~idx & zmask) == zmask and (idx & mask_) == mask_)
				x_sim(0) += i->value;
		}

		// MGroupCumomer hat immer dim_ == 1 -> kein Scaling
		gs = Stype(1.);

		return x_sim;
	}

public:
	/**
	 * Registriert alle Messwerte über eine einfache Schnittstelle.
	 *
	 * @param x_meas Messwerte
	 * @param x_stddev Standardabweichungen
	 */	
	virtual void setMValuesStdDev(
		double ts,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		);

        /**
	 * löscht alle registrierten Messwerte.
	 */
        virtual void removeMValuesStdDev();
        
	/**
	 * Statische Factory-Methode.
	 * Parst eine Poolliste und ruft den Constructor auf.
	 *
	 * @param s Zeichenkette mit allg. Cumomer-Notation
	 * @return Zeiger auf allokiertes Messgruppen-Objekt
	 */
	static MGroupCumomer * parseSpec(char const * s, int * state = 0);

	/**
	 * Gibt zu einem Timestamp Messwert und Standardabweichung.
	 * Beides Vektoren der Länge 1.
	 *
	 * @param ts Timestamp
	 * @param x_meas Messwert (Vektor der Länge 1)
	 * @param x_stddev Standardabweichung zum Messwert (Vektor der Länge 1)
	 * @return true, falls ts ein gültiger Timestamp
	 */
	bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const;

	/**
	 * Gibt die Simulationsmaske zurück (Schnittstellenimplementierung).
	 * Nur die 1- und x-Positionen müssen simuliert werden.
	 *
	 * @return Simulationsmaske (1- und x-Positionen)
	 */
	inline BitArray getSimMask() const { return xmask_ | mask_; }

	/**
	 * Gibt die Menge der zu simulierenden Unbekannten zurück
	 * (Isotopomer Fractions, Cumomer Fractions, EMUs).
	 *
	 * @param sdt Datenbasis der Simulation (Iso, Cumo, EMU)
	 * @return Menge von Indizes der zu simulierenden Unbekannten
	 */
	std::set< BitArray > getSimSet(SimDataType sdt) const;

	/**
	 * Gibt zu einem gegebenen Timestamp-Wert einen Zeiger auf einen
	 * Messwert (MValue) zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @return Zeiger auf Messwert
	 */
	MValue const * getMValue(double ts) const;
	
	/**
	 * Registriert einen neuen Messwert.
	 *
	 * @param mvalue Messwert-Objekt
	 */
	void registerMValue(MValue const & mvalue);
	
	/**
	 * Setzt die Anzahl der Atome und schneidet die Maske auf passende
	 * Länge zurecht. Evtl. wäre hier Fehlerprüfung noch wichtig.
	 *
	 * @param natoms Anzahl der Atome
	 */
	inline void setNumAtoms(unsigned int natoms)
	{
		natoms_ = natoms;
		mask_.resize(natoms,false);
	}
	
	/**
	 * Messwertspezifikation: SimpleMGroup beschreibt einen einzelnen
	 * Messwert.
	 *
	 * @param idx Index des Messwerts
	 * @return Messwertspezifikation
	 */
	inline char const * getSpec(size_t idx) const { return MGroup::getSpec(); }

	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class MGroupCumomer

/*
 * *****************************************************************************
 * Messgruppen-Klasse zur Abbildung generischer Messungen.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class MGroupGeneric : public MGroup
{
public:
	/** Mögliche Typen der generischen Messungen
	 * (nur Markierungsmessungen!)
	 */
	enum MGroupGenericType
	{
		mgg_MS = mg_MS,
		mgg_MSMS = mg_MSMS,
		mgg_1HNMR = mg_1HNMR,
		mgg_13CNMR = mg_13CNMR,
		mgg_CUMOMER = mg_CUMOMER, // verallgemeinerte Cumomer-Notation!
		mgg_MI_MS = mg_MIMS 
	};

protected:
	/** Abbildung Zeile und Timestamp auf Messwert */
	std::map< double,MValue* > * row_mvalue_map_;
	/** Array von Ausdrücken */
	symb::ExprTree ** E_;
	/** Anzahl der Unterausdrücke */
	size_t rows_;
	/** Array von Abbildungen auf die Messgruppen von E_[i] */
	charptr_map< MetaboliteMGroup* > * sub_groups_;

protected:
	/**
	 * Constructor (protected!).
	 *
	 * @param E Array mit Ausdrücken (fremd-allokiert!)
	 * @param rows Anzahl der Ausdrücke
	 * @param subgroups Array von Abbildungen auf Untergruppen (Länge rows)
	 */
	MGroupGeneric(
		symb::ExprTree ** E,
		size_t rows,
		charptr_map< MetaboliteMGroup * > * sub_groups
		);

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupGeneric(MGroupGeneric const & copy);

	/**
	 * Assignment operator.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	MGroupGeneric& operator= (MGroupGeneric const & copy);

	/**
	 * Destructor.
	 * Löscht die fremd-allokierten Arrays.
	 */
	virtual ~MGroupGeneric();

	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual MGroupGeneric * clone() const
	{
		return new MGroupGeneric(*this);
	}
	
public:
	/**
	 * Registriert alle Messwerte über eine einfache Schnittstelle.
	 *
	 * @param x_meas Messwerte
	 * @param x_stddev Standardabweichungen
	 */	
	virtual void setMValuesStdDev(
		double ts,
		la::MVector const & x_meas,
		la::MVector const & x_stddev
		);

        /**
	 * löscht alle registrierten Messwerte.
	 */
        virtual void removeMValuesStdDev();
        
	/**
	 * Statische Factory-Methode.
	 * Parst eine Zeichenkette mit einer Formel, welche die generische
	 * Messung beschreibt.
	 *
	 * @param spec 0-terminiertes Array von Expressions
	 * @param state Fehlercode des Parsers (optional)
	 */
	static MGroupGeneric * parseSpec(
		symb::ExprTree ** spec,
		int * state = 0
		);

	/**
	 * Alternative statische Factory-Methode.
	 *
	 * @param spec Zeichenkette mit Formeln (durch Semikolon abgetrennt)
	 * @param state Fehlercode des Parsers (optional)
	 */
	static MGroupGeneric * parseSpec(
		char const * spec,
		int * state = 0
		);

protected:
	/**
	 * Prüft die Komponenten eines Ausdrucks auf Konformität mit einem
	 * angegebenem Messungs-Typ. Wirft eine Exception zur genaueren
	 * Diagnose.
	 *
	 * @param R Ausdruck
	 * @param type Messungs-Typ
	 * @param sub_groups Map für Unter-Messgruppen
	 * @param dim ermittelte Dimension des Messwert-Vektors; initial -1
	 */
	static void checkExpr(
		symb::ExprTree * R,
		charptr_map< MetaboliteMGroup * > & sub_groups
		);

public:
	/**
	 * Gibt die Simulationsmaske zurück (Schnittstellenimplementierung).
	 *
	 * @return Simulationsmaske
	 */
	charptr_map< BitArray > getSimMasks() const;

	/**
	 * Setzt die Gruppen-Id (auch die aller Untergruppen)
	 *
	 * @param id Gruppen-Id
	 */
	void setGroupId(char const * id);

	/**
	 * Gibt den Ausdruck, welcher die Messung beschreibt, zurück.
	 *
	 * @return Ausdruck, welcher die Messung beschreibt
	 */
	inline symb::ExprTree * getExpression(size_t row) const
	{
		fASSERT( row < rows_ );
		return E_[row];
	}

	/**
	 * Schaltet die automatische Skalierung der Messwerte ein.
	 * Auf den Unter-Gruppen wird die Skalierung grundsätzlich abgeschaltet.
	 */
	void setScaleAuto();

	/**
	 * Gibt ein Array der in einem Unter-Ausdruck verwendeten
	 * Variablennamen zurück.
	 *
	 * @param row Zeile, Nummer des Ausdrucks
	 * @return Array der in der Spezifikation verwendeten Variablennamen
	 */
	charptr_array getVarNames(size_t row) const;
	
	/**
	 * Gibt ein Array der in den Ausdrücken verwendeten Variablennamen
	 * zurück.
	 *
	 * @return Array der in der Spezifikation verwendeten Variablennamen
	 */
	charptr_array getVarNames() const;

	/**
	 * Gibt ein Array mit den in der Spezifikation enthaltenen
	 * Poolbezeichnungen zurück.
	 *
	 * @return Array der in der Spezifikation enthaltenen Poolbezeichnungen
	 */
	charptr_array getPoolNames(size_t row) const;

	/**
	 * Gibt ein Array mit den in der Spezifikation enthaltenen
	 * Poolbezeichnungen zurück.
	 *
	 * @return Array der in der Spezifikation enthaltenen Poolbezeichnungen
	 */
	charptr_array getPoolNames() const;

	/**
	 * Gibt die Anzahl der Unter-Messgruppen (=Zeilen mit Formeln)
	 * zurück
	 *
	 * @return Anzahl der Unter-Messgruppen (=Zeilen mit Formeln)
	 */
	inline size_t getNumRows() const { return rows_; }

	/**
	 * Wertet das Messmodell auf Basis von EMUs/Cumomer-Fractions aus.
	 * EMUs eignen sich für MS-Messungen, Cumomere sind vornehmlich für
	 * NMR-Messungen geeignet.
	 *
	 * @param values Abbildung von Metabolit und (Cumomer-)Index auf Werte
	 * @param ts gültiger Timestamp
	 * @return MVector-Object mit simulierten Messwerten
	 */
	template< typename Stype,typename Ftype > la::GVector< Stype > evaluate(
		double ts,
		charptr_map<
			fhash_map< BitArray,Ftype,BitArray_hashf > const *
			> const & values,
		bool allow_scaling,
		Stype & gs
		) const
	{
		la::GVector< Stype > x_sim(dim_);
		for (size_t r=0; r<rows_; r++)
			x_sim.set(r,evaluateRek<Stype,Ftype>(ts,values,E_[r],r));

		if (allow_scaling and scale_auto_ and x_sim.dim() > 1)
		{
			la::MVector x_meas(dim_), x_stddev(dim_);
			if (getMValuesStdDev(ts,x_meas,x_stddev))
			{
				// Skalierungsfaktor bestimmen und Skalieren
				gs = compute_groupscale(x_sim,x_meas,x_stddev);
				// Skalieren
				x_sim *= gs;
			}
			else fTHROW(XMLException,"invalid timestamp [%f]", ts);
		}
		else gs = Stype(1.);

		return x_sim;
	}
	
	/**
	 * Wertet die Ableitung des Messmodells auf Basis von EMUs oder
	 * Cumomer-Fractions aus.
	 *
	 * @param ts gültiger Timestamp
	 * @param values Abbildung von Indices auf (EMU-/Cumomer-)Fractions
	 * @param dvalues Abbildung von Indices auf (EMU-/Cumomer-)Ableitungen
	 * @param allow_scaling Flag, automatische Messgruppen-Skalierung verwenden
	 * @param gs Verwendeter Skalierungsfaktor für Messgruppen (out)
	 * @param dgs Ableitung des verwendetet Skalierungsfaktors (out)
	 * @return Vektor mit Ableitungen der simulierten Messwerte
	 */
	template< typename Ftype,typename Stype > la::GVector< Stype > devaluate(
		double ts,
		charptr_map<
			fhash_map< BitArray,Ftype,BitArray_hashf > const *
			> const & values,
		charptr_map<
			fhash_map< BitArray,Ftype,BitArray_hashf > const *
			> const & dvalues,
		bool allow_scaling,
		Stype & gs,
		Stype & dgs
		) const
	{
		if (not (allow_scaling and scale_auto_ and dim_>1))
		{
			// Ohne Messwertskalierung wird das Messmodell einfach
			// noch einmal mit den Ableitungen ausgewertet.
			dgs = Stype(0.);
			return evaluate(ts,dvalues,false,gs);
		}

		// Bei eingeschalteter Messwertskalierung muß der Skalierungsfaktor
		// abgeleitet werden
		la::GVector< Stype > x_sim = evaluate(ts,values,false,gs);
		la::GVector< Stype > dx_sim_dflux = evaluate(ts,dvalues,false,gs);
		la::MVector x_meas(dim_), x_stddev(dim_);

		bool ts_valid = getMValuesStdDev(ts,x_meas,x_stddev);
		// ein ungültiger Timestamp fällt schon früher auf:
		fASSERT(ts_valid);

		// d omega/d flux und omega berechnen:
		dgs = compute_dgroupscale_dflux(x_sim,x_meas,x_stddev,dx_sim_dflux,gs);
		
		// Ableitung via Produktregel:
		return dgs * x_sim + gs * dx_sim_dflux;
	}

	/**
	 * Gibt zu einer Untergruppenspezifikation eine Untergruppen-Objekt
	 * zurück
	 *
	 * @param sgname Spezifikation einer Untergruppe
	 * @param row Zeile (beginnt bei 0 zu zählen)
	 * @return Zeiger auf Messgrupppenobjekt oder 0, falls nicht gefunden
	 */
	inline MetaboliteMGroup * getSubGroup(
		char const * sgname,
		size_t row
		) const
	{
		if (row >= rows_)
			return 0;
		MetaboliteMGroup ** MG = sub_groups_[row].findPtr(sgname);
		return MG ? *MG : 0;
	}

	/**
	 * (beinahe) Schnittstellen-Implementierung ;-)
	 */
	bool getMValuesStdDev(
		double ts,
		la::MVector & x_meas,
		la::MVector & x_stddev
		) const;

	/**
	 * Gibt zu einem gegebenen Timestamp-Wert einen Zeiger auf einen
	 * Messwert (MValue) zurück.
	 *
	 * @param ts Timestamp (-1 für stationär!)
	 * @param row Zeile
	 * @return Zeiger auf Messwert
	 */
	MValue const * getMValue(double ts, size_t row) const;
	
	/**
	 * Registriert einen neuen Messwert.
	 *
	 * @param mvalue Messwert-Objekt
	 */
	void registerMValue(MValue const & mvalue);

	/**
	 * Dummy; nicht aufrufen.
	 * Die norm() muss evaluate() aufrufen und evaluate() kann nur
	 * mit einer charptr_map< fhash_map< ... > > funktionieren.
	 */
	template< typename Stype > Stype norm(
		GenMaskedArray< Stype > & iso,
		double ts,
		Stype & gs
		) const
	{
		fASSERT_NONREACHABLE();
		return 0.;
	}

	/**
	 * Markierungs-Norm berechnen (auf Basis von EMUs/Cumomer-Fractions).
	 * Wertet zurnächst das Messmodell aus.
	 *
	 * @param values Map von Hashes mit den CumulativeVector< double > /
	 * 	CumulativeScalar< double >-Objekten
	 * @param ts Timestamp
	 * @return Markierungsnorm
	 */
	template< typename Stype,typename Ftype > Stype norm(
		charptr_map<
			fhash_map< BitArray,Ftype,BitArray_hashf > const *
			> const & values,
		double ts,
		Stype & gs
		) const
	{
		// mit Groupscales multiplizierte, simulierte Messwerte
		la::GVector< Stype > x_sim
			= evaluate< Stype,Ftype >(ts,values,true,gs);
		// reale Messwerte
		la::MVector x_meas(dim_), x_stddev(dim_);
		if (not getMValuesStdDev(ts,x_meas,x_stddev))
			fTHROW(XMLException,"invalid timestamp [%f]", ts);

		// Mit Standardabweichungen gewichtete L2 Norm berechnen
		size_t i;
		Stype w,ni,n=0.;

		for (i=0; i<x_sim.dim(); i++)
		{
			// (Relativen) Wichtungsfaktor aus Standardabweichung
			// berechnen.
			// Entspricht (x_sim-x_meas)^T*Sigma^-1*(x_sim-x_meas)
			w = 1./x_stddev.get(i);
			w *= w;

			// Norm
			ni = x_sim.get(i)-x_meas.get(i);
			ni = w*ni*ni;

			n += ni;
		}
		return n;
	}

	/**
	 * Ableitung der Markierungsnorm auf Basis von Cumomer-Fractions/EMUs.
	 *
	 * @param values Map von Hashes mit den
	 * 	CumulativeVector< double > / CumulativeScalar< double >-Objekten
	 * @param dvalues_dflux Werte der Ableitungen von values nach einem
	 * 	freien Fluss
	 * @param ts Timestamp
	 * @param gs Verwendeter Group-Scale-Faktor
	 * @return Ableitung der Markierungsnorm
	 */
	template< typename Stype,typename Ftype > Stype dnorm_dflux(
		charptr_map<
			fhash_map< BitArray,Ftype,BitArray_hashf > const *
			> const & values,
		charptr_map<
			fhash_map< BitArray,Ftype,BitArray_hashf > const *
			> const & dvalues_dflux,
		double ts,
		Stype & gs
		) const
	{
		la::GVector< Stype > xsim = evaluate(ts,values,false,gs);
		la::MVector xmeas(dim_), xstddev(dim_);
		la::GVector< Stype > dxsim_dflux(rows_);
		char fxn[] = "__reserved_flux__";
		size_t r;

		for (r=0; r<rows_; r++)
		{
			charptr_array rowvars = getVarNames(r);
			charptr_array::const_iterator rvi;
			charptr_map< charptr_array > dsymmap;

			// Variablen vom Fluss fxn abhängig machen
			for (rvi=rowvars.begin(); rvi!=rowvars.end(); rvi++)
				dsymmap[*rvi] = fxn;

			// Ableitung der Zeilengleichung nach fxn
			symb::ExprTree * dE_dflux = E_[r]->deval(fxn,&dsymmap);

			// Ableitung rekursiv auswerten
			dxsim_dflux(r) = evaluateRekDiff<Stype,Ftype>(
						ts,
						values,
						dvalues_dflux,
						dE_dflux,
						r
						);

			delete dE_dflux;
		}
		//
		// Jetzt geht's weiter wie oben:
		//
		
		bool ts_valid = getMValuesStdDev(ts,xmeas,xstddev);

		// ein ungültiger Timestamp fällt schon früher auf:
		fASSERT( ts_valid );

		gs = Stype(1.);
		Stype d_gs_d_flux(0.);
		Stype S(0.), n;

		// Bestimmung & Ableitung des Group-Scale-Faktors (nur falls benötigt)
		if (scale_auto_ and xsim.dim()>1)
			d_gs_d_flux = compute_dgroupscale_dflux(xsim,xmeas,xstddev,dxsim_dflux,gs);

		for (size_t i=0; i<dim_; i++)
		{
			n = xmeas.get(i) - gs * xsim.get(i);
			n *= d_gs_d_flux * xsim.get(i)
				+ gs * dxsim_dflux.get(i);
			n /= xstddev.get(i)*xstddev.get(i);

			S += n;
		}
		return -2.*S;
	}

protected:
	/**
	 * Rekursiver Worker zur Auswertung der Formel
	 * (geeignet für Cumomer und EMU).
	 *
	 * @param values Simulierte EMUs/Cumomer-Fractions
	 * @param ts Timestamp
	 * @param E Ausdrucksknoten
	 * @param row die E zugeordnete Zeile
	 * @return simulierter Messwert (Skalar)
	 */
	template< typename Stype,typename Ftype > Stype evaluateRek(
		double ts,
		charptr_map<
			fhash_map< BitArray,Ftype,BitArray_hashf > const *
			> const & values,
		symb::ExprTree * E,
		size_t row
		) const
	{
		Stype gs,L(0.),R(0.);

		if (E->isOperator())
		{
			if (E->Lval())
				L = evaluateRek<Stype,Ftype>(ts,values,E->Lval(),row);
			if (E->Rval())
				R = evaluateRek<Stype,Ftype>(ts,values,E->Rval(),row);
		}

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
			{
			MetaboliteMGroup * G = getSubGroup(E->getVarName(),row);
			fASSERT( G != 0 );

			fhash_map< BitArray,Ftype,BitArray_hashf > const ** valuesM
				= values.findPtr(G->getMetaboliteName());
			if (valuesM == 0)
				fTHROW(XMLException,"Isotopomer array for metabolite [%s] not found in supplied map",
						G->getMetaboliteName());

			switch (G->getType())
			{
			case mg_MS:
				{
				MGroupMS * Gms = static_cast< MGroupMS * >(G);
				return (Gms->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_MIMS:
				{
				MGroupMIMS * Gmims = static_cast< MGroupMIMS * >(G);
				return (Gmims->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_MSMS:
				{
				MGroupMSMS * Gmsms = static_cast< MGroupMSMS * >(G);
				return (Gmsms->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_1HNMR:
				{
				MGroup1HNMR * G1hnmr = static_cast< MGroup1HNMR * >(G);
				return (G1hnmr->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_13CNMR:
				{
				MGroup13CNMR * G13cnmr = static_cast< MGroup13CNMR * >(G);
				return (G13cnmr->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_CUMOMER:
				{
				MGroupCumomer * Gcumo = static_cast< MGroupCumomer * >(G);
				return (Gcumo->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_GENERIC:
			case mg_FLUX:
			case mg_POOL:
				fASSERT_NONREACHABLE();
				break;
			}
			return Stype(0.);
			}
		case symb::et_op_pow:
//			return ::pow(L,R);
		case symb::et_op_sqrt:
//			return ::sqrt(L);
		case symb::et_op_log:
//			return ::log(L);
		case symb::et_op_log2:
//			return ::log(L)/::log(2.);
		case symb::et_op_log10:
//			return ::log10(L);
		case symb::et_op_exp:
//			return ::exp(L);
		case symb::et_op_eq:
		case symb::et_op_neq:
		case symb::et_op_leq:
		case symb::et_op_geq:
		case symb::et_op_lt:
		case symb::et_op_gt:
		case symb::et_op_diff:
			fTHROW(XMLException,"Error: invalid operator (%s) found in generic group spec",
				E->nodeToString().c_str());
		}
		return Stype(0.);
	}

	/**
	 * Rekursiver Worker zur Auswertung der abgeleiteten Formel.
	 *
	 * @param values Simulierte Werte (EMUs oder Cumomer-Fractions)
	 * @param dvalues_dflux Ableitungen der simulierten Werte
	 * @param ts Timestamp
	 * @param E Ausdrucksknoten
	 * @param row die E zugeordnete Zeile
	 * @return Ableitung des simulierten Messwerts (Skalar)
	 */
	template< typename Stype,typename Ftype > Stype evaluateRekDiff(
		double ts,
		charptr_map<
			fhash_map< BitArray,Ftype,BitArray_hashf > const *
			> const & values,
		charptr_map<
			fhash_map< BitArray,Ftype,BitArray_hashf > const *
			> const & dvalues_dflux,
		symb::ExprTree * E,
		size_t row
		) const
	{
		Stype L(0.),R(0.);
		
		// rekursiv absteigen und auswerten.
		// Es wird nicht abgestiegen, falls es sich um einen
		// diff-Operator handelt, da hier einfach die bereits
		// zuvor berechneten Werte der Ableitungen eingesetzt
		// werden.
		if (E->isOperator() and E->getNodeType() != symb::et_op_diff)
		{
			if (E->Lval())
				L = evaluateRekDiff<Stype,Ftype>(ts,values,dvalues_dflux,E->Lval(),row);
			if (E->Rval())
				R = evaluateRekDiff<Stype,Ftype>(ts,values,dvalues_dflux,E->Rval(),row);
		}

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
		case symb::et_op_abs:
			if (L<0) return -L;
			return L;
		case symb::et_op_min:
			return L<=R?L:R;
		case symb::et_op_max:
			return L>=R?L:R;
		case symb::et_op_sqr:
			return L*L;
		case symb::et_literal:
			return E->getDoubleValue();
		case symb::et_variable:
			// je nach Operator im ursprünglichen Ausdruck kommt hier
			// noch die Variable in ihrer Reinform vor:
			{
			Stype gs;
			MetaboliteMGroup * G = getSubGroup(E->getVarName(),row);
			fASSERT( G != 0 );

			fhash_map< BitArray,Ftype,BitArray_hashf > const ** valuesM
				= values.findPtr(G->getMetaboliteName());
			if (valuesM == 0)
				fTHROW(XMLException,"simulation data for metabolite [%s] not found in supplied map",
					G->getMetaboliteName());
	
			switch (G->getType())
			{
			case mg_MS:
				{
				MGroupMS * Gms = static_cast< MGroupMS * >(G);
				return (Gms->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_MIMS:
				{
				MGroupMIMS * Gms = static_cast< MGroupMIMS * >(G);
				return (Gms->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_MSMS:
				{
				MGroupMSMS * Gmsms = static_cast< MGroupMSMS * >(G);
				return (Gmsms->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_1HNMR:
				{
				MGroup1HNMR * G1hnmr = static_cast< MGroup1HNMR * >(G);
				return (G1hnmr->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_13CNMR:
				{
				MGroup13CNMR * G13cnmr = static_cast< MGroup13CNMR * >(G);
				return (G13cnmr->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_CUMOMER:
				{
				MGroupCumomer * Gcumo = static_cast< MGroupCumomer * >(G);
				return (Gcumo->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_GENERIC:
			case mg_FLUX:
			case mg_POOL:
				fASSERT_NONREACHABLE();
				break;
			}
			return Stype(0.);
			}
		case symb::et_op_pow:
//			return ::pow(L,R);
		case symb::et_op_sqrt:
//			return ::sqrt(L);
		case symb::et_op_log:
//			return ::log(L);
		case symb::et_op_log2:
//			return ::log(L)/::log(2.);
		case symb::et_op_log10:
//			return ::log10(L);
		case symb::et_op_exp:
//			return ::exp(L);
		case symb::et_op_eq:
		case symb::et_op_neq:
		case symb::et_op_leq:
		case symb::et_op_geq:
		case symb::et_op_lt:
		case symb::et_op_gt:
			fTHROW(XMLException,"Error: invalid operator (%s) found in generic group spec",
				E->nodeToString().c_str());
			break;
		case symb::et_op_diff:
			{
			// E->Lval() eine Variable mit der Spezifikation,
			// E->Rval() ist eine Variable mit Namen "__reserved_flux__"
			fASSERT( E->Lval()->isVariable() );
			fASSERT( E->Rval()->isVariable() );
			
			Stype gs;
			MetaboliteMGroup * G = getSubGroup(E->Lval()->getVarName(),row);
			fASSERT( G != 0 );
			
			// MGroup.*::evaluate() wird hier mit der Ableitung (dvalues_dflux)
			// aufgerufen ...
			fhash_map< BitArray,Ftype,BitArray_hashf > const ** valuesM
				= dvalues_dflux.findPtr(G->getMetaboliteName());
			if (valuesM == 0)
				fTHROW(XMLException,"Simulation data for metabolite [%s] not found in supplied map",
					G->getMetaboliteName());
	
			switch (G->getType())
			{
			case mg_MS:
				{
				MGroupMS * Gms = static_cast< MGroupMS * >(G);
				return (Gms->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_MIMS:
				{
				MGroupMIMS * Gmims = static_cast< MGroupMIMS * >(G);
				return (Gmims->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_MSMS:
				{
				MGroupMSMS * Gmsms = static_cast< MGroupMSMS * >(G);
				return (Gmsms->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_1HNMR:
				{
				MGroup1HNMR * G1hnmr = static_cast< MGroup1HNMR * >(G);
				return (G1hnmr->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_13CNMR:
				{
				MGroup13CNMR * G13cnmr = static_cast< MGroup13CNMR * >(G);
				return (G13cnmr->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_CUMOMER:
				{
				MGroupCumomer * Gcumo = static_cast< MGroupCumomer * >(G);
				return (Gcumo->evaluate(ts,*(*valuesM),false,gs)).get(0);
				}
			case mg_GENERIC:
			case mg_FLUX:
			case mg_POOL:
				fASSERT_NONREACHABLE();
				break;
			}
			return Stype(0.);
			}
		}
		return Stype(0.);
	}

public:
	/**
	 * Gibt die Messgruppenspezifikation zurück. Hier wird eine durch
	 * Semikolons getrennte Formelliste zurückgegeben.
	 *
	 * @return Zeichenkette mit Spezifikation der Messgruppe
	 */
	inline char const * getSpec() const { return spec_; }

	/**
	 * Messwertspezifikation: SimpleMGroup beschreibt einen einzelnen
	 * Messwert.
	 *
	 * @param idx Index des Messwerts
	 * @return Messwertspezifikation
	 */
	inline char const * getSpec(size_t idx) const
	{
		fASSERT(idx < dim_);
		return row_spec_[idx];
	}

	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class MGroupGeneric

/*
 * *****************************************************************************
 * Messgruppen-Klasse zur Flussmessung.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class MGroupFlux : public SimpleMGroup
{
protected:
	/** Formel zur Flussmessung */
	symb::ExprTree * expr_;
	/** Flussliste */
	charptr_array fluxes_;
	/** Flag; bezieht sich Flussliste auf Netto oder Exchange-Flüsse? */
	bool is_net_;

protected:
	/**
	 * Constructor (protected!).
	 *
	 * @param fluxes Flussliste
	 * @param is_net true, falls Flussliste Netto-Flüsse beschreibt
	 */
	inline MGroupFlux(
		symb::ExprTree * expr,
		bool is_net
		)
		: SimpleMGroup(mg_FLUX,expr->toString().c_str()),
		  expr_(expr->clone()),
		  is_net_(is_net)
	{
		fluxes_ = expr_->getVarNames();
	}

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	inline MGroupFlux(MGroupFlux const & copy)
		: SimpleMGroup(copy),expr_(copy.expr_->clone()),
		  fluxes_(copy.fluxes_), is_net_(copy.is_net_) { }

	/**
	 * Destructor.
	 */
	inline ~MGroupFlux() { delete expr_; }

	/**
	 * Zuweisungsoperator.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	inline MGroupFlux & operator= (MGroupFlux const & copy)
	{
		SimpleMGroup::operator=(copy);
		fluxes_ = copy.fluxes_;
		is_net_ = copy.is_net_;
		return *this;
	}

	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual MGroupFlux * clone() const
	{
		return new MGroupFlux(*this);
	}

public:
	/**
	 * Statische Factory-Methode.
	 * Parst eine Flussliste und ruft den Constructor auf.
	 *
	 * @param s Zeichenkette mit Komma- oder Leerzeichen-getrennter
	 * 	Flussliste
	 * @param is_net true, falls Netto-Flüsse beschrieben werden
	 */
	static MGroupFlux * parseSpec(char const * spec, bool is_net)
	{
		symb::ExprTree * expr = symb::ExprTree::parse(spec);
		MGroupFlux * G = new MGroupFlux(expr,is_net);
		delete expr;
		return G;
	}

	/**
	 * Gibt ein Array der durch die Messung beschriebenen Flüsse zurück.
	 *
	 * @return Array mit Flussbezeichnungen
	 */
	inline charptr_array const & getFluxes() const { return fluxes_; }

	/**
	 * Gibt die zur Flussmessung gehörende Formel zurück
	 *
	 * @return Flussformel
	 */
	inline symb::ExprTree const * getFormula() const { return expr_; }

	/**
	 * Auswertung der Flussformel.
	 *
	 * @param values Flusswerte für Variablen der Flussformel
	 * @return Ergebnis der Auswertung
	 */
	inline double evaluate(
		charptr_map< double > const & values
		) const
	{
		return evaluateRek(expr_,values);
	}

	/**
	 * Auswertung der differenzierten Flussformel
	 *
	 * @param dexpr differenzierte Flussformel (gemäß Stöchiometrie)
	 * @param values Flusswerte für Variablen der diff. Flussformel
	 * @return Ergebnis der Auswertung
	 */
	double devaluate(
		symb::ExprTree * dexpr,
		charptr_map< double > const & values
		) const;

private:
	/**
	 * Rekursiver Worker zur Auswertung der Formel.
	 *
	 * @param expr (Unter-)Ausdruck der Flussformel
	 * @param values Flusswerte
	 */
	double evaluateRek(
		symb::ExprTree * expr,
		charptr_map< double > const & values
		) const;

public:
	/**
	 * Gibt true zurück, falls die Fluxxmessung sich auf Netto-Flüsse
	 * bezieht.
	 *
	 * @return true, falls Netto-Fluss-Messung
	 */
	inline bool isNet() const { return is_net_; }

	/**
	 * Gibt true zurück, falls die Flussmessung sich auf Exchange-Flüsse
	 * bezieht.
	 *
	 * @return true, falls Exchange-Fluss-Messung
	 */
	inline bool isExchange() const { return not is_net_; }

	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class MGroupFlux

/*
 * *****************************************************************************
 * Messgruppen-Klasse zur Poolgrößenmessung.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

class MGroupPool : public SimpleMGroup
{
protected:
	symb::ExprTree * expr_;
	charptr_array pools_;

protected:
	
	inline MGroupPool(
		symb::ExprTree * expr
		)
		: SimpleMGroup(mg_POOL,expr->toString().c_str()),
		  expr_(expr->clone())
	{
		pools_ = expr_->getVarNames();
	}

public:
	/**
	 * Copy-Constructor.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	inline MGroupPool(MGroupPool const & copy)
		: SimpleMGroup(copy),
		  expr_(copy.expr_->clone()), pools_(copy.pools_) { }

	/**
	 * Destructor.
	 */
	inline ~MGroupPool() { delete expr_; }

	/**
	 * Zuweisungsoperator.
	 *
	 * @param copy zu kopierendes Objekt
	 */
	inline MGroupPool & operator= (MGroupPool const & copy)
	{
		SimpleMGroup::operator=(copy);
		pools_ = copy.pools_;
		return *this;
	}

	/**
	 * Virtueller Copy-Constructor.
	 *
	 * @return Kopie des Objekts
	 */
	inline virtual MGroupPool * clone() const
	{
		return new MGroupPool(*this);
	}

public:
	/**
	 * Statische Factory-Methode.
	 * Parst eine Flussliste und ruft den Constructor auf.
	 *
	 * @param s Zeichenkette mit Komma- oder Leerzeichen-getrennter
	 * 	Flussliste
	 * @param is_net true, falls Netto-Flüsse beschrieben werden
	 */
	static MGroupPool * parseSpec(char const * spec)
	{
		symb::ExprTree * expr = symb::ExprTree::parse(spec);
		MGroupPool * G = new MGroupPool(expr);
		delete expr;
		return G;
	}

	/**
	 * Gibt ein Array der durch die Messung beschriebenen Flüsse zurück.
	 *
	 * @return Array mit Flussbezeichnungen
	 */
	inline charptr_array const & getPools() const { return pools_; }

	/**
	 * Gibt die zur Flussmessung gehörende Formel zurück
	 *
	 * @return Flussformel
	 */
	inline symb::ExprTree const * getFormula() const { return expr_; }

	/**
	 * Auswertung der Flussformel.
	 *
	 * @param values Flusswerte für Variablen der Flussformel
	 * @return Ergebnis der Auswertung
	 */
	inline double evaluate(
		charptr_map< double > const & values
		) const
	{
		return evaluateRek(expr_,values);
	}

	/**
	 * Auswertung der differenzierten Flussformel
	 *
	 * @param dexpr differenzierte Flussformel (gemäß Stöchiometrie)
	 * @param values Flusswerte für Variablen der diff. Flussformel
	 * @return Ergebnis der Auswertung
	 */
	double devaluate(
		symb::ExprTree * dexpr,
		charptr_map< double > const & values
		) const;

private:
	/**
	 * Rekursiver Worker zur Auswertung der Formel.
	 *
	 * @param expr (Unter-)Ausdruck der Flussformel
	 * @param values Flusswerte
	 */
	double evaluateRek(
		symb::ExprTree * expr,
		charptr_map< double > const & values
		) const;

public:
	/**
	 * Berechnet eine Prüfsumme über die Spezifikation des MGroup-Objekts.
	 *
	 * @param crc bisheriger Prüfsummen-Wert
	 * @param crc_scope Scope der Prüfsummen-Berechnung
	 * @return neuer Prüfsummen-Wert
	 */
	uint32_t computeCheckSum(uint32_t crc, int crc_scope) const;

}; // class MGroupPool



// Inline-Implementierungen:
template< typename Ftype,typename Stype > la::GVector< Stype >
MetaboliteMGroup::evaluate(
	double ts,
	fhash_map< BitArray,Ftype,BitArray_hashf > const & values,
	bool allow_scaling,
	Stype & gs
	) const
{
	// In C++ sind virtuelle template-Members leider verboten.
	// Das Multiplex-Verhalten wird hier mittels down-cast nachgebaut:
	switch (getType())
	{
	case mg_MS:
		return dynamic_cast< MGroupMS const * >(this)
			->evaluate< Stype >(ts,values,allow_scaling,gs);
         case mg_MIMS:
		return dynamic_cast< MGroupMIMS const * >(this)
			->evaluate< Stype >(ts,values,allow_scaling,gs);
	case mg_MSMS:
		return dynamic_cast< MGroupMSMS const * >(this)
			->evaluate< Stype >(ts,values,allow_scaling,gs);
	case mg_1HNMR:
		return dynamic_cast< MGroup1HNMR const * >(this)
			->evaluate< Stype >(ts,values,allow_scaling,gs);
	case mg_13CNMR:
		return dynamic_cast< MGroup13CNMR const * >(this)
			->evaluate< Stype >(ts,values,allow_scaling,gs);       
	default:
		// kann nicht vorkommen!
		break;
	}
	fASSERT_NONREACHABLE();
}

} // namespace flux::xml
} // namespace flux

#endif

