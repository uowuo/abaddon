function(add_imported_library library headers)
    add_library(rnnoise::rnnoise UNKNOWN IMPORTED)
    set_target_properties(rnnoise::rnnoise PROPERTIES
        IMPORTED_LOCATION ${library}
        INTERFACE_INCLUDE_DIRECTORIES ${headers}
    )

    set(rnnoise_FOUND 1 CACHE INTERNAL "rnnoise found" FORCE)
    set(rnnoise_LIBRARIES ${library}
        CACHE STRING "Path to rnnoise library" FORCE)
    set(rnnoise_INCLUDES ${headers}
        CACHE STRING "Path to rnnoise headers" FORCE)
    mark_as_advanced(FORCE rnnoise_LIBRARIES)
    mark_as_advanced(FORCE rnnoise_INCLUDES)
endfunction()

if (rnnoise_LIBRARIES AND rnnoise_INCLUDES)
    add_imported_library(${rnnoise_LIBRARIES} ${rnnoise_INCLUDES})
    return()
endif()

file(TO_CMAKE_PATH "$ENV{rnnoise_DIR}" _rnnoise_DIR)
find_library(rnnoise_LIBRARY_PATH
             NAMES librnnoise rnnoise
             PATHS
                ${_rnnoise_DIR}/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                /usr/lib
)

find_path(rnnoise_HEADER_PATH
          NAMES rnnoise.h
          PATHS
            ${_rnnoise_DIR}/include
            /usr/include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    rnnoise DEFAULT_MSG rnnoise_LIBRARY_PATH rnnoise_HEADER_PATH
)

if (rnnoise_FOUND)
    add_imported_library(
        "${rnnoise_LIBRARY_PATH};${rnnoise_LIBRARIES}"
        "${rnnoise_HEADER_PATH};${rnnoise_INCLUDE_DIRECTORIES}"
    )
endif()

