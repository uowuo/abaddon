set(GDKPIXBUF_LIBRARY_NAME gdk_pixbuf-2.0)
set(GDKPIXBUF_PKGCONF_NAME gdk-pixbuf-2.0)

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GDKPIXBUF QUIET ${GDKPIXBUF_PKGCONF_NAME})
  set(GDKPIXBUF_DEfINITIONS ${PC_GDKPIXBUF_CFLAGS_OTHER})
endif (PKG_CONFIG_FOUND)

set(GDKPIXBUF_INCLUDE_HINTS ${PC_GDKPIXBUF_INCLUDEDIR} ${PC_GDKPIXBUF_INCLUDE_DIRS})
set(GDKPIXBUF_LIBRARY_HINTS ${PC_GDKPIXBUF_LIBDIR}     ${PC_GDKPIXBUF_LIBRARY_DIRS})

find_path(GDKPIXBUF_INCLUDE_DIR
          NAMES gdk-pixbuf/gdk-pixbuf.h
          HINTS ${GDKPIXBUF_INCLUDE_HINTS}
                /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${GDKPIXBUF_LIBRARY_NAME}
                        gtk-3.0)

find_library(GDKPIXBUF_LIBRARY
             NAMES ${GDKPIXBUF_LIBRARY_NAME}
             HINTS ${GDKPIXBUF_LIBRARY_HINTS}
                   /usr/lib
                   /usr/local/lib
                   /opt/local/lib
             PATH_SUFFIXES ${GDKPIXBUF_LIBRARY_NAME}
                           ${GDKPIXBUF_LIBRARY_NAME}/include)

set(GDKPIXBUF_LIBRARIES    ${GDKPIXBUF_LIBRARY})
set(GDKPIXBUF_INCLUDE_DIRS ${GDKPIXBUF_INCLUDE_DIR};${GDKPIXBUF_CONFIG_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gdkpixbuf
                                  REQUIRED_VARS
                                    GDKPIXBUF_LIBRARY
                                    GDKPIXBUF_INCLUDE_DIR
                                  VERSION_VAR GDKPIXBUF_VERSION)

mark_as_advanced(GDKPIXBUF_INCLUDE_DIR GDKPIXBUF_LIBRARY)
