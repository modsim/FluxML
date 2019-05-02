#!/bin/sh
AFILES="aclocal.m4 autom4te.cache config/config.guess config.log \
	config.status config/config.sub configure config/depcomp \
	ThreadsPP-0.1 INSTALL config/install-sh libtool config/ltmain.sh \
	Makefile Makefile.in config/missing FLUX-1.0.tar.gz config.h \
	config/compile config.h.in config.h.in~ stamp-h1"

bootstrap_autotools()
{
	autoreconf -i -I m4
}

cleanup_autotools()
{

	if [ -f Makefile ]; then
		make distclean
	fi
	
	for i in $AFILES; do
		if [ -d "$i" ]; then
			echo "D[$i]"
			rm -r $i
		else
			if [ -f "$i" ]; then
				echo "F[$i]"
				rm $i
			fi
		fi
	done
	
#	for i in $(find . -regex '.*Makefile\(\|\.in\)'); do
#		if [ -f "$i" ]; then
#			echo "F[$i]"
#			rm $i
#		fi
#	done
	for i in $(find . -name Makefile); do
		if [ -f "$i" -a -f "$i.in" ]; then
			echo "F[$i, $i.in]"
			rm $i
			rm $i.in
		fi
	done
	
	for i in $(find . -type d -name .deps); do
		if [ -d "$i" ]; then
			echo "D[$i]"
			rm -r $i
		fi
	done
}

if [ ! -d ./config ]; then
	cd ..
fi

case "$1" in
	clean)
		cleanup_autotools
		;;
	boot)
		bootstrap_autotools
		;;
	*)
		echo "Usage: $0 {clean|boot}"
		exit 1
esac
exit 0

