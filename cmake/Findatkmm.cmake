set(ATKMM_LIBRARY_NAME atkmm-1.6)

find_package(atk)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(PKGCONFIG_ATKMM QUIET ${ATKMM_LIBRARY_NAME})
	set(ATKMM_DEFINITIONS ${PKGCONFIG_ATKMM_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(ATKMM_INCLUDE_HINTS ${PKGCONFIG_ATKMM_INCLUDEDIR} ${PKGCONFIG_ATKMM_INCLUDE_DIRS})
set(ATKMM_LIBRARY_HINTS ${PKGCONFIG_ATKMM_LIBDIR}     ${PKGCONFIG_ATKMM_LIBRARY_DIRS})

find_path(ATKMM_INCLUDE_DIR
          NAMES atkmm.h
          HINTS ${ATKMM_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${ATKMM_LIBRARY_NAME})

find_path(ATKMM_CONFIG_INCLUDE_DIR
          NAMES atkmmconfig.h
          HINTS ${ATKMM_LIBRARY_HINTS}
                /usr/lib
                /usr/local/lib
                /opt/local/lib
          PATH_SUFFIXES ${ATKMM_LIBRARY_NAME}/include)

find_library(ATKMM_LIBRARY
             NAMES ${ATKMM_LIBRARY_NAME}
                   atkmm
             HINTS ${ATKMM_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${ATKMM_LIBRARY_NAME}
                           ${ATKMM_LIBRARY_NAME}/include)

set(ATKMM_LIBRARIES    ${ATKMM_LIBRARY};${ATK_LIBRARIES})
set(ATKMM_INCLUDE_DIRS ${ATKMM_INCLUDE_DIR};${ATKMM_CONFIG_INCLUDE_DIR};${ATK_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(atkmm
                                  REQUIRED_VARS
                                    ATKMM_LIBRARY
                                    ATKMM_INCLUDE_DIRS
                                  VERSION_VAR ATKMM_VERSION)

mark_as_advanced(ATKMM_INCLUDE_DIR ATKMM_LIBRARY)
