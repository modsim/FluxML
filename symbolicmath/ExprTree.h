#ifndef EXPRTREE_H
#define EXPRTREE_H

#include <string>
#include <iostream>
#include <functional>
#include <cstdarg>
extern "C"
{
#include <stdint.h>
}
#include "ExprParser.h"
#include "Error.h"
#include "cstringtools.h"
#include "charptr_array.h"
#include "charptr_map.h"

namespace flux {
namespace symb {

/** Bits 0:4 für Operator; falls Bit 5 != 0, dann ist es kein Operator */
enum ExprType {
	et_op_add	= 0,
	et_op_sub,
	et_op_uminus,
	et_op_mul,
	et_op_div,
	et_op_pow,
	et_op_eq,
	et_op_neq,
	et_op_leq,
	et_op_geq,
	et_op_lt,
	et_op_gt,
	et_op_abs,
	et_op_min,
	et_op_max,
	et_op_sqr,
	et_op_sqrt,
	et_op_log,
	et_op_log2,
	et_op_log10,
	et_op_exp,
	et_op_diff,
        et_op_sin,
        et_op_cos,
	et_literal	= 32,
	et_variable	= 33
};

/**
 * Eine leere Exception-Klasse.
 */
class ExprTreeException {};

/*
 * Nützliche Makros
 */
#define IS_OP_ADD(n)	((n)->getNodeType() == et_op_add)
#define IS_OP_SUB(n)	((n)->getNodeType() == et_op_sub)
#define IS_OP_MUL(n)	((n)->getNodeType() == et_op_mul)
#define IS_OP_DIV(n)	((n)->getNodeType() == et_op_div)
#define IS_OP_UMINUS(n)	((n)->getNodeType() == et_op_uminus)
#define IS_OP_EQ(n)	((n)->getNodeType() == et_op_eq)
#define IS_OP_LEQ(n)	((n)->getNodeType() == et_op_leq)
#define IS_OP_LT(n)	((n)->getNodeType() == et_op_lt)
#define IS_OP_GEQ(n)	((n)->getNodeType() == et_op_geq)
#define IS_OP_GT(n)	((n)->getNodeType() == et_op_gt)
#define IS_OP_NEQ(n)	((n)->getNodeType() == et_op_neq)
#define IS_OP_ABS(n)	((n)->getNodeType() == et_op_abs)
#define IS_OP_EXP(n)	((n)->getNodeType() == et_op_exp)
#define IS_OP_MAX(n)	((n)->getNodeType() == et_op_max)
#define IS_OP_MIN(n)	((n)->getNodeType() == et_op_min)
#define IS_OP_SQRT(n)	((n)->getNodeType() == et_op_sqrt)
#define IS_OP_LOG(n)	((n)->getNodeType() == et_op_log)
#define IS_OP_LOG2(n)	((n)->getNodeType() == et_op_log2)
#define IS_OP_LOG10(n)	((n)->getNodeType() == et_op_log10)
#define IS_OP_SQR(n)	((n)->getNodeType() == et_op_sqr)
#define IS_OP_DIFF(n)	((n)->getNodeType() == et_op_diff)
#define IS_OP_SIN(n)	((n)->getNodeType() == et_op_sin)
#define IS_OP_COS(n)	((n)->getNodeType() == et_op_cos)

/**
 * Klasse für einen arithmetischen Ausdrucksbaum mit binären und
 * unären Operatoren. Unterstützt wird das Parsen von Formeln,
 * Partielle Auswertung und Algebraische Vereinfachung, Partielle
 * Differentiation.
 *
 * @author Michael Weitzel <info@13cflux.net>
 * @brief Klasse für einen arithmetischen Ausdrucksbaum
 * @class ExprTree ExprTree.h
 */
class ExprTree
{
private:
	/** Knoten-Typ (Operatoren, Literal, Variable) */
	ExprType node_type_;
	/** Markierung des Knotens (wird z.B. f.d. Compiler benötigt) */
	int8_t mark_;
	/** Hash-Wert des Unterbaums (wird rekursiv berechnet) */
	mutable size_t * hval_;
private:
	/** Wert des Knotens -- je nach Knotentyp <tt>node_type_</tt> */
	union
	{
		/** linke und rechte Unterbäume */
		struct { ExprTree * Lval_, * Rval_; } childs_;
		/** Literalwert */
		double lit_value_;
		/** Variablenname */
		char const * var_name_;
	} value_;

public:
    inline ExprTree()
    : node_type_(et_variable), mark_(0), hval_(0)
    {
        value_.var_name_ = strdup_alloc("");
        hashValue();
    }

