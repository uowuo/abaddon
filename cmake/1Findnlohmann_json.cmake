set(NLOHMANN_JSON_LIBRARY_NAME nlohmann_json)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_NLOHMANN_JSON QUIET ${NLOHMANN_JSON_LIBRARY_NAME})
    set(NLOHMANN_JSON_DEFINITIONS ${PC_NLOHMANN_JSON_CFLAGS_OTHER})
endif()

set(NLOHMANN_JSON_INCLUDE_HINTS ${PC_NLOHMANN_JSON_INCLUDEDIR}  ${PC_NLOHMANN_JSON_INCLUDE_DIRS})

set(NLOHMANN_JSON_ROOT_DIR "$ENV{NLOHMANN_JSON_ROOT_DIR}")

find_path(NLOHMANN_JSON_INCLUDE_DIR
          NAMES nlohmann/json.hpp
          PATHS $ENV{PROGRAMFILES}/include/
                ${NLOHMANN_JSON_ROOT_DIR}/
                ${NLOHMANN_JSON_ROOT_DIR}/include/
          HINTS ${NLOHMANN_JSON_INCLUDE_HINTS})

set(NLOHMANN_JSON_INCLUDE_DIRS ${NLOHMANN_JSON_INCLUDE_DIR})
set(NLOHMANN_JSON_LIBRARIES    "")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nlohmann_json
                                  REQUIRED_VARS
                                    NLOHMANN_JSON_INCLUDE_DIR
                                  VERSION_VAR NLOHMANN_JSON_VERSION)
