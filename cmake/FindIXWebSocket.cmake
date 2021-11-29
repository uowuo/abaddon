set(IXWebSocket_LIBRARY_NAME ixwebsocket)

find_path(IXWebSocket_INCLUDE_DIR
          NAMES ixwebsocket/IXWebSocket.h
          HINTS /usr/include
                /usr/local/include
                /opt/local/include
          PATH_SUFFIXES ${IXWebSocket_LIBRARY_NAME})


find_library(IXWebSocket_LIBRARY
             NAMES ${IXWebSocket_LIBRARY_NAME}
             PATH_SUFFIXES ${IXWebSocket_LIBRARY_NAME}
                           ${IXWebSocket_LIBRARY_NAME}/include)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(IXWebSocket
                                  REQUIRED_VARS
                                    IXWebSocket_LIBRARY
                                    IXWebSocket_INCLUDE_DIR)


mark_as_advanced(IXWebSocket_LIBRARY IXWebSocket_INCLUDE_DIR)

if (IXWebSocket_FOUND)
    find_package(OpenSSL QUIET)
    set(IXWebSocket_INCLUDE_DIRS "${IXWebSocket_INCLUDE_DIR};${OPENSSL_INCLUDE_DIR}")
    set(IXWebSocket_LIBRARIES "${IXWebSocket_LIBRARY};${OPENSSL_LIBRARIES}")
endif()