	/**
	 * Constructor für einen Operatorknoten
	 *
	 * @param et Operatortyp
	 * @param lval Zeiger auf linken Nachfolger
	 * @param rval Zeiger auf rechten Nachfolger
	 */
	inline ExprTree(ExprType et, ExprTree * lval, ExprTree * rval = 0)
	{
		node_type_ = et;
		mark_ = 0;
		fASSERT( isOperator() );
		value_.childs_.Lval_ = lval;
		value_.childs_.Rval_ = rval;
		hval_ = 0;
		hashValue();
	}
	
	/**
	 * Copy-Constructor für sauberes, rekursives Kopieren von
	 * Ausdrucksbäumen.
	 *
	 * @param copy Referenz auf den zu kopierenden Ausdruck
	 */
	ExprTree(ExprTree const & copy);

	/**
	 * Constructor für einen Variablenknoten (Blattknoten).
	 *
	 * @param vname Symbolname
	 */
	inline ExprTree(char const * vname)
		: node_type_(et_variable), mark_(0), hval_(0)
	{
		value_.var_name_ = strdup_alloc(vname);
		hashValue();
	}

	/**
	 * Constructor für einen Variablenknoten (Blattknoten).
	 *
	 * @param vname Symbolname
	 */
	inline ExprTree(std::string const & vname)
		: node_type_(et_variable), mark_(0), hval_(0)
	{
		value_.var_name_ = strdup_alloc(vname.c_str());
		hashValue();
	}

	/**
	 * Constructor für einen Literalknoten (Blattknoten).
	 *
	 * @param ltvalue Literalwert
	 */
	ExprTree(double ltvalue);
	
	/**
	 * Destructor.
	 * Gibt Speicher für evtl. allokierten Variablennamen frei und
	 * löscht linken und rechten Unterbaum.
	 */
	~ExprTree();
	
public:
	/**
	 * Auslesen des linken Nachfolgers
	 *
	 * @return der linke Nachfolger oder 0, wenn es keinen gibt
	 */
	inline ExprTree * Lval() const
	{
		if (isLeaf()) return 0;
		return value_.childs_.Lval_;
	}

	/**
	 * Belegen des linken Nachfolgers.
	 */
	inline void Lval(ExprTree * L)
	{
		fASSERT( isOperator() );
		value_.childs_.Lval_ = L;
	}

	/**
	 * Auslesen des rechten Nachfolgers
	 *
	 * @return der rechte Nachfolger oder 0, wenn es keinen gibt
	 */
	inline ExprTree * Rval() const
	{
		if (isLeaf()) return 0;
		return value_.childs_.Rval_;
	}

	/**
	 * Belegen des rechten Nachfolgers.
	 */
	inline void Rval(ExprTree * R)
	{
		fASSERT( isBinaryOp() or R==0 );
		value_.childs_.Rval_ = R;
	}

	/**
	 * Vertauschen des linken und rechten Nachfolgers (z.B. bei kommutativen
	 * Operatoren) -- für den Compiler.
	 */
	inline void swapLvalRval()
	{
		fASSERT( isBinaryOp() );
		ExprTree * tmp = value_.childs_.Lval_;
		value_.childs_.Lval_ = value_.childs_.Rval_;
		value_.childs_.Rval_ = tmp;
		rebuildHashValue();
	}

	/**
	 * Ersetzt das aktuelle Objekt durch das Objekt E. Dadurch wird
	 * der Unterbaum von Objekt E eingebaut. Zurückggegeben wird
	 * der neue Zeiger auf das ursprungliche E (= this).
	 *
	 * @param E Referenz auf Zeiger auf ExprTree-Objekt (nach dem Aufruf 0)
	 * @return Zeiger auf den Knoten, der E entspricht (=this)
	 */
	ExprTree * replaceBy(ExprTree * & E);

