#ifndef STOICHMATRIXINTEGER_H
#define STOICHMATRIXINTEGER_H

#include <cstddef>
extern "C"
{
#include <stdint.h>
}
#include "Error.h"
#include "charptr_array.h"
#include "GLabelMatrix.h"
#include "PMatrix.h"

namespace flux {
namespace la {

class MMatrix;

class StoichMatrixInteger : public GLabelMatrix< int64_t >
{
public:
	/**
	 * Constructor.
	 *
	 * @param nrows Anzahl der Zeilen (Metabolite)
	 * @param ncols Anzahl der Spalten (Flüsse)
	 * @param metabolite_names Metabolit-Namen
	 * @param reaction_names Flußbezeichnungen
	 */
	inline StoichMatrixInteger(
		size_t nrows, size_t ncols,
		char const ** metabolite_names,
		char const ** reaction_names
		) : GLabelMatrix< int64_t >(nrows,ncols,0)
	{
		size_t k;
		for (k=0; metabolite_names[k]!=0; ++k)
			setRowLabel(k, metabolite_names[k]);
		fASSERT(k==rows_);
		for (k=0; reaction_names[k]!=0; ++k)
			setColumnLabel(k, reaction_names[k]);
		fASSERT(k==cols_);
	}

	/**
	 * Copy-Constructor
	 */
	inline StoichMatrixInteger(StoichMatrixInteger const & copy) :
		GLabelMatrix< int64_t >(copy) { }

public:
	/**
	 * Gibt eine Array der Metabolitnamen zurück.
	 *
	 * @return Array der Metabolitnamen
	 */
	char const ** getMetaboliteNames() const { return getRowLabels(); }

	/**
	 * Gibt ein Array der Reaktionsnamen zurück.
	 *
	 * @return Array der Reaktionsnamen
	 */
	char const ** getReactionNames() const { return getColumnLabels(); }

	/**
	 * Gibt die einem Zeilenindex der Stöchiometrischen Matrix
	 * zugeordneten Pool (Name) zurück.
	 *
	 * @param i Zeilenindex
	 * @return Name des Pools mit Zeilenindex i
	 */
	inline char const * getMetaboliteName(size_t i) const
	{
		return getRowLabel(i);
	}

	/**
	 * Gibt den Zeilenindex eines Pools zurück. Der Pool wird
	 * durch einen String spezifiziert. Falls der Pool nicht existiert,
	 * wird eine -1 zurückgegeben.
	 *
	 * @param pname Bezeichnung des gesuchten Pools
	 * @return Zeilenindex des gesuchten Pools oder -1
	 */
	inline int getMetaboliteIndex(char const * pname) const
	{
		return getRowIndex(pname);
	}

	/**
	 * Gibt die einem Spaltenindex der Stöchiometrischen Matrix
	 * zugeordnete Reaktion (Name) zurück.
	 *
	 * @param j Spaltenindex
	 * @return Name der Reaktion mit Spaltenindex j
	 */
	inline char const * getReactionName(size_t j) const
	{
		return getColumnLabel(j);
	}

	/**
	 * Gibt den Spaltenindex einer Reaktion zurück. Die Reaktion wird
	 * durch einen String spezifiziert. Falls die Reaktion nicht existiert,
	 * wird eine -1 zurückgegeben.
	 *
	 * @param rname Bezeichnung der gesuchten Reaktion
	 * @return Spaltenindex der gesuchten Reaktion oder -1
	 */
	inline int getReactionIndex(char const * rname) const
	{
		return getColumnIndex(rname);
	}

	/**
	 * Debugging.
	 */
	void dump(FILE * outf = stdout, dump_t dt = dump_default, char const * fmt = "sg") const;

	/**
	 * Ein Cast-Operator von StoichMatrixInteger nach MMatrix
	 *
	 * @return ein MMatrix-Objekt mit dem Inhalt der StoichMatrixInteger
	 */
	operator MMatrix () const;

};

} // namespace flux::la
} // namespace flux

#endif

