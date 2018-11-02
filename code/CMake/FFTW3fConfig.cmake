
# defined since 2.8.3
if (CMAKE_VERSION VERSION_LESS 2.8.3)
  get_filename_component (CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
endif ()

# Allows loading FFTW3 settings from another project
set (FFTW3_CONFIG_FILE "${CMAKE_CURRENT_LIST_FILE}")

# set (FFTW3f_LIBRARIES fftw3f.a )
FIND_LIBRARY( FFTW3f_LIBRARIES NAMES fftw3f.a fftw3f PATHS ${FFTW_ROOT}/lib NO_DEFAULT_PATH  )
set (FFTW3f_LIBRARY_DIRS ${FFTW_ROOT}/lib)
set (FFTW3f_INCLUDE_DIRS ${FFTW_ROOT}/include)

if (CMAKE_VERSION VERSION_LESS 2.8.3)
  set (CMAKE_CURRENT_LIST_DIR)
endif ()
