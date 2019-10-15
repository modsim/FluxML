AC_PREREQ([2.13])
AC_INIT([FLUXML],[2.1])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.9 tar-pax])

AC_CONFIG_HEADER([fluxml_config.h])

AC_AIX

dnl Auswahl eines C++,C Compilers
AC_PROG_CXX([xlC_r g++ pathCC aCC CC pgCC KCC c++]) dnl [xlC_r g++ c++]
AC_PROG_CC([xlc_r gcc cc])

dnl C++ Code
AC_LANG(C++)

dnl configuriert diverse Compiler-Flags
AC_CXX_FLAGS_PRESET

dnl schaltet 64-Bit Übersetzung an (--enable-64bit)
AC_COMPILERS_ENABLE_64BITS

dnl Architektur-Check (setzt WORDS_BIGENDIAN)
AC_C_BIGENDIAN

dnl schaltet Compileroptimierung an (--enable-optimize)
AC_CXX_ENABLE_OPTIMIZE

dnl schaltet Debugging an (--enable-debug)
AC_CXX_ENABLE_DEBUG

dnl Fortran-Wrapper
AC_F77_WRAPPERS

dnl Cygwin/DLL support
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

dnl Verfügbarkeit von cstddef für Ipopt
AC_CHECK_HEADERS([cstddef])

dnl Verfügbarkeit von time.h
AC_HEADER_TIME

dnl Verfügbarkeit von Typ size_t
AC_TYPE_SIZE_T

dnl Plattform-Bits ermitteln (setzt SIZEOF_VOID_P)
AC_CHECK_SIZEOF([void *])

dnl setzt SIZEOF_INT
AC_CHECK_SIZEOF([int])

dnl setzt SIZEOF_SIZE_T
AC_CHECK_SIZEOF([size_t])

dnl -------------------------------------------
dnl     Python support
dnl -------------------------------------------
AM_PATH_PYTHON(2.7)
dnl AC_PYTHON_DEVEL(>= '2.7')

dnl -------------------------------------------
dnl     install support
dnl -------------------------------------------
AC_PROG_INSTALL

dnl -------------------------------------------
dnl     bison / yacc support
dnl -------------------------------------------
AC_PROG_YACC

dnl -------------------------------------------
dnl     patch support
dnl -------------------------------------------
AC_PATH_PROG([PATCH],[patch])

dnl -------------------------------------------
dnl     re2c support
dnl -------------------------------------------
AC_PATH_PROG([RE2C],[re2c])
AS_IF([test x"$RE2C" == x""], [AC_MSG_ERROR([Please install re2c first])])

dnl -------------------------------------------
dnl     Xerces-C support
dnl -------------------------------------------
dnl 1. es wird HAVE_XERCESC in der config.h gesetzt
dnl 2. es werden $(XERCESC_LIBS), $(XERCESC_CPPFLAGS), $(XERCESC_LDFLAGS)
dnl    gesetzt
ACX_XERCESC(AC_MSG_NOTICE([usable Xerces-C library detected]),
	AC_MSG_ERROR([Xerces-C library not functional]))

dnl -----------------------------------------------
dnl BLAS, LAPACK
dnl -----------------------------------------------
ACX_BLASLAPACK(
	AC_MSG_NOTICE([usable LAPACK/BLAS libraries detected]),
	AC_MSG_ERROR([no usable LAPACK/BLAS libraries found])
	)

dnl -------------------------------------------
dnl      Check for different platforms
dnl -------------------------------------------
P_LINUX="false"
case $host in
	*-*-msdos* | *-*-go32* | *-*-mingw32* | *-*-cygwin* | *-*-windows*)
		AC_DEFINE([P_WIN32], [1], [Microsoft Windows platform])
		AC_DEFINE([NOMINMAX], [1], [prevent Cygwin from defining min/max])
		SOEXT=".dll"
		;;
	*-*-linux*)
		AC_DEFINE([P_LINUX], [1], [Linux platform])
		SOEXT=".so"
		P_LINUX="true"
		;;
	*-*-darwin*)
		AC_DEFINE([P_DARWIN], [1], [MacOS X platform])
		SOEXT=".dylib"
		;;
	*-*-aix*)
		AC_DEFINE([P_AIX], [1], [AIX platform])
		SOEXT=".so"
		;;
	*)
		;;
esac
AC_SUBST([SOEXT])
AC_DEFINE_UNQUOTED([SOEXT], ["${SOEXT}"], [platform-specific dynamic library extension])
AM_CONDITIONAL(P_LINUX,test x${P_LINUX} = xtrue)

dnl -------------------------------------------
dnl      Double-Floating-Point epsilon
dnl -------------------------------------------
ACX_MACHEPS


dnl -------------------------------------------
dnl Installationspfad der Datendateien, Java- und Matlab-Dateien
dnl -------------------------------------------
flux_xml_dir=${datarootdir}/${PACKAGE_TARNAME}/xml
flux_etc_dir=/etc/${PACKAGE_TARNAME}
flux_lib_dir=${datarootdir}/${PACKAGE_TARNAME}/lib
flux_python_site_pkg=${pythondir}
AC_DEFINE_DIR([FLUX_XML_DIR],[flux_xml_dir],[FLUX installation directory for XML resources])
AC_DEFINE_DIR([FLUX_ETC_DIR],[flux_etc_dir],[FLUX installation directory for system settings])
AC_DEFINE_DIR([FLUX_LIB_DIR],[flux_lib_dir],[FLUX installation directory for optimization plug-ins])
AC_DEFINE_DIR([FLUX_PYTHON_SITE_PKG],[flux_python_site_pkg],[FLUX installation directory for python modules])

AC_CONFIG_FILES([
	Makefile
	apps/Makefile
	lib/Makefile
	symbolicmath/Makefile
	matrixmath/Makefile
	data/Makefile
	support/Makefile
	mathml/Makefile
	fluxml/Makefile
])
AC_OUTPUT

dnl ------------------------------------------
dnl Überblick über Compiler-Flags geben (TODO)
dnl ------------------------------------------
AC_ENV

