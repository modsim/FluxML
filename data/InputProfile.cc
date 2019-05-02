

#include "Combinations.h"
#include <cstddef>
#include "Error.h"
#include <string>
#include <list>
#include "LinearExpression.h"
#include <cstring>
#include <stdlib.h>
#include "cstringtools.h"
#include "InputProfile.h"
#include "ExprTree.h"

using namespace flux::symb;

namespace flux {
namespace data {

    
double InputProfile::eval(double t, bool* status)
{
        double val=0.;
        symb::ExprTree * ts = symb::ExprTree::val(t);
        std::list<symb::ExprTree>::const_iterator ic, iv;
        bool otherwise= false;        
        for(ic=conditions_.begin(),iv=values_.begin(); ic!=conditions_.end() || iv!=values_.end();++ic, ++iv)
        {
            ExprTree * c_tmp = ic->clone();
            ExprTree * v_tmp = iv->clone();
            if(ic->isLeaf())
            {
                v_tmp->subst("t", ts);
                v_tmp->eval(true);
                val = v_tmp->getDoubleValue();
                otherwise= true;
            }
            else
            {
                bool fulfilled= true;
                if((c_tmp->Lval()->isLeaf() or c_tmp->Lval()->isUnaryOp()) 
                    and (c_tmp->Rval()->isLeaf() or c_tmp->Rval()->isUnaryOp()))
                {
                    c_tmp->subst("t", ts);
                    c_tmp->eval(true);
                    if(c_tmp->getDoubleValue()<1)
                        fulfilled = false;
                }
                else
                {
                    ExprTree * l_c_tmp, * r_c_tmp;
                    l_c_tmp= c_tmp->Lval();
                    r_c_tmp= c_tmp->Rval();
                    if(l_c_tmp->isLeaf() or l_c_tmp->isUnaryOp())
                    {
                        r_c_tmp->subst("t", ts);
                        r_c_tmp->eval(true);
                        
                        if(r_c_tmp->getDoubleValue()<1)
                            fulfilled = false;
                        else
                        {
                            c_tmp->Rval(ExprTree::sym("t"));
                            c_tmp->subst("t", ts);
                            c_tmp->eval(true);
                            if(c_tmp->getDoubleValue()<1)
                                fulfilled= false;
                        }
                    }   
                    else if(r_c_tmp->isLeaf() or r_c_tmp->isUnaryOp())
                    {
                        l_c_tmp->subst("t", ts);
                        l_c_tmp->eval(true);
                        
                        if(l_c_tmp->getDoubleValue()<1)
                            fulfilled= false;
                        else
                        {
                            // Ersetze den linken Ausdruck durch variable t
                            c_tmp->Lval(ExprTree::sym("t"));
                            c_tmp->subst("t", ts);
                            c_tmp->eval(true);
                            if(c_tmp->getDoubleValue()<1)
                                fulfilled= false;
                        }
                    }
                    else
                    {
                        fERROR("Unsupported profile specification (%s)", ic->toString().c_str());
                        exit(EXIT_FAILURE);
                    }
                }
                if(fulfilled)
                {
                    v_tmp->subst("t", ts);
                    v_tmp->eval(true);
                    val = v_tmp->getDoubleValue();   
                    *status = true;
                    delete c_tmp;
                    delete v_tmp;
                    delete ts;
                    return val;
                }
            }
            delete c_tmp;
            delete v_tmp;
        }
        delete ts;
        *status = otherwise;
        return val;
}


} // namespace flux::data
} // namespace flux

