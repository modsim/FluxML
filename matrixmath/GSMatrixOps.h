#ifndef GSMATRIXOPS_H
#define GSMATRIXOPS_H

#include <list>
#include "Error.h"
#include "BitArray.h"
#include "DiGraph.h"
#include "PMatrix.h"
#include "GMatrix.h"
#include "GVector.h"
#include "GSMatrix.h"
#include "GMatrixOps.h"

namespace flux {
namespace la {
namespace GSMatrixOps {

template< typename T > bool SCCsolve(
	GSMatrix< T > const & A,
	GVector< T > const & b,
	GVector< T > & x
	)
{
	size_t i;
	size_t N = A.rows();
	std::list< unsigned int >::iterator u,v;
	
	fASSERT(A.rows() == A.cols());

	// falls nötig Größe von x anpassen
	if (x.dim() != A.cols())
		x = GVector< T >(A.cols());

	// Strukturelle Singularität falls nnz < dim
	if (A.nnz() < N)
	{
		fWARNING("structural singularity detected: nnz[%i] < dim[%i]",
			int(A.nnz()), int(N));
		return false;
	}
	
	// Jetzt wird der TRANSPONIERTE Graph zur Matrix A erstellt:
	// Im nicht-transponierten Graphen steht jeder Knoten für eine
	// Unbekannte und jede ausgehende Kante führt zu einer anderen
	// Unbekannten, von der die Lösung der ersten Unbekannten abhängt.
	// Im transponierten Graphen weisen die Kanten also in "Fließrichtung"
	// der Lösung und es ergibt sich eine obere rechts Block-Diagonal-
	// Matrix.
	DiGraph<> G_T(N), G(N);
	DiGraph<>::iterator w;
	
	typename GSMatrix< T >::iterator a_ij;
	for (a_ij=A.begin(); a_ij!=A.end(); a_ij++)
	{
		// Element als transponierte Kante eintragen.
		// => Kanten weisen zu den "fütternden" Knoten.
		G_T.makeEdge(a_ij->col, a_ij->row);
		// Element als nicht-transponierte Kante eintragen.
		// => Kanten weisen in "Flußrichtung" der Lösung.
		G.makeEdge(a_ij->row, a_ij->col);
	}

	// erstelle einen topologisch sortieren Graphen der SCCs von A
	DiGraph< std::list< unsigned int > > G_scc = G_T.makeSCCDAG(&G);

	// Initialisierung des Lösungsvektors (erforderlich!)
	for (i=0; i<N; i++) x(i) = 0.;
	
	// Lösen des GLS für jede Komponente:
	for (i=0; i<G_scc.nodes(); i++) // GLS für jede Komponente lösen ...
	{
		std::list< unsigned int > & nodes = G_scc[i];
		size_t sdim = nodes.size();
		size_t ui,vi;
		
		// Speed-Up für SCCs der Größe 1 und 2; Für Systeme der
		// Größe 2 wird die numerisch sehr stabile Cramer-Regel
		// verwendet. Ab SCC-Größe 3 wird schon der LU-Algorithmus
		// verwendet (Rechenaufwand für Cramer-Regel ist im Vergleich
		// bereits etwa doppelt so groß)
		if (sdim == 1)
		{
			ui = *(nodes.begin());
			T bs = b.get(ui);
			
			// Korrektur des b-Vektors mit Hilfe des nicht-transponierten
			// Graphen:
			for (w=G.begin(ui); w!=G.end(ui); w++) // Spalten
				bs -= A.get(ui,w->dest) * x(w->dest);
			
			// A(ui,ui)*x=bs
			x(ui) = bs / A.get(ui,ui);
			continue;
		}
		else if (sdim == 2)
		{
			u = nodes.begin();
			size_t k1,k2;
			T det, b1, b2;
			k1 = *u; u++;
			k2 = *u;
			b1 = b.get(k1); b2 = b.get(k2);

			// Korrektur des b-Vektors mit Hilfe des nicht-transponierten
			// Graphen:
			for (w=G.begin(k1); w!=G.end(k1); w++) // Spalten in Zeile k1
				b1 -= A.get(k1,w->dest) * x(w->dest);
			for (w=G.begin(k2); w!=G.end(k2); w++) // Spalten in Zeile k2
				b2 -= A.get(k2,w->dest) * x(w->dest);
			
			// Determinante
			det = A.get(k1,k1) * A.get(k2,k2) - A.get(k1,k2) * A.get(k2,k1);
			// Determinante mit b in erster Spalte
			x(k1) = (b1 * A.get(k2,k2) - A.get(k1,k2) * b2) / det;
			// Determinante mit b in zweiter Spalte
			x(k2) = (A.get(k1,k1) * b2 - b1 * A.get(k2,k1)) / det;
			continue;
		}

		GMatrix< T > As(sdim,sdim); // Teil-Systemmatrix
		GVector< T > bs(sdim); // Teil-b-Vektor f.d. Systemmatrix

		// Teil-Systemmatrix As und Konstantenvektor bs aufbauen:
		for (ui=0,u=nodes.begin(); u!=nodes.end(); u++,ui++)
		{
			// erster Schritt: Kopie der Werte des b-Vektors
			bs(ui) = b.get(*u);
			for(vi=0,v=nodes.begin(); v!=nodes.end(); v++,vi++)
				As(ui,vi) = A.get(*u,*v);
		}
		
		// Subtraktion der Komponenten des Matrix-Vektor-Produkts
		// rechts des aktuellen SCC-Blocks.
		// Diese neue Version nutzt den nicht-transponierten Graphen,
		// durch den sofort(!) bekannt ist, in welchen Elementen von A
		// von null verschiedene Werte stehen (da das Iterieren über
		// die Adjazenzen einen gewissen Mehraufwand bedeutet, kann
		// die konventionelle Variante bei starker Besetztheit evtl.
		// etwas schneller sein!):
		for (ui=0,u=nodes.begin(); u!=nodes.end(); u++,ui++) // Zeilen
			for (w=G.begin(*u); w!=G.end(*u); w++) // Spalten
				bs(ui) -= A.get(*u,w->dest) * x(w->dest);
		
		// Teil-System lösen -> Teil-Lösung xs:
		// Eigenes LU-Verfahren ohne Nachiteration
		PMatrix P(sdim);
		if (not GMatrixOps::_LUfactor(As,P))
			return false;
		GMatrixOps::_LUsolve(As,P,bs);

		// Teil-Lösung xs in x übertragen:
		for (ui=0,u=nodes.begin(); u!=nodes.end(); u++,ui++)
			x(*u) = bs(ui);
		// As, bs, xs verwerfen ...
	}
	return true;
}

template< typename T > bool SCCsolve(
	GSMatrix< T > const & A,
	GMatrix< T > const & B,
	GMatrix< T > & X
	)
{
	size_t i,j;
	size_t N = A.rows();
	size_t Nu = B.cols();
	std::list< unsigned int >::iterator u,v;
	
	fASSERT(A.rows() == A.cols() and B.rows() == A.rows());
	fASSERT(X.rows() == A.rows() and X.cols() == B.cols());
	
	// Strukturelle Singularität falls nnz < dim
	if (A.nnz() < N)
		return false;
	
	// Jetzt wird der TRANSPONIERTE Graph zur Matrix A erstellt:
	// Im nicht-transponierten Graphen steht jeder Knoten für eine
	// Unbekannte und jede ausgehende Kante führt zu einer anderen
	// Unbekannten, von der die Lösung der ersten Unbekannten abhängt.
	// Im transponierten Graphen weisen die Kanten also in "Fließrichtung"
	// der Lösung und es ergibt sich eine obere rechts Block-Diagonal-
	// Matrix.
	DiGraph<> G_T(N), G(N);
	DiGraph<>::iterator w;
	
	typename GSMatrix< T >::iterator a_ij;
	for (a_ij=A.begin(); a_ij!=A.end(); a_ij++)
	{
		// Element als transponierte Kante eintragen.
		// => Kanten weisen zu den "fütternden" Knoten.
		G_T.makeEdge(a_ij->col, a_ij->row);
		// Element als nicht-transponierte Kante eintragen.
		// => Kanten weisen in "Flußrichtung" der Lösung.
		G.makeEdge(a_ij->row, a_ij->col);
	}

	// erstelle einen topologisch sortieren Graphen der SCCs von A
	DiGraph< std::list< unsigned int > > G_scc = G_T.makeSCCDAG(&G);

	// Initialisierung der Lösungsmatrix (erforderlich!)
	for (i=0; i<N; i++)
		for (j=0; j<Nu; j++)
			X(i,j) = 0.;
	
	// Lösen des GLS für jede Komponente:
	for (i=0; i<G_scc.nodes(); i++) // GLS für jede Komponente lösen ...
	{
		size_t sdim = G_scc[i].size();
		std::list< unsigned int > & nodes = G_scc[i];
		size_t ui,vi;
		
		// [spezialbehandlung für sdim=1,2 entfernt; ggfs. wieder
		// einfügen]
		
		GMatrix< T > As(sdim,sdim); // Teil-Systemmatrix
		GMatrix< T > Bs(sdim,Nu); // Teil-B-Matrix

		// Teil-Systemmatrix As und Konstantenmatrix Bs aufbauen:
		for (ui=0,u=nodes.begin(); u!=nodes.end(); u++,ui++)
		{
			// erster Schritt: Kopie der Werte der B-Matrix
			for (vi=0; vi<Nu; vi++)
				Bs(ui,vi) = B.get(*u,vi);
			for (vi=0,v=nodes.begin(); v!=nodes.end(); v++,vi++)
				As(ui,vi) = A.get(*u,*v);
		}
		
		// Subtraktion der Komponenten des Matrix-Vektor-Produkts
		// rechts des aktuellen SCC-Blocks.
		// Diese neue Version nutzt den nicht-transponierten Graphen,
		// durch den sofort(!) bekannt ist, in welchen Elementen von A
		// von null verschiedene Werte stehen (da das Iterieren über
		// die Adjazenzen einen gewissen Mehraufwand bedeutet, kann
		// die konventionelle Variante bei starker Besetztheit evtl.
		// etwas schneller sein!):
		for (ui=0,u=nodes.begin(); u!=nodes.end(); u++,ui++) // Zeilen
			for (w=G.begin(*u); w!=G.end(*u); w++) // Spalten
				for (vi=0; vi<Nu; vi++) // Lösungsspalten
					Bs(ui,vi) -= A.get(*u,w->dest) * X.get(w->dest,vi);
		
		// Teil-System lösen -> Teil-Lösung Bs:

		// Multi-RHS LU-Verfahren ohne Nachiteration
		PMatrix P(sdim);
		if (not GMatrixOps::_LUfactor(As,P))
			return false;
		GMatrixOps::_LUsolve(As,P,Bs);
		
		// Teil-Lösung Xs in X übertragen:
		for (ui=0,u=nodes.begin(); u!=nodes.end(); u++,ui++)
			for (vi=0; vi<Nu; vi++)
				X(*u,vi) = Bs.get(ui,vi);
	}
	return true;
}

/**
 * Erkennung von Elementen die bei der Invertierung der übergebenen, dünn
 * besetzten Matrix A rein strukturell den Wert 0 haben werden.
 *
 * @param A Matrix
 * @return Matrix mit Besetzungsstruktur
 */
template< typename T > std::list< mxkoo > detectInversionZeros(GSMatrix< T > const & A)
{
	size_t i,j,N = A.rows();
	T zero(0);

	fASSERT(A.cols() == N);

	// Matrix -> Graph
	DiGraph<> G(N);
	for (i=0; i<N; ++i)
		for (j=0; j<N; ++j)
			if (not (A.get(i,j) == zero))
				G.makeEdge(i,j);

	// Graph -> Trans. Closure
	BitArray trans = G.transitiveClosure();

	// not(Trans. Closure) -> Liste
	std::list< mxkoo > zeros;
	for (i=0; i<N; ++i)
		for (j=0; j<N; ++j)
			if (not trans[i*N+j])
				zeros.push_back(mxkoo(i,j));

	return zeros;
}

} // namespace flux::la::GSMatrixOps
} // namespace flux::la
} // namespace flux

#endif

