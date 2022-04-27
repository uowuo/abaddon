set(gdkmm_LIBRARY_NAME gdkmm-3.0)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PKGCONFIG_gdkmm QUIET ${gdkmm_LIBRARY_NAME})
    set(gdkmm_DEFINITIONS                   ${PKGCONFIG_gdkmm_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(gdkmm_INCLUDE_HINTS ${PKGCONFIG_gdkmm_INCLUDEDIR} ${PKGCONFIG_gdkmm_INCLUDE_DIRS})
set(gdkmm_LIBRARY_HINTS ${PKGCONFIG_gdkmm_LIBDIR}     ${PKGCONFIG_gdkmm_LIBRARY_DIRS})

find_path(gdkmm_INCLUDE_DIR
          NAMES gdkmm.h
          HINTS ${gdkmm_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${gdkmm_LIBRARY_NAME})

find_path(gdkmm_CONFIG_INCLUDE_DIR
          NAMES gdkmmconfig.h
          HINTS ${gdkmm_LIBRARY_HINTS}
                /usr/lib
                /usr/local/lib
                /opt/local/lib
          PATH_SUFFIXES ${gdkmm_LIBRARY_NAME}/include)

find_library(gdkmm_LIBRARY
             NAMES ${gdkmm_LIBRARY_NAME}
                   gdkmm
             HINTS ${gdkmm_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${gdkmm_LIBRARY_NAME}
                           ${gdkmm_LIBRARY_NAME}/include)

find_package(gdk)

set(gdkmm_LIBRARIES    ${gdkmm_LIBRARY};${gdk_LIBRARIES})
set(gdkmm_INCLUDE_DIRS ${gdkmm_INCLUDE_DIR};${gdkmm_CONFIG_INCLUDE_DIRS};${gdkmm_CONFIG_INCLUDE_DIR};${gdk_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gdkmm
                                  REQUIRED_VARS
                                      gdkmm_LIBRARY
                                      gdkmm_INCLUDE_DIRS
                                  VERSION_VAR gdkmm_VERSION)

mark_as_advanced(gdkmm_INCLUDE_DIR gdkmm_LIBRARY)
