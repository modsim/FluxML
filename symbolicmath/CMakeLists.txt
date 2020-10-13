message('symbolicmath')


add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ExprScannerMM.inc" 
                   COMMAND "${RE2C}" "-W -o" "${CMAKE_CURRENT_BINARY_DIR}/ExprScannerMM.inc" "${CMAKE_CURRENT_BINARY_DIR}/ExprScannerMM.re" )
BISON_TARGET(ExprScanner ExprParser.y ${CMAKE_CURRENT_BINARY_DIR}/ExprParser.cc COMPILE_FLAGS "-p \"et\" ")

#TODO add dependcy of .inc file



#${BISON_ExprScanner_OUTPUTS}

