set(simpleini_LIBRARY_NAME simpleini)

find_path(simpleini_INCLUDE_DIR
          NAMES SimpleIni.h
          HINTS /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${simpleini_LIBRARY_NAME})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(simpleini
                                  REQUIRED_VARS
                                    simpleini_INCLUDE_DIR)

mark_as_advanced(simpleini_INCLUDE_DIR)
