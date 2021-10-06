set(SIGC++_LIBRARY_NAME sigc++-2.0)
set(SIGC++_LIBRARY_FILE sigc-2.0)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_SIGC++ QUIET ${SIGC++_LIBRARY_NAME})
    set(SIGC++_DEFINITIONS ${PC_SIGC++_CFLAGS_OTHER})
endif()

set(SIGC++_VERSION       ${PC_SIGC++_VERSION})
set(SIGC++_INCLUDE_HINTS ${PC_SIGC++_INCLUDEDIR}  ${PC_SIGC++_INCLUDE_DIRS})
set(SIGC++_LIBRARY_HINTS ${PC_SIGC++_LIBDIR}      ${PC_SIGC++_LIBRARY_DIRS})

find_path(SIGC++_INCLUDE_DIR
          NAMES sigc++/sigc++.h
          HINTS ${SIGC++_INCLUDE_HINTS}
          PATH_SUFFIXES ${SIGC++_LIBRARY_NAME})

find_path(SIGC++_CONFIG_INCLUDE_DIR
          NAMES sigc++config.h
          HINTS ${SIGC++_LIBRARY_HINTS}
          PATH_SUFFIXES ${SIGC++_LIBRARY_NAME}/include)

find_library(SIGC++_LIBRARY
             NAMES ${SIGC++_LIBRARY_FILE}
             HINTS $${SIGC++_LIBRARY_HINTS}
             PATH_SUFFIXES ${SIGC++_LIBRARY_NAME}
                           ${SIGC++_LIBRARY_NAME}/include)

set(SIGC++_LIBRARIES    ${SIGC++_LIBRARY})
set(SIGC++_INCLUDE_DIRS ${SIGC++_INCLUDE_DIR};${SIGC++_CONFIG_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(sigc++
                                  REQUIRED_VARS
                                    SIGC++_INCLUDE_DIR
                                    SIGC++_LIBRARY
                                  VERSION_VAR SIGC++_VERSION)

mark_as_advanced(SIGC++_INCLUDE_DIR SIGC++_LIBRARY)
