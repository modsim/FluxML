message('data')

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Notation.inc" 
                   COMMAND "${RE2C}" "-W -o" "${CMAKE_CURRENT_BINARY_DIR}/Notation.inc" "${CMAKE_CURRENT_BINARY_DIR}/Notation.re" )
                   
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Notation2.inc" 
                   COMMAND "${RE2C}" "-W -o" "${CMAKE_CURRENT_BINARY_DIR}/Notation2.inc" "${CMAKE_CURRENT_BINARY_DIR}/Notation2.re" )

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Notation3.inc" 
                   COMMAND "${RE2C}" "-W -o" "${CMAKE_CURRENT_BINARY_DIR}/Notation3.inc" "${CMAKE_CURRENT_BINARY_DIR}/Notation3.re" )
                   
add_custom_target(NOTATION_RE "" DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Notation.inc" "${CMAKE_CURRENT_BINARY_DIR}/Notation2.inc" "${CMAKE_CURRENT_BINARY_DIR}/Notation3.inc")

#TODO add dependecy to the notation files, probably on the fluxML target
