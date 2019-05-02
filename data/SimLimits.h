#ifndef SIMLIMITS_H
#define SIMLIMITS_H

/*
 * Maximale Anzahl von Atomen.
 */
#define LIMIT_MAX_ATOMS		1024

/*
 * Maximale Anzahl aktiver Atome.
 * 16 => bis zu 2^16=65536 aktive Cumomere in einem Pool
 */
#define LIMIT_MAX_ACTIVE_ATOMS	16

/*
 * Dimensionslimit für Kaskadenebene
 */
#define LIMIT_MAX_LEVEL_DIM	(1<<16)

#endif