	/**
	 * Auslesen des Werte der Markierung -- für den Compiler.
	 *
	 * @return Wert der Markierung
	 */
	inline int8_t getMark() const { return mark_; }

	/**
	 * Setzen des Werts des Markierung -- für den Compiler.
	 *
	 * @param m neuer Wert der Markierung
	 */
	inline void setMark(int8_t m) { mark_ = m; }

public:
	/**
	 * Vergleichsoperator.
	 * <b>Struktureller</b> Vergleich zweier Ausdrucksbäume
	 *
	 * @param E zu vergleichender Ausdruck
	 * @return true im Falle struktureller Gleichheit
	 */
	bool operator== (ExprTree const & E) const;

	/**
	 * Knoten-Typ-Vergleich; Ordnungsrelation:
	 *
	 *  [zahl] < [variable]
	 *  [zahl|variable] < [operator]
	 */
	bool operator< (ExprTree const & E) const;

private:
	/**
	 * Konvertierung eines Operators in einen String.
	 *
	 * @return string(operator)
	 */
	std::string subTreeToString() const;

public:
	/**
	 * Konvertierung eines einzelnen Knotens in einen String.
	 *
	 * @return string(this)
	 */
	std::string nodeToString() const;
	
	/**
	 * Konvertierung eines einzelnen Knotens in einen String
	 * (Source-Code Notation).
	 *
	 * @return string(this)
	 */
	std::string nodeToSource() const;
	
	/**
	 * Konvertiert den Ausdruck in einen String
	 *
	 * @return string(Ausdruck)
	 */
	std::string toString() const;

	/**
	 * Erzeugt einen String mit der Präfix-Notation des
	 * arithmetischen Ausdrucks. Mit Hilfe dieser Methode
	 * läßt sich auch Source-Code erzeugen. Dazu den Parameter
	 * makesrc auf true setzen:
	 *
	 * a+2 ergibt dann: ExprTree::add(ExprTree::sym("a"),ExprTree::val(2))
	 *
	 * @param makesrc auf true setzen, alls Source-Code erwünscht
	 * @return arithmetischer Ausdruck in Präfix-Notation
	 */
	std::string toPrefixString(bool makesrc = false) const;

	/**
	 * Auslesen des Knotentyps.
	 *
	 * @return Der Knotentyp
	 */
	inline ExprType getNodeType() const { return node_type_; }

	/**
	 * Setzen des Knotentyps (vorsichtig verwenden!).
	 *
	 * @param nt neuer Knotentyp
	 */
	inline void setNodeType(ExprType nt) { node_type_=nt; }

	/**
	 * Auslesen des Literalwerts
	 *
	 * @return der Literalwert
	 */
	inline double getDoubleValue() const
	{
		fASSERT( isLiteral() );
		return value_.lit_value_;
	}

	/**
	 * Ändert einen Literalwert.
	 *
	 * @param val Literalwert
	 */
	inline void setDoubleValue(double val)
	{
		fASSERT( isLiteral() );
		value_.lit_value_ = val;
	}

	/**
	 * Auslesen des Variablennamens
	 *
	 * @return der Variablenname
	 */
	inline char const * getVarName() const
	{
		if (isVariable())
			return value_.var_name_;
		fASSERT_NONREACHABLE();
		return 0;
	}

	/**
	 * Ändert einen Variablennamen.
	 *
	 * @param vn Variablenname
	 */
	inline void setVarName(char const * vn)
	{
		fASSERT( isVariable() );
		delete[] value_.var_name_;
		value_.var_name_ = strdup_alloc(vn);
	}

	/**
	 * Sammelt alle Variablennamen des Ausdrucks
	 *
	 * @return Liste der Variablennamen
	 */
	charptr_array getVarNames() const;

private:
	/**
	 * Rekursiver Worker von getVarNames()
	 *
	 * @param Array mit Variablennamen
	 */
	void getVarNames_descent(charptr_array & vn) const;

public:
	/**
	 * Gibt true zurück, falls es sich bei dem Knoten um einen Operator
	 * handelt.
	 *
	 * @return true, falls der Knoten ein Operator ist
	 */
	inline bool isOperator() const { return node_type_ < 32; }

