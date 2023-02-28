
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

if(DEFINED FFTW_ROOT) # custom FFTW build
	set ( FFTW3f_DIR ../CMake )
	find_package( FFTW3f REQUIRED )
	include_directories( ${FFTW3f_INCLUDE_DIRS} )
	message( "FFTW Include:" 	${FFTW3f_INCLUDE_DIRS} )
	message( "FFTW Lib:" 		${FFTW3f_LIBRARIES} )
else()
	find_package(PkgConfig REQUIRED)
	pkg_search_module(FFTW REQUIRED fftw3f IMPORTED_TARGET)
	link_libraries     (PkgConfig::FFTW)
endif()

# boost
set ( Boost_NO_BOOST_CMAKE ON )
if(DEFINED BOOST_ROOT) # custom boost build
	set ( Boost_NO_SYSTEM_PATHS ON ) # don't use system installed boost
endif()
set ( Boost_USE_STATIC_LIBS ON )
add_definitions ( -DBOOST_ALL_NO_LIB )
find_package(Boost REQUIRED COMPONENTS program_options system date_time )
include_directories( ${Boost_INCLUDE_DIRS} )
message ( "Boost_INCLUDE_DIRS " ${Boost_INCLUDE_DIRS} )
message ( "Boost_LIBRARY_DIRS " ${Boost_LIBRARY_DIRS} )

find_package(SoapySDR REQUIRED) # this reverts CMAKE_CXX_STANDARD to '11'
#set ( CMAKE_CXX_STANDARD 17 )
include_directories( ${SoapySDR_INCLUDE_DIRS} )


include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 871ed52d350214a034f6ef8a3b8f51c5ce1bd400) # The commit hash for 1.9.0. Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)


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
					${CMAKE_SOURCE_DIR}/sondehub/sondehub_uploader.cpp
					${CMAKE_SOURCE_DIR}/common/http_request.cpp
					${CMAKE_SOURCE_DIR}/common/GpsDistance.cpp
					${CMAKE_SOURCE_DIR}/common/sentence_parse.cpp
					${CMAKE_SOURCE_DIR}/common/utc_now_iso.cpp
					${CMAKE_SOURCE_DIR}/Decoder/CRC.cpp
					main.cpp
			)

# compile in git repo version
configure_file("${PROJECT_SOURCE_DIR}/common/git_repo_sha1.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_repo_sha1.cpp" @ONLY)
list(APPEND websocketServer_src "${CMAKE_CURRENT_BINARY_DIR}/git_repo_sha1.cpp" ${PROJECT_SOURCE_DIR}/common/git_repo_sha1.h)

add_executable ( 		habdecWebsocketServer ${websocketServer_src} )
target_link_libraries( 	habdecWebsocketServer
							# atomic # uncomment under raspbian
							Decoder
							IQSource
							${Boost_LIBRARIES}
							cpr::cpr
							${PlatformSpecificLinking} )

install ( TARGETS habdecWebsocketServer DESTINATION ${CMAKE_INSTALL_PREFIX} )
install ( 	FILES ${PROJECT_SOURCE_DIR}/habLogger/habLogger.py ${PROJECT_SOURCE_DIR}/habLogger/fileLogger.sh
		PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
		DESTINATION  ${CMAKE_INSTALL_PREFIX} )


file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_loop.sh "#!/bin/sh\n while true; do ./habdecWebsocketServer --config ./habdecWebsocketServer.opts; done\n" )
install (       FILES ${CMAKE_CURRENT_BINARY_DIR}/run_loop.sh
                PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                DESTINATION  ${CMAKE_INSTALL_PREFIX} )

