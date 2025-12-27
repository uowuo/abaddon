function(add_imported_library library headers)
    add_library(uiohook::uiohook UNKNOWN IMPORTED)
    set_target_properties(uiohook::uiohook PROPERTIES
        IMPORTED_LOCATION "${library}"
        INTERFACE_INCLUDE_DIRECTORIES "${headers}"
    )

    set(uiohook_FOUND 1 CACHE INTERNAL "uiohook found" FORCE)
    set(uiohook_LIBRARIES "${library}" CACHE STRING "Path to uiohook library" FORCE)
    set(uiohook_INCLUDES "${headers}" CACHE STRING "Path to uiohook headers" FORCE)
    mark_as_advanced(FORCE uiohook_LIBRARIES)
    mark_as_advanced(FORCE uiohook_INCLUDES)
endfunction()

if(uiohook_LIBRARIES AND uiohook_INCLUDES)
    add_imported_library(${uiohook_LIBRARIES} ${uiohook_INCLUDES})
    return()
endif()

set(_uiohook_DIR "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/libuiohook")

find_library(uiohook_LIBRARY_PATH
    NAMES libuiohook uiohook
    PATHS
        "${_uiohook_DIR}/lib"
        /usr/lib
)

find_path(uiohook_HEADER_PATH
    NAMES uiohook.h
    PATHS
        "${_uiohook_DIR}/include"
        /usr/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    uiohook DEFAULT_MSG uiohook_LIBRARY_PATH uiohook_HEADER_PATH
)

if(uiohook_FOUND)
    add_imported_library(
        "${uiohook_LIBRARY_PATH}"
        "${uiohook_HEADER_PATH}"
    )
endif()