	/**
	 * Gibt true zurück, falls es sich bei dem Knoten um ein Blatt handelt.
	 *
	 * @return true, falls der Knoten ein Blatt ist
	 */
	inline bool isLeaf() const { return node_type_ >= 32; }

	/**
	 * Gibt true zurück, falls es sich bei dem Knoten um eine Variable
	 * handelt.
	 *
	 * @return true, falls der Knoten eine Variable ist
	 */
	inline bool isVariable() const { return node_type_ == et_variable; }

	/**
	 * Gibt true zurück, falls es sich bei dem Knoten um ein Literal
	 * handelt.
	 *
	 * @return true, falls der Knoten ein Literal ist
	 */
	inline bool isLiteral() const { return node_type_ == et_literal; }

	/**
	 * Gibt true zurück, falls es sich bei dem Knoten um die Wurzel einer
	 * Gleichung handelt.
	 *
	 * @return true, falls der Knoten Wurzel einer Gleichung ist
	 */
	inline bool isEquality() const { return node_type_ == et_op_eq; }

	/**
	 * Gibt true zurück, falls es sich bei dem Knoten um die Wurzel einer
	 * Ungleichung handelt.
	 *
	 * @return true, falls der Knoten Wurzel einer Ungleichung ist
	 */
	inline bool isInEquality() const
	{
		switch (node_type_)
		{
		case et_op_leq:
		case et_op_lt:
		case et_op_geq:
		case et_op_gt:
		case et_op_neq:
			return true;
		default:
			return false;
		}
	}

	/**
	 * Gibt true zurück, falls es sich bei dem Knoten um einen unären
	 * Operator handelt.
	 *
	 * @return true, falls der Knoten ein unärer Operator ist
	 */
	inline bool isUnaryOp() const
	{
		switch (node_type_)
		{
		case et_op_uminus:
		case et_op_abs:
		case et_op_exp:
		case et_op_sqrt:
                case et_op_sin:
                case et_op_cos:
		case et_op_log:
		case et_op_log2:
		case et_op_log10:
		case et_op_sqr:
			return true;
		default:
			return false;
		}
	}

	/**
	 * Gibt true zurück, falls es sich bei dem Knoten um einen binären
	 * Operator handelt.
	 *
	 * @return true, falls der Knoten ein binärer Operator ist
	 */
	inline bool isBinaryOp() const
	{
		switch (node_type_)
		{
		case et_op_add:
		case et_op_sub:
		case et_op_mul:
		case et_op_div:
		case et_op_pow:
		case et_op_eq:
		case et_op_leq:
		case et_op_lt:
		case et_op_geq:
		case et_op_gt:
		case et_op_neq:
		case et_op_max:
		case et_op_min:
		case et_op_diff:
			return true;
		default:
			return false;
		}
	}

	/**
	 * Gibt true zurück, falls es sich bei dem Knoten um einen kommutativen
	 * Operator handelt.
	 *
	 * @return true, falls der Knoten ein kommutativer Operator ist
	 */
	inline bool isCommutativeOp() const
	{
		switch (node_type_)
		{
		case et_op_add:
		case et_op_mul:
		case et_op_eq:
		case et_op_neq:
		case et_op_max:
		case et_op_min:
			return true;
		default:
			return false;
		}
	}
private:
	/**
	 * Vereinfachung des Ausdrucks durch Zusammenziehen von Additionen
	 * und Subtraktionen
	 */
	void compressAddSub();

	/**
	 * Vereinfachung des Ausdrucks durch Zusammenfassung von Brüchen.
	 */
	void compressMulDiv();

public:
	/**
	 * Berechnet die Ableitung des Ausdrucks nach der angegebenen Variablen
	 * und gibt sie als neuen Ausdrucksbaum zurück.
	 *
	 * @param x Variable, nach der der Ausdruck abgeleitet werden soll
	 * @param dsymmap Abbildung eines Symbolnamen auf eine Liste von
	 * 	Variablennamen (von denen der Symbolname abhängt)
	 * @return ein neuer Ausdrucksbaum, mit der Ableitung des Ausdrucks
	 * 	nach er Variablen x
	 */
	ExprTree * deval(
		char const * x,
		charptr_map< charptr_array > const * dsymmap = 0
		) const;

