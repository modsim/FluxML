#ifndef EXPRPARSER_H
#define EXPRPARSER_H

#include <iostream>
#include <string>

namespace flux {
namespace symb {

/**
 * Exception-Klasse für Parser-Fehler.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class ExprParserException
{
	friend std::ostream & operator<<(std::ostream &, ExprParserException &);
private:
	/** die Fehlermeldung */
	std::string err_msg;
public:
	/**
	 * Constructor
	 *
	 * @param e die Fehlermeldung
	 */
	ExprParserException(std::string e) : err_msg(e) {}

	/**
	 * Gibt die Fehlermeldung als string-Objekt zurück
	 *
	 * @retval string(Fehlermeldung)
	 */
	std::string toString() { return err_msg; }
};

/**
 * OStream-Ausgabe-Operator für ExprParserException-Objekt
 *
 * @param s std::ostream-Objekt
 * @param e ExprParserException-Objekt mit Fehlermeldung
 * @retval modifiziertes ostream-Objekt mit Fehlermeldung
 */
inline std::ostream & operator<<(std::ostream & s, ExprParserException & e)
{
	return s << e.err_msg;
}

/** Forward-Deklaration für die ExprTree-Klasse */
class ExprTree;

} // namespace flux::symb
} // namespace flux

/** Wurzelknoten des ge-parse-ten Ausdrucks */
extern flux::symb::ExprTree * et_root;
/** Input-String des Parsers */
extern char * et_inputstring;
/** Position im Input-String des Parsers */
extern int et_inputstring_pos;
/** Prototyp für den Parser */
extern int et_parse();
/** Prototyp für den Scanner (default) */
extern int et_lex_default();
/** Prototyp für den Scanner (Messmodell) */
extern int et_lex_mm();
/** Funktionspointer für den Scanner */
extern int (*et_lex)();

#endif

