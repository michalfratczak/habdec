cmake_minimum_required( VERSION 3.8.2 )

project (habdec)
set ( CMAKE_CXX_STANDARD 11 )

# set ( CMAKE_BUILD_TYPE "Debug" )
# set ( CMAKE_BUILD_TYPE "Release" )
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if ( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
    set ( CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install dir" FORCE)
endif()
message ( "CMAKE_INSTALL_PREFIX: " ${CMAKE_INSTALL_PREFIX} )


add_subdirectory("Decoder")
add_subdirectory("IQSource")
add_subdirectory("ssdv_build")
add_subdirectory("websocketServer")

option( fltkGUI "fltkGUI" OFF )
if(fltkGUI)
	add_subdirectory("fltkGUI")
endif()
