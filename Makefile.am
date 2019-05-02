ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = lib \
	  symbolicmath \
	  matrixmath \
	  data \
	  support \
	  mathml \
	  fluxml \
	  apps

AM_CPPFLAGS = -I$(top_srcdir)/data \
              -I$(top_srcdir)/fluxml \
              -I$(top_srcdir)/lib \
              -I$(top_srcdir)/mathml \
              -I$(top_srcdir)/matrixmath \
              -I$(top_srcdir)/support \
              -I$(top_srcdir)/symbolicmath \
	      $(GMP_CPPFLAGS) \
	      $(XERCESC_CPPFLAGS)
AM_LDFLAGS = $(GMP_LDFLAGS) $(GMP_LIBS) \
	     $(XERCESC_LDFLAGS) $(XERCESC_LIBS) \
             $(BLAS_LDFLAGS) $(LAPACK_LDFLAGS) \
             $(LAPACK_LIBS) $(BLAS_LIBS) $(LIBS) $(FLIBS) \
             -lz

lib_LTLIBRARIES = libFluxML.la

libFluxML_la_SOURCES = data/Constraint.cc data/Constraint.h \
                       data/ConstraintSystem.cc data/ConstraintSystem.h \
                       data/DataException.h data/Flux.h data/Info.cc data/Info.h \
                       data/InputPool.cc data/InputPool.h \
                       data/InputProfile.cc data/InputProfile.h \
                       data/Notation.cc data/Notation.h data/Notation.inc \
                       data/Notation2.inc data/Notation3.inc \
                       data/Pool.cc data/Pool.h data/IsoReaction.cc data/IsoReaction.h \
                       data/SimLimits.h \
                       fluxml/FluxMLConfiguration.cc fluxml/FluxMLConfiguration.h \
		       fluxml/FluxMLConstraints.cc fluxml/FluxMLConstraints.h \
		       fluxml/FluxMLContentObject.h fluxml/FluxMLDocument.cc \
		       fluxml/FluxMLDocument.h \
		       fluxml/FluxML.h fluxml/FluxMLInfo.cc fluxml/FluxMLInfo.h fluxml/FluxMLInput.cc \
		       fluxml/FluxMLInput.h fluxml/FluxMLMetabolitePools.cc \
		       fluxml/FluxMLMetabolitePools.h fluxml/FluxMLPool.cc fluxml/FluxMLPool.h \
		       fluxml/FluxMLReaction.cc fluxml/FluxMLReaction.h \
		       fluxml/FluxMLReactionNetwork.cc fluxml/FluxMLReactionNetwork.h \
		       fluxml/FluxMLUnicodeConstants.cc fluxml/FluxMLUnicodeConstants.h \
		       fluxml/MGroup.cc fluxml/MGroup.h fluxml/MMData.cc fluxml/MMData.h \
		       fluxml/MMDocument.cc fluxml/MMDocument.h fluxml/MMModel.cc fluxml/MMModel.h \
		       fluxml/MMUnicodeConstants.cc fluxml/MMUnicodeConstants.h \
		       fluxml/MValue.h \
		       fluxml/Configuration.cc fluxml/Configuration.h \
                       lib/Error.cc lib/Error.h \
		       lib/BitArray.h lib/BitArray_impl.h \
		       lib/charptr_array.cc lib/charptr_array.h \
		       lib/fhash_map.h lib/charptr_map.cc lib/charptr_map.h \
		       lib/hash_functions.cc lib/hash_functions.h \
		       lib/Combinations.cc lib/Combinations.h \
		       lib/cstringtools.cc lib/cstringtools.h \
		       lib/MaskedArray.h \
		       lib/readstream.cc lib/readstream.h \
		       lib/Sort.h lib/Array.h lib/fRegEx.cc lib/fRegEx.h \
		       lib/spawn_child.c lib/Stat.cc lib/Stat.h\
		       lib/Conversions.h lib/IntegerMath.h lib/NLgetopt.h \
		       lib/DynamicLibrary.h \
                       mathml/MathMLContentObject.h \
		       mathml/MathMLDeclare.cc mathml/MathMLDeclare.h \
		       mathml/MathMLDocument.cc mathml/MathMLDocument.h \
		       mathml/MathMLElement.cc mathml/MathMLElement.h \
		       mathml/MathMLExpression.cc mathml/MathMLExpression.h \
		       mathml/MathMLLambdaExpression.cc mathml/MathMLLambdaExpression.h \
		       mathml/MathMLMatrix.cc mathml/MathMLMatrix.h \
		       mathml/MathMLUnicodeConstants.cc mathml/MathMLUnicodeConstants.h \
		       mathml/MathMLVector.cc mathml/MathMLVector.h \
		       mathml/MathML.h \
                       matrixmath/GMatrix.h matrixmath/GVector.h matrixmath/GMatrixOps.h \
		       matrixmath/MatrixInterface.h matrixmath/MMatrix.cc matrixmath/MMatrix.h \
		       matrixmath/MMatrixOps.cc matrixmath/MMatrixOps.h matrixmath/LAPackWrap.cc matrixmath/LAPackWrap.h \
		       matrixmath/MVector.cc matrixmath/MVector.h matrixmath/PMatrix.cc matrixmath/PMatrix.h \
		       matrixmath/SMatrix.cc matrixmath/SMatrix.h \
		       matrixmath/MatrixExponential_.h matrixmath/StemFunction_.h \
		       matrixmath/StoichMatrixInteger.cc matrixmath/StoichMatrixInteger.h \
		       matrixmath/VectorInterface.h \
		       matrixmath/StandardForm.cc matrixmath/StandardForm.h \
		       matrixmath/GSMatrixOps.h matrixmath/GSMatrix.h \
                       support/XMLElement.cc support/XMLElement.h \
		       support/UnicodeTools.cc support/UnicodeTools.h \
		       support/XMLException.cc support/XMLException.h \
		       support/XMLFramework.cc support/XMLFramework.h \
		       support/DOMReader.h support/DOMReaderImpl.h \
		       support/DOMReaderImplL3.cc support/DOMReaderImplL3.h \
		       support/DOMWriter.h support/DOMWriterImpl.h \
		       support/DOMWriterImplL3.cc support/DOMWriterImplL3.h \
		       support/EntityResolver.cc support/EntityResolver.h \
		       support/DOMErrorHandler.cc support/DOMErrorHandler.h \
		       support/XMLUnicodeConstants.cc support/XMLUnicodeConstants.h \
                       symbolicmath/ExprTree.cc symbolicmath/ExprTree.h \
		       symbolicmath/ExprParser.cc symbolicmath/ExprParser.h \
		       symbolicmath/LinearExpression.cc symbolicmath/LinearExpression.h

bin_PROGRAMS = fmllint

fmllint_SOURCES = apps/FluxMLLint.cc

fmllint_LDADD = libFluxML.la


flux_includedir = $(includedir)/@PACKAGE@
flux_include_HEADERS = fluxml_config.h

