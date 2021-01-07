set(GDKMM_LIBRARY_NAME gdkmm-3.0)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(PKGCONFIG_GDKMM QUIET ${GDKMM_LIBRARY_NAME})
	set(GDKMM_DEFINITIONS ${PKGCONFIG_GDKMM_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(GDKMM_INCLUDE_HINTS ${PKGCONFIG_GDKMM_INCLUDEDIR} ${PKGCONFIG_GDKMM_INCLUDE_DIRS})
set(GDKMM_LIBRARY_HINTS ${PKGCONFIG_GDKMM_LIBDIR}     ${PKGCONFIG_GDKMM_LIBRARY_DIRS})

find_path(GDKMM_INCLUDE_DIR
          NAMES gdkmm.h
          HINTS ${GDKMM_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${GDKMM_LIBRARY_NAME})

find_path(GDKMM_CONFIG_INCLUDE_DIR
          NAMES gdkmmconfig.h
          HINTS ${GDKMM_LIBRARY_HINTS}
                /usr/lib
                /usr/local/lib
                /opt/local/lib
          PATH_SUFFIXES ${GDKMM_LIBRARY_NAME}/include)

find_library(GDKMM_LIBRARY
             NAMES ${GDKMM_LIBRARY_NAME}
                  gdkmm
             HINTS ${GDKMM_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${GDKMM_LIBRARY_NAME}
                           ${GDKMM_LIBRARY_NAME}/include)

set(GDKMM_LIBRARIES    ${GDKMM_LIBRARY})
set(GDKMM_INCLUDE_DIRS ${GDKMM_INCLUDE_DIR};${GDKMM_CONFIG_INCLUDE_DIRS};${GDKMM_CONFIG_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gdkmm
                                  FOUND_VAR GDKMM_FOUND
                                  REQUIRED_VARS
                                    GDKMM_LIBRARY
                                    GDKMM_INCLUDE_DIRS
                                  VERSION_VAR GDKMM_VERSION)

mark_as_advanced(GDKMM_INCLUDE_DIR GDKMM_LIBRARY)
