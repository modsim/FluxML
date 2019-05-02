#ifndef READSTREAM_H
#define READSTREAM_H

#include <cstdio>
#include <cstddef>

/**
 * Lesen eines geöffneten Streams in einen allokierten Puffer. Übergeben wird
 * ein FILE-Stream, von dem gelesen wird. Wird size=0 übergeben, wird der
 * komplette Stream eingelesen. Nach dem Lesen enthält size die Anzahl der,
 * gelesenen Bytes / die Größe des allokierten Puffers. Der zurückgegebene
 * Puffer wird terminiert. Falls ein Fehler aufgetreten war, gibt die Funktion
 * NULL zurück.
 *
 * @param stream geöffneter Stream
 * @param size maximale Speichergröße, die eingelesen wird; 0 für unendlich
 * @return Zeiger auf den allokierten Speicher mit dem Stream-Inhalt;
 * 	0 bei Fehler
 */
char * readstream(FILE * stream, size_t & size);

/**
 * Kopiert eine Datei zwischen Verzeichnissen.
 * Schnittstelle wie symlink(2).
 *
 * @param oldpath Pfad und Name der Quell-Datei
 * @param newpath Pfad und Name der Ziel-Datei
 * @return 0 bei Erfolg, -1 bei Fehler (errno wird gesetzt)
 */
int copyfile(char const * oldpath, char const * newpath);

/**
 * Stdout nach Stderr umlenken.
 *
 * @return Filedescriptor für altes stdout
 */
int redirect_stdout(char const * dest = 0);

/**
 * Stdout wiederherstellen.
 *
 * @param old_stdout Filedescriptor für altes stdout
 */
void restore_stdout(int old_stdout);

#endif