	/**
	 * Lokale Auswertung und Vereinfachung eines Knotens (nicht-rekursiv).
	 * Die Funktion allokiert keinen Speicher, sondern arbeitet lediglich
	 * auf dem aktuellen Knoten. Zurückgegeben wird immer "this".
	 *
	 * @param force Bei force==false wird Bruchrechnung verwendet (sonst FP)
	 * @param dsymmap Abbildung eines Symbolnamen auf eine Liste von
	 * 	Variablennamen (von denen der Symbolname abhängt)
	 * @param deleteChilds wenn false, darf evalNode keine Nachfolger
	 * 	löschen
	 */
	ExprTree * evalNode(
		bool force/* = false*/,
		charptr_map< charptr_array > const * dsymmap/* = 0*/,
		bool deleteChilds/* = true*/
		);

	/**
	 * Partial-Evalutator.
	 * Bottom-Up-Auswertung und Zusammenfassung/Vereinfachung von
	 * Ausdrücken.
	 *
	 * @param force fp-Auswertung von Brüchen forcieren
	 * @param dsymmap Abbildung eines Symbolnamen auf eine Liste von
	 * 	Variablennamen (von denen der Symbolname abhängt)
	 */
	void eval(
		bool force = false,
		charptr_map< charptr_array > * dsymmap = 0
		);

	/**
	 * Ausdrucksvereinfachung
	 */
	void simplify()
	{
		// einfache algebraische Optimierung
		eval(true);
		// Ausdruck expandieren
		while (not expand());
		// algebraische Optimierung 1
		eval(true);
		// algebraische Optimierung 2
		eval(true);
	}

	/**
	 * Ersetzt abs, min und max durch differenzierbare Pendants.
	 *
	 * @param st Glättungsoperator
	 * @param sp Ausdruck für den Glättungsparameter (wird beim Einsetzen geclont)
	 */
	void smoothen(
		ExprTree const * sp
		);

	/**
	 * Vereinfacht einen Ausdruck und macht ihn kanonisch.
	 * Aufrufen mit while (not E->canonicalize());. Verwendet eine
	 * Art Fixpunkt-Iteration.
	 *
	 * @return true, falls Ausdruck kanonisch geworden ist, false, falls
	 * 	noch weitere Aufrufe nötig
	 */
	bool canonicalize();

	/**
	 * Substituiert eine Variable durch einen Ausdruck.
	 * Also etwa x+a, a := 2*k => x+2*k.
	 *
	 * @param var zu definierendes Symbol
	 * @param newE einzusetzende Definition
	 */
	void subst(char const * var, ExprTree * const & newE);

private:
	void subst_descent(char const * var, ExprTree * const & newE);

public:

	/**
	 * Erzeugt eine für die Semantik (möglichst) eindeutige Id.
	 *
	 * @return (möglichst) eindeutige Id für die Semantik
	 */
	size_t semanticHashValue() const;

	/**
	 * Aktualisiert/generiert die Hash-Werte.
	 *
	 * @return Hash-Wert
	 */
	size_t rebuildHashValue() const;

	/**
	 * Erzeugt eine (möglichst) eindeutige Id für die Struktur des
	 * Ausdrucks.
	 *
	 * @return (möglichst) eindeutige Id für die Struktur
	 */
	size_t hashValue() const;

	/**
	 * Löscht / Invalidiert die Hashwerte.
	 * Ein neuer Hash-Wert kann mit hashValue() erzeugt werden
	 */
	void clearAllHashValues();

	void clearHashValue();

public:
	/**
	 * Auflösen einer linearen Gleichung nach 0: a=b => a-b=0
	 */
	ExprTree * solve0() const;

	/**
	 * Expandieren eines Ausdrucks unter Ausnutzung von Operator-
	 * Distributivität: a*(b+c) => a*b+a*c
	 *
	 * @return true, falls etwas expandiert wurde, false sonst
	 */
	bool expand();

	/**
	 * Wandelt, falls möglich, die floating-point-Zahlen in einem Ausdruck
	 * ohne Verlust von Genauigkeit in Brüche um.
	 *
	 * @param maxmag maximaler Betrag für Zähler und Nenner
	 * @return true, falls etwas rationalisiert wurde, false sonst
	 */
	bool rationalize(int maxmag = 1000000);

