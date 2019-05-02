#ifndef STAT_H_FLUX
#define STAT_H_FLUX

extern "C"
{
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
}

#define FILE_ERR_NON_EXISTENT		1
#define FILE_ERR_PERMISSION_DENIED	2
#define FILE_ERR_NON_DIRECTORY		4
#define FILE_ERR_NAME_TOO_LONG		8
#define FILE_ERR_OUT_OF_MEMORY		16
#define FILE_ERR_BAD_FILE_DESC		32
#define FILE_ERR_BAD_ADDRESS		64

/**
 * Ein Wrapper für stat(2) für saubere Dateioperationen unter Unix.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class Stat
{
private:
	/** Resultat des stat()-Aufrufs */
	struct stat statbuf_;
	/** Fehler-Codes von stat() */
	int error_;
	/** effektive User-ID des laufenden Prozesses */
	uid_t uid_;
	/** effektive Group-ID des laufenden Prozesses */
	gid_t gid_;

public:
	/**
	 * Constructor.
	 *
	 * @param fn Dateiname
	 */
	Stat(char const * fn);
public:
	/**
	 * Prüft auf das Vorhandensein der Datei
	 *
	 * @return true, falls die Datei existiert.
	 */
	bool exists() const;
	
	/**
	 * Gibt den Fehlerstatus des stat()-Aufrufs zurück. Wert 0 steht für
	 * keinen Fehler. Alle weiteren Fehler entsprechen den oben definierten
	 * Bits.
	 *
	 * @return 0, falls kein Fehler bei stat() aufgetreten ist
	 */
	int getError() const;
	
	/**
	 * Prüft auf Ausführbarkeit der Datei
	 *
	 * @return true, falls die Datei ausführbar ist.
	 */
	bool isExecutable() const;
	
	/**
	 * Prüft auf Lesbarkeit der Datei
	 *
	 * @return true, falls die Datei lesbar ist.
	 */
	bool isReadable() const;
	
	/**
	 * Prüft auf Schreibbarkeit der Datei
	 *
	 * @return true, falls die Datei schreibbar ist.
	 */
	bool isWritable() const;
	
	/**
	 * Prüft, ob es sich bei der Datei um einen Symlink handelt
	 *
	 * @return true, falls die Datei ein Symlink ist
	 */
	bool isSymLink() const;
	
	/**
	 * Prüft, ob es sich bei der Datei um eine reguläre Datei handelt.
	 * 
	 * @return true, falls es sich um eine reguläre Datei handelt
	 */
	bool isRegularFile() const;
	
	/**
	 * Prüft, ob es sich bei dem Dateinamen um ein Verzeichnis handelt.
	 * 
	 * @return true, falls es sich um ein Verzeichnis handelt
	 */
	bool isDirectory() const;

	/**
	 * Prüft, ob es sich bei dem Dateinamen um einen Socket handelt.
	 * 
	 * @return true, falls es sich um einen Socket
	 */
	bool isSocket() const;
	
	/**
	 * Gibt die Benutzer-ID der Datei zurück.
	 *
	 * @return Benutzer-ID der Datei
	 */
	inline uid_t getUID() const
	{
		return error_==0 ? statbuf_.st_uid : 0;
	}
	
	/**
	 * Gibt die Gruppen-ID der Datei zurück.
	 *
	 * @return Gruppen-ID der Datei
	 */
	inline gid_t getGID() const
	{
		return error_==0 ? statbuf_.st_gid : 0;
	}
	
	/**
	 * Gibt die codierten Zugriffsrechte der Datei zurück (z.B. 0644).
	 *
	 * @return Zugriffsrechte der Datei
	 */
	uint16_t getPermissions() const
	{
		return error_==0 ? statbuf_.st_mode : 0;
	}

	/**
	 * Gibt die Größe der Datei in Bytes zurück
	 *
	 * @return die Größe der Datei in Bytes
	 */
	uint64_t getSize() const
	{
		return error_==0 ? uint64_t(statbuf_.st_size) : 0;
	}

	/**
	 * Gibt den Zeitpunkt des letzten Zugriffs zurück
	 *
	 * @return Zeitpunkt des letzten Zugriffs
	 */
	time_t getATime() const
	{
		return error_==0 ? statbuf_.st_atime : 0;
	}

	/**
	 * Gibt den Zeitpunkt der letzten Änderung zurück
	 *
	 * @return Zeitpunkt der letzten Änderung
	 */
	time_t getMTime() const
	{
		return error_==0 ? statbuf_.st_mtime : 0;
	}

	/**
	 * Gibt den Zeitpunkt der letzten Statusänderung zurück.
	 *
	 * @return Zeitpunkt der letzten Statusänderung
	 */
	time_t getCTime() const
	{
		return error_==0 ? statbuf_.st_ctime : 0;
	}

};

#endif

