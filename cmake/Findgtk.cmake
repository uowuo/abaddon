set(GTK_LIBRARY_NAME   gtk-3.0)
set(GTK_PKGCONFIG_NAME gtk+-3.0)

find_package(glib REQUIRED)
find_package(gdkpixbuf)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GTK QUIET ${GTK_PKGCONFIG_NAME})
    set(GTK_DEFINITIONS ${PC_GTK_CFLAGS_OTHER})
endif()

set(GTK_INCLUDE_HINTS ${PC_GTK_INCLUDEDIR} ${PC_GTK_INCLUDE_DIRS})
set(GTK_LIBRARY_HINTS ${PC_GTK_LIBDIR} ${PC_GTK_LIBRARY_DIRS})

find_path(GTK_INCLUDE_DIR
          NAMES gtk/gtk.h
          HINTS ${GTK_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${GTK_LIBRARY_NAME})

#find_path(GDK_CONFIG_INCLUDE_DIR
#         NAMES gdkconfig.h
#         HINTS ${GTK_LIBRARY_HINTS}
#               /usr/lib
#               /usr/local/lib
#               /opt/local/lib
#               /usr/include
#               /usr/local/include
#               /opt/local/include
#         PATH_SUFFIXES ${GTK_LIBRARY_NAME}/include
#                       ${GTK_LIBRARY_NaME}/gdk)

find_library(GTK_LIBRARY
             NAMES gtk-3.0
                   gtk-3
                   gtk
             HINTS ${GTK_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib)

set(GTK_LIBRARIES    ${GTK_LIBRARY};${GDKPIXBUF_LIBRARIES})
set(GTK_INCLUDE_DIRS ${GTK_INCLUDE_DIR};${GDK_CONFIG_INCLUDE_DIR};${GDKPIXBUF_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gtk
                                  REQUIRED_VARS
                                    GTK_LIBRARY
                                    GTK_INCLUDE_DIR
                                  VERSION_VAR GTK_VERSION)

mark_as_advanced(GTK_INCLUDE_DIR GTK_LIBRARY)