	/**
	 * Größe des Ausdrucks (Anzahl der Knoten im Baum)
	 *
	 * @return Anzahl der Knoten im Baum
	 */
	size_t size();

public:
	/**
	 * Aufruf des Parsers. Methode ist static; Aufruf mit
	 * ExprNode *x = ExprNode::parser("a+b");
	 * Bei Parser-Fehlern wird einen Exception vom Typ
	 * ExprParserException geworfen. Vorsicht: Methode ist
	 * nicht "Thread-safe".
	 *
	 * @param s zu parsender Ausdruck
	 * @return Zeiger auf den neu erzeugten Ausdrucksbaum
	 */
	static ExprTree * parse(char const * s, int(*scanner)()=0);

	/*
	 * Factory-Methoden
	 */
	
	/**
	 * Klonen eines ExprTree.
	 *
	 * @return Kopie von *this
	 */
	inline ExprTree * clone() const
	{
		return new ExprTree(*this);
	}
	
	/**
	 * static-Variante von clone() zum Klonen eines ExprTree.
	 *
	 * @param E zu kopierender ExprTree
	 * @return Kopie von E
	 */
	inline static ExprTree * clone(ExprTree const * E)
	{
		if (E == 0)
			return 0;
		return new ExprTree(*E);
	}
	
	/**
	 * Erzeugt ein neues Symbol.
	 * Der varargs-Parameter funktioniert wie bei sprintf.
	 *
	 * @param sfmt Formatierungsstring
	 * @return Zeiger auf neuen Symbol-Knoten
	 */
	static ExprTree * sym(char const * sfmt, ...);

	/**
	 * Erzeugt einen Wert.
	 *
	 * @param v double-Wert
	 * @return neuer Knoten
	 */
	inline static ExprTree * val(double v)
	{
		// aus -0. ein 0. machen:
		if (v == 0.) v = 0.;
		return new ExprTree(v);
	}

	/**
	 * Addition.
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a+b
	 */
	inline static ExprTree * add(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_add, a, b);
	}
	
	/**
	 * Subtraktion.
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a-b
	 */
	inline static ExprTree * sub(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_sub, a, b);
	}
	
