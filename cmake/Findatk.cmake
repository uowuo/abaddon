set(ATK_LIBRARY_NAME atk-1.0)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_ATK QUIET atk)
  set(ATK_DEfINITIONS ${PC_ATK_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(ATK_INCLUDE_HINTS ${PC_ATK_INCLUDEDIR} ${PC_ATK_INCLUDE_DIRS})
set(ATK_LIBRARY_HINTS ${PC_ATK_LIBDIR}     ${PC_ATK_LIBRARY_DIRS})

find_path(ATK_INCLUDE_DIR
          NAMES atk/atk.h
          HINTS ${ATK_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${ATK_LIBRARY_NAME})

find_library(ATK_LIBRARY
             NAMES ${ATK_LIBRARY_NAME}
             HINTS ${ATK_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${ATK_LIBRARY_NAME}
                           ${ATK_LIBRARY_NAME}/include)

set(ATK_LIBRARIES    ${ATK_LIBRARY})
set(ATK_INCLUDE_DIRS ${ATK_INCLUDE_DIR};${ATK_CONFIG_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(atk
                                  REQUIRED_VARS
                                    ATK_LIBRARY
                                    ATK_INCLUDE_DIR
                                  VERSION_VAR ATK_VERSION)

mark_as_advanced(ATK_INCLUDE_DIR ATK_LIBRARY)
