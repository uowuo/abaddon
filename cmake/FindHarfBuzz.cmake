set(HARFBUZZ_LIBRARY_NAME harfbuzz)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_HARFBUZZ QUIET ${HARFBUZZ_LIBRARY_NAME})
  set(HARFBUZZ_DEfINITIONS ${PC_HARFBUZZ_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(HARFBUZZ_INCLUDE_HINTS ${PC_HARFBUZZ_INCLUDEDIR} ${PC_HARFBUZZ_INCLUDE_DIRS})
set(HARFBUZZ_LIBRARY_HINTS ${PC_HARFBUZZ_LIBDIR}     ${PC_HARFBUZZ_LIBRARY_DIRS})

find_path(HARFBUZZ_INCLUDE_DIR
          NAMES hb.h
          HINTS ${HARFBUZZ_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${HARFBUZZ_LIBRARY_NAME})

find_library(HARFBUZZ_LIBRARY
             NAMES ${HARFBUZZ_LIBRARY_NAME}
             HINTS ${HARFBUZZ_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${HARFBUZZ_LIBRARY_NAME}
                           ${HARFBUZZ_LIBRARY_NAME}/include)

set(HARFBUZZ_LIBRARIES    ${HARFBUZZ_LIBRARY})
set(HARFBUZZ_INCLUDE_DIRS ${HARFBUZZ_INCLUDE_DIR};${HARFBUZZ_CONFIG_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HarfBuzz
                                  REQUIRED_VARS
                                    HARFBUZZ_LIBRARY
                                    HARFBUZZ_INCLUDE_DIR
                                  VERSION_VAR HARFBUZZ_VERSION)

mark_as_advanced(HARFBUZZ_INCLUDE_DIR HARFBUZZ_LIBRARY)
