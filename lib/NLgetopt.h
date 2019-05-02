#ifndef NLGETOPT_H
#define NLGETOPT_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

#define getopt_long_newlib getopt_long

#ifdef __cplusplus
}
#endif

#endif

