dnl @synopsis ACX_MACHEPS([FALLBACK-VALUE])
dnl 
dnl
AC_DEFUN([ACX_MACHEPS],[
	AC_LANG_PUSH([C])
	AC_MSG_CHECKING([double precision floating point epsilon])
	AC_RUN_IFELSE([
	AC_LANG_PROGRAM([[#include <stdio.h>]],[[double o_eps, eps=0.5;
while (1.+eps != 1.) { o_eps = eps; eps /= 2.; } eps = o_eps;
printf("%.15e\n", eps);]])],
	[macheps_val=$(./conftest${ac_exeext})],[
	if test ! -z "$1"; then
		macheps_val="$1"
	else
		macheps_val="2.22044604925031e-16"
	fi
	AC_MSG_RESULT([${macheps_val}])
	],[])

	AC_DEFINE_UNQUOTED([MACHEPS],[${macheps_val}],
		[double precision floating point epsilon: 1.0+MACHEPS==1.0])
	AC_LANG_POP([C])
])
