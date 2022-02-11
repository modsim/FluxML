ExprParser.cc : ExprParser.y ExprScannerMM.inc
	test -z "$(YACC)" || $(YACC) -o $@ -p "et_" $<
	touch $@

ExprScannerMM.inc : ExprScannerMM.re
	$(RE2C) -o $@ $<

BUILT_SOURCES = ExprParser.cc
EXTRA_DIST = ExprParser.y ExprScannerMM.re ExprScannerMM.inc

flux_includedir = $(includedir)/@PACKAGE@
flux_include_HEADERS = ExprParser.h ExprTree.h \
		       LinearExpression.h

