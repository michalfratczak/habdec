
include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

set ( FLTK_SKIP_FLUID True )
find_package( FLTK REQUIRED )
include_directories( ${FLTK_INCLUDE_DIR} )
message( "FLTK Include:" 	${FLTK_INCLUDE_DIR} )
message( "FLTK Lib:" 		${FLTK_LIBRARIES} )

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

add_executable (habdec_fltk main.cpp gui_utils.h )
target_link_libraries( habdec_fltk Decoder ${FLTK_LIBRARIES} IQSource Decoder pthread )

install ( TARGETS habdec_fltk DESTINATION ${CMAKE_INSTALL_PREFIX} )
