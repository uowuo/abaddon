set(PANGOMM_LIBRARY_NAME pangomm-1.4)

find_package(pango REQUIRED)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(PC_PANGOMM QUIET ${PANGOMM_LIBRARY_NAME})
	set(PANGOMM_DEFINITIONS ${PC_PANGOMM_CFLAGS_OTHER})
endif()

set(PANGOMM_INCLUDE_HINTS ${PC_PANGOMM_INCLUDEDIR} ${PC_PANGOMM_INCLUDE_DIRS})
set(PANGOMM_LIBRARY_HINTS ${PC_PANGOMM_LIBDIR}     ${PC_PANGOMM_LIBRARY_DIRS})

find_path(PANGOMM_INCLUDE_DIR
          NAMES pangomm.h
          HINTS ${PANGOMM_INCLUDE_HINTS}
                /usr/lib
                /usr/local/lib
                /opt/local/lib
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${PANGOMM_LIBRARY_NAME}
                        ${PANGOMM_LIBRARY_NAME}/include
                        pangomm
                        pangomm/include)

find_path(PANGOMM_CONFIG_INCLUDE_DIR
          NAMES pangommconfig.h
          HINTS ${PANGOMM_LIBRARY_HINTS}
                /usr/lib
                /usr/local/lib
                /opt/local/lib
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${PANGOMM_LIBRARY_NAME}
                        ${PANGOMM_LIBRARY_NAME}/include
                        pangomm
                        pangomm/include)

find_library(PANGOMM_LIBRARY
             NAMES ${PANGOMM_LIBRARY_NAME}
                   pangomm
             HINTS ${PANGOMM_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${PANGO_LIBRARY_NAME}
                           ${PANGO_LIBRARY_NAME}/include
                           pangomm
                           pangomm/include)

set(PANGOMM_LIBRARIES    ${PANGOMM_LIBRARY};${PANGO_LIBRARIES})
set(PANGOMM_INCLUDE_DIRS ${PANGOMM_INCLUDE_DIR};${PANGOMM_CONFIG_INCLUDE_DIR};${PANGO_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pangomm
                                  REQUIRED_VARS
                                    PANGOMM_LIBRARY
                                    PANGOMM_INCLUDE_DIRS
                                  VERSION_VAR PANGOMM_VERSION)

mark_as_advanced(PANGOMM_INCLUDE_DIR PANGOMM_LIBRARY)
