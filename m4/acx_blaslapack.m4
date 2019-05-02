AC_DEFUN([ACX_BLASLAPACK],
[
	AC_ARG_WITH([blas-link],
		AC_HELP_STRING([--with-blas-link=...],
			[tell how to link against the BLAS library; default: -lblas]),
		[BLAS_LIBS=${withval}])
	AC_ARG_WITH([blas-libdir],
		AC_HELP_STRING([--with-blas-libdir=DIR],
			[search BLAS library in DIR]),
		[BLAS_LDFLAGS="-L${withval}"])
	AC_ARG_WITH([lapack-link],
		AC_HELP_STRING([--with-lapack-link=...],
			[tell how to link against the LAPACK library; default: -llapack]),
		[LAPACK_LIBS=${withval}])
	AC_ARG_WITH([lapack-libdir],
		AC_HELP_STRING([--with-lapack-libdir=DIR],
			[search LAPACK library in DIR]),
		[LAPACK_LDFLAGS="-L${withval}"])
	AC_ARG_VAR([BLAS_LIBS],[The list of arguments required to link against the BLAS library])
	AC_ARG_VAR([BLAS_LDFLAGS],[extra LDFLAGS required for the BLAS library])
	AC_ARG_VAR([LAPACK_LIBS],[The list of arguments required to link against the LAPACK library])
	AC_ARG_VAR([LAPACK_LDFLAGS],[extra LDFLAGS required for the LAPACK library])

	AC_LANG_PUSH([C++])

	dnl Linker-Namen für Fortran-Namen (BLAS, LAPACK)
	AC_F77_FUNC([dgemm],[ac_BLAS_DGEMM])
	AC_F77_FUNC([dgesv],[ac_LAPACK_DGESV])

	dnl *** BLAS ***
	acx_blas_ok=no
	AC_MSG_CHECKING([for BLAS library])
	if test -z "${BLAS_LIBS}"; then
		BLAS_LIBS="-lblas"
	fi
	dnl evtl. vorhandene Umgebungsvariablen nutzen
	ac_save_LIBS="${LIBS}"
	LIBS="${LIBS} ${BLAS_LIBS}"
	ac_save_LDFLAGS="${LDFLAGS}"
	LDFLAGS="${LDFLAGS} ${BLAS_LDFLAGS}"

	AC_TRY_LINK_FUNC([$ac_BLAS_DGEMM],[acx_blas_ok=yes],[BLAS_LIBS=""])
	LIBS=${ac_save_LIBS}
	LDFLAGS=${ac_save_LDFLAGS}
	AC_MSG_RESULT([$acx_blas_ok])

	dnl *** LAPACK ***
	acx_lapack_ok=no
	AC_MSG_CHECKING([for LAPACK library])
	if test -z "${LAPACK_LIBS}"; then
		LAPACK_LIBS="-llapack"
	fi
	dnl evtl. vorhandene Umgebungsvariablen nutzen
	ac_save_LIBS="${LIBS}"
	LIBS="${LIBS} ${BLAS_LIBS} ${LAPACK_LIBS} ${FLIBS}"
	ac_save_LDFLAGS="${LDFLAGS}"
	LDFLAGS="${LDFLAGS} ${BLAS_LDFLAGS} ${LAPACK_LDFLAGS}"

	AC_TRY_LINK_FUNC([$ac_LAPACK_DGESV],[acx_lapack_ok=yes],[LAPACK_LIBS=""])
	LIBS=${ac_save_LIBS}
	LDFLAGS=${ac_save_LDFLAGS}
	AC_MSG_RESULT([$acx_lapack_ok])

	if test "$acx_blas_ok" = "yes" -a "$acx_lapack_ok" = "yes"; then
		AC_SUBST([BLAS_LIBS])
		AC_SUBST([BLAS_LDFLAGS])
		AC_SUBST([LAPACK_LIBS])
		AC_SUBST([LAPACK_LDFLAGS])
		ifelse([$1],,AC_DEFINE(HAVE_XERCESC,1,[Define if you have the Xerces-C library.]),[$1])
		:
	else
		$2
		:
	fi

	AC_LANG_POP([C++])
])
