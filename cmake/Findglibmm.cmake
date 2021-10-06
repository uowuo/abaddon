set(GLIBMM_LIBRARY_NAME glibmm-2.4)
set( GIOMM_LIBRARY_NAME  giomm-2.4)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GLIBMM QUIET ${GLIBMM_LIBRARY_NAME})
    pkg_check_modules(PC_GIOMM  QUIET  ${GIOMM_LIBRARY_NAME})
    set(GLIBMM_DEFINITIONS ${PC_GLIBMM_CFLAGS_OTHER})
endif()

set(GLIBMM_VERSION       ${PC_GLIBMM_VERSION})
set(GLIBMM_INCLUDE_HINTS ${PC_GLIBMM_INCLUDEDIR} ${PC_GLIBMM_INCLUDE_DIRS})
set(GLIBMM_LIBRARY_HINTS ${PC_GLIBMM_LIBDIR}     ${PC_GLIBMM_LIBRARY_DIRS})
set( GIOMM_INCLUDE_HINTS ${PC_GIOMM_INCLUDEDIR}  ${PC_GIOMM_INCLUDE_DIRS})
set( GIOMM_LIBRARY_HINTS ${PC_GIOMM_LIBDIR}      ${PC_GIOMM_LIBRARY_DIRS})

find_path(GLIBMM_INCLUDE_DIR
          NAMES glibmm.h
          HINTS ${GLIBMM_INCLUDE_HINTS}
          PATH_SUFFIXES ${GLIBMM_LIBRARY_NAME})

find_path(GLIBMM_CONFIG_INCLUDE_DIR
          NAMES glibmmconfig.h
          HINTS ${GLIBMM_LIBRARY_HINTS}
          PATH_SUFFIXES ${GLIBMM_LIBRARY_NAME}/include)

find_library(GLIBMM_LIBRARY
             NAMES ${GLIBMM_LIBRARY_NAME}
                   glibmm
                   glibmm-2.0
                   glibmm-2.4
             HINTS ${GLIBMM_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${GLIBMM_LIBRARY_NAME}
                           ${GLIBMM_LIBRARY_NAME}/include)

find_path(GIOMM_INCLUDE_DIR
          NAMES giomm.h
          PATH_SUFFIXES ${GIOMM_LIBRARY_NAME})

find_path(GIOMM_CONFIG_INCLUDE_DIR
          NAMES giommconfig.h
          HINTS ${GIOMM_LIBRARY_HINTS}
          PATH_SUFFIXES ${GIOMM_LIBRARY_NAME}
                        ${GIOMM_LIBRARY_NAME}/include)

find_library(GIOMM_LIBRARY
             NAMES ${GIOMM_LIBRARY_NAME}
                   giomm
             HINTS ${GIOMM_INCLUDE_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${GIOMM_LIBRARY_NAME}/include)

set(GLIBMM_LIBRARIES   	${GLIBMM_LIBRARY};${GIOMM_LIBRARY};${GLIB_LIBRARIES};${SIGC++_LIBRARIES})
set(GLIBMM_INCLUDE_DIRS	${GLIBMM_INCLUDE_DIR};${GLIBMM_CONFIG_INCLUDE_DIR};${GIOMM_INCLUDE_DIR};${GLIB_INCLUDE_DIRS};${SIGC++_INCLUDE_DIRS};${GIOMM_CONFIG_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(glibmm
                                  REQUIRED_VARS
                                    GLIBMM_LIBRARY
                                    GLIBMM_INCLUDE_DIR
                                  VERSION_VAR GLIBMM_VERSION)

mark_as_advanced(GLIBMM_INCLUDE_DIR GLIBMM_LIBRARY)
