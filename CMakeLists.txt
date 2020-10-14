# General settings
cmake_minimum_required (VERSION 3.1)
project (libFluxML)
set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set (CMAKE_CXX_STANDARD 11)

find_package(BISON REQUIRED)
find_package(RE2C REQUIRED)
find_package(XercesC REQUIRED)
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)
find_package(GMP REQUIRED)

message('main')

add_subdirectory(apps)
add_subdirectory(data)
add_subdirectory(fluxml)
add_subdirectory(lib)
add_subdirectory(mathml)
add_subdirectory(matrixmath)
add_subdirectory(support)
add_subdirectory(symbolicmath)

message('main')
# Set code location variables
set (FLUXML_SRC
     data/Constraint.cc
     data/ConstraintSystem.cc
     data/Info.cc
     data/InputPool.cc
     data/InputProfile.cc
     data/Notation.cc
     data/Pool.cc
     data/IsoReaction.cc
     fluxml/FluxMLConfiguration.cc
     fluxml/FluxMLConstraints.cc
     fluxml/FluxMLDocument.cc
     fluxml/FluxMLInfo.cc
     fluxml/FluxMLInput.cc
     fluxml/FluxMLMetabolitePools.cc
     fluxml/FluxMLPool.cc
     fluxml/FluxMLReaction.cc
     fluxml/FluxMLReactionNetwork.cc
     fluxml/FluxMLUnicodeConstants.cc
     fluxml/MGroup.cc
     fluxml/MMData.cc
     fluxml/MMDocument.cc
     fluxml/MMModel.cc
     fluxml/MMUnicodeConstants.cc
     fluxml/Configuration.cc
     lib/Error.cc
     lib/charptr_array.cc
     lib/charptr_map.cc
     lib/hash_functions.cc
     lib/Combinations.cc
     lib/cstringtools.cc
     lib/readstream.cc
     lib/fRegEx.cc
     lib/spawn_child.c
     lib/Stat.cc
     mathml/MathMLDeclare.cc
     mathml/MathMLDocument.cc
     mathml/MathMLElement.cc
     mathml/MathMLExpression.cc
     mathml/MathMLLambdaExpression.cc
     mathml/MathMLMatrix.cc
     mathml/MathMLUnicodeConstants.cc
     mathml/MathMLVector.cc
     matrixmath/MMatrix.cc
     matrixmath/MMatrixOps.cc
     matrixmath/LAPackWrap.cc
     matrixmath/MVector.cc
     matrixmath/PMatrix.cc
     matrixmath/SMatrix.cc
     matrixmath/StoichMatrixInteger.cc
     matrixmath/StandardForm.cc
     support/XMLElement.cc
     support/UnicodeTools.cc
     support/XMLException.cc
     support/XMLFramework.cc
     support/DOMReaderImplL3.cc
     support/DOMWriterImplL3.cc
     support/EntityResolver.cc
     support/DOMErrorHandler.cc
     support/XMLUnicodeConstants.cc
     symbolicmath/ExprTree.cc
     ${BISON_ExprScanner_OUTPUTS}
     symbolicmath/LinearExpression.cc
)



include_directories (${PROJECT_SOURCE_DIR}/data
                     ${PROJECT_SOURCE_DIR}/fluxml
                     ${PROJECT_SOURCE_DIR}/lib
                     ${PROJECT_SOURCE_DIR}/mathml
                     ${PROJECT_SOURCE_DIR}/matrixmath
                     ${PROJECT_SOURCE_DIR}/support
                     ${PROJECT_SOURCE_DIR}/symbolicmath
                     ${CMAKE_CURRENT_BINARY_DIR}
                     ${GMP_INCLUDE_DIRS}
                     ${XercesC_INCLUDE_DIRS}
                    )

add_library (libFluxML SHARED ${FLUXML_SRC})
set_target_properties (libFluxML PROPERTIES OUTPUT_NAME FluxML)
add_dependencies (libFluxML NOTATION_RE)

# Set install destination variables
set (DEST_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
set (DEST_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
set (DEST_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/libFluxML)
set (DEST_DOC_DIR ${CMAKE_INSTALL_PREFIX}/share/doc/libFluxML)
set (DEST_DESKTOP_DIR /usr/share/applications)
set (DEST_PIXMAP_DIR /usr/share/pixmaps)
set (FLUX_XML_DIR ${CMAKE_INSTALL_PREFIX}/fluxml/share/fluxml/xml)

configure_file(fluxml_dirs.h.in fluxml_dirs.h)
