AM_CPPFLAGS = -I$(top_srcdir)/lib \
	      -I$(top_srcdir)/matrixmath \
	      -I$(top_srcdir)/symbolicmath \
	      -I$(top_srcdir)/data \
	      -I$(top_srcdir)/fluxml \
	      -I$(top_srcdir)/mathml \
	      -I$(top_srcdir)/support \
	      $(XERCESC_CPPFLAGS)

AM_LDFLAGS = $(XERCESC_LDFLAGS) $(XERCESC_LIBS)

EXTRA_DIST = fmlstats ftbl2fml sbml2fml fml2sbml \
	     setparameters setmeasurements setinputs \
             flux.py setup.py multiply_fml \
	     updatefml


all-local:
	$(PYTHON) setup.py build

install-exec-local:
	$(PYTHON) setup.py install --prefix=$$DESTDIR$(prefix) --install-layout=deb

clean-local:
	rm -rf build

