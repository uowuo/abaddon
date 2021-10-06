set(PANGO_LIBRARY_NAME pango-1.0)
set(PANGOCAIRO_LIBRARY_NAME pangocairo-1.0)
set(PANGOFT2_LIBRARY_NAME pangoft2-1.0)

find_package(HarfBuzz)
find_package(cairo)
find_package(Freetype)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PANGO QUIET ${PANGO_LIBRARY_NAME})
  set(PANGO_DEFINITIONS ${PC_PANGO_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(PANGO_INCLUDE_HINTS ${PC_PANGO_INCLUDEDIR} ${PC_PANGO_INCLUDE_DIRS})
set(PANGO_LIBRARY_HINTS ${PC_PANGO_LIBDIR}     ${PC_PANGO_LIBRARY_DIRS})

find_path(PANGO_INCLUDE_DIR
          NAMES pango/pango.h
          HINTS ${PANGO_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${PANGO_LIBRARY_NAME}
                        ${PANGO_LIBRARY_NAME}/include
                        pango)

find_path(PANGO_CONFIG_INCLUDE_DIR
          NAMES pangoconfig.h
                pango/pangoconfig.h
          HINTS ${PANGO_LIBRARY_HINTS}
                /usr/lib
                /usr/local/lib
                /opt/local/lib
          PATH_SUFFIXES ${PANGO_LIBRARY_NAME}/include)

find_library(PANGO_LIBRARY
             NAMES ${PANGO_LIBRARY_NAME}
                   pango
             HINTS ${PANGO_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${PANGO_LIBRARY_NAME}
                           ${PANGO_LIBRARY_NAME}/include)

find_library(PANGOCAIRO_LIBRARY
             NAMES ${PANGOCAIRO_LIBRARY_NAME}
                   pangocairo
             HINTS ${PANGO_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${PANGO_LIBRARY_NAME}
                           ${PANGO_LIBRARY_NAME}/include)

find_library(PANGOFT2_LIBRARY
             NAMES ${PANGOFT2_LIBRARY_NAME}
                   pangoft2
             HINTS ${PANGO_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${PANGO_LIBRARY_NAME}
                           ${PANGO_LIBRARY_NAME}/include)

set(PANGO_LIBRARIES    ${PANGO_LIBRARY};${HARFBUZZ_LIBRARIES};${CAIRO_LIBRARIES};${FREETYPE_LIBRARIES};${PANGOCAIRO_LIBRARY};${PANGOFT2_LIBRARY})
set(PANGO_INCLUDE_DIRS ${PANGO_INCLUDE_DIR};${PANGO_CONFIG_INCLUDE_DIRS};${HARFBUZZ_INCLUDE_DIR};${CAIRO_INCLUDE_DIRS};${FREETYPE_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pango
                                  REQUIRED_VARS
                                    PANGO_LIBRARY
                                    PANGO_INCLUDE_DIR
                                  VERSION_VAR PANGO_VERSION)

mark_as_advanced(PANGO_INCLUDE_DIR PANGO_LIBRARY)
