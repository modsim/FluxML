#include <cstddef>
#include <gmpxx.h>
#include "fluxml_config.h" // MACHEPS für GMatrixOps.h
#include "Error.h"
#include "ExprTree.h"
#include "LinearExpression.h"
#include "MMatrix.h"
#include "MVector.h"
#include "MMatrixOps.h"
#include "GMatrixOps.h"
#include "LAPackWrap.h"
#include "StoichMatrixInteger.h"
#include "StandardForm.h"
#include "Constraint.h"
#include "ConstraintSystem.h"

/*
 * *****************************************************************************
 * Schnnittstellen-Implementierung für ConstraintSystem
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */

#define MIN2(a,b)	((a)<=(b)?(a):(b))
#define MAX2(a,b)	((a)>=(b)?(a):(b))

using namespace flux::la;
using namespace flux::symb;

namespace flux {
namespace data {

ConstraintSystem::ConstraintSystem(
	StoichMatrixInteger const & S,
	charptr_array const & fFluxes_net,
	charptr_array const & fFluxes_xch,
        charptr_array const & fPools_size,
	std::list< Constraint > const & cEqList,
	std::list< Constraint > const & cInEqList, 
        bool stationary
	) : S_(S), fFluxes_net_(fFluxes_net), fFluxes_xch_(fFluxes_xch), /*cfg_(cfg),*/
	    cEqList_(cEqList), cInEqList_(cInEqList),
	    validation_state_(cm_unvalidated),
	    vnet_(S_.cols()), vxch_(S_.cols()),
	    Pcnet_(S_.cols()), Pcxch_(S_.cols()),
	    v_type_net_(S_.cols()), v_type_xch_(S_.cols()),
	    cons_tol_(1.e-9),fluxes_dirty_(true),
	    change_count_(0u), 
            is_stationary_(stationary),
            fPools_size_(fPools_size),
            vpool_(S_.rows(),1.),
	    Pcpool_(S_.rows()),
            v_type_pool_(S_.rows())

{
	fFluxes_net_.sort();
	fFluxes_xch_.sort();
        fPools_size_.sort();
	v_type_net_.fill(f_undefined);
	v_type_xch_.fill(f_undefined);
        v_type_pool_.fill(p_undefined);
	
	for (size_t i = 0; i < S_.rows(); i++)
  		pool2idx_[S_.getMetaboliteName(i)] = int(i);
        
        
	// Schritt 0: Eine Abbildung Flussname->Index berechnen
	for (size_t i = 0; i < S_.cols(); i++)
		flux2idx_[S_.getReactionName(i)] = int(i);

	if (not prepare())
	{
		fWARNING("preparation of constraint system failed");
		return;
	}
	if (not solve())
	{
		fWARNING("solution of constraint system failed");
		return;
	}

} // ConstraintSystem::ConstraintSystem()

void ConstraintSystem::make_free_flux_suggestion(int dfree, bool net)
{
	size_t j;
	int rank;
	bool result;
	charptr_array sugg;
	PMatrix Pc, Pc_orig;
	MMatrix N, K;
	MVector b;

	if (net)
	{
		Pc = Pcnet_;
		Pc_orig = Pcnet_;
		N = Nnet_;
		b = MVector(Nnet_.rows());
	}
	else
	{
		Pc = Pcxch_;
		Pc_orig = Pcxch_;
		N = Nxch_;
		b = MVector(Nxch_.rows());
	}
	result = MMatrixOps::gaussJordan(N,b,K,Pc,rank);
	if (result == false and rank == -3)
	{
		for (j=0; j<Pc.dim(); ++j)
		{
			// nur Vorschläge
			if (Pc(j) != 2) continue;
			char const * rn = S_.getReactionName(j);
			// nur ursprünglich als frei markierte Flüsse
			if ((net and fFluxes_net_.exists(rn))
				or ((not net) and fFluxes_xch_.exists(rn)))
				sugg.add(rn);
		}
	}

	if (sugg.size())
	{
		fWARNING("hint: try deallocating %i out of these free %s fluxes:",
				dfree, net?"NET":"XCH");
		charptr_array::const_iterator si;
		for (si=sugg.begin(); si!=sugg.end(); ++si)
			fWARNING("  %s", *si);
	}
	else
		fWARNING("sorry, no suggestion how to solve this problem");
}

// Möglicher Ausgang:
//
// Bei Rückgabewert "false":
//   cm_invalid_free_vars
//   cm_invalid_constr
//   cm_nonlinear_constr
//   cm_linear_dep_constr
//   cm_too_many_free_vars
//   cm_too_many_constr
//
// Bei Rückgabewert "true":
//   cm_unvalidated
//   cm_too_few_constr
bool ConstraintSystem::prepare()
{
	int idx, rank;
	size_t i,j;
	size_t inet, ixch, knet, kxch;
	size_t nfree_net = fFluxes_net_.size();
	size_t nfree_xch = fFluxes_xch_.size();
	char const * simple_vn;
	std::list< Constraint >::const_iterator ci;
        
        fINFO("user requests %i free fluxes (%i free net + %i free xch).",
		int(nfree_net + nfree_xch), int(nfree_net), int(nfree_xch));
        
        size_t ipool,  kpool;
	size_t nfree_pool = fPools_size_.size();
      
        if(not is_stationary_)
        {
            fINFO("user requests %i free pool sizes.",
		int(nfree_pool));
        }
	validation_state_ = cm_unvalidated;

	// Um ein überbestimmtes System zu vermeiden, wird die Stöchiometrie
	// zunächst Zeilen-reduziert (Integer-Arithmetik):
	GMatrix< int64_t > Sr(S_);
	try
	{
		if (Sr.rows() <= Sr.cols())
		{
			// der exakte Gauss-Jordan-Algorithmus kann verwendet werden
			MMatrixOps::rowReduce(Sr);
			fINFO("size of stoichiometry is (%dx%d); exact rank is %d.",
				int(S_.rows()), int(S_.cols()), int(Sr.rows()));
		}
		else
		{
			// verwende numerischen QR-Algorithmus
			// nach double konvertieren
			MMatrix Sn(Sr.rows(),Sr.cols());
			for (std::size_t i=0; i<Sn.rows(); ++i)
				for (std::size_t j=0; j<Sn.cols(); ++j)
					Sn(i,j) = double(Sr(i,j));
			MMatrixOps::rowReduceQR(Sn);
			// wieder nach int64_t konvertieren (verlustfrei!)
			Sr = GMatrix< int64_t >(Sn.rows(),Sn.cols());
			for (std::size_t i=0; i<Sn.rows(); ++i)
				for (std::size_t j=0; j<Sn.cols(); ++j)
					Sr(i,j) = int64_t(Sn(i,j));
			fINFO("size of stoichiometry is (%dx%d); numerical rank is %d.",
				int(S_.rows()), int(S_.cols()), int(Sr.rows()));
		}
	}
	catch (IntegerOverflow &)
	{
		fWARNING("integer overflow in row reduction ... "
			"please report this");
		Sr = S_;
	}
	if (S_.rows() - Sr.rows() > 0)
		fINFO("using row-reduced stoichiometry (-%d lines)",
			(int)(S_.rows() - Sr.rows()));

	// Anzahl der netto- und exchange-Gleichheits-Constraints zählen.
	// Hier wird nur eine obere Grenze bestimmt, da Werte-Constraints
	// (="simple" constraints) nicht mit in die Gleichungssysteme
	// aufgenommen werden:
        size_t neqnet = 0, neqxch = 0, neqpool = 0;
	for (ci=cEqList_.begin(); ci!=cEqList_.end(); ci++)
	{
		if (not ci->isSimple())
		{
            switch (ci->getParameterType())
            {
                    case NET:
                            neqnet++;
                            break;
                    case XCH:
                            neqxch++;
                            break;
                    case POOL:
                            neqpool++;
                            break;
            };
		}
	}
	// Netto-Constraint-Matrix (enthält die Stöchiometrie)
	MMatrix Nnet(Sr.rows() + neqnet, S_.cols());
	// Exchange-Constraint-Matrix (unabhängig von der Stöchiometrie)
	MMatrix Nxch(neqxch, S_.cols());
	
	// Matrix der Netto-Benutzer-Constraints (dient nur der Fehlererkennung)
	MMatrix Nc_net(neqnet, S_.cols());
	// Matrix der Exchange-Benutzer-Constraints (dient nur der Fehlererkennung)
	MMatrix Nc_xch(neqxch, S_.cols());

	// rechte Seite der Netto-Constraint-Matrix
	MVector bnet(Nnet.rows());
	// rechte Seite der Exchange-Constraint-Matrix
	MVector bxch(Nxch.rows());
        
        // Pool-Constraint-Matrix (unabhängig von der Stöchiometrie)
	MMatrix Npool(neqpool, S_.rows());
	// Matrix der Poolgröße-Constraints (dient nur der Fehlererkennung)
	MMatrix Nc_pool(neqpool, S_.rows());

	// rechte Seite der Pool-Constraint-Matrix
	MVector bpool(Npool.rows());
                
	// Netto-Flussvektor
	MVector vnet(S_.cols()); // Flüsse v,q,r
	// Exchange-Flussvektor
	MVector vxch(S_.cols()); // Flüsse v,q,r
        // Poolgrößen-Vektor
	MVector vpool(S_.rows()); //Metaboliten e.g. A, B, C
        
	// Schritt 1: Reduzierte Stöchiometrie in Nnet übernehmen
	for (i=0; i<Sr.rows(); i++)
		for (j=0; j<Sr.cols(); j++)
			Nnet(i,j) = Sr(i,j);

	// freie Flüsse
	//
	//  - alle Variablen sind zunächst abhängig
	//  - Variablen, die von Wert-Constraints ("simple constraint")
	//    betroffen (z.B. v1.net=3) sind, sind immer freie Variablen!
	//    (wäre ihr Wert von anderen Variablen abhängig, so würde das
	//    Constraint verletzbar sein).
	//  - Bei Ketten von Constraints, etwa (v1.xch=v2.xch,v1=0) entstehen
	//    lineare Abhängigkeiten in den Constraints (Nc), die als Fehler
	//    erkannt werden können.
	Pcnet_.fill(1); // alle Variablen zunächst abhängig
	Pcxch_.fill(1);
        Pcpool_.fill(1);
        
	// Schritt 2: Constraints eintragen
	// es gilt i=Sr.rows();
	v_type_net_.fill(f_undefined);
	v_type_xch_.fill(f_undefined);
	v_type_pool_.fill(p_undefined);

	// Zeilen-Zähler für Matrizen Nnet, Nxch
	inet = Sr.rows(); // in Nnet gibt es schon die Stöchiometrie!
	ixch = 0; // Nxch ist noch leer
	
        // Zeilen-Zähler für Matrizen Nc_net, Nc_xch
	knet = 0;
	kxch = 0;
 
        // Zeilen-Zähler für Matrizen Npool, Nc_pool
        ipool = 0;
	kpool = 0; 

	for (ci = cEqList_.begin(); ci != cEqList_.end(); ci++)
	{
		//
		// FALSCHE/EINFACHE CONSTRAINTS (z.B. v1.net = 3)
		//
		// Einfache Constraints erfordern zusätzliche Freiheitsgrade
		// vom System:
		if (ci->isSimple())
		{
			simple_vn = ci->getSimpleVarName();

			fASSERT( simple_vn != 0 );
			switch (ci->getParameterType())
			{
				case NET:
				case XCH:
					idx = flux2idx_[ simple_vn ];
					break;
				case POOL:
					idx = pool2idx_[ simple_vn ];
					break;
			};
			// sollte vorher sichergestellt sein
			fASSERT( idx != -1 );
			// Der Variablenname im Wert-Constraint wird zusätzlich
			// als Freie Variable aufgenommen
                        switch (ci->getParameterType())
			{
				case NET:
					if (fFluxes_net_.find(simple_vn) != fFluxes_net_.end())
					{
						fERROR("free fluxes (as %s (net)) "
							"must not be constraint",
							simple_vn);
						validation_state_ = cm_invalid_free_vars;
						return false;
					}
				
					// Flusswert ist unver�nderlich
					v_type_net_(idx) = f_constraint;
					// Variable freihalten:
					Pcnet_(idx) = 0;
					
					nfree_net++;
					break;
				case XCH:
					if (fFluxes_xch_.find(simple_vn) != fFluxes_xch_.end())
					{
						fERROR("free fluxes (as %s (xch)) "
							"must not be constraint",
							simple_vn);
						validation_state_ = cm_invalid_free_vars;
						return false;
					}
					
					// Flusswert ist inver�nderlich
					v_type_xch_(idx) = f_constraint;
					// Variable freihalten:
					Pcxch_(idx) = 0;
	
					nfree_xch++;
					break;
				case POOL:
					if (fPools_size_.find(simple_vn) != fPools_size_.end())
					{
						fERROR("free poolsize (as %s ) "
							"must not be constraint",
							simple_vn);
						validation_state_ = cm_invalid_free_vars;
						return false;
					}
				
					// Poolgöße ist unver�nderlich
					v_type_pool_(idx) = p_constraint;
					// Variable freihalten:
					Pcpool_(idx) = 0;
					
					nfree_pool++;
					break;
			};
                        
			// es macht keinen Sinn für freie Variable einen
			// Eintrag in der Matrix vorzunehmen -- bzw. wäre es
			// sogar schädlich, da der Rang künstlich erhöht werden
			// würde!
			continue;
		} // if (ci->isSimple())

		//
		// ECHTE CONSTRAINTS (z.B. v1.net = v2.net+v3.net)
		//
		// Echte Constraints erhöhen den Rang und zurren das System
		// weiter fest.
		try
		{
			// das Constraint in eine (hoffentlich) lineare
			// Gleichung umwandeln: (...)=0
			LinearExpression lE( ci->getConstraint() );
                        
			// Koeffizienten der linearen Gleichung
			charptr_map< double > const & C = lE.getLinearCoeffs();
			charptr_map< double >::const_iterator coeff_i;

			// Eintragen der Koeffizienten:
			for (coeff_i = C.begin(); coeff_i != C.end(); coeff_i++)
			{
				// ist der Koeffizent ein "echter" Koeffizient
				// oder gehört er zur speziellen Variable "1"
				// (Konstante)?
				if (strcmp(coeff_i->key, "1" ) == 0)
				{
                                    // Den Wert negieren, da er auf der
                                    // linken Seite steht, aber auf der
                                    // rechten Seite eingetragen werden
                                    // soll:
                                    switch(ci->getParameterType())
					{
						case NET:
							bnet(inet) = - coeff_i->value;
							break;
						case XCH:
							bxch(ixch) = - coeff_i->value;
							break;
						case POOL:
							bpool(ipool) = - coeff_i->value;
							break;

					};
				}
				else
				{
                                    // falls die Reaktionsbezeichnung
                                    // unbekannt ist, ist das ein Fehler
                                    // der FluxML-Datei
                                    // Spaltenindex
                                    switch(ci->getParameterType())
                                    {
                                            case NET:
                                            case XCH:
                                                    if (flux2idx_.find(coeff_i->key) == flux2idx_.end())
                                                    {
                                                            validation_state_ = cm_invalid_constr;
                                                            return false;
                                                    }
                                                    idx = flux2idx_[ coeff_i->key ];
                                                    break;
                                            case POOL:
                                                    if (pool2idx_.find(coeff_i->key) == pool2idx_.end())
                                                    {
                                                            validation_state_ = cm_invalid_constr;
                                                            return false;
                                                    }
                                                    idx = pool2idx_[ coeff_i->key ];
                                                    break;
                                    };
                                        
					// den Koeffizienten in die richtige
					// Matrix eintragen:
                                        switch(ci->getParameterType())
					{
						case NET:
							Nnet(inet,idx) = Nc_net(knet,idx) = coeff_i->value;
							break;
						case XCH:
							Nxch(ixch,idx) = Nc_xch(kxch,idx) = coeff_i->value;
							break;
						case POOL:
							Npool(ipool,idx) = Nc_pool(kpool,idx) = coeff_i->value;
							break;
					};
				}
			} // for ( Koeffizienten )
			// Pro Zeile ein Constraint:
			// Zeilenindex von Nnet/Nxch und Nc_net/Nc_xch erhöhen
                        switch(ci->getParameterType())
			{
				case NET:
					inet++;	knet++;
					break;
				case XCH:
					ixch++; kxch++;
					break;
				case POOL:
					ipool++; kpool++;
					break;
			};
		}
		catch (ExprTreeException &)
		{
			// ein Constraint ist anscheinend nicht-linear
			// -- fataler Fehler
			validation_state_ = cm_nonlinear_constr;
			return false;
		}
	}
	// Möglicherweise sind nicht alle Constraints eintragbar.
	// Nur "echte" Constraints wurden eingetragen:
        Npool_ = Nnet_ = Nxch_ = MMatrix();
	bpool_ = bnet_ = bxch_ = MVector();
        
	if (inet > 0)
	{
		Nnet_ = Nnet.getSlice(0,0,inet-1,S_.cols()-1);
		bnet_ = bnet.getSlice(0,inet-1);
	}
	if (ixch > 0)
	{
		Nxch_ = Nxch.getSlice(0,0,ixch-1,S_.cols()-1);
		bxch_ = bxch.getSlice(0,ixch-1);
	}
        if (ipool > 0)
	{
		Npool_ = Npool.getSlice(0,0,ipool-1,S_.rows()-1);
		bpool_ = bpool.getSlice(0,ipool-1);
	}

	// Matrix der Netto-Benutzer-Constraints ebenfalls reduzieren:
	if (knet>0)
	{
		Nc_net = Nc_net.getSlice(0,0,knet-1,S_.cols()-1);
		// Rangbestimmung in den Benutzer-Constraints
		if (((size_t)(MMatrixOps::rank(Nc_net)) < MIN2(Nc_net.rows(),Nc_net.cols())))
		{
			// die vom Benutzer vorgegebenen Constraints sind
			// untereinander linear abhängig -- fataler Fehler
			fWARNING("linear dependencies between user-specified net constraints!");
			validation_state_ = cm_linear_dep_constr;
			return false;
		}
	}
	// Dasselbe für die Exchange-Benutzer-Constraints:
	if (kxch>0)
	{
		Nc_xch = Nc_xch.getSlice(0,0,kxch-1,S_.cols()-1);
		// Rangbestimmung in den Benutzer-Constraints
		if (((size_t)(MMatrixOps::rank(Nc_xch)) < MIN2(Nc_xch.rows(),Nc_xch.cols())))
		{
			// die vom Benutzer vorgegebenen Constraints sind
			// untereinander linear abhängig -- fataler Fehler
			fWARNING("linear dependencies between user-specified exchange constraints!");
			validation_state_ = cm_linear_dep_constr;
			return false;
		}
	}
        // Dasselbe für die Poolgrößen-Benutzer-Constraints:
        if (kpool>0)
	{
		Nc_pool = Nc_pool.getSlice(0,0,kpool-1,S_.rows()-1);
		// Rangbestimmung in den Benutzer-Constraints
		if (((size_t)(MMatrixOps::rank(Nc_pool)) < MIN2(Nc_pool.rows(),Nc_pool.cols())))
		{
			// die vom Benutzer vorgegebenen Constraints sind
			// untereinander linear abh�ngig -- fataler Fehler
			fWARNING("linear dependencies between user-specified poolsize constraints!");
			validation_state_ = cm_linear_dep_constr;
			return false;
		}
	}
	
	// Freie net/xch-Flüsse in Pcnet_/Pcxch_ eintragen
	charptr_array::const_iterator ffi;

	// einige Variablen frei
	for (ffi = fFluxes_net_.begin(); ffi != fFluxes_net_.end(); ffi++)
	{
		// sollte vorher geprüft sein ...
		fASSERT( flux2idx_.findPtr(*ffi) != 0 );
		Pcnet_[flux2idx_[*ffi]] = 0;
	}
	for (ffi = fFluxes_xch_.begin(); ffi != fFluxes_xch_.end(); ffi++)
	{
		// sollte vorher geprüft sein ...
		fASSERT( flux2idx_.findPtr(*ffi) != 0 );
		Pcxch_[flux2idx_[*ffi]] = 0;
	}
        
        if(!is_stationary_)
        {
            charptr_array::const_iterator psi;
            for (psi = fPools_size_.begin(); psi != fPools_size_.end(); psi++)
            {
                    // sollte vorher gepr�ft sein ...
                    fASSERT( pool2idx_.findPtr(*psi) != 0 );
                    Pcpool_[pool2idx_[*psi]] = 0;
            }
        }

	// Netto-Constraint-Matrix: Zeilen reduzieren, falls Rangabfall
	rank = MMatrixOps::rank(Nnet_);
	fINFO("size of NET constraint system is (%dx%d); numerical rank is %d",
		int(Nnet_.rows()), int(Nnet_.cols()), rank);

	// Rangabfall?
	if ( size_t(rank) < MIN2(Nnet_.rows(),Nnet_.cols()) )
	{
		fWARNING("some user-specified net constraints are implicitly "
			"contained in the stoichiometry! eliminating these "
			"constraints ...");
		MMatrixOps::rowReduceQR(Nnet_,bnet_);
		
		// Falls die folgende Assertion fehlschlägt gibt es ein Problem
		// mit der Rangbestimmung ...
		fASSERT( size_t(rank) == MIN2(Nnet_.rows(),Nnet_.cols()) );
		// das Unreparierbare reparieren ...
		if ( size_t(rank) != MIN2(Nnet_.rows(),Nnet_.cols()))
		{
			fERROR("error row reduction (xch): dissonance about numerical "
				"rank; check carefully");
			rank = MIN2(Nnet_.rows(),Nnet_.cols());
		}
	}
	
	// Exchange-Constraint-Matrix: Zeilen reduzieren, falls Rangabfall
	if (Nxch_.rows() > 0)
	{
		rank = MMatrixOps::rank(Nxch_);
		fINFO("size of XCH constraint system is (%dx%d); numerical rank is %d",
				int(Nxch_.rows()), int(Nxch_.cols()), rank);

		// Rangabfall?
		if ( size_t(rank) < MIN2(Nxch_.rows(),Nxch_.cols()) )
		{
			fWARNING("some user-specified exchange constraints are implicitly "
					"contained in the stoichiometry! eliminating these "
					"constraints ...");
			MMatrixOps::rowReduceQR(Nxch_,bxch_);

			// Falls die folgende Assertion fehlschlägt gibt es ein Problem
			// mit der Rangbestimmung ...
			fASSERT( size_t(rank) == MIN2(Nxch_.rows(),Nxch_.cols()) );
			// das Unreparierbare reparieren ...
			if ( size_t(rank) != MIN2(Nxch_.rows(),Nxch_.cols()))
			{
				fERROR("error row reduction (xch): dissonance about numerical "
						"rank; check carefully");
				rank = MIN2(Nxch_.rows(),Nxch_.cols());
			}
		}
	}

        if(!is_stationary_)
        {
            if (Npool_.rows() > 0)
            {
                    rank = MMatrixOps::rank(Npool_);
                    fINFO("size of POOL constraint system is (%dx%d); numerical rank is %d",
                                    int(Npool_.rows()), int(Npool_.cols()), rank);

                    if ( size_t(rank) < MIN2(Npool_.rows(),Npool_.cols()) )
                    {
                            fWARNING("some user-specified poolsize(should not depend on stoichiomitry) constraints are implicitly "
                                            "contained in the stoichiometry! eliminating these "
                                            "constraints ...");
                            MMatrixOps::rowReduce(Npool_,bpool_);

                            fASSERT( size_t(rank) == MIN2(Npool_.rows(),Npool_.cols()) );

                            if ( size_t(rank) != MIN2(Npool_.rows(),Npool_.cols()))
                            {
                                    fERROR("error row reduction (pool): dissonance about numerical "
                                                    "rank; check carefully");
                                    rank = MIN2(Npool_.rows(),Npool_.cols());
                            }
                    }
            }
        }

	if (Nnet_.rows() > Nnet_.cols())
	{
		// Zu viele Constraints?
		if (nfree_net > 0)
		{
			// Soweit zusätzlich freie Flüsse definiert sind, liegt
			// ein Fehler vor.
			validation_state_ = cm_too_many_free_vars;
			fWARNING("specification allocates too many free NET variables.");
			make_free_flux_suggestion(Nnet_.rows() - Nnet_.cols(), true);
		}
		else
		{
			// es wurden zwar keine zusätzlichen freien Flüsse
			// deiniert, aber trotzdem ist das System überbestimmt.
			// Eine Least-Squares-Lösung wäre prinzipiell möglich
			// (auch sinnvoll?).
			validation_state_ = cm_too_many_constr;
			fWARNING("specification allocates too many NET constraints.");
		}
		return false;
	}
	if (Nxch_.rows() > Nxch_.cols())
	{
		if (nfree_xch > 0)
		{
			validation_state_ = cm_too_many_free_vars;
			fWARNING("specification allocates too many free XCH variables.");
			make_free_flux_suggestion(Nxch_.rows() - Nxch_.cols(), false);
		}
		else
		{
			validation_state_ = cm_too_many_constr;
			fWARNING("specification allocates too many XCH constraints.");
		}
		return false;
	}
        
        if(!is_stationary_)
        {
            if (Npool_.rows() > Npool_.cols())
            {
                    if (nfree_pool > 0)
                    {
                            validation_state_ = cm_too_many_free_vars;
                            fWARNING("specification allocates too many free POOL variables.");
                            make_free_pool_suggestion(Npool_.rows() - Npool_.cols());
                    }
                    else
                    {
                            validation_state_ = cm_too_many_constr;
                            fWARNING("specification allocates too many POOL constraints.");
                    }
                    return false;
            }
        }
	
	if (nfree_net > Nnet_.cols() - Nnet_.rows()) // es gilt: N_.cols() >= N_.rows()
	{
		// es wurden zu viele Variablen als frei deklariert -- das
		// System hat zu wenige Freiheitsgrade und daran läßt sich
		// auch nichts ändern -- fataler Fehler
		validation_state_ = cm_too_many_free_vars;
		fWARNING("specification allocates too many free NET variables.");
		// Vorschlag machen
		make_free_flux_suggestion(nfree_net - (Nnet_.cols() - Nnet_.rows()), true);
		return false;
	}
	if (Nxch_.rows() > 0 and (nfree_xch > Nxch_.cols() - Nxch_.rows()))
	{
		validation_state_ = cm_too_many_free_vars;
		fWARNING("specification allocates too many free XCH variables.");
		// Vorschlag machen
		make_free_flux_suggestion(nfree_xch - (Nxch_.cols() - Nxch_.rows()), false);
		return false;
	}
        
        if(!is_stationary_)
        {
            if (Npool_.rows() > 0 and (nfree_pool > Npool_.cols() - Npool_.rows()))
            {
                    validation_state_ = cm_too_many_free_vars;
                    fWARNING("specification allocates too many free POOL variables.");
                    // Vorschlag machen
                    make_free_pool_suggestion(nfree_pool - (Npool_.cols() - Npool_.rows()));
                    return false;
            }
        }
	
	if (nfree_net < Nnet_.cols() - Nnet_.rows()) // es gilt: N_.cols() >= N_.rows()
	{
		// es gibt zu wenige Variablen, die als frei deklariert wurden
		// -- das System hat zusätzliche Freiheitsgrade.
		validation_state_ = cm_too_few_constr;
		fWARNING("%d missing free NET flux(es) will be chosen "
				"automatically (value set to 0)",
				(int)(Nnet_.cols()-Nnet_.rows()-nfree_net));
		return true;
	}
	if (nfree_xch < Nxch_.cols() - Nxch_.rows())
	{
		validation_state_ = cm_too_few_constr;
		fWARNING("%d missing free XCH flux(es) will be chosen "
				"automatically (value set to 0)",
				(int)(Nxch_.cols()-Nxch_.rows()-nfree_xch));
		return true;
	}

        if(!is_stationary_)
        {
            if (nfree_pool < Npool_.cols() - Npool_.rows())
            {
                    validation_state_ = cm_too_few_constr;
                    fWARNING("%d missing free POOL size(es) will be chosen "
                                    "automatically (value set to 0)",
                                    (int)(Npool_.cols()-Npool_.rows()-nfree_pool));
                    return true;
            }
        }

	return true;
} // ConstraintSystem::prepare()

// nach ConstraintSystem::prepare() aufrufen!
//
// Bei Eingang:
//   cm_unvalidated
//   cm_too_few_constr
//
// Möglicher Ausgang:
//
// Bei Rückgabewert "false":
//   cm_invalid_free_vars
//   cm_too_many_free_vars
//   cm_ineqs_infeasible
//
// Bei Rückgabewert "true":
//   cm_ok
//   cm_too_few_constr

bool ConstraintSystem::solve()
{
	int rank;
	size_t j,k;
	char const * rn;
	bool result;

	fASSERT( validation_state_ == cm_unvalidated
		or validation_state_ == cm_too_few_constr );
	
	// Listen freier Flüsse zurücksetzen
	fFluxes_xch_.clear();
	fFluxes_net_.clear();
        fPools_size_.clear();
	// Schritt 3: System lösen (NETTO)
	//
	// Es kann vorkommen, dass die Stöchiometrie Redundanz enthält, oder
	// Benutzer-Constraints zusammen mit der Stöchiometrie Redundanz
	// erzeugen -- man könnte das mit etwas mehr Intelligenz abfangen und
	// dann den Anwender auf einzelne Constraints, die implizit schon in
	// der Stöchiometrie enthalten sind, aufmerksam machen...
	if (Nnet_.rows() <= Nnet_.cols())
	{
#if 0
		GMatrix< int64_t > iN;
		GVector< int64_t > ib;
		if (false and MMatrixOps::integerSystem(Nnet_,bnet_,iN,ib))
		{
			GMatrix< int64_t > iV;
			fINFO("solving pure integer system");
			if (not (result = MMatrixOps::gaussJordan(iN,ib,iV,Pcnet_,rank)))
			{
				fWARNING("solution of the constraint system failed:");
				switch (-rank)
				{
				case 1:	fWARNING("#rows > #cols!?");
					fASSERT_NONREACHABLE(); // sollte nicht passieren
					break;
				case 2: fWARNING("too many free net fluxes!");
					fASSERT_NONREACHABLE(); // sollte nicht passieren
					break;
				case 3: fWARNING("combination of free net fluxes is infeasible");
					break;
				case 4: fWARNING("infeasible free net fluxes / system has no solution");
					break;
				default:
					fERROR("gaussJordan failed with unknown reason?!");
				}
				validation_state_ = cm_invalid_free_vars;
				return false;
			}

			// Abhängige Variablen (floating-point Lösung erzeugen)
			Vnet_ = MMatrix(iV.rows(),iV.cols());
			for (k=0; k<(size_t)rank; k++)
			{
				fASSERT(iN(k,Pcnet_(k)) != 0ll);
				double sk = iN(k,Pcnet_(k));
				for (j=0; j<Vnet_.cols(); j++)
					Vnet_(Pcnet_(k),j) = iV(Pcnet_(k),j)/sk;
			}
			// Freie Variablen
			for (; k<Vnet_.rows(); k++)
				for (j=0; j<Vnet_.cols(); j++)
					Vnet_(Pcnet_(k),j) = iV(Pcnet_(k),j);
		}
		else if (not (result = MMatrixOps::gaussJordan(Nnet_,bnet_,Vnet_,Pcnet_,rank)))
		{
			fWARNING("solution of the NET constraint system failed:");
			switch (-rank)
			{
			case 1:	fWARNING("  #rows > #cols!?");
				fASSERT_NONREACHABLE(); // sollte nicht passieren
				break;
			case 2: fWARNING("  too many free net fluxes!");
				fASSERT_NONREACHABLE(); // sollte nicht passieren
				break;
			case 3: fWARNING("  combination of free net fluxes is infeasible");
				break;
			case 4: fWARNING("  infeasible free net fluxes / system has no solution");
				break;
			default:
				fWARNING("  (unknown reason)");
			}
			validation_state_ = cm_invalid_free_vars;
			return false;
		}
#endif
		// Arbitrary-Precision-Alternative ...
		GMatrix< mpq_class > qN(Nnet_.rows(),Nnet_.cols());
		GVector< mpq_class > qb(bnet_.dim());
		GMatrix< mpq_class > qV;
		for (j=0; j<qN.rows(); ++j)
			for (k=0; k<qN.cols(); ++k)
				qN(j,k) = Nnet_.get(j,k);
		for (k=0; k<qb.dim(); ++k)
			qb(k) = bnet_.get(k);

		if (not (result = GMatrixOps::gaussJordan(qN,qb,qV,Pcnet_,rank)))
		{
			fWARNING("solution of the NET constraint system failed:");
			switch (-rank)
			{
			case 1:	fWARNING("  #rows > #cols!?");
				fASSERT_NONREACHABLE(); // sollte nicht passieren
				break;
			case 2: fWARNING("  too many free net fluxes!");
				fASSERT_NONREACHABLE(); // sollte nicht passieren
				break;
			case 3: fWARNING("  combination of free net fluxes is infeasible");
				break;
			case 4: fWARNING("  infeasible free net fluxes / system has no solution");
				break;
			default:
				fWARNING("  (unknown reason)");
			}
			validation_state_ = cm_invalid_free_vars;
			return false;
		}
		// double-Matrix Vnet aus qV aufbauen:
		Vnet_ = MMatrix(qV.rows(),qV.cols());
		for (j=0; j<qV.rows(); j++)
			for (k=0; k<qV.cols(); k++)
				Vnet_(j,k) = qV.get(j,k).get_d();

		// Bei nicht erfüllbaren Constraints wird eine leere Matrix
		// zurückgegeben. Der Benutzer hat dann entweder zu viele Freie
		// Flüsse oder zu viele Constraints eingegeben
		if (Vnet_.rows() == 0)
		{
			validation_state_ = cm_too_many_free_vars;
			return false;
		}
		
		// ansonsten ist alles okay. Den vorherigen Status nur dann
		// überschreiben, falls er "cm_unvalidated" war (damit wird
		// der Status "cm_too_few_constr" gerettet):
		if (validation_state_ == cm_unvalidated)
			validation_state_ = cm_ok;

		// Welche Flüsse wurden zu abhängigen Flüssen?
		for (j=0; j<Nnet_.rows(); j++)
		{
			// abhängige Variablen:
			k = Pcnet_(j);

			// bislang undefinierte Flüsse sind jetzt abhängige Flüsse
			fASSERT(v_type_net_(k) == f_undefined);
			v_type_net_(k) = f_dependent;
		}

		// Welche Flüsse wurden zusätzlich als frei gewählt:
		// Es geht los bei rank=N.rows():
		for (j=Nnet_.rows(); j<Nnet_.cols(); j++)
		{
			k = Pcnet_(j);
			// Der Reaktionsname steht in der Stöchiometrie
			rn = S_.getReactionName(k);

			// bis zu diesem Punkt sind alle freien Flüsse entweder
			// Wert-Constraints oder undefiniert
			fASSERT(v_type_net_(k) == f_undefined or v_type_net_(k) == f_constraint);

			// freie Netto-Flüsse aufsammeln:
			// "echter" freier Netto-Fluß (kein Constraint):
			if (v_type_net_(k) == f_undefined)
			{
				v_type_net_(k) = f_free;
				fFluxes_net_.add(rn);
			}
		}
	}
	else
	{
		// In diesem Fall gibt es zu viele Constraints und erst recht
		// keinen Platz für freie Flüsse. Falls es freie Flüsse gibt,
		// proviziert der User hier Unsinn und es sollte eine
		// Fehlermeldung geben. prepare() fängt den Fall ab.
		fASSERT_NONREACHABLE();
	}

	//
	// System lösen (EXCHANGE)
	//
	if (Nxch_.rows() > 0)
	{
		if (Nxch_.rows() <= Nxch_.cols())
		{
#if 0
			GMatrix< int64_t > iN;
			GVector< int64_t > ib;
			if (false and MMatrixOps::integerSystem(Nxch_,bxch_,iN,ib))
			{
				GMatrix< int64_t > iV;
				fINFO("solving pure integer system");
				if (not (result = MMatrixOps::gaussJordan(iN,ib,iV,Pcxch_,rank)))
				{
					fWARNING("solution of the XCH constraint system failed:");
					switch (-rank)
					{
					case 1:	fWARNING("#rows > #cols!?");
						fASSERT_NONREACHABLE(); // sollte nicht passieren
						break;
					case 2: fWARNING("too many free fluxes!");
						fASSERT_NONREACHABLE(); // sollte nicht passieren
						break;
					case 3: fWARNING("combination of free exchange fluxes is infeasible");
						break;
					case 4: fWARNING("infeasible free exchange fluxes / system has no solution");
						break;
					default:
						fERROR("gaussJordan failed with unknown reason?!");
					}
					validation_state_ = cm_invalid_free_vars;
					return false;
				}

				// Abhängige Variablen (floating-point Lösung erzeugen)
				Vxch_ = MMatrix(iV.rows(),iV.cols());
				for (k=0; k<(size_t)rank; k++)
				{
					fASSERT(iN(k,Pcxch_(k)) != 0ll);
					double sk = iN(k,Pcxch_(k));
					for (j=0; j<Vxch_.cols(); j++)
						Vxch_(Pcxch_(k),j) = iV(Pcxch_(k),j)/sk;
				}
				// Freie Variablen
				for (; k<Vxch_.rows(); k++)
					for (j=0; j<Vxch_.cols(); j++)
						Vxch_(Pcxch_(k),j) = iV(Pcxch_(k),j);
			}
			else if (not (result = MMatrixOps::gaussJordan(Nxch_,bxch_,Vxch_,Pcxch_,rank)))
			{
				fWARNING("solution of the XCH constraint system failed:");
				switch (-rank)
				{
				case 1:	fWARNING("  #rows > #cols!?");
					fASSERT_NONREACHABLE(); // sollte nicht passieren
					break;
				case 2: fWARNING("  too many free exchange fluxes!");
					fASSERT_NONREACHABLE(); // sollte nicht passieren
					break;
				case 3: fWARNING("  combination of free exchange fluxes is infeasible");
					// TODO: mit gaussJordanMatch nach einem Vorschlag scannen
					break;
				case 4: fWARNING("  infeasible free exchange fluxes / system has no solution");
					break;
				default:
					fWARNING("  (unknown reason)");
				}
				validation_state_ = cm_invalid_free_vars;
				return false;
			}
#endif
			// Arbitrary-Precision-Alternative ...
			GMatrix< mpq_class > qN(Nxch_.rows(),Nxch_.cols());
			GVector< mpq_class > qb(bxch_.dim());
			GMatrix< mpq_class > qV;
			for (j=0; j<qN.rows(); ++j)
				for (k=0; k<qN.cols(); ++k)
					qN(j,k) = Nxch_.get(j,k);
			for (k=0; k<qb.dim(); ++k)
				qb(k) = bxch_.get(k);
	
			if (not (result = GMatrixOps::gaussJordan(qN,qb,qV,Pcxch_,rank)))
			{
				fWARNING("solution of the XCH constraint system failed:");
				switch (-rank)
				{
				case 1:	fWARNING("  #rows > #cols!?");
					fASSERT_NONREACHABLE(); // sollte nicht passieren
					break;
				case 2: fWARNING("  too many free exchange fluxes!");
					fASSERT_NONREACHABLE(); // sollte nicht passieren
					break;
				case 3: fWARNING("  combination of free exchange fluxes is infeasible");
					// TODO: mit gaussJordanMatch nach einem Vorschlag scannen
					break;
				case 4: fWARNING("  infeasible free exchange fluxes / system has no solution");
					break;
				default:
					fWARNING("  (unknown reason)");
				}
				validation_state_ = cm_invalid_free_vars;
				return false;
			}
			// double-Matrix Vxch aus qV aufbauen:
			Vxch_ = MMatrix(qV.rows(),qV.cols());
			for (j=0; j<qV.rows(); j++)
				for (k=0; k<qV.cols(); k++)
					Vxch_(j,k) = qV.get(j,k).get_d();

			// Bei nicht erfüllbaren Constraints wird eine leere Matrix
			// zurückgegeben. Der Benutzer hat dann entweder zu viele Freie
			// Flüsse oder zu viele Constraints eingegeben
			if (Vxch_.rows() == 0)
			{
				validation_state_ = cm_too_many_free_vars;
				return false;
			}

			// ansonsten ist alles okay. Den vorherigen Status nur dann
			// überschreiben, falls er "cm_unvalidated" war (damit wird
			// der Status "cm_too_few_constr" gerettet):
			if (validation_state_ == cm_unvalidated)
				validation_state_ = cm_ok;

			// Welche Flüsse wurden zu abhängigen Flüssen?
			for (j=0; j<Nxch_.rows(); j++)
			{
				// abhängige Variablen:
				k = Pcxch_(j);

				// bislang undefinierte Flüsse sind jetzt abhängige Flüsse
				fASSERT(v_type_xch_(k) == f_undefined);
				v_type_xch_(k) = f_dependent;
			}

			// Welche Flüsse wurden zusätzlich als frei gewählt:
			// Es geht los bei rank=N.rows():
			for (j=Nxch_.rows(); j<Nxch_.cols(); j++)
			{
				k = Pcxch_(j);
				// Der Reaktionsname steht in der Stöchiometrie
				rn = S_.getReactionName(k);

				// bis zu diesem Punkt sind alle freien Flüsse entweder
				// Wert-Constraints oder undefiniert
				fASSERT(v_type_xch_(k) == f_undefined or v_type_xch_(k) == f_constraint);

				// freie Exchange-Flüsse aufsammeln:
				// "echter" freier Exchange-Fluß (kein Constraint):
				if (v_type_xch_(k) == f_undefined)
				{
					v_type_xch_(k) = f_free;
					fFluxes_xch_.add(rn);
				}
			}
		}
		else
		{
			// In diesem Fall gibt es zu viele Constraints und erst recht
			// keinen Platz für freie Flüsse. Falls es freie Flüsse gibt,
			// proviziert der User hier Unsinn und es sollte eine
			// Fehlermeldung geben. prepare() fängt den Fall ab.
			fASSERT_NONREACHABLE();
		}
	}
	else
	{
		// es gibt keine Gleichungs-Constraints für die
		// Exchange-Flüsse. Gleichungssystemlösung
		// simulieren => Vxch_ = [ 0, eye(S_.cols()) ]
		Vxch_ = MMatrix(S_.cols(),1+S_.cols());
		Pcxch_ = PMatrix(S_.cols());
		Pcxch_.initIdent();
		for (j=0; j<S_.cols(); j++)
		{
			Vxch_.set(j,j+1,1.);
			if (v_type_xch_(j) == f_undefined)
			{
				v_type_xch_(j) = f_free;
				fFluxes_xch_.add(S_.getReactionName(j));
			}
		}
	}

        //
	// System lösen (POOL)
	//
        if (Npool_.rows() > 0)
	{
		if (Npool_.rows() <= Npool_.cols())
		{
			// Arbitrary-Precision-Alternative ...
			GMatrix< mpq_class > qN(Npool_.rows(),Npool_.cols());
			GVector< mpq_class > qb(bpool_.dim());
			GMatrix< mpq_class > qV;
			for (j=0; j<qN.rows(); ++j)
				for (k=0; k<qN.cols(); ++k)
					qN(j,k) = Npool_.get(j,k);
			for (k=0; k<qb.dim(); ++k)
				qb(k) = bpool_.get(k);
	
			if (not (result = GMatrixOps::gaussJordan(qN,qb,qV,Pcpool_,rank)))
			{
				fWARNING("solution of the POOL constraint system failed:");
				switch (-rank)
				{
				case 1:	fWARNING("  #rows > #cols!?");
					fASSERT_NONREACHABLE(); // sollte nicht passieren
					break;
				case 2: fWARNING("  too many free POOL sizes!");
					fASSERT_NONREACHABLE(); // sollte nicht passieren
					break;
				case 3: fWARNING("  combination of free POOL sizes is infeasible");
					// TODO: mit gaussJordanMatch nach einem Vorschlag scannen                                                                                
					break;
				case 4: fWARNING("  infeasible free POOL sizes / system has no solution");
					break;
				default:
					fWARNING("  (unknown reason)");
				}
				validation_state_ = cm_invalid_free_vars;
				return false;
			}
			// double-Matrix Vxch aus qV aufbauen:
			Vpool_ = MMatrix(qV.rows(),qV.cols());
			for (j=0; j<qV.rows(); j++)
				for (k=0; k<qV.cols(); k++)
					Vpool_(j,k) = qV.get(j,k).get_d();

			// Bei nicht erf�llbaren Constraints wird eine leere Matrix
			// zur�ckgegeben. Der Benutzer hat dann entweder zu viele Freie
			// Poolgrößen oder zu viele Constraints eingegeben
			if (Vpool_.rows() == 0)
			{
				validation_state_ = cm_too_many_free_vars;
				return false;
			}

			// ansonsten ist alles okay. Den vorherigen Status nur dann
			// �berschreiben, falls er "cm_unvalidated" war (damit wird
			// der Status "cm_too_few_constr" gerettet):
			if (validation_state_ == cm_unvalidated)
				validation_state_ = cm_ok;

			// Welche Poolgrößen wurden zu abh�ngigen Poolgrößen?
			for (j=0; j<Npool_.rows(); j++)
			{
				// abh�ngige Variablen:
				k = Pcpool_(j);

				// bislang undefinierte Poolgrößen sind jetzt abh�ngige Poolgrößen
				fASSERT(v_type_pool_(k) == p_undefined);
				v_type_pool_(k) = p_dependent;
			}
                        
			// Welche Poolgrößen wurden zus�tzlich als frei gewaehlt:
			// Es geht los bei rank=N.rows():
			for (j=Npool_.rows(); j<Npool_.cols(); j++)
			{
				k = Pcpool_(j);
				// Der Metabolitename steht in der Stoechiometrie
				rn = S_.getMetaboliteName(k);
                                
				// bis zu diesem Punkt sind alle freien Poolgrößen entweder
				// Wert-Constraints oder undefiniert
				fASSERT(v_type_pool_(k) == p_undefined or v_type_pool_(k) == p_constraint);

				// freie Poolgrößen aufsammeln:
				if (v_type_pool_(k) == p_undefined)
				{
					v_type_pool_(k) = p_free;
					fPools_size_.add(rn);
				}
			}                        
		}
		else
		{
			// In diesem Fall gibt es zu viele Constraints und erst recht
			// keinen Platz f�r freie Poolgrößen. Falls es freie Poolgrößen gibt,
			// proviziert der User hier Unsinn und es sollte eine
			// Fehlermeldung geben. prepare() f�ngt den Fall ab.
			fASSERT_NONREACHABLE();
		}
	}
	else
	{
		// es gibt keine Gleichungs-Constraints f�r die
		// Pool-Größen. Gleichungssysteml�sung
		// simulieren => Vpool_ = [ 0, eye(S_.rows()) ]
		Vpool_ = MMatrix(S_.rows(),1+S_.rows());
		Pcpool_ = PMatrix(S_.rows());
		Pcpool_.initIdent();
		for (j=0; j<S_.rows(); j++)
		{
			Vpool_.set(j,j+1,1.);
			if (v_type_pool_(j) == p_undefined)
			{
				v_type_pool_(j) = p_free;
				fPools_size_.add(S_.getMetaboliteName(j));
			}
		}                                
        }
	
	// Falls es irgendwie noch Sinn macht die Gleichheits-Constraints
	// noch eintragen:
	switch (validation_state_)
	{
	case cm_ok: // super ...
	case cm_too_few_constr: // nicht tragisch ...
		impose_constraints();
		break;
	default:
		return false;
	}
	
	// mit der gewonnenen Aufteilung in freie/abhängige Flüsse
	// ist es jetzt möglich die Ungleichungen auf feasibility
	// zu prüfen -- es muss allerdings v_const_ angelegt sein,
	// was in impose_constraints() passiert:
	if (not validate_ineqs_feasibility())
	{
		validation_state_ = cm_ineqs_infeasible;
		return false;
	}

	// Abhängige Flüsse, die vollständig durch Constraint-Flüsse
	// definiert sind, sind "pseudo-Constraints". Diese Flüsse
	// mit f_quasicons markieren und melden:
	charptr_array pcons;
	charptr_array::const_iterator pc;
	pcons = reportQuasiConstraintFluxes(true,logNOTICE);
	for (pc=pcons.begin(); pc!=pcons.end(); ++pc)
		v_type_net_(flux2idx_[*pc]) = f_quasicons;
	pcons = reportQuasiConstraintFluxes(false,logNOTICE);
	for (pc=pcons.begin(); pc!=pcons.end(); ++pc)
		v_type_xch_(flux2idx_[*pc]) = f_quasicons;

        pcons = reportQuasiConstraintPools(logNOTICE);
	for (pc=pcons.begin(); pc!=pcons.end(); ++pc)
		v_type_pool_(pool2idx_[*pc]) = p_quasicons;

	return true;
} // ConstraintSystem::solve()

// wird von ConstraintSystem::solve() aufgerufen
void ConstraintSystem::impose_constraints()
{
	size_t idx;
	int nfree_net = Vnet_.cols() - 1;
	int ndep_net = S_.cols() - nfree_net;
	// die Exchange-Matrix Nxch_ kann leer sein:
	int nfree_xch = Vxch_.cols() - 1;
	int ndep_xch = S_.cols() - nfree_xch;
        int nfree_pool = Vpool_.cols() - 1;
	int ndep_pool = S_.rows() - nfree_pool;
	char const * simple_vn;
	double simple_vv;
	PMatrix Pcnet_inv = Pcnet_.inverse();
	PMatrix Pcxch_inv = Pcxch_.inverse();
        PMatrix Pcpool_inv = Pcpool_.inverse();

	fASSERT(int(Vnet_.cols()) == nfree_net + 1);
	fASSERT(ndep_net+nfree_net == int(Vnet_.rows()));
	fASSERT(Vnet_.rows() == vnet_.dim());

	fASSERT(int(Vxch_.cols()) == nfree_xch + 1);
	fASSERT(ndep_xch+nfree_xch == int(Vxch_.rows()));
	fASSERT(Vxch_.rows() == vxch_.dim());

        fASSERT(int(Vpool_.cols()) == nfree_pool + 1);
	fASSERT(ndep_pool+nfree_pool == int(Vpool_.rows()));
	fASSERT(Vpool_.rows() == vpool_.dim());
        
	v_const_net_ = MVector(1+nfree_net);
	v_const_xch_ = MVector(1+nfree_xch);
        
	// Multiplikator der speziellen Lösung:
	v_const_net_(0) = 1.;
	v_const_xch_(0) = 1.;
        
        v_const_pool_ = MVector(1+nfree_pool);
	v_const_pool_(0) = 1.;

	std::list< Constraint >::const_iterator ci;
	for (ci = cEqList_.begin(); ci != cEqList_.end(); ci++)
	{
		if (not ci->isSimple())
			continue;

		simple_vn = ci->getSimpleVarName();
		simple_vv = ci->getSimpleVarValue();
		
		fASSERT( simple_vn != 0 );

		// Index des Flusses bestimmen
		switch(ci->getParameterType())
		{
			case NET:
			case XCH:
				idx = flux2idx_[ simple_vn ];
				break;
			case POOL:
				idx = pool2idx_[ simple_vn ];
				break;
		};

		// Wert der freien constraint-Variablen in v_free eintragen;
		// hier wird bei "+1" eingetragen, da v_free immer mit dem Wert
		// "1" beginnt (=Multiplikator der speziellen Lösung). Die
		// Suche nach i mit Pc_(i) == idx kann durch die inverse
		// Permutation Pc_inv ersetzt werden:
		// Pc_(i) == idx <=> Pc_inv(idx) = i
                switch(ci->getParameterType())
		{
			case NET:
				v_const_net_(Pcnet_inv(idx)-ndep_net+1) = simple_vv;
				// Der Flusstyp muss f_constraint sein (wurde bereits vorher
				// festgelegt):
				fASSERT(v_type_net_(idx) == f_constraint);
				break;
			case XCH:
				v_const_xch_(Pcxch_inv(idx)-ndep_xch+1) = simple_vv;
				fASSERT(v_type_xch_(idx) == f_constraint);
				break;
			case POOL:
				v_const_pool_(Pcpool_inv(idx)-ndep_pool+1) = simple_vv;
				fASSERT(v_type_pool_(idx) == p_constraint);
				break;
		}

	}
} // void ConstraintSystem::impose_constraints()

// wird von ConstraintSystem::solve() aufgerufen
bool ConstraintSystem::validate_ineqs_feasibility() const
{
	StandardForm SF;
	fDEBUG(0,"checking feasibility of inequality constraints ...");
	if (not fillStandardForm(SF))
	{
		validation_state_ = cm_invalid_constr;
		fASSERT_NONREACHABLE(); // genauer untersuchen falls das passiert!
		return false;
	}
	
	if (SF.getNumVars() == 0)
	{
		fWARNING("constraints / inequalities leave no degree of freedom");
		return true;
	}
	else if (not SF.isFeasible())
	{
		fERROR("infeasible system of inequality constraints");
		validation_state_ = cm_ineqs_infeasible;
                SF.dump();
		return false;
	}
	fDEBUG(0,"ok, inequalities are feasible");
	return true;
} // ConstraintSystem::validate_ineqs_feasibility()

// wird automatisch von getFlux() aufgerufen
void ConstraintSystem::eval() const
{
	size_t j;
	size_t nfree_net = Vnet_.cols() - 1;
	size_t ndep_net = S_.cols() - nfree_net;
	// für Exchange kann Nxch_ leer sein:
	size_t nfree_xch = Vxch_.cols() - 1;
	size_t ndep_xch = S_.cols() - nfree_xch;
        // für Pool kann Npool_ leer sein:
	size_t nfree_pool = Vpool_.cols() - 1;
	size_t ndep_pool = S_.rows() - nfree_pool;
        
	MVector v_free_net(1+nfree_net);
	MVector v_free_xch(1+nfree_xch);
	MVector v_free_pool(1+nfree_pool);
        
	switch (validation_state_)
	{
	case cm_ok:
	case cm_too_few_constr:
	case cm_linear_dep_constr:
	case cm_ineqs_violated:
		break;
	default:
		fWARNING("failed to evaluate fluxes (inconsistent system)");
		vnet_.fill(0.);
		vxch_.fill(0.);
                vpool_.fill(1.);
		return;
	}

	// Werte freier Flüsse aus dem Flussvektor v_ nach
	// v_free_ kopieren:
	// Netto:
	for (j=ndep_net; j<ndep_net+nfree_net; j++)
		if (v_type_net_.get(Pcnet_.get(j)) == f_free)
			v_free_net(j-ndep_net+1) = vnet_.get(Pcnet_.get(j));
	// Exchange:
	for (j=ndep_xch; j<ndep_xch+nfree_xch; j++)
		if (v_type_xch_.get(Pcxch_.get(j)) == f_free)
			v_free_xch(j-ndep_xch+1) = vxch_.get(Pcxch_.get(j));
        // Poolgrößen:
        for (j=ndep_pool; j<ndep_pool+nfree_pool; j++)
		if (v_type_pool_.get(Pcpool_.get(j)) == p_free)
			v_free_pool(j-ndep_pool+1) = vpool_.get(Pcpool_.get(j));
        
	// Berechnung der Flüsse und Poolgrößen ...
	vnet_ = Vnet_ * (v_free_net + v_const_net_);
	vxch_ = Vxch_ * (v_free_xch + v_const_xch_);
        vpool_ = Vpool_ * (v_free_pool + v_const_pool_);

	// Prüfung des Stöchiometrie-Residuums
	MMatrix S(S_);
	double norm2 = (S*vnet_).norm2();
	fDEBUG(0, "residual of stoichiometry is ||S*v_(net)||_2 = %g ... ", norm2);
	norm2 = log10(norm2);
	/*
	if (norm2 <= log10(MACHEPS))
		fINFO("excellent!");
	else if (norm2 <= -6)
		fINFO("passed!");
	else if (norm2 <= -4)
		fINFO("CRITICAL!");
	else
		fINFO("FAILED! continuing anyway...");
	*/
	if (norm2 > -4)
		fWARNING("bad residual of stoichiometry: ||S*v_(net)||_2 = %g", norm2);
	
	// die Flusswerte sind jetzt aktuell:
	fluxes_dirty_ = false;

	// ... aber die verletzen möglicherweise die Ungleichungen!
	if (not validate_ineqs())
		validation_state_ = cm_ineqs_violated;
	else if (validation_state_ == cm_ineqs_violated)
		validation_state_ = cm_ok;
} // ConstraintSystem::eval()

bool ConstraintSystem::validate_ineqs() const
{
	bool result = true;
	fASSERT( fluxes_dirty_ == false );
	std::list< Constraint >::const_iterator ci;
	for (ci=cInEqList_.begin(); ci!=cInEqList_.end(); ci++)
	{
		LinearExpression lE( ci->getConstraint() );
		bool violated = false;
		double sum = 0.;

		// Koeffizienten der linearen Gleichung
		charptr_map< double > const & C = lE.getLinearCoeffs();
		charptr_map< double >::const_iterator coeff_i;

		// Eintragen der Koeffizienten:
		for (coeff_i = C.begin(); coeff_i != C.end(); coeff_i++)
		{
			// ist der Koeffizent ein "echter" Koeffizient oder
			// gehört er zur speziellen Variable "1" (Konstante)?
			if (strcmp(coeff_i->key, "1") == 0)
				sum += coeff_i->value;
			else
			{
				double net, xch;
                             
				// falls die Reaktionsbezeichnung unbekannt ist,
				// ist das ein Fehler der FluxML-Datei
                bool found;
				double pool;
				switch(ci->getParameterType())
				{
					case NET:
						found = getFlux(coeff_i->key,net,xch);
						fASSERT(found);
						sum += (coeff_i->value) * net;
						break;
					case XCH:
						found = getFlux(coeff_i->key,net,xch);
						fASSERT(found);
						sum += (coeff_i->value) * xch;
						break;
					case POOL:
						found = getPoolSize(coeff_i->key,pool);
						fASSERT(found);
						sum += (coeff_i->value) * pool;

				}
			}
		}

		switch (ci->getConstraint()->getNodeType())
		{
		case et_op_leq:
			violated = not (sum <= 0.+cons_tol_);
			break;
		case et_op_lt:
			violated = not (sum < 0.+cons_tol_);
			break;
		case et_op_geq:
			violated = not (sum >= 0.-cons_tol_);
			break;
		case et_op_gt:
			violated = not (sum > 0.-cons_tol_);
			break;
		case et_op_neq:
			violated = not (::fabs(sum) < cons_tol_);
			break;
		default:
			violated = true;
			fASSERT_NONREACHABLE(); // keine Ungleichung!
		}

		if (violated)
		{
			result = false;
                        switch(ci->getParameterType())
                        {
                                case NET:
                                        fWARNING("%s ineq. constraint \"%s\""
                                                " [%s] is violated by %e",
                                                "NET",
                                                ci->getName(),
                                                ci->getConstraint()->toString().c_str(),
                                                sum
                                                );
                                        break;
                                case XCH:
                                        fWARNING("%s ineq. constraint \"%s\""
                                                " [%s] is violated by %e",
                                                "XCH",
                                                ci->getName(),
                                                ci->getConstraint()->toString().c_str(),
                                                sum
                                                );
                                        break;
                                case POOL:
                                        fWARNING("%s ineq. constraint \"%s\""
                                                " [%s] is violated by %e",
                                                "POOL",
                                                ci->getName(),
                                                ci->getConstraint()->toString().c_str(),
                                                sum
                                                );
                                        break;

                        }
		}
	}
	return result;
} // ConstraintSystem::validate_ineqs()

bool ConstraintSystem::getFlux(
	char const * fluxname,
	double & net,
	double & xch
	) const
{
	fASSERT( validation_state_ == cm_ok
		|| validation_state_ == cm_too_few_constr
		|| validation_state_ == cm_too_many_constr
		|| validation_state_ == cm_ineqs_violated);

	size_t * idx = flux2idx_.findPtr(fluxname);
	if (idx == 0)
		return false;

	if (fluxes_dirty_)
		eval();

	net = vnet_.get(*idx);
	xch = vxch_.get(*idx);
	return true;
} // ConstraintSystem::getFlux()

bool ConstraintSystem::getFlux(
	char const * fluxname,
	double & val
	) const
{
	fASSERT( validation_state_ == cm_ok
		|| validation_state_ == cm_too_few_constr
		|| validation_state_ == cm_too_many_constr
		|| validation_state_ == cm_ineqs_violated);

	bool is_net;
	if (strEndsWith(fluxname,".n"))
		is_net = true;
	else if (strEndsWith(fluxname,".x"))
		is_net = false;
	else
		return false;

	char * fn = strdup_alloc(fluxname);
	fn[strlen(fn)-2] = '\0';
	size_t * idx = flux2idx_.findPtr(fn);
	delete[] fn;
	if (idx == 0)
		return false;

	if (fluxes_dirty_)
		eval();

	if (is_net)
		val = vnet_.get(*idx);
	else
		val = vxch_.get(*idx);
	return true;
} // ConstraintSystem::getFlux()

bool ConstraintSystem::setNetFlux(
	char const * fluxname,
	double net
	)
{
	size_t * idx = flux2idx_.findPtr(fluxname);
	fASSERT( idx != 0 );
	if (v_type_net_(*idx) != f_free)
	{
		fERROR("%s (net) is not free", fluxname);
		fASSERT_NONREACHABLE();
		return false;
	}

	if (vnet_(*idx) != net)
	{
		fluxes_dirty_ = true;
		change_count_++;
		vnet_(*idx) = net;
	}
	return true;
} // ConstraintSystem::setNetFlux()

bool ConstraintSystem::setXchFlux(
	char const * fluxname,
	double xch
	)
{
	size_t * idx = flux2idx_.findPtr(fluxname);
	fASSERT( idx != 0 );
	if (v_type_xch_(*idx) != f_free)
	{
		fERROR("%s (xch) is not free", fluxname);
		fASSERT_NONREACHABLE();
		return false;
	}
	
	if (vxch_(*idx) != xch)
	{
		fluxes_dirty_ = true;
		change_count_++;
		vxch_(*idx) = xch;
	}
	return true;
} // ConstraintSystem::setXchFlux()

bool ConstraintSystem::setFlux(
	char const * fluxname,
	double val
	)
{
	bool is_net;
	if (strEndsWith(fluxname,".n"))
		is_net = true;
	else if (strEndsWith(fluxname,".x"))
		is_net = false;
	else
	{
		fERROR("invalid suffix for net/xch flux name \"%s\"; expected (.n|.x)",
			fluxname);
		fASSERT_NONREACHABLE();
		return false;
	}

	char * fn = strdup_alloc(fluxname);
	fn[strlen(fn)-2] = '\0';
	bool rv = is_net ? setNetFlux(fn,val) : setXchFlux(fn,val);
	delete[] fn;
	return rv;
} // ConstraintSystem::setFlux()

charptr_array ConstraintSystem::getFluxNames() const
{
	charptr_array fnames = flux2idx_.getKeys();
	fnames.sort();
	return fnames;
}

charptr_array ConstraintSystem::getFluxNamesByType(
	FluxType ftype,
	bool net
	) const
{
	charptr_array fnames;
	charptr_map< size_t >::const_iterator i;
	for (i=flux2idx_.begin(); i!=flux2idx_.end(); i++)
	{
		if (net)
		{
			if (v_type_net_.get(i->value) == ftype)
				fnames.add(i->key);
		}
		else
		{
			if (v_type_xch_.get(i->value) == ftype)
				fnames.add(i->key);
		}
	}

	// f_quasicons-Flüsse sind auch dependent ...
//	if (ftype == f_dependent)
//		fnames.add(getFluxNamesByType(f_quasicons,net));

	fnames.sort();
	return fnames;
} // ConstraintSystem::getFluxNamesByType()

ConstraintSystem::FluxType ConstraintSystem::getFluxType(
	char const * fname,
	bool net
	) const
{
	size_t * idx = flux2idx_.findPtr(fname);
	fASSERT( idx != 0 );
	if (idx == 0)
		return f_undefined;
	return net ? v_type_net_.get(*idx) : v_type_xch_.get(*idx);
} // ConstraintSystem::getFluxType()

ExprTree * ConstraintSystem::getSymbolicPoolFlux(
	char const * name,
	ParameterType parameter_type,
	bool formula
	) const
{
	size_t * idxp, idx;
	size_t i, j;
	char const * idx_name;
	ExprTree * E = 0, * idx_sym;
	double coeff;

	switch (validation_state_)
	{
	case cm_ok:
	case cm_too_few_constr:
	case cm_linear_dep_constr:
	case cm_ineqs_violated:
		break;
	default:
		// die Funktion sollte in diesem Validierungszustand
		// nicht aufgerufen werden!
		fASSERT_NONREACHABLE();
		return 0;
	}
	
	// gaussJordan sollte Erfolg gehabt haben
	fASSERT(Vnet_.rows() > 0 and Vnet_.cols() > 0);
	fASSERT(Vxch_.rows() > 0 and Vxch_.cols() > 0);
	fASSERT(Vpool_.rows() > 0 and Vpool_.cols() > 0);

	// Index des Flusses bestimmen
	switch(parameter_type)
	{
		case NET:
		case XCH:
			idxp = flux2idx_.findPtr(name);
			break;
		case POOL:
			idxp = pool2idx_.findPtr(name);
			break;
	}

	if (idxp == 0)
	{
		// es sollte ein g�ltiger Flussname �bergeben werden
		fASSERT_NONREACHABLE();
		return 0;
	}
	idx = *idxp;
	
	int nfree_net;
	int ndep_net;
	int nfree_xch;
	int ndep_xch;
	int nfree_pool;
	int ndep_pool;
			
	switch(parameter_type)
	{
		case NET:
		
			nfree_net = Vnet_.cols() - 1;
			ndep_net = S_.cols() - nfree_net;

			switch (v_type_net_.get(idx))
			{
			case f_undefined:
			// kein Flusswert sollte jetzt noch undefiniert sein
				fASSERT_NONREACHABLE();
				return 0;
			case f_dependent:
			case f_quasicons:
				{
				// Zeile von V_
				i = idx;
				if (Vnet_.get(i,0))
					E = ExprTree::val(Vnet_.get(i,0));
				for (j=1; j<Vnet_.cols(); j++)
				{
					coeff = Vnet_.get(i,j);
					if (coeff == 0.)
						continue;
					// Flussindex idx:
					idx = Pcnet_.get(j-1+ndep_net);
					idx_name = S_.getReactionName(idx);
					// hat der Fluss einen festen Wert? Dann wird dieser
					// gegen�ber dem Symbol bevorzugt ... (eval sorgt f�r
					// das �brige)
					if (getFluxType(idx_name,true) == f_constraint)
					{
						idx_sym = getSymbolicPoolFlux(
							idx_name,NET,formula
							);
					}
					else
						idx_sym = ExprTree::sym("%s.n",idx_name);

					if (coeff < 0.)
					{
						ExprTree * P = ExprTree::mul(
								ExprTree::val(-coeff),
								idx_sym
								);
						if (E)
							E = ExprTree::sub(E,P);
						else
							E = ExprTree::minus(P);
					}
					else
					{
						ExprTree * P = ExprTree::mul(
								ExprTree::val(coeff),
								idx_sym
								);
						if (E)
							E = ExprTree::add(E,P);
						else
							E = P;
					}
				}
				if (E)
					E->eval(true);
				else
				{
					// AHHH! die Zeile ist komplett 0
					// ... es gibt nur eine L�sung: 0 ;-)
					E = ExprTree::val(0); // richtig?
				}
				return E;
				}
			case f_free:
				// freie Fl�sse sind ihre eigene L�sung
				return ExprTree::sym("%s.n",name);
			case f_constraint:
				{
				// Constraint-Fl�sse haben einen festen Wert
				if (formula)
				{
					return ExprTree::sym("%s.n",name);
				}
				else
				{
					PMatrix Pcnet_inv = Pcnet_.inverse();
					return ExprTree::val(
						v_const_net_.get(
							Pcnet_inv(idx)-ndep_net+1
							)
						);
				}
				}
			};
			break;
		case XCH:

			nfree_xch = Vxch_.cols() - 1;
			ndep_xch = S_.cols() - nfree_xch;

			switch (v_type_xch_.get(idx))
			{
			case f_undefined:
				// kein Flusswert sollte jetzt noch undefiniert sein
				fASSERT_NONREACHABLE();
				return 0;
			case f_dependent:
			case f_quasicons:
				{
				// Zeile von V_
				i = idx;
				if (Vxch_.get(i,0))
					E = ExprTree::val(Vxch_.get(i,0));
				for (j=1; j<Vxch_.cols(); j++)
				{
					coeff = Vxch_.get(i,j);
					if (coeff == 0.)
						continue;
					// Flussindex idx:
					idx = Pcxch_.get(j-1+ndep_xch);
					idx_name = S_.getReactionName(idx);
					// hat der Fluss einen festen Wert? Dann wird dieser
					// gegen�ber dem Symbol bevorzugt ... (eval sorgt f�r
					// das �brige)
					if (getFluxType(idx_name,false) == f_constraint)
					{
						idx_sym = getSymbolicPoolFlux(
							idx_name,XCH,formula
							);
					}
					else
						idx_sym = ExprTree::sym("%s.x",idx_name);

					if (coeff < 0.)
					{
						ExprTree * P = ExprTree::mul(
								ExprTree::val(-coeff),
								idx_sym
								);
						if (E)
							E = ExprTree::sub(E,P);
						else
							E = ExprTree::minus(P);
					}
					else
					{
						ExprTree * P = ExprTree::mul(
								ExprTree::val(coeff),
								idx_sym
								);
						if (E)
							E = ExprTree::add(E,P);
						else
							E = P;
					}
				}
				if (E)
					E->eval(true);
				else
					// AHHH! die Zeile ist komplett 0
					// ... es gibt nur eine L�sung: 0 ;-)
					E = ExprTree::val(0); // richtig?
				return E;
				}
			case f_free:
				// freie Fl�sse sind ihre eigene L�sung
				return ExprTree::sym("%s.x",name);
			case f_constraint:
				{
				// Constraint-Fl�sse haben einen festen Wert
				if (formula)
				{
					return ExprTree::sym("%s.x",name);
				}
				else
				{
					PMatrix Pcxch_inv = Pcxch_.inverse();
					return ExprTree::val(
						v_const_xch_.get(
							Pcxch_inv(idx)-ndep_xch+1
							)
						);
				}
				}
			};
			break;
		case POOL:

			nfree_pool = Vpool_.cols() - 1;
			ndep_pool = S_.rows() - nfree_pool;

			switch (v_type_pool_.get(idx))
			{
			case p_undefined:
				// kein Flusswert sollte jetzt noch undefiniert sein
				fASSERT_NONREACHABLE();
				return 0;
			case p_dependent:
			case p_quasicons:
				{
				// Zeile von V_
				i = idx;
				if (Vpool_.get(i,0))
					E = ExprTree::val(Vpool_.get(i,0));
				for (j=1; j<Vpool_.cols(); j++)
				{
					coeff = Vpool_.get(i,j);
					if (coeff == 0.)
						continue;
					// Flussindex idx:
					idx = Pcpool_.get(j-1+ndep_pool);
					idx_name = S_.getMetaboliteName(idx);
					// hat der Fluss einen festen Wert? Dann wird dieser
					// gegen�ber dem Symbol bevorzugt ... (eval sorgt f�r
					// das �brige)
					if (getPoolType(idx_name) == p_constraint)
					{
						idx_sym = getSymbolicPoolFlux(
							idx_name,POOL,formula
							);
					}
					else
						idx_sym = ExprTree::sym("%s",idx_name);

					if (coeff < 0.)
					{
						ExprTree * P = ExprTree::mul(
								ExprTree::val(-coeff),
								idx_sym
								);
						if (E)
							E = ExprTree::sub(E,P);
						else
							E = ExprTree::minus(P);
					}
					else
					{
						ExprTree * P = ExprTree::mul(
								ExprTree::val(coeff),
								idx_sym
								);
						if (E)
							E = ExprTree::add(E,P);
						else
							E = P;
					}
				}
				if (E)
					E->eval(true);
				else
					// AHHH! die Zeile ist komplett 0
					// ... es gibt nur eine L�sung: 0 ;-)
					E = ExprTree::val(0); // richtig?
				return E;
				}
			case p_free:
				// freie Fl�sse sind ihre eigene L�sung
				return ExprTree::sym("%s",name);
			case p_constraint:
				{
				// Constraint-Fl�sse haben einen festen Wert
				if (formula)
				{
					return ExprTree::sym("%s",name);
				}
				else
				{
					PMatrix Pcpool_inv = Pcpool_.inverse();
					return ExprTree::val(
						v_const_pool_.get(
							Pcpool_inv(idx)-ndep_pool+1
							)
						);
				}
				}
			};
		
		break;
	}

	// sollte nicht erreicht werden ...
	fASSERT_NONREACHABLE();
	return 0;
} // ConstraintSystem::getSymbolicPoolFlux()

ExprTree * ConstraintSystem::getSymbolicFluxNetXch(
	char const * fluxname,
	ParameterType parameter_type,
	bool formula
	) const
{
	size_t * idxp, idx;
	size_t i, j;
	char const * idx_name;
	ExprTree * E = 0, * idx_sym;
	double coeff;

	switch (validation_state_)
	{
	case cm_ok:
	case cm_too_few_constr:
	case cm_linear_dep_constr:
	case cm_ineqs_violated:
		break;
	default:
		// die Funktion sollte in diesem Validierungszustand
		// nicht aufgerufen werden!
		fASSERT_NONREACHABLE();
		return 0;
	}
	
	// gaussJordan sollte Erfolg gehabt haben
	fASSERT(Vnet_.rows() > 0 and Vnet_.cols() > 0);
	fASSERT(Vxch_.rows() > 0 and Vxch_.cols() > 0);

	// Index des Flusses bestimmen
	idxp = flux2idx_.findPtr(fluxname);
	if (idxp == 0)
	{
		// es sollte ein gültiger Flussname übergeben werden
		fASSERT_NONREACHABLE();
		return 0;
	}
	idx = *idxp;
	
	if (parameter_type==data::NET)
	{
		int nfree_net = Vnet_.cols() - 1;
		int ndep_net = S_.cols() - nfree_net;

		switch (v_type_net_.get(idx))
		{
		case f_undefined:
			// kein Flusswert sollte jetzt noch undefiniert sein
			fASSERT_NONREACHABLE();
			return 0;
		case f_dependent:
		case f_quasicons:
			{
			// Zeile von V_
			i = idx;
			if (Vnet_.get(i,0))
				E = ExprTree::val(Vnet_.get(i,0));
			for (j=1; j<Vnet_.cols(); j++)
			{
				coeff = Vnet_.get(i,j);
				if (coeff == 0.)
					continue;
				// Flussindex idx:
				idx = Pcnet_.get(j-1+ndep_net);
				idx_name = S_.getReactionName(idx);
				// hat der Fluss einen festen Wert? Dann wird dieser
				// gegenüber dem Symbol bevorzugt ... (eval sorgt für
				// das Übrige)
				if (getFluxType(idx_name,true) == f_constraint)
				{
					idx_sym = getSymbolicFluxNetXch(
						idx_name,data::NET,formula
						);
				}
				else
					idx_sym = ExprTree::sym("%s.n",idx_name);

				if (coeff < 0.)
				{
					ExprTree * P = ExprTree::mul(
							ExprTree::val(-coeff),
							idx_sym
							);
					if (E)
						E = ExprTree::sub(E,P);
					else
						E = ExprTree::minus(P);
				}
				else
				{
					ExprTree * P = ExprTree::mul(
							ExprTree::val(coeff),
							idx_sym
							);
					if (E)
						E = ExprTree::add(E,P);
					else
						E = P;
				}
			}
			if (E)
				E->eval(true);
			else
			{
				// AHHH! die Zeile ist komplett 0
				// ... es gibt nur eine Lösung: 0 ;-)
				E = ExprTree::val(0); // richtig?
			}
			return E;
			}
		case f_free:
			// freie Flüsse sind ihre eigene Lösung
			return ExprTree::sym("%s.n",fluxname);
		case f_constraint:
			{
			// Constraint-Flüsse haben einen festen Wert
			if (formula)
			{
				return ExprTree::sym("%s.n",fluxname);
			}
			else
			{
				PMatrix Pcnet_inv = Pcnet_.inverse();
				return ExprTree::val(
					v_const_net_.get(
						Pcnet_inv(idx)-ndep_net+1
						)
					);
			}
			}
		}
	}
	else // Exchange-Fluß
	{
		int nfree_xch = Vxch_.cols() - 1;
		int ndep_xch = S_.cols() - nfree_xch;

		switch (v_type_xch_.get(idx))
		{
		case f_undefined:
			// kein Flusswert sollte jetzt noch undefiniert sein
			fASSERT_NONREACHABLE();
			return 0;
		case f_dependent:
		case f_quasicons:
			{
			// Zeile von V_
			i = idx;
			if (Vxch_.get(i,0))
				E = ExprTree::val(Vxch_.get(i,0));
			for (j=1; j<Vxch_.cols(); j++)
			{
				coeff = Vxch_.get(i,j);
				if (coeff == 0.)
					continue;
				// Flussindex idx:
				idx = Pcxch_.get(j-1+ndep_xch);
				idx_name = S_.getReactionName(idx);
				// hat der Fluss einen festen Wert? Dann wird dieser
				// gegenüber dem Symbol bevorzugt ... (eval sorgt für
				// das Übrige)
				if (getFluxType(idx_name,false) == f_constraint)
				{
					idx_sym = getSymbolicFluxNetXch(
						idx_name,data::XCH,formula
						);
				}
				else
					idx_sym = ExprTree::sym("%s.x",idx_name);

				if (coeff < 0.)
				{
					ExprTree * P = ExprTree::mul(
							ExprTree::val(-coeff),
							idx_sym
							);
					if (E)
						E = ExprTree::sub(E,P);
					else
						E = ExprTree::minus(P);
				}
				else
				{
					ExprTree * P = ExprTree::mul(
							ExprTree::val(coeff),
							idx_sym
							);
					if (E)
						E = ExprTree::add(E,P);
					else
						E = P;
				}
			}
			if (E)
				E->eval(true);
			else
				// AHHH! die Zeile ist komplett 0
				// ... es gibt nur eine Lösung: 0 ;-)
				E = ExprTree::val(0); // richtig?
			return E;
			}
		case f_free:
			// freie Flüsse sind ihre eigene Lösung
			return ExprTree::sym("%s.x",fluxname);
		case f_constraint:
			{
			// Constraint-Flüsse haben einen festen Wert
			if (formula)
			{
				return ExprTree::sym("%s.x",fluxname);
			}
			else
			{
				PMatrix Pcxch_inv = Pcxch_.inverse();
				return ExprTree::val(
					v_const_xch_.get(
						Pcxch_inv(idx)-ndep_xch+1
						)
					);
			}
			}
		}
	}

	// sollte nicht erreicht werden ...
	fASSERT_NONREACHABLE();
	return 0;
} // ConstraintSystem::getSymbolicFluxNetXch()


ExprTree * ConstraintSystem::getSymbolicFluxFwdBwd(
	char const * fluxname,
	bool get_fwd,
	bool formula
	) const
{
	ExprTree * net = getSymbolicFluxNetXch(fluxname,data::NET,formula);
	ExprTree * xch = getSymbolicFluxNetXch(fluxname,data::XCH,formula);

	// möglichst ordentliche Formeln:
	LinearExpression Lnet(net); delete net;
	LinearExpression Lxch(xch); delete xch;
	net = Lnet.get()->clone();
	xch = Lxch.get()->clone();

	if (get_fwd)
		return ExprTree::netxch2fwd(net,xch);
	else
		return ExprTree::netxch2bwd(net,xch);
} // ConstraintSystem::getSymbolicFluxFwdBwd()

// Forward/Backward => net/xch
//   net = fwd - bwd;
//   xch = min(fwd,bwd);
// net/xch01 => net/xch
//   net = net
//   xch = xch01 / (1 - xch01)
// net/xch => net/xch01
//   net = net
//   xch01 = xch / (1 + xch)
// net/xch => Forward/Backward
//   fwd = xch + max(net,0)
//   bwd = xch + max(-net,0)
bool ConstraintSystem::fwdFluxAdmissible(char const * fluxname) const
{
	size_t * idxp = flux2idx_.findPtr(fluxname);
	fASSERT( idxp != 0 );
	size_t idx = *idxp; // Index des Flusses

	// Wenn es keine Constraints auf den xch-Fluss gibt, dann
	// fließt auch der fwd-Fluss:
	if (v_type_xch_.get(idx) != f_constraint)
		return true;
	// Wenn der xch-Fluss nicht auf 0 festgelegt wurde, darf
	// der fwd-Fluss fließen (in Treue und Glauben wird hier
	// v_ zum Auslesen des Constraint-Fluss-Wertes genutzt):
	if (vxch_.get(idx) != 0.)
		return true;
	
	if (v_type_net_.get(idx) == f_constraint)
	{
		if (vnet_.get(idx) > 0.)
			return true;
		else
			return false;
	}
	else
	{
		// auskommentiert; siehe Warnung in bwdFluxAdmissible
		/*
		// existiert ein net<=0 Ungleichungs-Constraint? Dann darf
		// der fwd Fluss nicht fließen
		std::list< Constraint >::const_iterator ci;
		for (ci = cInEqList_.begin(); ci != cInEqList_.end(); ci++)
		{
			if (not ci->isNetConstraint())
				continue;
			if (not ci->isSimple())
				continue;
			if (not (IS_OP_LT(ci->getConstraint()) or IS_OP_LEQ(ci->getConstraint())))
				continue;
			if (strcmp(ci->getSimpleVarName(),fluxname) != 0)
				continue;
			if (ci->getSimpleVarValue() != 0.)
				continue;
			return false;
		}
		*/
		return true;
	}

	// wird nicht erreicht:
	fASSERT_NONREACHABLE();
	return false;
} // ConstraintSystem::fwdFluxAdmissible()

bool ConstraintSystem::bwdFluxAdmissible(char const * fluxname) const
{
	size_t * idxp = flux2idx_.findPtr(fluxname);
	fASSERT( idxp != 0 );
	size_t idx = *idxp; // Index des Flusses

	// Wenn es keine Constraints auf den xch-Fluss gibt, dann
	// fließt auch der bwd-Fluss:
	if (v_type_xch_.get(idx) != f_constraint)
		return true;
	// Wenn der xch-Fluss nicht auf 0 festgelegt wurde, darf
	// der bwd-Fluss fließen (in Treue und Glauben wird hier
	// v_ zum Auslesen des Constraint-Fluss-Wertes genutzt):
	if (vxch_.get(idx) != 0.)
		return true;
	
	if (v_type_net_.get(idx) == f_constraint)
	{
		if (vnet_.get(idx) < 0.)
			return true; // net<0 und xch=0 => bwd>0
		else
			return false;
	}
	else
	{
		// ACHTUNG: HIER WERDEN NUR GANZ EINFACHE CONSTRAINT GETESTET!
		// Es geht mehr darum den "Willen" des Parsers zu erfüllen,
		// der bei Attribut bidirectional="no" die Constraints xch=0
		// und net>0 einfügt

		// existiert ein net>0 Ungleichungs-Constraint? Dann darf
		// der bwd Fluss nicht fließen
		std::list< Constraint >::const_iterator ci;
		for (ci = cInEqList_.begin(); ci != cInEqList_.end(); ci++)
		{
			if (ci->getParameterType()==XCH)
				continue;
			if (not ci->isSimple())
				continue;
			if (not (IS_OP_GT(ci->getConstraint()) or IS_OP_GEQ(ci->getConstraint())))
				continue;
			if (strcmp(ci->getSimpleVarName(),fluxname) != 0)
				continue;

			// siehe Warnhinweis oben
			if (ci->getSimpleVarValue() != 0.)
				continue;
			return false;
		}
		return true;
	}

	// wird nicht erreicht:
	fASSERT_NONREACHABLE();
	return false;
} // ConstraintSystem::bwdFluxAdmissible()

ExprTree * ConstraintSystem::rebuildLinearExpression(
	ExprTree const * Eorig,
	ParameterType parameter_type
	) const
{
	ExprTree * E = Eorig->clone();
	charptr_array varnames = E->getVarNames();
	charptr_array::const_iterator vni;

	// Abh�ngige Variablen durch ihre L�sungsformeln ersetzen
	// (an Namen freier Fl�sse wird ein Suffix angeh�ngt) 
	for (vni = varnames.begin(); vni != varnames.end(); vni++)
	{
		ExprTree * Fdep = getSymbolicPoolFlux(*vni,parameter_type,false);
		// Formel des abh�ngigen Flusses einsetzen
		E->subst(*vni,Fdep);
		// Analytische L�sung freigeben
		delete Fdep;
	}

	// hier wird evtl. eine NonLinearExpressionException geworfen,
	// falls E nicht linear ist:
	LinearExpression lE(E);
	delete E;
	return lE.get()->clone();
}

ExprTree * ConstraintSystem::rebuildLinearExpression(
	ExprTree const * Eorig
	) const
{
	ParameterType parameter_type;
	PoolType pt;
	FluxType ft;
	char * fn;
	ExprTree * E = Eorig->clone();
	charptr_array varnames = E->getVarNames();
	charptr_array::const_iterator vni, vnj;

	// Abh�ngige Variablen durch ihre L�sungsformeln ersetzen
	for (vni = varnames.begin(); vni != varnames.end(); vni++)
	{
		parameter_type = POOL;
		fn = strdup_alloc(*vni);
		
		if (strlen(*vni) > 2)
		{
			if (strEndsWith(*vni,".n"))
			{
				parameter_type = NET;
				fn[strlen(fn)-2] = '\0';
			}
			else if (strEndsWith(*vni,".x"))
			{
				parameter_type = XCH;
				fn[strlen(fn)-2] = '\0';
			};
		}

		size_t * idx;
		switch(parameter_type)
		{
			case NET:
				idx = flux2idx_.findPtr(fn);
				if (idx == 0)
					fTHROW(ExprTreeException);
				ft = v_type_net_.get(*idx);

				if (ft == f_dependent or ft == f_constraint)
				{
					ExprTree * Fdep = getSymbolicPoolFlux(fn, NET, false);
					// Formel des abh�ngigen Flusses einsetzen
					E->subst(*vni,Fdep);
					// Analytische L�sung freigeben
					delete Fdep;
				}
				break;
			case XCH:
				idx = flux2idx_.findPtr(fn);
				if (idx == 0)
					fTHROW(ExprTreeException);
				ft = v_type_xch_.get(*idx);
	
				if (ft == f_dependent or ft == f_constraint)
				{
					ExprTree * Fdep = getSymbolicPoolFlux(fn, XCH, false);
					// Formel des abh�ngigen Flusses einsetzen
					E->subst(*vni,Fdep);
					// Analytische L�sung freigeben
					delete Fdep;
				}
				break;
			case POOL:
				idx = pool2idx_.findPtr(fn);
				if (idx == 0)
					fTHROW(ExprTreeException);
				pt = v_type_pool_.get(*idx);
	
				if (pt == p_dependent or pt == p_constraint)
				{
					ExprTree * Fdep = getSymbolicPoolFlux(fn,POOL,false);
					// Formel des abh�ngigen Flusses einsetzen
					E->subst(*vni,Fdep);
					// Analytische L�sung freigeben
					delete Fdep;
				}
				break;
		
		}
		delete[] fn;
	}

	// hier wird evtl. eine NonLinearExpressionException geworfen,
	// falls E nicht linear ist:
	LinearExpression lE(E);
	delete E;
	return lE.get()->clone();
}

// trägt die Ungleichungen in eine Standard-Form ein
bool ConstraintSystem::fillStandardForm(
	StandardForm & SF,
	bool fill_net,
	bool fill_xch,
	bool fill_psize
	) const
{
	bool status = true;

	ParameterType parameter_type;
	charptr_array fnames;
	charptr_array::const_iterator fi;

	// ein Aufruf der Funktion macht nur Sinn wenn eines der beiden
	// Flags gesetzt ist:
	fASSERT(fill_net or fill_xch or fill_psize);

	// Sicherstellen, dass alle freien Fl�sse in der StandardForm
	// vorkommen:
	if (fill_net)
	{
		// freie Netto-Fl�sse registrieren
		fnames = getFluxNamesByType(f_free,true);
		for (fi = fnames.begin(); fi!=fnames.end(); fi++)
		{
			char * fn = new char[(strlen(*fi)+3)*sizeof(char)];
			strcpy(fn,*fi);
			strcat(fn,".n");
			SF.registerVariable(fn);
			delete[] fn;
		}
	}

	if (fill_xch)
	{
		// freie Exchange-Fl�sse registrieren
		fnames = getFluxNamesByType(f_free,false);
		for (fi = fnames.begin(); fi!=fnames.end(); fi++)
		{
			char * fn = new char[(strlen(*fi)+3)*sizeof(char)];
			strcpy(fn,*fi);
			strcat(fn,".x");
			SF.registerVariable(fn);
			delete[] fn;
		}
	}

	if (fill_psize)
	{
		// freie Poolgrößen registrieren
		fnames = getPoolNamesByType(p_free);
		for (fi = fnames.begin(); fi!=fnames.end(); fi++)
		{
			char * fn = new char[(strlen(*fi)+1)*sizeof(char)];
			strcpy(fn,*fi);
			strcat(fn,"");                        
			SF.registerVariable(fn);
			delete[] fn;
		}
	}

	std::list< Constraint >::const_iterator ci;
	for (ci = cInEqList_.begin(); ci != cInEqList_.end(); ci++)
	{

		parameter_type = ci->getParameterType();
		
                // Ungleichungen nach net/xch filtern:
		if (not (((parameter_type==NET) and fill_net) 
			or ((parameter_type==XCH) and fill_xch)
			or ((parameter_type==POOL) and fill_psize)))
			continue;

		// Den Ausdruck zerlegen ...
		ExprTree * E = rebuildLinearExpression(
			ci->getConstraint(),
			parameter_type
			);

		// Eintragen der Ungleichung
		char * vn = 0;
		double vv;
		if (Constraint::checkSimplicity(E,&vn,&vv))
		{
			switch(parameter_type)
			{
				case NET:
					fASSERT(strEndsWith(vn,".n"));
					break;
				case XCH:
					fASSERT(strEndsWith(vn,".x"));
					break;
				case POOL:
					break;
			};
			if (E->getNodeType() == et_op_leq)
			{
				if (not SF.setUpperBound(vn,vv))
				{
					fERROR("failed setting upper bound for %s to %f",
						vn, vv);
				}
			}
			else if (E->getNodeType() == et_op_geq)
			{
				if (not SF.setLowerBound(vn,vv))
				{
					fERROR("failed setting lower bound for %s to %f",
						vn, vv);
				}
			}
			else fASSERT_NONREACHABLE(); // nur Ungleichungen!
		}
		else
		{
			SF.addConstraint(E);
		}
		
		// Constraint-Ungleichung freigeben
		delete E;
		// vn freigeben
		if (vn) delete[] vn;
	}
	
	// es ist kein Fehler, falls es keine Variablen mehr gibt
	// ... sondern Schicksal ...
	if (SF.getNumVars() == 0)
		return status;

	return status;
} // ConstraintSystem::fillStandardForm()


charptr_array ConstraintSystem::reportQuasiConstraintFluxes(
	bool net,
	LogLevel loglevel
	) const
{
	size_t i,j,k;
	double c;
	charptr_array qconst;
	charptr_array dep = getFluxNamesByType(f_dependent,net);
	charptr_array::const_iterator df;

	MMatrix const & V = net ? Vnet_ : Vxch_;
	PMatrix const & Pc = net ? Pcnet_ : Pcxch_;

	int nfree = V.cols() - 1;
	int ndep = S_.cols() - nfree;
	bool is_qconst;

	for (df=dep.begin(); df!=dep.end(); ++df)
	{
		i = flux2idx_.find(*df)->value; // Flussindex

		is_qconst = true;
		for (j=1; j<V.cols() and is_qconst; j++)
		{
			if ((c = V.get(i,j)) == 0.)
				continue;

			k = Pc.get(j-1+ndep);
			if (getFluxType(S_.getReactionName(k),net) == f_free)
				is_qconst = false;
		}

		if (is_qconst)
			qconst.add(*df);
	}

	if (loglevel != logQUIET and qconst.size())
	{
		fLOG(loglevel,
			"the following %s-fluxes are completely "
			"determined by constraint-fluxes:", net?"NET":"XCH");
		for (df=qconst.begin(); df!=qconst.end(); ++df)
		{
			// detaillierte Formel holen:
   			ExprTree * E;
			ExprTree * Eval;
			if(net)
			{
				E = getSymbolicPoolFlux(*df,NET,true);
				Eval = getSymbolicPoolFlux(*df,NET,false);
			}
			else
			{
				E = getSymbolicPoolFlux(*df,XCH,true);
				Eval = getSymbolicPoolFlux(*df,XCH,false);
			};
			LinearExpression lE(E);
			fLOG(loglevel,
				"   %s.%c [= %s] = %s", *df, net?'n':'x',
				lE.get()->toString().c_str(), Eval->toString().c_str());
			delete E;
			delete Eval;
		}
	}
	return qconst;
} // ConstraintSystem::reportQuasiConstraintFluxes()


void ConstraintSystem::dump() const
{
	size_t k;
	char const * rn;
	
	if (fluxes_dirty_)
		eval();
	
	switch (validation_state_)
	{
	case cm_ok:
	case cm_too_few_constr:
	case cm_linear_dep_constr:
	case cm_ineqs_violated:
		break;
	default:
		fWARNING("dumping invalid stoichiometry");
	}

	char sn[][5] = {"UDEF", "DEPD", "QCON", "FREE", "CONS", "COFA"};

	for (k=0; k<S_.cols(); k++)
	{
		rn = S_.getReactionName(k);
		fINFO("%18s: net=%+13.6g (%s), xch=%13.6g (%s)", rn,
			vnet_.get(k), sn[v_type_net_.get(k)],
			vxch_.get(k), sn[v_type_xch_.get(k)]);
	}

        /** Sortierte Liste der Pool und deren Poolgrößen-Typ und -Wert **/
        if(!is_stationary_)
        {
            charptr_array pn ;
            charptr_array::const_iterator pni;

            double psize;
            for (k=0; k<S_.rows(); k++)
                pn.add(S_.getMetaboliteName(k));
            pn.sort();
            for (pni=pn.begin();pni!=pn.end(); ++pni)
            {
                psize = 1.;
                getPoolSize(*pni, psize);
                if(psize!=0)
                    fINFO("%18s: pool=%+13.6g (%s)", *pni,
                        psize, 
                        sn[getPoolType(*pni)]);
                else
                    // Cofaktoren werden nur bei Stiochemitrie benötigt, und
                    // spielen bei ILE keine Rolle, da sie nicht C-balanziert  
                    // sind und deshlab die Poolgrößen auf Null gesetzt.
                    fINFO("%18s: pool=%+13.6g (COFA)", *pni, 0.0);
            }
        }
	if (GETLOGLEVEL() >= logDEBUG)
	{
		if (GETLOGLEVEL() >= logDEBUG1)
		{
			std::list< Constraint >::const_iterator li;
			fDEBUG(1,"EQUALITY constraints:");
			for (li=cEqList_.begin(); li!=cEqList_.end(); ++li)
				switch(li->getParameterType())
				{
					case NET:
						fDEBUG(1,"%s\t%s: %s", li->getName(),
							"NET",
							li->getConstraint()->toString().c_str());
						break;
					case XCH:
						fDEBUG(1,"%s\t%s: %s", li->getName(),
							"XCH",
							li->getConstraint()->toString().c_str());
						break;
					case POOL:
						fDEBUG(1,"%s\t%s: %s", li->getName(),
							"POOL",
							li->getConstraint()->toString().c_str());
						break;
				};
			fDEBUG(1,"INEQUALITY constraints:");
			for (li=cInEqList_.begin(); li!=cInEqList_.end(); ++li)
				switch(li->getParameterType())
				{
					case NET:
						fDEBUG(1,"%s\t%s: %s", li->getName(),
							"NET",
							li->getConstraint()->toString().c_str());
						break;
					case XCH:
						fDEBUG(1,"%s\t%s: %s", li->getName(),
							"XCH",
							li->getConstraint()->toString().c_str());
						break;
					case POOL:
						fDEBUG(1,"%s\t%s: %s", li->getName(),
							"POOL",
							li->getConstraint()->toString().c_str());
						break;
				};
		}
		fDEBUG(0,"it is assumed that the following fwd/bwd fluxes are present:");
		for (k=0; k<S_.cols(); k++)
		{
			rn = S_.getReactionName(k);
			fDEBUG(0,"%20s: (fwd: %s, bwd: %s)",
					rn,
					fwdFluxAdmissible(rn)?"yes":" no",
					bwdFluxAdmissible(rn)?"yes":" no"
			      );
		}
	}
} // ConstraintSystem::dump()


void ConstraintSystem::make_free_pool_suggestion(int dfree)
{
        size_t j;
        int rank;
        bool result;
        charptr_array sugg;
        PMatrix Pc, Pc_orig;
        MMatrix N, K;
        MVector b;


        Pc = Pcpool_;
        Pc_orig = Pcpool_;
        N = Npool_;
        b = MVector(Npool_.rows());

        result = MMatrixOps::gaussJordan(N,b,K,Pc,rank);
        if (result == false and rank == -3)
        {
                for (j=0; j<Pc.dim(); ++j)
                {
                        // nur Vorschl�ge
                        if (Pc(j) != 2) continue;
                        char const * rn = S_.getMetaboliteName(j);
                        if (fPools_size_.exists(rn))
                                sugg.add(rn);
                }
        }

        if (sugg.size())
        {
                fWARNING("hint: try deallocating %i out of these free POOL sizes:",
                                dfree);
                charptr_array::const_iterator si;
                for (si=sugg.begin(); si!=sugg.end(); ++si)
                        fWARNING("  %s", *si);
        }
        else
                fWARNING("sorry, no suggestion how to solve this problem");
}

bool ConstraintSystem::getPoolSize(
char const * poolname,
double & size
) const
{
            fASSERT( validation_state_ == cm_ok
                    || validation_state_ == cm_too_few_constr
                    || validation_state_ == cm_too_many_constr
                    || validation_state_ == cm_ineqs_violated);

            size_t * idx = pool2idx_.findPtr(poolname);
            if (idx == 0)
                    return false;

            if (fluxes_dirty_)
                    eval();

            size = vpool_.get(*idx);
            return true;
    }
    

bool ConstraintSystem::setPoolSize(
char const * poolname,
double size
)
{
            size_t * idx = pool2idx_.findPtr(poolname);
            fASSERT( idx != 0 );
            if (v_type_pool_(*idx) != p_free)
            {
                    fERROR("%s (pool) is not free", poolname);
                    fASSERT_NONREACHABLE();
                    return false;
            }

            if (vpool_(*idx) != size)
            {
                    change_count_++;
                    vpool_(*idx) = size;
            }
            return true;
    } 


charptr_array ConstraintSystem::getPoolNames() const
{
        charptr_array pnames = pool2idx_.getKeys();
        pnames.sort();
        return pnames;
}

    
charptr_array ConstraintSystem::getPoolNamesByType(
        PoolType ptype
        ) const
{
        charptr_array pnames;
        charptr_map< size_t >::const_iterator i;
        for (i=pool2idx_.begin(); i!=pool2idx_.end(); i++)
        {
                if (v_type_pool_.get(i->value) == ptype)
                        pnames.add(i->key);
        }

        // f_quasicons-Fl�sse sind auch dependent ...
        if (ptype == p_dependent)
                pnames.add(getPoolNamesByType(p_quasicons));

        pnames.sort();
        return pnames;
}


ConstraintSystem::PoolType ConstraintSystem::getPoolType(
char const * poolname
) const
{
        size_t * idx = pool2idx_.findPtr(poolname);
        fASSERT( idx != 0 );
        if (idx == 0)
                return p_undefined;
        return v_type_pool_.get(*idx);
} 

charptr_array ConstraintSystem::reportQuasiConstraintPools(
LogLevel loglevel
) const
{
    size_t i,j,k;
    double c;
    charptr_array qconst;
    charptr_array dep = getPoolNamesByType(p_dependent);
    charptr_array::const_iterator df;

    MMatrix const & V = Vpool_;
    PMatrix const & Pc = Pcpool_;

    int nfree = V.cols() - 1;
    int ndep = S_.rows() - nfree;
    bool is_qconst;

    for (df=dep.begin(); df!=dep.end(); ++df)
    {
            i = pool2idx_.find(*df)->value; // Flussindex

            is_qconst = true;
            for (j=1; j<V.cols() and is_qconst; j++)
            {
                    if ((c = V.get(i,j)) == 0.)
                            continue;

                    k = Pc.get(j-1+ndep);
                    if (getPoolType(S_.getMetaboliteName(k)) == p_free)
                            is_qconst = false;
            }

            if (is_qconst)
                    qconst.add(*df);
    }

    if (loglevel != logQUIET and qconst.size())
    {
            fLOG(loglevel,
                    "the following Poolsizes are completely "
                    "determined by constraint-pools");
            for (df=qconst.begin(); df!=qconst.end(); ++df)
            {
                    // detaillierte Formel holen:
                    ExprTree * E = getSymbolicPoolFlux(*df,POOL,true);
                    ExprTree * Eval = getSymbolicPoolFlux(*df,POOL,false);
                    LinearExpression lE(E);
                    fLOG(loglevel,
                            "   %s [= %s] = %s", *df,
                            lE.get()->toString().c_str(), Eval->toString().c_str());
                    delete E;
                    delete Eval;
            }
    }
    return qconst;
}


} // namespace data
} // namespace flux

