#ifndef DYNAMICLIBRARY_H
#define DYNAMICLIBRARY_H

#include <cstdio>
#include <cstdlib>
#include "Error.h"
#include "cstringtools.h"

extern "C"
{
#ifdef P_WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include "fexpand.h"
#endif
}

/**
 * Klasse DynamicLibrary. Nachladen einer Funktion zur Laufzeit.
 * 
 * Modelliert den Zugriff auf eine Bibliotheksfunktion in einem
 * ELF Shared-Library oder einer Windows-DLL
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class DynamicLibrary
{
private:
	/** Name des Shared-Library / der DLL */
	char * lib_name_;

	/** Handle für den Zugriff auf das Shared-Library / die DLL */
#ifdef P_WIN32
	HINSTANCE
#else
	void *
#endif
		handle_;
public:
	/**
	 * Lädt ein Symbol aus einem Shared Library oder einer DLL. Wurde
	 * vorher nicht open() aufgerufen, so wird dies automatisch
	 * nachgeholt.
	 *
	 * @param sym_name Name des zu ladenden Symbols
	 * @retval Zeiger des angegebenen Typs (Funktions- oder Daten-Zeiger)
	 */
	template< typename symptr_t > symptr_t loadSymbol(char const * sym_name)
	{
		symptr_t sym;

		if (!handle_ && !open())
			return symptr_t(0);
#ifdef P_WIN32
		sym = symptr_t(GetProcAddress(handle_, sym_name));
#else
		sym = symptr_t(dlsym(handle_, sym_name));
		char *err = dlerror();
		if (err) perror(err);
#endif
		return sym;
	}

public:
	/**
	 * Constructor.
	 */
	DynamicLibrary(const char *lib_name) : handle_(0)
	{
		fASSERT(lib_name);
#ifdef P_WIN32
		lib_name_ = strdup_alloc(lib_name);
#else
		lib_name_ = fexpand(lib_name);
#endif
	}

	/**
	 * Destructor.
	 * Schließt das ggfs. geöffnete Shared Library / DLL
	 */
	~DynamicLibrary()
	{
		this->close();
#ifdef P_WIN32
		delete[] lib_name_;
#else
		free(lib_name_);
#endif
	}

public:
	/**
	 * Öffnet das Shared-Library (Aufruf ist optional)
	 *
	 * @return true, falls das Shared-Library/DLL geöffnet werden konnte
	 */
	bool open()
	{
		if (handle_) close();
#ifdef P_WIN32
		handle_ = (HINSTANCE)::LoadLibrary(lib_name_);
#else
		handle_ = dlopen(lib_name_, /*RTLD_LAZY*/RTLD_NOW|RTLD_LOCAL);
#endif
		if (!handle_)
		{
#ifdef P_WIN32
			fprintf(stderr, "Fehler beim Laden der DLL \"%s\"\n", lib_name_);
#else
			perror(dlerror());
#endif
			return false;
		}
		return true;
	}

	/**
	 * Schließt das Shared-Library.
	 * Diese Funktion wird automatisch vom Destructor aufgerufen, so daß
	 * der Aufruf optional ist.
	 *
	 * @return true, falls das Shared-Library/DLL geschlossen werden konnte
	 */
	bool close()
	{
		if (handle_)
		{
#ifdef P_WIN32
			(HINSTANCE)::FreeLibrary(handle_);
#else
			dlclose(handle_);
			char *err = dlerror();
			if (err) { perror(err); return false; }
#endif
			handle_ = 0;
			return true;
		}
		return false;
	}
};
#endif

