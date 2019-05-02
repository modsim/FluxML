AC_DEFUN([AC_CXX_FLAGS_PRESET],[

dnl AC_SUBST(CXXFLAGS)
AC_SUBST(CXX_OPTIM_FLAGS)
AC_SUBST(CXX_DEBUG_FLAGS)
AC_SUBST(CXX_LIBS)
AC_SUBST(AR)
AC_SUBST(AR_FLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(RANLIB)


AR_FLAGS="-cru"
prevCXXFLAGS=$CXXFLAGS

AC_MSG_CHECKING([whether using $CXX preset flags])
AC_ARG_ENABLE(cxx-flags-preset,
AS_HELP_STRING([--enable-cxx-flags-preset],
[Enable C++ compiler flags preset @<:@default yes@:>@]),[],[enableval='yes'])

if test "$enableval" = yes ; then
	ac_cxx_flags_preset=yes
	case "$CXX" in
	*xlc++*|*xlC*) dnl IBM Visual Age C++ http://www.ibm.com/
		CXX_VENDOR="IBM"
		CXXFLAGS="-qrtti=all"
		CXX_OPTIM_FLAGS="-O3 -qstrict -qstrict_induction -qinline -qmaxmem=8192 -qansialias -qhot -qunroll=yes -DNDEBUG"
		CXX_DEBUG_FLAGS="-O0 -g -DDEBUG"
	;;      
	*icpc*|*icc*) dnl Intel icc http://www.intel.com/
		CXX_VENDOR="Intel"
		CXXFLAGS="-ansi" dnl -strict_ansi flag causes trouble
		CXX_OPTIM_FLAGS="-O3 -Zp16 -ip -ansi_alias -DNDEBUG"
		CXX_DEBUG_FLAGS="-O0 -C -g -DDEBUG"
	;;
	*g++*|*c++*) dnl GNU C++ http://gcc.gnu.org/
		CXX_VENDOR="GNU" 
		GCC_V=`$CXX --version`
		gcc_version=`expr "$GCC_V" : '.* \(@<:@0-9@:>@\)\..*'`
		gcc_release=`expr "$GCC_V" : '.* @<:@0-9@:>@\.\(@<:@0-9@:>@\).*'`
		if test $gcc_version -lt "3" ; then
			CXXFLAGS="-ftemplate-depth-40"
			CXX_OPTIM_FLAGS="-O2 -fno-gcse -DNDEBUG"
		else
			CXXFLAGS=""
			CXX_ARCH_FLAG=""
			if test $gcc_version -ge "4" ; then
				CXX_ARCH_FLAG="-march=native";
			fi
			CXX_OPTIM_FLAGS="$CXX_ARCH_FLAG -O3 -fomit-frame-pointer -DNDEBUG"
		fi
		CXX_DEBUG_FLAGS="-Wall -O0 -fno-inline -g -DDEBUG"
		LDFLAGS+="-rdynamic"
	;;
	*KCC*) dnl KAI C++ http://www.kai.com/
		CXX_VENDOR="KAI"
		CXXFLAGS="--restrict"
		CXX_OPTIM_FLAGS="+K3"
		CXX_DEBUG_FLAGS="+K0 -g -DDEBUG"
		AR="$CXX"
		AR_FLAGS="-o"
		case "$target" in
		*sgi*) dnl SGI C backend compiler
			CXX_OPTIM_FLAGS="$CXX_OPTIM_FLAGS --backend -Ofast -DNDEBUG"
		;;
		*ibm*) dnl IBM xlC backend compiler
			CXX_OPTIM_FLAGS="$CXX_OPTIM_FLAGS -O5 --backend -qstrict --backend -qstrict_induction -DNDEBUG"
		;;
		*) dnl other C backend compiler
			CXX_OPTIM_FLAGS="$CXX_OPTIM_FLAGS -O -DNDEBUG"
		;;
		esac
	;;
	*aCC*) dnl HP aCC http://www.hp.com/go/c++
		CXX_VENDOR="HP"
		CXXFLAGS="-AA"
		CXX_OPTIM_FLAGS="+O2 -DNDEBUG"
		CXX_DEBUG_FLAGS="-g -DDEBUG"
	;;
	*pgCC*) dnl Portland Group http://www.pgroup.com
		CXX_VENDOR="PGI"
		CXXFLAGS=""
		CXX_OPTIM_FLAGS="-O4 -Mnoframe -Mnodepchk -Minline=levels:25 -DNDEBUG"
		CXX_DEBUG_FLAGS="-g -O0 -DDEBUG"
	;;
	*pathCC*) dnl Pathscale pathCC compiler http://www.pathscale.com
		CXX_VENDOR="pathCC"
		CXXFLAGS="-ansi"
		CXX_OPTIM_FLAGS="-O3 -fstrict-aliasing -finline-functions -DNDEBUG"
		CXX_DEBUG_FLAGS="-g -DDEBUG"
		AR="$CXX"
		AR_FLAGS="-ar -o"
	;;
	*CC*) 
		case "$target" in
		*sgi*) dnl SGI C++ http://www.sgi.com
			CXX_VENDOR="SGI"
			CXXFLAGS="-LANG:std -LANG:restrict -no_auto_include"
			CXX_OPTIM_FLAGS="-O3 -IPA -OPT:Olimit=0:alias=typed:swp=ON -DNDEBUG"
			AR="$CXX"
			AR_FLAGS="-ar -o"
		;;
		*solaris*) dnl SunPRO C++ http://www.sun.com
			CXX_VENDOR="SUN"
			CXXFLAGS="-features=tmplife -library=stlport4"
			CXX_OPTIM_FLAGS="-O3 -DNDEBUG"
		;;
		*cray*) dnl Cray C++
			CXX_VENDOR="Cray"
			CXXFLAGS="-h instantiate=used"
			CXX_OPTIM_FLAGS="-O3 -hpipeline3 -hunroll -haggress -hscalar2 -DNDEBUG"
		;;
		esac
		CXX_DEBUG_FLAGS="-g -DDEBUG"
	;;
	*) 
		ac_cxx_flags_preset=no
	;;
	esac
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

if test "$ac_cxx_flags_preset" = yes ; then
	if test "$CXX_VENDOR" = GNU ; then
		AC_MSG_NOTICE([Setting compiler flags for $CXX_VENDOR $CXX])
	else
		AC_MSG_NOTICE([Setting compiler flags for $CXX_VENDOR $CXX])
	fi
else
	AC_MSG_NOTICE([No flags preset found for $CXX])
fi

CXXFLAGS="$prevCXXFLAGS $CXXFLAGS"
])

