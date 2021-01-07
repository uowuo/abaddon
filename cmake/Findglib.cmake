find_package(PkgConfig)
pkg_check_modules(PC_GLIB2 QUIET glib-2.0)

find_path(GLIB_INCLUDE_DIR
          NAMES glib.h
          HINTS ${PC_GLIB2_INCLUDEDIR}
                ${PC_GLIB2_INCLUDE_DIRS}
                $ENV{GLIB2_HOME}/include
                $ENV{GLIB2_ROOT}/include
                /usr/local/include
                /usr/include
                /glib2/include
                /glib-2.0/include
          PATH_SUFFIXES glib2 glib-2.0 glib-2.0/include
)
set(GLIB_INCLUDE_DIRS ${GLIB_INCLUDE_DIR})

find_library(GLIB_LIBRARIES
             NAMES glib2
                   glib-2.0
             HINTS ${PC_GLIB2_LIBDIR}
                   ${PC_GLIB2_LIBRARY_DIRS}
                   $ENV{GLIB2_HOME}/lib
                   $ENV{GLIB2_ROOT}/lib
                   /usr/local/lib
                   /usr/lib
                   /lib
                   /glib-2.0/lib
             PATH_SUFFIXES glib2 glib-2.0
)

get_filename_component(_GLIB2_LIB_DIR "${GLIB_LIBRARIES}" PATH)
find_path(GLIB_CONFIG_INCLUDE_DIR
    NAMES glibconfig.h
    HINTS ${PC_GLIB2_INCLUDEDIR}
          ${PC_GLIB2_INCLUDE_DIRS}
          $ENV{GLIB2_HOME}/include
          $ENV{GLIB2_ROOT}/include
          /usr/local/include
          /usr/include
          /glib2/include
          /glib-2.0/include
          ${_GLIB2_LIB_DIR}
          ${CMAKE_SYSTEM_LIBRARY_PATH}
    PATH_SUFFIXES glib2 glib-2.0 glib-2.0/include
)
if (GLIB_CONFIG_INCLUDE_DIR)
    set(GLIB_INCLUDE_DIRS ${GLIB_INCLUDE_DIRS} ${GLIB_CONFIG_INCLUDE_DIR})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(glib
                                  FOUND_VAR GLIB_FOUND
                                  REQUIRED_VARS
                                    GLIB_LIBRARIES
                                    GLIB_INCLUDE_DIRS
                                  VERSION_VAR GLIB_VERSION)
mark_as_advanced(GLIB_INCLUDE_DIR GLIB_CONFIG_INCLUDE_DIR)