	/**
	 * Multiplikation.
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a*b
	 */
	inline static ExprTree * mul(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_mul, a, b);
	}
	
	/**
	 * Division.
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a/b
	 */
	inline static ExprTree * div(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_div, a, b);
	}
	
	/**
	 * Potenz.
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a^b
	 */
	inline static ExprTree * pow(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_pow, a, b);
	}
	
	/**
	 * Unäres Minus.
	 *
	 * @param a Argument
	 * @return neuer Knoten mit -a
	 */
	inline static ExprTree * minus(ExprTree * a)
	{
		return new ExprTree(et_op_uminus, a, 0);
	}

	/**
	 * Gleichung.
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a=b
	 */
	inline static ExprTree * eq(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_eq, a, b);
	}
	
	/**
	 * Ungleichung (<=).
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a<=b
	 */
	inline static ExprTree * leq(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_leq, a, b);
	}
	
	/**
	 * Ungleichung (<).
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a<b
	 */
	inline static ExprTree * lt(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_lt, a, b);
	}
	
	/**
	 * Ungleichung (>=).
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a>=b
	 */
	inline static ExprTree * geq(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_geq, a, b);
	}
	
	/**
	 * Ungleichung (>).
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a>b
	 */
	inline static ExprTree * gt(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_gt, a, b);
	}
	
	/**
	 * Ungleichung (!=).
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit a!=b
	 */
	inline static ExprTree * neq(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_neq, a, b);
	}
	
	/**
	 * Absolutbetrag (abs).
	 *
	 * @param a Argument
	 * @return neuer Knoten mit abs(a)
	 */
	inline static ExprTree * abs(ExprTree * a)
	{
		return new ExprTree(et_op_abs, a, 0);
	}
	
	/**
	 * e-Funktion (exp).
	 *
	 * @param a Argument
	 * @return neuer Knoten mit exp(a)
	 */
	inline static ExprTree * exp(ExprTree * a)
	{
		return new ExprTree(et_op_exp, a, 0);
	}
	
	/**
	 * Maximum.
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit max(a,b)
	 */
	inline static ExprTree * max(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_max, a, b);
	}
	
	/**
	 * Minimum.
	 *
	 * @param a erstes Argument
	 * @param b zweites Argument
	 * @return neuer Knoten mit min(a,b)
	 */
	inline static ExprTree * min(ExprTree * a, ExprTree * b)
	{
		return new ExprTree(et_op_min, a, b);
	}
	
	/**
	 * Quadratwurzel (sqrt).
	 *
	 * @param a Argument
	 * @return neuer Knoten mit sqrt(a)
	 */
	inline static ExprTree * sqrt(ExprTree * a)
	{
		return new ExprTree(et_op_sqrt, a, 0);
	}
	
	/**
	 * Natürlicher Logarithmus (ln,log).
	 *
	 * @param a Argument
	 * @return neuer Knoten mit ln(a)
	 */
	inline static ExprTree * log(ExprTree * a)
	{
		return new ExprTree(et_op_log, a, 0);
	}
	
	/**
	 * Logarithmus dualis (ld,log2).
	 *
	 * @param a Argument
	 * @return neuer Knoten mit log2(a)
	 */
	inline static ExprTree * log_2(ExprTree * a)
	{
		return new ExprTree(et_op_log2, a, 0);
	}
	
	/**
	 * 10er-Logarithmus (lg,log10).
	 *
	 * @param a Argument
	 * @return neuer Knoten mit log10(a)
	 */
	inline static ExprTree * log_10(ExprTree * a)
	{
		return new ExprTree(et_op_log10, a, 0);
	}

	/**
	 * Quadrat.
	 *
	 * @param a Argument
	 * @return neuer Knoten mit a*a
	 */
	inline static ExprTree * sqr(ExprTree * a)
	{
		return new ExprTree(et_op_sqr, a, 0);
	}
        
        /**
	 * Sinus.
	 *
	 * @param a Argument
	 * @return neuer Knoten mit a*a
	 */
	inline static ExprTree * sin(ExprTree * a)
	{
		return new ExprTree(et_op_sin, a, 0);
	}

        
        /**
	 * Cosinus.
	 *
	 * @param a Argument
	 * @return neuer Knoten mit a*a
	 */
	inline static ExprTree * cos(ExprTree * a)
	{
		return new ExprTree(et_op_cos, a, 0);
	}
        
	/**
	 * Partielle Ableitung.
	 *
	 * @param a Argument
	 * @param v Variablenname
	 * @return neuer Knoten mit da/dv
	 */
	inline static ExprTree * diff(ExprTree * a, char const * v)
	{
		return new ExprTree(et_op_diff, a, sym(v));
	}
	
	/*
	 * Composite-Operatoren:
	 */
	
	/**
	 * Reziproke: 1/a
	 *
	 * @param a Argument
	 * @return 1/a
	 */
	inline static ExprTree * recip(ExprTree * a)
	{
		return div(val(1.),a);
	}
	
	/**
	 * Geschlossene Formel für Summe derr Geometrischen Reihe: 1/(1-a)
	 *
	 * @param a Argument
	 * @return Summe der Geometrischen Reihe von a
	 */
	inline static ExprTree * gseries(ExprTree * a)
	{
		return recip(sub(val(1),a));
	}
		
	/**
	 * max(x,y) mit "alpha-Glättung": max(x,y) = 0.5*(x+y+|x-y|)
	 * Geglättet wird hier nur die abs-Funktion.
	 * 
	 * @param x erstes Argument
	 * @param y zweites Argument
	 * @param a Glättungsparameter alpha
	 * @return max(x,y) geglättet
	 */
	static ExprTree * max_alpha(ExprTree * x, ExprTree * y, ExprTree * a);

	/**
	 * min(x,y) mit "alpha-Glättung": min(x,y) = 0.5*(x+y-|x-y|)
	 * Geglättet wird hier nur die abs-Funktion.
	 *
	 * @param x erstes Argument
	 * @param y zweites Argument
	 * @param a Glättungsparameter alpha
	 * @return min(x,y) geglättet
	 */
	static ExprTree * min_alpha(ExprTree * x, ExprTree * y, ExprTree * a);

	/**
	 * max(x,0)-Funktion ohne Glättung.
	 *
	 * @param x Argument
	 * @return max(x,0)
	 */
	inline static ExprTree * max0(ExprTree * x)
	{
		return max(x,val(0));
	}
	
	/**
	 * max(x,0)-Funktion mit "alpha-Glättung": max(x,0) = 0.5*(x+|x|)
	 * Geglättet wird hier nur die abs-Funktion.
	 *
	 * @param x Argument
	 * @param a Glättungsparameter alpha
	 * @return max(x,0) geglättet
	 */
	inline static ExprTree * max0_alpha(ExprTree * x, ExprTree * a)
	{
		// max(x,0) = 0.5*(x+|x|)
		return mul(val(.5),add(x,abs_alpha(clone(x),a)));
	}

	/**
	 * Abs-Funktion mit "alpha-Glättung": (x, a) -> sqrt(a+x^2)
	 *
	 * @param x Argument
	 * @param a Glättungsparameter alpha
	 * @return abs(x) geglättet
	 */
	inline static ExprTree * abs_alpha(ExprTree * x, ExprTree * a)
	{
		return sqrt(add(a,sqr(x)));
	}

	/**
	 * Netto/eXchange nach Forward Konvertierung: fwd = xch + max(net,0)
	 *
	 * @param net Netto-Wert
	 * @param xch Exchange-Wert
	 * @return fwd = xch + max(net,0)
	 */
	inline static ExprTree * netxch2fwd(ExprTree * net, ExprTree * xch)
	{
		return add(xch,max0(net));
	}
	
	/**
	 * Netto/eXchange nach Backward Konvertierung: bwd = xch + max(-net,0)
	 *
	 * @param net Netto-Wert
	 * @param xch Exchange-Wert
	 * @return bwd = xch + max(-net,0)
	 */
	inline static ExprTree * netxch2bwd(ExprTree * net, ExprTree * xch)
	{
		return add(xch,max0(minus(net)));
	}
	
	/**
	 * Glatte Signum-Funktion ("alpha"-Glättung)
	 *
	 * @param x Argument
	 * @param a Glättungsparameter alpha
	 * @return sign(x) geglättet
	 */
	static ExprTree * sign_alpha(ExprTree * x, ExprTree * a);

	/**
	 * Rationale Zahl.
	 *
	 * @param fp Floating-Point Wert
	 * @return ein fp entsprechender Bruch
	 */
	static ExprTree * frac(double fp);

}; // class ExprTree

