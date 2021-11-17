set(simpleini_LIBRARY_NAME simpleini)

find_path(simpleini_INCLUDE_DIR
          NAMES SimpleIni.h
          HINTS /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${simpleini_LIBRARY_NAME})

find_library(simpleini_LIBRARY
             NAMES ${simpleini_LIBRARY_NAME}
             HINTS /usr/lib
                   /usr/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(simpleini
                                  REQUIRED_VARS
                                    simpleini_INCLUDE_DIR
                                    simpleini_LIBRARY
                                    )

if (${simpleini_FOUND})
  add_library(simpleini::simpleini UNKNOWN IMPORTED)
  set_target_properties(simpleini::simpleini
                        PROPERTIES
                          INTERFACE_INCLUDE_DIRECTORIES ${simpleini_INCLUDE_DIR}
                          IMPORTED_LOCATION ${simpleini_LIBRARY})
endif()

mark_as_advanced(simpleini_INCLUDE_DIR)
