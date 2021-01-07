find_path(CPR_INCLUDE_DIR
		  NAMES cpr/cpr.h)

find_library(CPR_LIBRARY
		     NAMES cpr
		     HINTS ${CPR_LIBRARY_ROOT})

set(CPR_LIBRARIES 	 ${CPR_LIBRARY})
set(CPR_INCLUDE_DIRS ${CPR_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cpr
                                  FOUND_VAR CPR_FOUND
                                  REQUIRED_VARS
                                    CPR_LIBRARY
                                    CPR_INCLUDE_DIR
                                  VERSION_VAR CPR_VERSION)
