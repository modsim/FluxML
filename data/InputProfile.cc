

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
#include <limits>

using namespace flux::symb;

namespace flux {
namespace data {

    
double InputProfile::eval(double t, bool* status)
{
    std::list<double>::iterator ic;
    std::list<symb::ExprTree>::const_iterator iv= values_.begin();
    symb::ExprTree * ts = symb::ExprTree::val(t);
    double val=0.;
    /** Ausnahmefall: profile nicht explizit definiert (d.h Bedingungsliste
     *  enthält nur 0)
     **/
    if(conditions_.size()==1)
    {
        ExprTree * v_tmp = iv->clone();
        v_tmp->subst("t", ts);
        v_tmp->eval(true);
        val = v_tmp->getDoubleValue();
        *status= true;
//            fWARNING("eval: t=%.6f \t--> val: %.3f", t, val);
        delete ts;
        delete v_tmp;
        return val;
    }

    double act_val=0., next_val=0.;
    bool fulfilled= false;

    for(ic = conditions_.begin();iv!=values_.end();++iv)
    {
        ExprTree * v_tmp = iv->clone();
        act_val = *ic;
        ++ic;
        if(ic!=conditions_.end())
            next_val = *ic;
        else
            next_val = std::numeric_limits<double>::infinity(); // INF

        if(t >= act_val and t < next_val)
        {
            v_tmp->subst("t", ts);
            v_tmp->eval(true);
            val = v_tmp->getDoubleValue();
            fulfilled= true;
//                fWARNING("eval: %.2f <= t=%.6f < %.2f \t--> val: %.3f", act_val,t,next_val, val);
            delete v_tmp;
            break;
        }
        delete v_tmp;
    }

    *status= fulfilled;
    delete ts;
    return val;
}


} // namespace flux::data
} // namespace flux

