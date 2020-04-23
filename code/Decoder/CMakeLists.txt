

include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

set ( FFTW3f_DIR ../CMake )
find_package( FFTW3f REQUIRED )
include_directories( ${FFTW3f_INCLUDE_DIRS} )
message( "FFTW Include:" 	${FFTW3f_INCLUDE_DIRS} )
message( "FFTW Lib:" 		${FFTW3f_LIBRARIES} )


# RTTY DECODER
set ( Decoder_src
					AFC.h
					Average.h
					${PROJECT_SOURCE_DIR}/common/console_colors.h
					${PROJECT_SOURCE_DIR}/common/console_colors.cpp
					CRC.h 	CRC.cpp
					Decimator.h
					Decoder.h
					FFT.h 	FFT.cpp
					filtercoef.h
					FirFilter.h
					FSK2_Demod.h
					func_timer.h
					habdec_windows.h
					IQVector.h
					print_habhub_sentence.h  print_habhub_sentence.cpp
					RTTY.h
					sentence_extract.h	sentence_extract.cpp
					SpectrumInfo.h
					SymbolExtractor.h
					ssdv_wrapper.cpp
			)

SET( CMAKE_CXX_FLAGS  " -O3 " )
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wshadow" )
	SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-class-memaccess" )
	SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Werror=return-type" )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	SET(CMAKE_CXX_FLAGS "/EHsc")
	add_definitions( -D_USE_MATH_DEFINES )
endif()

set (PlatformSpecificLinking "")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set (PlatformSpecificLinking "pthread")
endif()

add_library( Decoder  ${Decoder_src})
target_link_libraries( 	Decoder
						ssdv_lib
						#${SoapySDR_LIBRARIES}
						${FFTW3f_LIBRARIES}
						${PlatformSpecificLinking} )

