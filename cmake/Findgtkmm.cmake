set(GTKMM_LIBRARY_NAME gtkmm-3.0)
set(GDKMM_LIBRARY_NAME gdkmm-3.0)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GTKMM QUIET   ${GTKMM_LIBRARY_NAME})
    pkg_check_modules(PC_GDKMM QUIET   ${GDKMM_LIBRARY_NAME})
    pkg_check_modules(PC_PANGOMM QUIET ${PANGOMM_LIBRARY_NAME})
    set(GTKMM_DEFINITIONS              ${PC_GTKMM_CFLAGS_OTHER})
endif ()

find_package(gtk)
find_package(glibmm)
find_package(atkmm)
find_package(gdkmm)
find_package(sigc++)
find_package(pangomm)
find_package(cairomm)

set(GTKMM_VERSION         ${PC_GTKMM_VERSION})
set(GTKMM_INCLUDE_HINTS   ${PC_GTKMM_INCLUDEDIR}   ${PC_GTKMM_INCLUDE_DIRS})
set(GTKMM_LIBRARY_HINTS   ${PC_GTKMM_LIBDIR}       ${PC_GTKMM_LIBRARY_DIRS})
set(PANGOMM_INCLUDE_HINTS ${PC_PANGOMM_INCLUDEDIR} ${PC_PANGOMM_INCLUDE_DIRS})
set(GDKMM_INCLUDE_HINTS   ${PC_GTKMM_LIBDIR}       ${PC_GTKMM_LIBRARY_DIRS}
                          ${PC_GDKMM_INCLUDEDIR}   ${PC_GDKMM_INCLUDE_DIRS})

find_path(GTKMM_INCLUDE_DIR
          NAMES gtkmm.h
          HINTS ${GTKMM_INCLUDE_HINTS}
          PATH_SUFFIXES ${GTKMM_LIBRARY_NAME})

find_path(GTKMM_CONFIG_INCLUDE_DIR
          NAMES gtkmmconfig.h
          HINTS ${GTKMM_LIBRARY_HINTS}
          PATH_SUFFIXES ${GTKMM_LIBRARY_NAME}/include)

find_library(GTKMM_LIB
             NAMES ${GTKMM_LIBRARY_NAME}
                   gtkmm
             HINTS ${GTKMM_LIBRARY_HINTS}
             PATH_SUFFIXES ${GTKMM_LIBRARY_NAME}
                           ${GTKMM_LIBRARY_NAME}/include)

find_path(GDKMM_CONFIG_INCLUDE_DIR
          NAMES gdkmmconfig.h
          HINTS ${GDKMM_INCLUDE_HINTS}
          PATH_SUFFIXES ${GDKMM_LIBRARY_NAME}/include)

set(GTKMM_LIBRARIES    ${GTKMM_LIB};${gdkmm_LIBRARIES};${GTK_LIBRARIES};${GLIBMM_LIBRARIES};${PANGOMM_LIBRARIES};${CAIROMM_LIBRARIES};${ATKMM_LIBRARIES};${SIGC++_LIBRARIES})
set(GTKMM_INCLUDE_DIRS ${GTKMM_INCLUDE_DIR};${GTKMM_CONFIG_INCLUDE_DIR};${gdkmm_INCLUDE_DIRS};${gdkmm_CONFIG_INCLUDE_DIR};${GTK_INCLUDE_DIRS};${GLIBMM_INCLUDE_DIRS};${PANGOMM_INCLUDE_DIRS};${CAIROMM_INCLUDE_DIRS};${ATKMM_INCLUDE_DIRS};${SIGC++_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gtkmm
                                  REQUIRED_VARS
                                      GTKMM_LIB
                                      GTKMM_INCLUDE_DIRS
                                  VERSION_VAR GTKMM_VERSION)

mark_as_advanced(GTKMM_INCLUDE_DIR GTKMM_LIBRARY)
