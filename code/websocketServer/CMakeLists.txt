
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

set ( FFTW3f_DIR ../CMake )
find_package( FFTW3f REQUIRED )
include_directories( ${FFTW3f_INCLUDE_DIRS} )

# boost
set ( Boost_NO_BOOST_CMAKE ON )
set ( Boost_NO_SYSTEM_PATHS ON )
set ( Boost_USE_STATIC_LIBS ON )
add_definitions ( -DBOOST_ALL_NO_LIB )
find_package(Boost REQUIRED COMPONENTS program_options system date_time )
include_directories( ${Boost_INCLUDE_DIRS} )
message ( "Boost_INCLUDE_DIRS " ${Boost_INCLUDE_DIRS} )
message ( "Boost_LIBRARY_DIRS " ${Boost_LIBRARY_DIRS} )

find_package(SoapySDR REQUIRED) # this reverts CMAKE_CXX_STANDARD to '11'
#set ( CMAKE_CXX_STANDARD 17 )
include_directories( ${SoapySDR_INCLUDE_DIRS} )


set( CMAKE_CXX_FLAGS  " -O3 " )
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	# set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wshadow" )
	set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-class-memaccess" )
	set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-unused-variable" )
	set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-unused-parameter" )
	set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized" )
	set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Werror=return-type" )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	SET(CMAKE_CXX_FLAGS "/EHsc")
	add_definitions( -D_USE_MATH_DEFINES )
endif()

set (PlatformSpecificLinking "")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set (PlatformSpecificLinking "pthread")
endif()

set ( websocketServer_src
					program_options.h 	program_options.cpp
					habdec_ws_protocol.h habdec_ws_protocol.cpp
					http_session.h http_session.cpp
					listener.h listener.cpp
					websocket_session.h websocket_session.cpp
					ws_server.h ws_server.cpp
					CompressedVector.h CompressedVector.cpp
					${CMAKE_SOURCE_DIR}/habitat/habitat_upload.cpp
					${CMAKE_SOURCE_DIR}/habitat/habitat_list_flights.cpp
					${CMAKE_SOURCE_DIR}/common/http_request.cpp
					${CMAKE_SOURCE_DIR}/common/GpsDistance.cpp
					main.cpp
			)

# compile in git repo version
configure_file("${PROJECT_SOURCE_DIR}/common/git_repo_sha1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_repo_sha1.cpp" @ONLY)
list(APPEND websocketServer_src "${CMAKE_CURRENT_BINARY_DIR}/git_repo_sha1.cpp" ${PROJECT_SOURCE_DIR}/common/git_repo_sha1.h)

add_executable ( 		habdecWebsocketServer ${websocketServer_src} )
target_link_libraries( 	habdecWebsocketServer
							Decoder
							IQSource
							${Boost_LIBRARIES}
							${PlatformSpecificLinking} )

install ( TARGETS habdecWebsocketServer DESTINATION ${CMAKE_INSTALL_PREFIX} )
install ( 	FILES ${PROJECT_SOURCE_DIR}/habLogger/habLogger.py ${PROJECT_SOURCE_DIR}/habLogger/fileLogger.sh
		PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
		DESTINATION  ${CMAKE_INSTALL_PREFIX} )


file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_loop.sh "#!/bin/sh\n while true; do ./habdecWebsocketServer --config ./habdecWebsocketServer.opts; done\n" )
install (       FILES ${CMAKE_CURRENT_BINARY_DIR}/run_loop.sh
                PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                DESTINATION  ${CMAKE_INSTALL_PREFIX} )

