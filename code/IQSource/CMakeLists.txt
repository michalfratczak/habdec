

#include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )


find_package(SoapySDR REQUIRED) # this reverts CMAKE_CXX_STANDARD to '11'
#set ( CMAKE_CXX_STANDARD 17 )
include_directories( ${SoapySDR_INCLUDE_DIRS} )
message( "SoapySDR_INCLUDE_DIRS:" ${SoapySDR_INCLUDE_DIRS} )
message( "SoapySDR_LIBRARIESL:" ${SoapySDR_LIBRARIES} )

# RTTY DECODER
set ( IQSource_src
					IQSource_File.h
					IQSource.h
					IQSource_SoapySDR.h  IQSource_SoapySDR.cpp
			)


if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wshadow" )
	SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-class-memaccess" )
	SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Werror=return-type" )
	SET( CMAKE_CXX_FLAGS  " -O3 " )
endif()


add_library( IQSource  ${IQSource_src})
target_link_libraries( IQSource ${SoapySDR_LIBRARIES} )
