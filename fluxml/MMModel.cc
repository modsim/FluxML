    #include "Error.h"
    #include "charptr_array.h"

    #include "Notation.h"

    #include "XMLElement.h"
    #include "MMUnicodeConstants.h"
    #include "FluxMLUnicodeConstants.h"
    #include "UnicodeTools.h"

    #include "MathML.h"
    #include "MathMLDocument.h"
    #include "MathMLDeclare.h"
    #include "MathMLVector.h"

    #include "MMDocument.h"
    #include "MMModel.h"


    // Xerces C++ Namespace einbinden
    XERCES_CPP_NAMESPACE_USE

    using namespace flux::symb;

    namespace flux {
    namespace xml {

    // möglicherweise ist kein Child vorhanden -> was dann?
    void MMModel::parse(DOMNode * model)
    {
            DOMNode * child;

            if (not XMLElement::match(model,mm_model,mm_xmlns_uri))
                    fTHROW(XMLException,model,"element \"model\" expected");

            child = XMLElement::skipJunkNodes(model->getFirstChild());

            if (XMLElement::match(child,mm_labelingmeasurement,mm_xmlns_uri))
            {
                    parseLabelingMeasurement(child);
                    child = XMLElement::nextNode(child);
            }

            if (XMLElement::match(child,mm_fluxmeasurement,mm_xmlns_uri))
            {
                    parseFluxMeasurement(child);
                    child = XMLElement::nextNode(child);
            }

            if (XMLElement::match(child,mm_fluxratios,mm_xmlns_uri))
            {
                    parseFluxRatios(child);
                    child = XMLElement::nextNode(child);
            }
            
            if (XMLElement::match(child,mm_poolsizeratios,mm_xmlns_uri))
            {
                    parsePoolsizeRatios(child);
                    child = XMLElement::nextNode(child);
            }
            
            if (XMLElement::match(child,mm_poolmeasurement,mm_xmlns_uri) ||
                XMLElement::match(child,mm_poolsizemeasurement,mm_xmlns_uri))
            {
                    parsePoolMeasurement(child);
                    child = XMLElement::nextNode(child);
            }

            // es dürfen keine Elemente mehr folgen!
            if (child != 0)
                    fTHROW(XMLException,child,"unexpected extra content in element \"model\"");
    }

    void MMModel::parseLabelingMeasurement(DOMNode * labelingmeasurement)
    {
            DOMNode * child;

            child = XMLElement::skipJunkNodes(labelingmeasurement->getFirstChild());

            while (child != 0)
            {
                    // (MSgroup|MSMSgroup|NMR1Hgroup|NMR13Cgroup|group)
                    if (XMLElement::match(child,mm_group,mm_xmlns_uri))
                            parseGroup(child);
                    else
                    {
                            U2A asc_child(XMLElement::getName(child,mm_xmlns_uri));
                            fTHROW(XMLException,child,"unexpected node \"%s\" found",
                                    (char const *)asc_child);
                    }
                    child = XMLElement::nextNode(child);
            }
    }

    bool MMModel::parseGroupScaleAttributeIsAuto(DOMNode * node)
    {
            DOMNamedNodeMap * nnm;
            DOMAttr * gsAttr;
            // is_auto=true  => Groupscale wird automatisch berechnet (default)
            // is_auto=false => Groupscale wird auf 1 gesetzt
            bool is_auto = true;

            fASSERT( node != 0 );

            nnm = node->getAttributes();
            if (nnm == 0) return is_auto;
            gsAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_scale));
            if (gsAttr != 0)
            {
                    if (XMLString::equals(gsAttr->getValue(),mm_auto))
                            is_auto = true;
                    else if (XMLString::equals(gsAttr->getValue(),mm_one))
                            is_auto = false;
                    else
                    {
                            U2A gsv(gsAttr->getValue());
                            fTHROW(XMLException,node,
                                    "invalid value for attribute groupscale: \"%s\"",
                                    (char const*)gsv);
                    }
            }
            return is_auto;
    }

    void MMModel::parseFluxMeasurement(DOMNode * fluxmeasurement)
    {
            ExprTree * expr;
            DOMNamedNodeMap * nnm;
            DOMNode * child, * childchild;
            DOMAttr * idAttr;
            MGroup * mg;

            child = XMLElement::skipJunkNodes(fluxmeasurement->getFirstChild());

            while (child != 0)
            {
                    if (XMLElement::match(child,mm_netflux,mm_xmlns_uri))
                    {
                            nnm = child->getAttributes();
                            if (nnm == 0)
                                    fTHROW(XMLException,child,
                                            "element netflux lacks attributes (id,fluxes)"
                                            );

                            idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_id));

                            if (idAttr == 0)
                                    fTHROW(XMLException,
                                            child,
                                            "element netflux lacks attribute \"id\""
                                            );
                            if (nnm->getNamedItem(mm_times) != 0)
                                    fTHROW(XMLException,child,
                                            "attribute \"times\" not allowed for flux measurements");

                            U2A asc_id(idAttr->getValue());

                            childchild = XMLElement::skipJunkNodes(child->getFirstChild());
                            if (XMLElement::match(childchild,fml_math,fml_xmlns_mathml_uri))
                                    expr = parseFluxMeasurementMath(childchild);
                            else if (XMLElement::match(childchild,fml_textual,fml_xmlns_uri))
                                    expr = parseFluxMeasurementTextual(childchild);
                            else
                                    fTHROW(XMLException,childchild,
                                            "formula in textual- or MathML notation expected");

                            mg = MGroupFlux::parseSpec(expr->toString().c_str(), true);
                            delete expr;
                            mg->setGroupId((char const*)asc_id);
                            doc_->registerGroup(child,mg);
                    }
                    else if (XMLElement::match(child,mm_xchflux,mm_xmlns_uri))
                    {
                            nnm = child->getAttributes();
                            if (nnm == 0)
                                    fTHROW(XMLException,
                                            child,
                                            "element netflux lacks attributes (id,fluxes)"
                                            );

                            idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_id));

                            if (idAttr == 0)
                                    fTHROW(XMLException,
                                            child,
                                            "element xchflux lacks attribute \"id\""
                                            );
                            if (nnm->getNamedItem(mm_times) != 0)
                                    fTHROW(XMLException,child,
                                            "attribute \"times\" not allowed for flux measurements");

                            U2A asc_id(idAttr->getValue());

                            childchild = XMLElement::skipJunkNodes(child->getFirstChild());
                            if (XMLElement::match(childchild,fml_math,fml_xmlns_mathml_uri))
                                    expr = parseFluxMeasurementMath(childchild);
                            else if (XMLElement::match(childchild,fml_textual,fml_xmlns_uri))
                                    expr = parseFluxMeasurementTextual(childchild);
                            else
                                    fTHROW(XMLException,childchild,
                                            "formula in textual- or MathML notation expected");

                            mg = MGroupFlux::parseSpec(expr->toString().c_str(), false);
                            delete expr;
                            mg->setGroupId((char const*)asc_id);
                            doc_->registerGroup(child,mg);
                    }
                    else
                    {
                            U2A asc_child(XMLElement::getName(child,mm_xmlns_uri));
                            fTHROW(XMLException,child,"unexpected node \"%s\" found",
                                    (char const *)asc_child);
                    }
                    child = XMLElement::nextNode(child);
            }
    }

    void MMModel::parseFluxRatios(DOMNode * fluxratios)
    {
            ExprTree * expr;
            DOMNamedNodeMap * nnm;
            DOMNode * child, * childchild;
            DOMAttr * idAttr;
            MGroup * mg;

            child = XMLElement::skipJunkNodes(fluxratios->getFirstChild());

            while (child != 0)
            {
                    if (XMLElement::match(child,mm_netratio,mm_xmlns_uri))
                    {
                            nnm = child->getAttributes();
                            if (nnm == 0)
                                    fTHROW(XMLException,child,
                                            "element netratio lacks attributes (id,fluxes)"
                                            );

                            idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_id));

                            if (idAttr == 0)
                                    fTHROW(XMLException,
                                            child,
                                            "element netratio lacks attribute \"id\""
                                            );
                            if (nnm->getNamedItem(mm_times) != 0)
                                    fTHROW(XMLException,child,
                                            "attribute \"times\" not allowed for netratio measurements");

                            U2A asc_id(idAttr->getValue());

                            childchild = XMLElement::skipJunkNodes(child->getFirstChild());
                            if (XMLElement::match(childchild,fml_math,fml_xmlns_mathml_uri))
                                    expr = parseFluxMeasurementMath(childchild);
                            else if (XMLElement::match(childchild,fml_textual,fml_xmlns_uri))
                                    expr = parseFluxMeasurementTextual(childchild);
                            else
                                    fTHROW(XMLException,childchild,
                                            "formula in textual- or MathML notation expected");

                            mg = MGroupFlux::parseSpec(expr->toString().c_str(), true);
                            delete expr;
                            mg->setGroupId((char const*)asc_id);
                            doc_->registerGroup(child,mg);
                    }
                    else if (XMLElement::match(child,mm_xchratio,mm_xmlns_uri))
                    {
                            nnm = child->getAttributes();
                            if (nnm == 0)
                                    fTHROW(XMLException,
                                            child,
                                            "element xchratio lacks attributes (id,fluxes)"
                                            );

                            idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_id));

                            if (idAttr == 0)
                                    fTHROW(XMLException,
                                            child,
                                            "element xchratio lacks attribute \"id\""
                                            );
                            if (nnm->getNamedItem(mm_times) != 0)
                                    fTHROW(XMLException,child,
                                            "attribute \"times\" not allowed for xchratio measurements");

                            U2A asc_id(idAttr->getValue());

                            childchild = XMLElement::skipJunkNodes(child->getFirstChild());
                            if (XMLElement::match(childchild,fml_math,fml_xmlns_mathml_uri))
                                    expr = parseFluxMeasurementMath(childchild);
                            else if (XMLElement::match(childchild,fml_textual,fml_xmlns_uri))
                                    expr = parseFluxMeasurementTextual(childchild);
                            else
                                    fTHROW(XMLException,childchild,
                                            "formula in textual- or MathML notation expected");

                            mg = MGroupFlux::parseSpec(expr->toString().c_str(), false);
                            delete expr;
                            mg->setGroupId((char const*)asc_id);
                            doc_->registerGroup(child,mg);
                    }
                    else
                    {
                            U2A asc_child(XMLElement::getName(child,mm_xmlns_uri));
                            fTHROW(XMLException,child,"unexpected node \"%s\" found",
                                    (char const *)asc_child);
                    }
                    child = XMLElement::nextNode(child);
            }
    }

    void MMModel::parsePoolsizeRatios(DOMNode * poolsizeratios)
    {
            ExprTree * expr;
            DOMNamedNodeMap * nnm;
            DOMNode * child, * childchild;
            DOMAttr * idAttr;
            MGroup * mg;

            child = XMLElement::skipJunkNodes(poolsizeratios->getFirstChild());

            while (child != 0)
            {
                    if (XMLElement::match(child,mm_poolsizeratio,mm_xmlns_uri))
                    {
                            nnm = child->getAttributes();
                            if (nnm == 0)
                                    fTHROW(XMLException,child,
                                            "element poolsizeratio lacks attributes (id,poolsizes)"
                                            );

                            idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_id));

                            if (idAttr == 0)
                                    fTHROW(XMLException,
                                            child,
                                            "element poolsizeratio lacks attribute \"id\""
                                            );
                            if (nnm->getNamedItem(mm_times) != 0)
                                    fTHROW(XMLException,child,
                                            "attribute \"times\" not allowed for poolsizeratio measurements");

                            U2A asc_id(idAttr->getValue());

                            childchild = XMLElement::skipJunkNodes(child->getFirstChild());
                            if (XMLElement::match(childchild,fml_math,fml_xmlns_mathml_uri))
                                    expr = parseFluxMeasurementMath(childchild);
                            else if (XMLElement::match(childchild,fml_textual,fml_xmlns_uri))
                                    expr = parseFluxMeasurementTextual(childchild);
                            else
                                    fTHROW(XMLException,childchild,
                                            "formula in textual- or MathML notation expected");

                            mg = MGroupPool::parseSpec(expr->toString().c_str());
                            delete expr;
                            mg->setGroupId((char const*)asc_id);
                            doc_->registerGroup(child,mg);
                    }
                    else
                    {
                            U2A asc_child(XMLElement::getName(child,mm_xmlns_uri));
                            fTHROW(XMLException,child,"unexpected node \"%s\" found",
                                    (char const *)asc_child);
                    }
                    child = XMLElement::nextNode(child);
            }
    }
    
    ExprTree * MMModel::parseFluxMeasurementTextual(DOMNode * textual)
    {
            ExprTree * expr;
            DOMNode * child;

            if (not XMLElement::match(textual,mm_textual))
                    fTHROW(XMLException,"element node (textual) expected.");

            child = textual->getFirstChild();
            // unter Element textual ist alles erlaubt -- ausgewertet
            // werden nur TEXT_NODEs
            while (child != 0 && child->getNodeType() != DOMNode::TEXT_NODE)
                    child = child->getNextSibling();

            if (child == 0)
                    fTHROW(XMLException,textual,"missing flux measurement formula");

            U2A cblock(static_cast< DOMText* >(child)->getData());
            try
            {
                    expr = ExprTree::parse((char const *)cblock);
            }
            catch (ExprParserException &)
            {
                    fTHROW(XMLException,child,
                            "parse error in flux measurement formula (%s)",
                            (char const *)cblock);
            }
            return expr;
    }

    ExprTree * MMModel::parseFluxMeasurementMath(DOMNode * math)
    {
            ExprTree * expr;
            MathMLDocument * mathml;
            MathMLDeclare const * mathml_def;
            MathMLContentObject const * mathml_cont;
            MathMLExpression const * mathml_expr;

            // den MathML-Parser anwerfen:
            try
            {
                    mathml = new MathMLDocument(math);
            }
            catch (XMLException & e)
            {
                    fTHROW(XMLException,math,
                            "MathML-error in flux measurement spec: %s", (char const*)e);
            }

            // Parsen abgeschlossen. Klassifikation der Inhalte
            std::list< MathMLDeclare const * > mml_defs
                    = mathml->getDefinitionsByRegExp(".*");

            if (mml_defs.size() > 1)
                    fTHROW(XMLException,math,
                            "more than one MathML definition found in flux measurement spec");

            // Sinnüberprüfung der Constraints
            mathml_def = *(mml_defs.begin());
            mathml_cont = mathml_def->getValue();

            // Constraints sind einfache MathMLExpressions:
            if (mathml_cont->getType() != MathMLContentObject::co_expression)
                    fTHROW(XMLException,math,"MathML expression expected");

            mathml_expr = static_cast< MathMLExpression const * >(mathml_cont);
            expr = mathml_expr->get()->clone();

            // MathML-Dokument freigeben:
            delete mathml;
            
            return expr;
    }

    void MMModel::parseTimesAttr(DOMNode * group, MGroup * mg, DOMAttr * timesAttr)
    {
            double ts;
            char * end_ptr;

            if (doc_->isStationary())
            {
                    if (mg) mg->registerTimeStamp(-1);
                    else doc_->registerTimeStamp(-1);

                    // times-Attribut im stationären Fall nicht erlaubt
                    if (timesAttr != 0)
                            fTHROW(XMLException,group,
                            "attribute \"times\" not allowed for stationary measurements");
                    return;
            }

            if (not doc_->isStationary() and timesAttr == 0)
            {
                    fTHROW(XMLException,group,
                            "attribute \"times\" required for non-stationary measurements");
            }

            U2A asc_times_str(timesAttr->getValue());
            charptr_array asc_times = charptr_array::split((char const*)asc_times_str," \t\n\r\f,;");
            charptr_array::const_iterator i;
            int j;

            for (i=asc_times.begin(),j=1; i!=asc_times.end(); i++,j++)
            {
                    ts = strtod(*i,&end_ptr);
                    if (end_ptr == *i)
                            fTHROW(XMLException,
                                    group,
                                    "attribute \"times\": error parsing value of argument #%d: [%s]",
                                    j, *i);
                    if (mg)
                    {
                            if (not mg->registerTimeStamp(ts))
                                    fTHROW(XMLException,
                                            group,
                                            "attribute \"times\": duplicate timestamp: %s",
                                            *i);
                    }
                    else doc_->registerTimeStamp(ts);
            }
    }

    void MMModel::parseGroup(DOMNode * group)
    {
            DOMNamedNodeMap * nnm;
            DOMAttr * idAttr, * timesAttr;
            DOMNode * child;
            MGroup * mg = 0;
            int pstate;
            ExprTree ** group_spec = 0;
            ExprTree ** error_model = 0;

            nnm = group->getAttributes();
            if (nnm == 0)
                    fTHROW(XMLException,
                            group,
                            "element group lacks attributes (id,type,times?)"
                            );

            idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_id));
            timesAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_times));

            if (idAttr == 0)
                    fTHROW(XMLException,
                            group,
                            "element group lacks required attribute \"id\""
                            );
            
            if ((child = XMLElement::skipJunkNodes(group->getFirstChild())) == 0)
                        fTHROW(XMLException,group,"empty group element");
            
            while (child != 0)
            {
                if (XMLElement::match(child,mm_errormodel))
                {
                    error_model = parseErrorModel(child);
                }
                else
                {
                    // MathML und Text-Einträge parsen:
                    if (child == 0)
                            fTHROW(XMLException,group,"empty group element");
                    else if (XMLElement::match(child,mm_textual))
                            group_spec = parseTextual(child,true);
                    else if (XMLElement::match(child,mm_math))
                            group_spec = parseMath(child);
                    else
                    {
                            fTHROW(XMLException,child,
                                    "measurement formula spec in textual- or MathML notation expected"
                                    );
                    }

                    // wirft eine XMLException bei Parse-Fehler / Semantik-Fehlern
                    if (group_spec[1] == 0 and group_spec[0]->isVariable())
                    {
                            char * spec = strdup_alloc(group_spec[0]->getVarName());
                            int spec_type = data::Notation::check_spec(spec);

                            // keine echte generische Messgruppe
                            switch (spec_type)
                            {
                                case -1:
                                        fTHROW(XMLException,child,
                                                "error parsing measurement specification: \"%s\"",
                                                spec);
                                        	break;
                                case 1: // MS
                                        mg = parseGroupSpecMS(child,spec);
                                        break;
                                case 2: // MSMS
                                        mg = parseGroupSpecMSMS(child,spec);
                                        break;
                                case 3: // 1HNMR
                                        mg = parseGroupSpec1HNMR(child,spec);
                                        break;
                                case 4: // 13CNMR
                                        mg = parseGroupSpec13CNMR(child,spec);
                                        break;
                                case 6: // MI-MS für multi-isotopic Tracer                        
                                        mg = parseGroupSpecMIMS(child,spec);
                                        break;
                                case 5: // generic
                                default:
                                        mg = MGroupGeneric::parseSpec(group_spec,&pstate);
                                        break;
                            }
                            delete[] spec;
                            if (spec_type <= 4 || spec_type==6)
                            {
                                    delete group_spec[0];
                                    delete[] group_spec;
                            }
                    }
                    else
                    {
                            // eine "echte" generische Messgruppe
                            mg = MGroupGeneric::parseSpec(group_spec,&pstate);
                    }

                    U2A asc_id(idAttr->getValue());

                    mg->setGroupId(asc_id);

                    parseTimesAttr(group,mg,timesAttr);

                    if (parseGroupScaleAttributeIsAuto(group))
                            mg->setScaleAuto();
                    else
                            mg->setUnscaled();      
                }

                // zum nächsten Element-Knoten vorrücken
                child = XMLElement::nextNode(child);
            }
            
            if (error_model != 0)
            {
                    // es gibt entweder nur eine Formel für alle
                    // Messwerte -- oder für jeden Messwert eine
                    // Formel:
                    int c = 0;
                    while (error_model[c]) ++c;

                    if (c != 1 and c != int(mg->getDim()))
                    {
                            fTHROW(XMLException,group,
                                    "invalid number of error model formulas "
                                    "(found %i, expected either 1 or %i)",
                                    c, int(mg->getDim()));
                    }
                    mg->setErrorModel(error_model);

                    // Speicher freigeben
                    for (c=0; error_model[c]; ++c)
                            delete error_model[c];
                    delete[] error_model;
            }
            
            doc_->registerGroup(group,mg);
    }

    ExprTree ** MMModel::parseTextual(DOMNode * textual, bool mgroup_spec)
    {
            std::list< ExprTree * > formula_list;
            DOMNode * child;
            size_t rowlimit = 256; // maximal 256 Formeln erlaubt

            if (not XMLElement::match(textual,mm_textual))
                    fTHROW(XMLException,textual,"element node (textual) expected.");

            child = textual->getFirstChild();
            while (child != 0)
            {
                    // unter Element textual ist alles erlaubt -- ausgewertet
                    // werden nur TEXT_NODEs
                    while (child != 0 && child->getNodeType() != DOMNode::TEXT_NODE)
                            child = child->getNextSibling();

                    if (child == 0)
                            break;

                    U2A cblock(static_cast< DOMText* >(child)->getData());
                    charptr_array expr_list = charptr_array::split((char const*)cblock,";");
                    charptr_array::const_iterator i;

                    for (i=expr_list.begin(); i!=expr_list.end(); i++)
                    {
                            char const * w;
                            for (w=*i; *w!='\0'; w++)
                                    if (*w > 32) break;
                            if (*w == '\0')
                                    continue;

                            try
                            {
                                    ExprTree * expr = ExprTree::parse(*i,
                                            mgroup_spec ? et_lex_mm : 0);
                                    if (formula_list.size() <= rowlimit)
                                            formula_list.push_back(expr);
                                    else
                                    {
                                            delete expr;
                                            fTHROW(XMLException,child,
                                                    "exceeded maximum number of rows (%i) in textual formula",
                                                    int(rowlimit)
                                                    );
                                    }
                            }
                            catch (ExprParserException &)
                            {
                                    fTHROW(XMLException,child,
                                            "parse error in formula (%s)\n",
                                            *i
                                            );
                            }
                    }
                    child = child->getNextSibling();
            }

            ExprTree ** lst = new ExprTree*[formula_list.size()+1];
            std::list< ExprTree* >::iterator fi;
            size_t k;
            for (fi=formula_list.begin(),k=0; fi!=formula_list.end(); fi++,k++)
                    lst[k] = *fi;
            lst[k] = 0;
            return lst;
    }

    ExprTree ** MMModel::parseMath(DOMNode * math)
    {
            size_t rowlimit = 256; // maximal 256 Formeln erlaubt
            MathMLDocument * mathml;
            MathMLDeclare const * mathml_def;
            MathMLContentObject const * mathml_cont;
            MathMLVector const * mathml_vec;
            MathMLExpression const * mathml_expr;

            // den MathML-Parser anwerfen:
            try
            {
                    mathml = new MathMLDocument(math);
            }
            catch (XMLException & e)
            {
                    fTHROW(XMLException,math,"MathML-error: %s", (char const*)e);
            }

            // Parsen abgeschlossen. Klassifikation der Inhalte
            std::list< MathMLDeclare const * > mml_defs
                    = mathml->getDefinitionsByRegExp(".*");

            if (mml_defs.size() > 1)
            {
                    fTHROW(XMLException,math,
                            "expected exactly one MathML definition; found %i",
                            int(mml_defs.size()));
            }

            // Sinnüberprüfung der Constraints
            mathml_def = *(mml_defs.begin());
            mathml_cont = mathml_def->getValue();

            // Constraints sind einfache MathMLExpressions:
            if (mathml_cont->getType() != MathMLContentObject::co_vector
                    and mathml_cont->getType() != MathMLContentObject::co_expression)
            {
                    fTHROW(XMLException,math,
                            "MathML expression or vector expression expected");
            }

            ExprTree ** lst;

            if (mathml_cont->getType() == MathMLContentObject::co_vector)
            {
                    mathml_vec = static_cast< MathMLVector const * >(mathml_cont);

                    if (mathml_vec->dim() > int(rowlimit))
                    {
                            fTHROW(XMLException,math,
                                    "exceeded maximum number of rows (%i) in MathML formula",
                                    int(rowlimit)
                                    );
                    }
                    lst = new ExprTree*[mathml_vec->dim()+1];
                    size_t k;
                    for (k=0; k<size_t(mathml_vec->dim()); k++)
                            lst[k] = mathml_vec->get(k)->clone();
                    lst[k] = 0;
            }
            else
            {
                    mathml_expr = static_cast< MathMLExpression const * >(mathml_cont);
                    lst = new ExprTree*[2];
                    lst[0] = mathml_expr->get();
                    lst[1] = 0;
            }

            // MathML-Dokument freigeben:
            delete mathml;
            
            return lst;
    }

    MGroup * MMModel::parseGroupSpecMS(DOMNode * group, char const * spec)
    {
            int pstate;
            MGroup * mg = MGroupMS::parseSpec(spec,&pstate);
            if (mg == 0)
            {
                    switch (pstate)
                    {
                    case 1:
                            fTHROW(XMLException,
                                    group,
                                    "parse error in MS spec: [%s]",
                                    spec
                                    );
                            break;
                    case 2: fTHROW(XMLException,
                                    group,
                                    "invalid range in MS spec: [%s]",
                                    spec
                                    );
                    		break;
                    case 3: fTHROW(XMLException,
                                    group,
                                    "overlapping ranges in MS spec: [%s]",
                                    spec
                                    );
                    		break;
                    case 4: fTHROW(XMLException,
                                    group,
                                    "fewer positions than labelings in MS spec: [%s]",
                                    spec
                                    );
            						break;
                    case 5: fTHROW(XMLException,
                                    group,
                                    "invalid weight in MS spec: [%s]",
                                    spec
                                    );
            						break;
                    default: fTHROW(XMLException,
                                    group,
                                    "unknown error in MS spec: [%s]",
                                    spec
                                    );
                    }
            }
            return mg;
    }

    MGroup * MMModel::parseGroupSpecMIMS(DOMNode * group, char const * spec)
    {
            int pstate;
            MGroup * mg = MGroupMIMS::parseSpec(spec,&pstate);
            
            if (mg == 0)
            {
                    switch (pstate)
                    {
                    case 1:
                            fTHROW(XMLException,
                                    group,
                                    "parse error in Multi-Isotopic Tracer MS spec: [%s]",
                                    spec
                                    );
                            		break;
                    case 2: fTHROW(XMLException,
                                    group,
                                    "invalid range in Multi-Isotopic Tracer MS spec: [%s]",
                                    spec
                                    );
                    				break;
                    case 3: fTHROW(XMLException,
                                    group,
                                    "overlapping ranges in Multi-Isotopic Tracer MS spec: [%s]",
                                    spec
                                    );
                    				break;
                    case 4: fTHROW(XMLException,
                                    group,
                                    "fewer positions than labelings in Multi-Isotopic Tracer MS spec: [%s]",
                                    spec
                                    );
                    				break;
                    case 5: fTHROW(XMLException,
                                    group,
                                    "invalid weight in Multi-Isotopic Tracer MS spec: [%s]",
                                    spec
                                    );
                    				break;
                    default: fTHROW(XMLException,
                                    group,
                                    "unknown error in Multi-Isotopic Tracer MS spec: [%s]",
                                    spec
                                    );
                    }
            }
            return mg;
    }
    
    MGroup * MMModel::parseGroupSpecMSMS(DOMNode * group, char const * spec)
    {
            int pstate;
            MGroup * mg = MGroupMSMS::parseSpec(spec,&pstate);
            if (mg == 0)
            {
                    switch (pstate)
                    {
                    case 1:
                            fTHROW(XMLException,
                                    group,
                                    "parse error in MSMS spec: [%s]",
                                    spec
                                    );
                            break;
                    case 2: fTHROW(XMLException,
                                    group,
                                    "invalid range in MSMS spec: [%s]",
                                    spec
                                    );
                    		break;
                    case 3: fTHROW(XMLException,
                                    group,
                                    "overlapping ranges in MSMS spec: [%s]",
                                    spec
                                    );
                    		break;
                    case 4: fTHROW(XMLException,
                                    group,
                                    "fewer positions than labelings in MSMS spec: [%s]",
                                    spec
                                    );
                    		break;
                    case 5: fTHROW(XMLException,
                                    group,
                                    "invalid weight in MSMS spec: [%s]",
                                    spec
                                    );
                    		break;
                    default: fTHROW(XMLException,
                                    group,
                                    "unknown error in MSMS spec: [%s]",
                                    spec
                                    );
                    }
            }
            return mg;
    }

    MGroup * MMModel::parseGroupSpec1HNMR(DOMNode * group, char const * spec)
    {
            int pstate;
            MGroup * mg = MGroup1HNMR::parseSpec(spec,&pstate);
            if (mg == 0)
            {
                    switch (pstate)
                    {
                    case 1:	fTHROW(XMLException,
                                    group,
                                    "parse error in 1H-NMR spec: [%s]",
                                    spec
                                    );
                    		break;
                    case 2: fTHROW(XMLException,
                                    group,
                                    "invalid position list in 1H-NMR spec: [%s]",
                                    spec
                                    );
                    		break;
                    case 3: fTHROW(XMLException,
                                    group,
                                    "a position appears more than once in 1H-NMR spec: [%s]",
                                    spec
                                    );
                    		break;
                    default: fTHROW(XMLException,
                                    group,
                                    "unknown error in 1H-NMR spec: [%s]",
                                    spec
                                    );
                    }
            }
            return mg;
    }

    MGroup * MMModel::parseGroupSpec13CNMR(DOMNode * group, char const * spec)
    {
            int pstate;
            MGroup * mg = MGroup13CNMR::parseSpec(spec,&pstate);
            if (mg == 0)
            {
                    switch (pstate)
                    {
                    case 1:
                            fTHROW(XMLException,
                                    group,
                                    "parse error in 13C-NMR spec: [%s]",
                                    spec
                                    );
                            break;
                    case 2: fTHROW(XMLException,
                                    group,
                                    "invalid position list in 13C-NMR spec: [%s]",
                                    spec
                                    );
                    		break;
                    case 3: fTHROW(XMLException,
                                    group,
                                    "a position appears more than once in 13C-NMR spec: [%s]",
                                    spec
                                    );
                    		break;
                    default: fTHROW(XMLException,
                                    group,
                                    "unknown error in 13C-NMR spec: [%s]",
                                    spec
                                    );
                    }
            }
            return mg;
    }

    ExprTree ** MMModel::parseErrorModel(DOMNode * errormodel)
    {
            DOMNode * child;
            ExprTree ** model_spec = 0;
            ExprTree ** E;

            child = XMLElement::skipJunkNodes(errormodel->getFirstChild());

            // MathML und Text-Einträge parsen:
            if (XMLElement::match(child,mm_textual))
                    model_spec = parseTextual(child,false);
            else if (XMLElement::match(child,mm_math))
                    model_spec = parseMath(child);
            else
            {
                    fTHROW(XMLException,child,
                            "error model spec in textual- or MathML notation expected");
            }

            if (model_spec == 0)
            {
                    fTHROW(XMLException,child,
                            "non-empty error model expected");
            }

            // Gültigkeit des Modells prüfen. Es dürfen nur die Variablen
            // meas_real, std_real, meas_sim auftreten
            for (E = model_spec; *E; ++E)
            {
                    charptr_array::const_iterator vi;
                    charptr_array vnames = (*E)->getVarNames();

                    for (vi=vnames.begin(); vi!=vnames.end(); ++vi)
                    {
                            if (strcmp(*vi, "std_real") == 0 or
                                    strcmp(*vi, "meas_real") == 0 or
                                    strcmp(*vi, "meas_sim") == 0) continue;
                            else
                            {
                                    fTHROW(XMLException,child,
                                            "illegal variable name in errormodel: %s",
                                            *vi);
                            }
                    }
            }

            return model_spec;
    }

            void MMModel::parsePoolMeasurement(DOMNode * poolmeasurement)
            {
                ExprTree * expr;
                DOMNamedNodeMap * nnm;
                DOMNode * child, * childchild;
                DOMAttr * idAttr;
                MGroup * mg;

                child = XMLElement::skipJunkNodes(poolmeasurement->getFirstChild());

                while (child != 0)
                {
                        if (XMLElement::match(child, mm_poolsize, mm_xmlns_uri))
                        {
                                nnm = child->getAttributes();
                                if (nnm == 0)
                                        fTHROW(XMLException,
                                                child,
                                                "element poolsize lacks attributes (id,pools)"
                                                );

                                idAttr = static_cast< DOMAttr* >(nnm->getNamedItem(mm_id));

                                if (idAttr == 0)
                                        fTHROW(XMLException,
                                                child,
                                                "element poolsize lacks attribute \"id\""
                                                );
                                // SA: Poolgrößen sind konstant (zeitunabhängig)
                                if (nnm->getNamedItem(mm_times) != 0)
                                        fTHROW(XMLException,child,
                                                "attribute \"times\" not allowed for poolsize-measurements");

                                U2A asc_id(idAttr->getValue());

                                childchild = XMLElement::skipJunkNodes(child->getFirstChild());
                                if (XMLElement::match(childchild,fml_math,fml_xmlns_mathml_uri))
                                        expr = parsePoolMeasurementMath(childchild);
                                else if (XMLElement::match(childchild,fml_textual,fml_xmlns_uri))
                                        expr = parsePoolMeasurementTextual(childchild);
                                else
                                        fTHROW(XMLException,childchild,
                                                "formula in textual- or MathML notation expected");

                                mg = MGroupPool::parseSpec(expr->toString().c_str());
                                delete expr;
                                mg->setGroupId((char const*)asc_id);
                                doc_->registerGroup(child,mg);
                        }
                        else
                        {
                                U2A asc_child(XMLElement::getName(child,mm_xmlns_uri));
                                fTHROW(XMLException,child,"unexpected node \"%s\" found",
                                        (char const *)asc_child);
                        }
                        child = XMLElement::nextNode(child);
                }
            } 

            ExprTree * MMModel::parsePoolMeasurementTextual(DOMNode * textual)
            {
                ExprTree * expr;
                DOMNode * child;

                if (not XMLElement::match(textual,mm_textual))
                        fTHROW(XMLException,"element node (textual) expected.");

                child = textual->getFirstChild();
                // unter Element textual ist alles erlaubt -- ausgewertet
                // werden nur TEXT_NODEs
                while (child != 0 && child->getNodeType() != DOMNode::TEXT_NODE)
                        child = child->getNextSibling();

                if (child == 0)
                        fTHROW(XMLException,textual,"missing poolsize-measurement formula");

                U2A cblock(static_cast< DOMText* >(child)->getData());
                try
                {
                        expr = ExprTree::parse((char const *)cblock);
                }
                catch (ExprParserException &)
                {
                        fTHROW(XMLException,child,
                                "parse error in poolsize-measurement formula (%s)",
                                (char const *)cblock);
                }
                return expr;
            }

             ExprTree * MMModel::parsePoolMeasurementMath(DOMNode * math)
             {
                ExprTree * expr;
                MathMLDocument * mathml;
                MathMLDeclare const * mathml_def;
                MathMLContentObject const * mathml_cont;
                MathMLExpression const * mathml_expr;

                // den MathML-Parser anwerfen:
                try
                {
                        mathml = new MathMLDocument(math);
                }
                catch (XMLException & e)
                {
                        fTHROW(XMLException,math,
                                "MathML-error in poolsize-measurement spec: %s", (char const*)e);
                }

                // Parsen abgeschlossen. Klassifikation der Inhalte
                std::list< MathMLDeclare const * > mml_defs
                        = mathml->getDefinitionsByRegExp(".*");

                if (mml_defs.size() > 1)
                        fTHROW(XMLException,math,
                                "more than one MathML definition found in poolsize-measurement spec");

                // Sinn�berpr�fung der Constraints
                mathml_def = *(mml_defs.begin());
                mathml_cont = mathml_def->getValue();

                // Constraints sind einfache MathMLExpressions:
                if (mathml_cont->getType() != MathMLContentObject::co_expression)
                        fTHROW(XMLException,math,"MathML expression expected");

                mathml_expr = static_cast< MathMLExpression const * >(mathml_cont);
                expr = mathml_expr->get()->clone();

                // MathML-Dokument freigeben:
                delete mathml;

                return expr;
           }
    } // namespace flux::xml
    } // namespace flux

