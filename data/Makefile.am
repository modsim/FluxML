Notation.cc : Notation.inc Notation2.inc Notation3.inc

.re.inc :
	$(RE2C) -W -o $@ $<

EXTRA_DIST = Notation.re Notation2.re Notation3.re

flux_includedir = $(includedir)/@PACKAGE@
flux_include_HEADERS = Constraint.h ConstraintSystem.h DataException.h \
		       Flux.h Info.h InputPool.h InputProfile.h IsoReaction.h \
		       Notation.h Pool.h SimLimits.h \
		       Notation.inc Notation2.inc Notation3.inc

