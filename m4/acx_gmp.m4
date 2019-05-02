dnl @synopsis ACX_GMP([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This library looks for the GMP library and introduces additional
dnl configuration switches and reads/modifies the following env. variables:
dnl
dnl   $(GMP_CPPFLAGS)  -- include paths; -I...
dnl   $(GMP_LIBS)      -- library; -lgmp -lgmpxx
dnl   $(GMP_LDFLAGS)   -- linker flags; -L/path/...
dnl
dnl The user may also use the following command line switches:
dnl   --with-gmp-incdir=DIR    -- include path for GMP (without "-I")
dnl   --with-gmp-libdir=DIR    -- library path for GMP (without "-L")
dnl   --with-gmp-link="-lgmp -lgmpxx"   -- link instruction
dnl
dnl @category InstalledPackages
dnl @author Michael Weitzel 
dnl @version 2009-09-16
dnl @license GPLWithACException

AC_DEFUN([ACX_GMP],
[
	GMP_LIBS="-lgmp -lgmpxx"

	AC_ARG_WITH([gmp-incdir],
		AC_HELP_STRING([--with-gmp-incdir=DIR],
			[search GMP library header files in DIR]),
		[GMP_CPPFLAGS=-I${withval}])
	AC_ARG_WITH([gmp-libdir],
		AC_HELP_STRING([--with-gmp-libdir=DIR],
			[search GMP library in DIR]),
		[GMP_LDFLAGS=-L${withval}])
	AC_ARG_WITH([gmp-link],
		AC_HELP_STRING([--with-gmp-link],
			[tell how to link against the GMP library; default: -lgmp -lgmpxx]),
		[GMP_LIBS=${withval}])
	AC_ARG_VAR([GMP_LIBS],[The list of arguments required to link against the GMP library])
	AC_ARG_VAR([GMP_LDFLAGS],[extra LDFLAGS required for the GMP library])
	AC_ARG_VAR([GMP_CPPFLAGS],[extra CPPFLAGS required for the GMP header files])
	
	AC_LANG_PUSH([C++])

	dnl GMP header files
	AC_MSG_CHECKING([for GMP library header files])
	ac_save_CPPFLAGS=${CPPFLAGS}
	if test -n "${GMP_CPPFLAGS}"; then
		CPPFLAGS="${CPPFLAGS} ${GMP_CPPFLAGS}"
	fi
	ac_gmp_headers=no
	AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <gmpxx.h>]])],[ac_gmp_headers=yes],[AC_MSG_ERROR([not found])])
	AC_MSG_RESULT([$ac_gmp_headers])

	AC_MSG_CHECKING(for recent GMP)
	AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
	#include "gmp.h"
	#if (__GNU_MP_VERSION*100+__GNU_MP_VERSION_MINOR*10 < 410)
	# error "min GMP version is 4.1.0"
	error
	#endif
	]])],[AC_MSG_RESULT(yes)],[
	AC_MSG_RESULT(no)
	AC_MSG_ERROR([GMP 4.1.0 min required])
	])

	dnl GMP library
	ac_gmp_libs=no
	AC_MSG_CHECKING([for GMP C++ library])

	ac_save_LIBS=${LIBS}
	ac_save_LDFLAGS=${LDFLAGS}
	ac_save_CFLAGS=${CFLAGS}
	LIBS="${LIBS} ${GMP_LIBS}"
	LDFLAGS="${LDFLAGS} ${GMP_LDFLAGS}"
	CFLAGS="${CFLAGS} ${GMP_CPPFLAGS}"

	AC_LINK_IFELSE(
		[AC_LANG_PROGRAM([#include <gmpxx.h>],[mpq_class a;])],
		[ac_gmp_libs=yes],[AC_MSG_ERROR([linking failed])])
	AC_MSG_RESULT([$ac_gmp_libs])
	
	AC_LANG_POP([C++])
	
	LIBS=${ac_save_LIBS}
	LDFLAGS=${ac_save_LDFLAGS}
	CFLAGS=${ac_save_CFLAGS}
	CPPFLAGS=${ac_save_CPPFLAGS}

	AC_SUBST([GMP_LIBS])
	AC_SUBST([GMP_CPPFLAGS])
	AC_SUBST([GMP_LDFLAGS])
])

