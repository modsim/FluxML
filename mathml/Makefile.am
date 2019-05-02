EXTRA_DIST = make_mml_constants.pl mml_constants.txt mathml2_xsd.tar.gz

install-exec-hook:
	mkdir -p $$DESTDIR$(datadir)/@PACKAGE@/xml
	gzip -dc mathml2_xsd.tar.gz | (cd $$DESTDIR$(datadir)/@PACKAGE@/xml && tar -xv)

flux_includedir = $(includedir)/@PACKAGE@
flux_include_HEADERS = MathML.h MathMLContentObject.h MathMLDeclare.h \
		       MathMLDocument.h MathMLElement.h MathMLExpression.h \
		       MathMLLambdaExpression.h MathMLMatrix.h \
		       MathMLUnicodeConstants.h MathMLVector.h

