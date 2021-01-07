find_path(IXWEBSOCKET_INCLUDE_DIR
		  NAMES ixwebsocket/IXWebSocket.h)

find_library(IXWEBSOCKET_LIBRARY
		     NAMES ixwebsocket
		     HINTS ${IXWEBSOCKET_LIBRARY_ROOT})

set(IXWEBSOCKET_LIBRARIES    ${IXWEBSOCKET_LIBRARY})
set(IXWEBSOCKET_INCLUDE_DIRS ${IXWEBSOCKET_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ixwebsocket
                                  FOUND_VAR IXWEBSOCKET_FOUND
                                  REQUIRED_VARS
                                    IXWEBSOCKET_LIBRARY
                                    IXWEBSOCKET_INCLUDE_DIR
                                  VERSION_VAR IXWEBSOCKET_VERSION)
