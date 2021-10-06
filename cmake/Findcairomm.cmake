set(CAIROMM_LIBRARY_NAME cairomm-1.0)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CAIROMM QUIET ${CAIROMM_LIBRARY_NAME})
  set(CAIROMM_DEfINITIONS ${PC_CAIROMM_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(CAIROMM_INCLUDE_HINTS ${PC_CAIROMM_INCLUDEDIR} ${PC_CAIROMM_INCLUDE_DIRS})
set(CAIROMM_LIBRARY_HINTS ${PC_CAIROMM_LIBDIR}     ${PC_CAIROMM_LIBRARY_DIRS})

find_path(CAIROMM_INCLUDE_DIR
          NAMES cairomm/cairomm.h
          HINTS ${CAIROMM_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${CAIROMM_LIBRARY_NAME})

find_path(CAIROMM_CONFIG_INCLUDE_DIR
          NAMES cairommconfig.h
          HINTS ${CAIROMM_INCLUDE_HINTS}
                /usr/lib
                /usr/local/lib
                /opt/local/lib
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${CAIROMM_LIBRARY_NAME}
                        ${CAIROMM_LIBRARY_NAME}/include
                        cairomm
                        cairomm/include)

find_library(CAIROMM_LIBRARY
             NAMES ${CAIROMM_LIBRARY_NAME}
             HINTS ${CAIROMM_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${CAIROMM_LIBRARY_NAME}
                           ${CAIROMM_LIBRARY_NAME}/include)

set(CAIROMM_LIBRARIES    ${CAIROMM_LIBRARY})
set(CAIROMM_INCLUDE_DIRS ${CAIROMM_INCLUDE_DIR};${CAIROMM_CONFIG_INCLUDE_DIRS};${CAIROMM_CONFIG_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cairomm
                                  REQUIRED_VARS
                                    CAIROMM_LIBRARY
                                    CAIROMM_INCLUDE_DIR
                                  VERSION_VAR CAIROMM_VERSION)

mark_as_advanced(CAIROMM_INCLUDE_DIR CAIROMM_LIBRARY)
