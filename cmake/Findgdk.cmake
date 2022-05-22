find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_gdk QUIET gdk-3.0)
    set(gdk_DEFINITIONS ${PC_gdk_CFLAGS_OTHER})
endif ()

set(gdk_INCLUDE_HINTS ${PC_gdk_INCLUDEDIR} ${PC_gdk_INCLUDE_DIRS})
set(gdk_LIBRARY_HINTS ${PC_gdk_LIBDIR} ${PC_gdk_LIBRARY_DIRS})

find_path(gdk_INCLUDE_DIR
          NAMES gdk/gdk.h
          HINTS ${gdk_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES gdk-3.0)

find_library(gdk_LIBRARY
        NAMES gdk-3.0
              gdk-3
              gdk
        HINTS ${gdk_LIBRARY_HINTS}
              /usr/lib
              /usr/local/lib
              /opt/local/lib)

set(gdk_LIBRARIES ${gdk_LIBRARY})
set(gdk_INCLUDE_DIRS ${gdk_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gdk
        REQUIRED_VARS
            gdk_LIBRARY
            gdk_INCLUDE_DIR
        VERSION_VAR gdk_VERSION)

mark_as_advanced(gdk_INCLUDE_DIR gdk_LIBRARY)
