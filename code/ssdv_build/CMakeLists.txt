
set ( ssdv_lib_src
    ${PROJECT_SOURCE_DIR}/ssdv/rs8.c
    ${PROJECT_SOURCE_DIR}/ssdv/ssdv.c
)
set_source_files_properties ( ${ssdv_lib_src} LANGUAGE "C"  )
add_library( ssdv_lib  ${ssdv_lib_src} )


if(NOT WIN32)
    set ( ssdv_exe_src
        ${PROJECT_SOURCE_DIR}/ssdv/rs8.c
        ${PROJECT_SOURCE_DIR}/ssdv/ssdv.c
        ${PROJECT_SOURCE_DIR}/ssdv/main.c
    )
    set_source_files_properties ( ${ssdv_exe_src} LANGUAGE "C"  )
    add_executable( ssdv  ${ssdv_exe_src} )
ENDIF(WIN32)
