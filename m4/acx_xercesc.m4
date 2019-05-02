dnl @synopsis ACX_XERCESC([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This library looks for the Xerces-C library and introduces additional
dnl configuration switches and reads/modifies the following env. variables:
dnl
dnl   $(XERCESC_CPPFLAGS)  -- include paths; -I...
dnl   $(XERCESC_LIBS)      -- library; -lxerces-c
dnl   $(XERCESC_LDFLAGS)   -- linker flags; -L/path/...
dnl
dnl The user may also use the following command line switches:
dnl   --with-xercesc-incdir=DIR    -- include path for Xerces-C (without "-I")
dnl   --with-xercesc-libdir=DIR    -- library path for Xerces-C (without "-L")
dnl   --with-xercesc-link="-lxercesc" -- link instruction
dnl
dnl @category InstalledPackages
dnl @author Michael Weitzel <info@13cflux.net>
dnl @version 2007-06-23
dnl @license GPLWithACException

AC_DEFUN([ACX_XERCESC],
[
	AC_ARG_WITH([xercesc-incdir],
		AC_HELP_STRING([--with-xercesc-incdir=DIR],
			[search Xerces-C header files in DIR]),
		[XERCESC_CPPFLAGS="-I${withval}"])
	AC_ARG_WITH([xercesc-libdir],
		AC_HELP_STRING([--with-xercesc-libdir=DIR],
			[search Xerces-C library in DIR]),
		[XERCESC_LDFLAGS="-L${withval}"])
	AC_ARG_WITH([xercesc-link],
		AC_HELP_STRING([--with-xercesc-link],
			[tell how to link against the Xerces-C library; default: -lxerces-c]),
		[XERCESC_LIBS=${withval}])
	AC_ARG_VAR([XERCESC_LIBS],[The list of arguments required to link against the Xerces-C library])
	AC_ARG_VAR([XERCESC_LDFLAGS],[extra LDFLAGS required for the Xerces-C library])
	AC_ARG_VAR([XERCESC_CPPFLAGS],[extra CPPFLAGS required for the Xerces-C header files])

	AC_LANG_PUSH([C++])

	dnl Xerces-C header files
	AC_MSG_CHECKING([for Xerces-C header files])
	ac_save_CPPFLAGS="${CPPFLAGS}"
	if test ! -z "${XERCESC_CPPFLAGS}"; then
		dnl CPPFLAGS -> Include-Pfad; -I...
		CPPFLAGS="${CPPFLAGS} ${XERCESC_CPPFLAGS}"
	fi

	ac_xerces_headers=no
	AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM([[#include <xercesc/util/PlatformUtils.hpp>]])
		],[ac_xerces_headers=yes])
	
	AC_MSG_RESULT([$ac_xerces_headers])

	dnl Xerces-C library
	ac_xerces_libs=no
	if test "$ac_xerces_headers" = "yes"; then
		AC_MSG_CHECKING([for Xerces-C library])
		if test -z "${XERCESC_LIBS}"; then
			dnl LIBS -> Libraries; -l...
			XERCESC_LIBS="-lxerces-c"
		fi

		ac_save_LIBS="${LIBS}"
		LIBS="${LIBS} ${XERCESC_LIBS}"
		dnl LDFLAGS -> Lib-Suchpfade; -L...
		ac_save_LDFLAGS="${LDFLAGS}"
		LDFLAGS="${LDFLAGS} ${XERCESC_LDFLAGS}"
		AC_TRY_LINK([#include <xercesc/util/PlatformUtils.hpp>
#ifdef XERCES_CPP_NAMESPACE_USE
XERCES_CPP_NAMESPACE_USE
#endif
],[try { XMLPlatformUtils::Initialize(); } catch (...) { }
XMLPlatformUtils::Terminate();],[ac_xerces_libs=yes])
		LIBS=${ac_save_LIBS}
		LDFLAGS=${ac_save_LDFLAGS}
		AC_MSG_RESULT([$ac_xerces_libs])
	fi
	CPPFLAGS="${ac_save_CPPFLAGS}"

	if test "$ac_xerces_headers" = "yes" -a "$ac_xerces_libs" = "yes"; then
		AC_SUBST([XERCESC_LIBS])
		AC_SUBST([XERCESC_CPPFLAGS])
		AC_SUBST([XERCESC_LDFLAGS])
		AC_DEFINE(HAVE_XERCESC,1,[Define if you have the Xerces-C library.])
		$1
		:
	else
		$2
		:
	fi
	
	AC_LANG_POP([C++])
])

