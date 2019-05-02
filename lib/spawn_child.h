#ifndef SPAWNCHILD_H
#define SPAWNCHILD_H

#ifdef __cplusplus
#include <cstdio>
extern "C"
{
#else
#include <stdio.h>
#endif

/**
 * Startet einen Tochterprozess über das Kommando cmd und gibt
 * file-Descriptoren auf stdin, stdout und stderr des Tocherprozesses
 * zurück. Damit ist es möglich, dass ein aufrufendes Programm direkt
 * mit dem aufgerufenen Programm kommuniziert.
 *
 * @param cmd aufzurufendes Kommando
 * @param pipe_in stdin-Stream des aufgerufenen Prozesses (beschreibbar)
 * @param pipe_out stdout-Stream des aufgerufenen Prozesses (lesbar)
 * @param pipe_err stderr-Stream des aufgerufenen Prozesses (lesbar)
 * @retval Prozessnummer des aufgerufenen Prozesses
 */
pid_t spawn_child_fd(
	char const * cmd,
	int *pipe_in,
	int *pipe_out,
	int *pipe_err
	);

/**
 * Startet einen Tochterprozess über das Kommando cmd und gibt
 * FILE-Stream-Objekte auf stdin, stdout und stderr des Tocherprozesses
 * zurück. Damit ist es möglich, dass ein aufrufendes Programm direkt
 * mit dem aufgerufenen Programm kommuniziert. Diese Funktion ist ein
 * Wrapper für die Funktion spawn_child_fd(). Tip: es ist zweckmäßig
 * das Buffering des pipe_in-FILE-Objekts abzuschalten, um prompte
 * Reaktion des Tochterprozesses zu erhalten: <tt>setbuf(pipe_in,NULL)</tt>.
 *
 * @param cmd aufzurufendes Kommando
 * @param pipe_in stdin-Stream des aufgerufenen Prozesses (beschreibbar)
 * @param pipe_out stdout-Stream des aufgerufenen Prozesses (lesbar)
 * @param pipe_err stderr-Stream des aufgerufenen Prozesses (lesbar)
 * @retval Prozessnummer des aufgerufenen Prozesses
 */
pid_t spawn_child_stream(
	char const * cmd,
	FILE **pipe_in,
	FILE **pipe_out,
	FILE **pipe_err
	);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

