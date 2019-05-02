#ifndef FLUX_H
#define FLUX_H

#include "Combinations.h"

namespace flux {
namespace data {

/**
 * Umrechnung zwischen Fluss-Koordinatensystemen.
 * Rechenregeln:
 *
 * Forward/Backward => net/xch
 *   net = fwd - bwd;
 *   xch = min(fwd,bwd);
 * net/xch01 => net/xch
 *   net = net
 *   xch = xch01 / (1 - xch01)
 * net/xch => net/xch01
 *   net = net
 *   xch01 = xch / (1 + xch)
 * net/xch => Forward/Backward
 *   fwd = xch + max(net,0)
 *   bwd = xch + max(-net,0)
 *
 * @author Michael Weitzel <info@13cflux.net>
 */

inline void fwd_bwd_2_net_xch(
	double & net,
	double & xch,
	double const & fwd,
	double const & bwd
	)
{
	net = fwd - bwd;
	xch = MIN2(fwd,bwd);
}

inline void net_xch_2_fwd_bwd(
	double & fwd,
	double & bwd,
	double const & net,
	double const & xch
	)
{
	fwd = xch + MAX2(net,0.);
	bwd = xch + MAX2(-net,0.);
}

inline double xch_2_xch01(double const & xch)
{
	return xch / (1. + xch);
}

inline double xch01_2_xch(double const & xch01)
{
	return xch01 / (1. - xch01);
}

inline void net_xch01_2_fwd_bwd(
	double & fwd,
	double & bwd,
	double const & net,
	double const & xch
	)
{
	net_xch_2_fwd_bwd(fwd,bwd,net,xch_2_xch01(xch));
}

inline void fwd_bwd_2_net_xch01(
	double & net,
	double & xch01,
	double const & fwd,
	double const & bwd
	)
{
	fwd_bwd_2_net_xch(net,xch01,fwd,bwd);
	xch01 = xch_2_xch01(xch01);
}

} // namespace flux::data
} // namespace flux

#endif

