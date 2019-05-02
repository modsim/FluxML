#ifndef MMMODEL_H
#define MMMODEL_H

#include <xercesc/dom/DOM.hpp>
#include "MGroup.h"
#define XN XERCES_CPP_NAMESPACE_QUALIFIER

namespace flux {
namespace xml {

class MMDocument;

class MMModel
{
private:
	MMDocument * doc_;

public:
	MMModel(MMDocument * mmdoc)
		: doc_(mmdoc) { }

private:
	void parseLabelingMeasurement(XN DOMNode * labelingmeasurement);
	void parseFluxMeasurement(XN DOMNode * fluxmeasurement);
	void parseFluxRatios(XN DOMNode * fluxratios);
        void parsePoolsizeRatios(XN DOMNode * poolsizeratios);
        
	symb::ExprTree * parseFluxMeasurementTextual(XN DOMNode * textual);
	symb::ExprTree * parseFluxMeasurementMath(XN DOMNode * math);

	void parsePoolMeasurement(XN DOMNode * poolmeasurement);

	void parseTimesAttr(XN DOMNode * group, MGroup * mg, XN DOMAttr * timesAttr);
	bool parseGroupScaleAttributeIsAuto(XN DOMNode * node);

	void parseGroup(XN DOMNode * group);
	symb::ExprTree ** parseTextual(XN DOMNode * textual, bool mgroup_spec);
	symb::ExprTree ** parseMath(XN DOMNode * math);
	MGroup * parseGroupSpecMS(XN DOMNode * group, char const * spec);
	MGroup * parseGroupSpecMIMS(XN DOMNode * group, char const * spec);
	MGroup * parseGroupSpecMSMS(XN DOMNode * group, char const * spec);
	MGroup * parseGroupSpec1HNMR(XN DOMNode * group, char const * spec);
	MGroup * parseGroupSpec13CNMR(XN DOMNode * group, char const * spec);
	symb::ExprTree ** parseErrorModel(XN DOMNode * errormodel);
	symb::ExprTree ** parseErrorModelTextual(XN DOMNode * textual);
	symb::ExprTree ** parseErrorModelMath(XN DOMNode * math);

public:
	/**
	 * Parst das MMDocument aus einem DOM-Knoten
	 *
	 * @param model DOM-Knoten
	 */
	void parse(XN DOMNode * model);
        
private:        
        
        symb::ExprTree * parsePoolMeasurementTextual(XN DOMNode * textual);
	symb::ExprTree * parsePoolMeasurementMath(XN DOMNode * math);

};

} // namespace flux::xml
} // namespace flux

#undef XN
#endif