/**
 * Typen-Definition für ExprTree-Zeiger
 */
typedef ExprTree * exprptr;

/**
 * Ein Vergleichs-Funktor für ExprTree-Zeiger
 */
struct exprptr_eq : public std::equal_to<exprptr>
{
	inline bool operator()(exprptr const & x, exprptr const & y) const
	{
		// Strukturelle Gleichheit (und Gleicheit der Pointer)
		return (*x == *y);
#if 0
		// semantische Gleichheit
		//   == strukturelle Gleichheit der (optimierten) Normalformen
		bool eq;
		ExprTree * x_copy = x->clone();
		ExprTree * y_copy = y->clone();
		x_copy->eval(false);
		y_copy->eval(false);
		eq = (*x_copy == *y_copy);
		delete x_copy;
		delete y_copy;
		return eq;
#endif
	}
};

/**
 * Eine Hash-Funktion für ExprTree-Zeiger
 */
size_t exprptr_hashf(exprptr const & expr);

/**
 * OStream-Ausgabe-Operator für ExprTree-Zeiger
 *
 * @param s std::ostream-Objekt
 * @param x Zeiger auf den Ausdruck
 */
inline std::ostream & operator<< (std::ostream & s, ExprTree const * x)
{
	if (x != 0)
		return s << x->toString();
	else
		return s << std::string("(null)");
}

/**
 * OStream-Ausgabe-Operator für ein ExprTree-Objekt
 *
 * @param s std::ostream-Objekt
 * @param x Referenz auf den Ausdruck
 */
inline std::ostream & operator<< (std::ostream & s, ExprTree const & x)
{
	return s << x.toString();
}

} // namespace flux::symb
} // namespace flux

#endif

