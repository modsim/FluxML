fluxxmldir = $(datadir)/@PACKAGE@/xml
dist_fluxxml_DATA = fluxml.xsd

EXTRA_DIST = fml_constants.txt make_fml_constants.pl \
	     mm_constants.txt make_mm_constants.pl

FluxMLUnicodeConstants.cc : FluxMLUnicodeConstants.h

FluxMLUnicodeConstants.h : fml_constants.txt
	./make_fml_constants.pl


MMUnicodeConstants.cc : MMUnicodeConstants.h

MMUnicodeConstants.h : mm_constants.txt
	./make_mm_constants.pl


flux_includedir = $(includedir)/@PACKAGE@
flux_include_HEADERS = Configuration.h FluxMLConfiguration.h \
		       FluxMLConstraints.h FluxMLContentObject.h \
		       FluxMLDocument.h FluxML.h FluxMLInfo.h \
		       FluxMLInput.h FluxMLMetabolitePools.h \
		       FluxMLPool.h FluxMLReaction.h \
		       FluxMLReactionNetwork.h FluxMLUnicodeConstants.h \
		       MGroup.h MMData.h MMDocument.h MMModel.h \
		       MMUnicodeConstants.h MValue.h

