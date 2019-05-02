#ifndef MMDATA_H
#define MMDATA_H

#include <xercesc/dom/DOM.hpp>
#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Parsen von Mess-Dokument.
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */
    
class MMDocument;

class MMData
{
private:
	MMDocument * doc_;

public:
	/**
	 * Constructor.
	 */
	MMData(MMDocument * mmdoc) : doc_(mmdoc) { }

private:
	/**
	 * Parst den Info-Datensatz am Kopf der Daten
	 */
	void parseInfo(XN DOMNode * info);

public:
	/**
	 * Parst das MMDocument aus einem DOM-Knoten
	 *
	 * @param data DOM-Knoten
	 */
	void parse(XN DOMNode * data);
	
};

} // namespace flux::xml
} // namespace flux

#endif

