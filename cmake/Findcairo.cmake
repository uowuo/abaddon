set(CAIRO_LIBRARY_NAME cairo)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CAIRO QUIET ${CAIRO_LIBRARY_NAME})
  set(CAIRO_DEfINITIONS ${PC_CAIRO_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(CAIRO_INCLUDE_HINTS ${PC_CAIRO_INCLUDEDIR} ${PC_CAIRO_INCLUDE_DIRS})
set(CAIRO_LIBRARY_HINTS ${PC_CAIRO_LIBDIR}     ${PC_CAIRO_LIBRARY_DIRS})

find_path(CAIRO_INCLUDE_DIR
          NAMES cairo.h
          HINTS ${CAIRO_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${CAIRO_LIBRARY_NAME})

find_library(CAIRO_LIBRARY
             NAMES ${CAIRO_LIBRARY_NAME}
             HINTS ${CAIRO_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${CAIRO_LIBRARY_NAME}
                           ${CAIRO_LIBRARY_NAME}/include)

set(CAIRO_LIBRARIES    ${CAIRO_LIBRARY})
set(CAIRO_INCLUDE_DIRS ${CAIRO_INCLUDE_DIR};${CAIRO_CONFIG_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cairo
                                  REQUIRED_VARS
                                    CAIRO_LIBRARY
                                    CAIRO_INCLUDE_DIR
                                  VERSION_VAR CAIRO_VERSION)

mark_as_advanced(CAIRO_INCLUDE_DIR CAIRO_LIBRARY)
