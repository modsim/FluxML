#include <list>
#include <xercesc/dom/DOM.hpp>
#include "Error.h"
#include "cstringtools.h"

#include "ExprTree.h"
#include "LinearExpression.h"

#include "MathML.h"
#include "MathMLDocument.h"
#include "MathMLDeclare.h"
#include "MathMLExpression.h"

#include "Constraint.h"

#include "UnicodeTools.h"
#include "XMLException.h"
#include "XMLElement.h"
#include "FluxMLUnicodeConstants.h"
#include "FluxMLContentObject.h"
#include "FluxMLConstraints.h"

// Xerces C++ Namespace einbinden
XERCES_CPP_NAMESPACE_USE

using namespace flux::symb;

namespace flux {
namespace xml {

/*
 * *****************************************************************************
 * Parst die Pools einer Netzwerkspezifikation aus einem metabolitepools-
 * Element-Knoten eines DOM-Dokuments
 * 
 * @author Michael Weitzel <info@13cflux.net> (stationary MFA)
 * @author Salah Azzouzi <info@13cflux.net> (non-stationary MFA)
 * *****************************************************************************
 */    
    
void FluxMLConstraints::parseConstraints(
	DOMNode * node
	)
{
	DOMNode * child, * childchild;
	
	if (not XMLElement::match(node,fml_constraints,fml_xmlns_uri))
		fTHROW(XMLException,node,"element node (constraints) expected.");

	// ein leeres constraints-Element (<constraints/> oder
	// <constraints></constraints>) ist erlaubt:
	child = XMLElement::skipJunkNodes(node->getFirstChild());
	while (child != 0)
	{
		childchild = XMLElement::skipJunkNodes(child->getFirstChild());

		if (XMLElement::match(child,fml_net,fml_xmlns_uri))
		{
			if (XMLElement::match(childchild,fml_math,fml_xmlns_mathml_uri))
				parseConstraintsMathML(childchild,data::NET);
			else if (XMLElement::match(childchild,fml_textual,fml_xmlns_uri))
				parseConstraintsTextual(childchild,data::NET);
			else
				fTHROW(XMLException,childchild,
					"netto constraints in textual- or MathML notation expected");
		}
		else if (XMLElement::match(child,fml_xch,fml_xmlns_uri))
		{
			if (XMLElement::match(childchild,fml_math,fml_xmlns_mathml_uri))
				parseConstraintsMathML(childchild,data::XCH);
			else if (XMLElement::match(childchild,fml_textual,fml_xmlns_uri))
				parseConstraintsTextual(childchild,data::XCH);
			else
				fTHROW(XMLException,childchild,
					"exchange constraints in textual- or MathML notation expected");
		}
                // hier beide Elemente betrachtet und die Rückwärtskompatibilität zu gewährleisten
                else if (XMLElement::match(child,fml_psize,fml_xmlns_uri) || XMLElement::match(child,fml_poolsize,fml_xmlns_uri))
		{
			if (XMLElement::match(childchild,fml_textual,fml_xmlns_uri))
				parseConstraintsTextual(childchild,data::POOL);
			else
				fTHROW(XMLException,childchild,
					"psize constraints in textual notation expected");
		}
		else
			fTHROW(XMLException,child,"\"net\" or \"xch\" or \"psize\" element expected");

		child = XMLElement::nextNode(child);
	}

	// Die weitere Prüfung der Mengen equalities_ und inequalities_ muß
	// bis zur eigentlichen Netzwerk-Lösung verzögert werden, da an dieser
	// Stelle nichts über den Aufbau des Netzwerks bekannt ist. Hier könnte
	// offensichtlicher Unsinn wie "a-b < -b+a" erkannt werden -- der
	// (Programmier-)Aufwand wäre aber zu hoch.
}

void FluxMLConstraints::parseConstraintsMathML(
	DOMNode * node,
	data::ParameterType parameter_type
	)
{
	MathMLDocument * mathml;
	MathMLDeclare const * mathml_def;
	MathMLContentObject const * mathml_cont;
	MathMLExpression const * mathml_expr;
	std::string mathml_cname;
	ExprTree const * expr;

	// den MathML-Parser anwerfen:
	try
	{
		mathml = new MathMLDocument( node );
	}
	catch (XMLException & e)
	{
		// MathML-Fehler in FluxML-Fehler konvertieren:
		fTHROW(XMLException,node,"MathML-error in constraints: %s", e.toString());
	}

	// Parsen der Constraints ist abgeschlossen. Jetzt folgt eine
	// semantische Prüfung und Klassifikation in Gleichungen und
	// Ungleichungen
	
	// handelt es sich bei den Constraints aus dem MathML-Dokument wirklich
	// um Constraints?
	std::list< MathMLDeclare const * > mml_constr = mathml->getDefinitionsByRegExp(".*");
	
	// Sinnüberprüfung der Constraints
	std::list< MathMLDeclare const * >::iterator l_iter = mml_constr.begin();
	while (l_iter != mml_constr.end())
	{
		mathml_def = *l_iter;
		mathml_cont = mathml_def->getValue();
		mathml_cname = mathml_def->getName();

		// Constraints sind einfache MathMLExpressions:
		if (mathml_cont->getType() != MathMLContentObject::co_expression)
			fTHROW(XMLException,node,"constraint %s: simple MathML-expression expected.",
					mathml_cname.c_str());

		mathml_expr = static_cast<MathMLExpression const*>(mathml_cont);
		expr = mathml_expr->get();

		if (cfg_->createConstraint(mathml_cname.c_str(), expr, parameter_type) == 0)
			fTHROW(XMLException,node,"constraint %s is not linear.",
					mathml_cname.c_str());
                l_iter++;
	}

	// MathML-Dokument freigeben:
	delete mathml;
}

void FluxMLConstraints::parseConstraintsTextual(
	DOMNode * node,
        data::ParameterType parameter_type
	)
{
	DOMNode * child;
	char const anon[] = "anonymous";
	if (not XMLElement::match(node,fml_textual,fml_xmlns_uri))
		fTHROW(XMLException,node,"element node (textual) expected.");
	child = node->getFirstChild();

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
			ExprTree * expr;
			int ci;
			char * cname, * u;
			char const * w;
			for (w=*i; *w!='\0'; w++)
				if (*w > 32) break;
			if (*w == '\0')
				continue;
			
			charptr_array nv_pair = charptr_array::split(*i,":");
			switch (nv_pair.size())
			{
			case 1:
				ci = 0;
				cname = strdup_alloc(anon);
				break;
			case 2:
				ci = 1;
				cname = strdup_alloc(nv_pair[0]);
				for (u=cname; *u!='\0'; u++)
					if (*u < 32) *u = ' ';
				strtrim_inplace(cname);
				break;
			default:
				fTHROW(XMLException,child,
					"parse error in constraint (%s): "
					"confused by too many ':'", *i);
			}
			
			try
			{
				expr = ExprTree::parse(nv_pair[ci]);
			}
			catch (ExprParserException &)
			{
				fTHROW(XMLException,child,"parse error in constraint %s(%s)",
					cname, nv_pair[ci]);
			}

			// Überprüfung der Variablennamen
			charptr_array vnames = expr->getVarNames();
			charptr_array::const_iterator vni;
			switch(parameter_type)
			{
				case data::NET:
				case data::XCH:
					for (vni = vnames.begin(); vni != vnames.end(); vni++)
					{
						if (doc_->findReaction(*vni) == 0)
							fTHROW(XMLException,child,
								"invalid reaction name \"%s\" in "
								"constraint \"%s\" (%s)",
								*vni, cname, nv_pair[ci]);
					};
					break;
				case data::POOL:
					for (vni = vnames.begin(); vni != vnames.end(); vni++)
					{
						if (doc_->getPoolMap()->findPtr(*vni) == 0)
							fTHROW(XMLException,child,
								"invalid pool name \"%s\" in "
								"constraint \"%s\" (%s)",
								*vni, cname, nv_pair[ci]);
                                                
                                                // Co-faktor-pools abfangen und Meldung ausgeben
                                                data::Pool ** poolptr;
                                                poolptr = doc_->getPoolMap()->findPtr(*vni);
                                                if ((*poolptr)->getNumAtoms() == 0)
                                                    fTHROW(XMLException,child,
                                                                    "invalid poolsize constraint \"%s\". The pool \"%s\" is isotopically not balanced and should thus be removed!!",
                                                                    cname, *vni);
					};
					break;
					
			}
                        if (cfg_->createConstraint(cname,expr,parameter_type) == 0)
				fTHROW(XMLException,child,
					"constraint \"%s\" (%s) is invalid/not linear.",
					cname, expr->toString().c_str()
					);
                        
			delete[] cname;
			delete expr;
		}
		child = child->getNextSibling();
	}        
}

} // namespace flux::xml
} // namespace flux

