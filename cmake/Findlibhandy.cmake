set(libhandy_LIBRARY_NAME libhandy-1)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_libhandy QUIET ${libhandy_LIBRARY_NAME})
  set(libhandy_DEfINITIONS ${PC_libhandy_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(libhandy_INCLUDE_HINTS ${PC_libhandy_INCLUDEDIR} ${PC_libhandy_INCLUDE_DIRS})
set(libhandy_LIBRARY_HINTS ${PC_libhandy_LIBDIR}     ${PC_libhandy_LIBRARY_DIRS})

find_path(libhandy_INCLUDE_DIR
          NAMES handy.h
          HINTS ${libhandy_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${libhandy_LIBRARY_NAME})

find_library(libhandy_LIBRARY
             NAMES ${libhandy_LIBRARY_NAME} handy-1
             HINTS ${libhandy_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${libhandy_LIBRARY_NAME}
                           ${libhandy_LIBRARY_NAME}/include)

set(libhandy_LIBRARIES    ${libhandy_LIBRARY})
set(libhandy_INCLUDE_DIRS ${libhandy_INCLUDE_DIR};${libhandy_CONFIG_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libhandy
                                  REQUIRED_VARS
                                    libhandy_LIBRARY
                                    libhandy_INCLUDE_DIR
                                  VERSION_VAR libhandy_VERSION)

mark_as_advanced(libhandy_INCLUDE_DIR libhandy_LIBRARY)
