cmake_minimum_required(VERSION 3.16)

project(abaddon)

set(CMAKE_CXX_STANDARD 17)

option(USE_PKGCONFIG "Use pkg-config" OFF)
option(DISABLE_MBEDTLS "Disable mbedtls" OFF)

add_compile_definitions(NOMINMAX)

if(NOT USE_PKGCONFIG)
	find_path(GTKMM_INCLUDE_DIRS gtkmm.h REQUIRED)
	find_library(GTKMM_LIBRARY gtkmm)
	link_libraries(${GTKMM_LIBRARY})
	include_directories(${GTKMM_INCLUDE_DIRS})

	find_library(GLIB_LIBRARY glib-2.0)
	link_libraries(${GLIB_LIBRARY})

	find_path(GLIBMM_INCLUDE_DIRS glibmm.h)
	find_library(GLIBMM_LIBRARY glibmm)
	link_libraries(${GLIBMM_LIBRARY})
	include_directories(${GLIBMM_INCLUDE_DIRS})

	find_path(SIGC_INCLUDE_DIRS sigc++/sigc++.h)
	find_library(SIGC_LIBRARY sigc-2.0)
	link_libraries(${SIGC_LIBRARY})
	include_directories(${SIGC_INCLUDE_DIRS})

	find_path(GIOMM_INCLUDE_DIRS giomm.h)
	find_library(GIOMM_LIBRARY giomm)
	link_libraries(${GIOMM_LIBRARY})
	include_directories(${GIOMM_INCLUDE_DIRS})

	find_path(GDKMM_INCLUDE_DIRS gdkmm.h)
	find_library(GDKMM_LIBRARY gdkmm)
	link_libraries(${GDKMM_LIBRARY})
	include_directories(${GDKMM_INCLUDE_DIRS})

	find_path(ATKMM_INCLUDE_DIRS atkmm.h)
	find_library(ATKMM_LIBRARY atkmm)
	link_libraries(${ATKMM_LIBRARY})
	include_directories(${ATKMM_INCLUDE_DIRS})

	find_package(curl)
	find_library(CURL_LIBRARY libcurl)
	link_libraries(${CURL_LIBRARY})

	find_path(CPR_INCLUDE_DIRS cpr/cpr.h)
	find_library(CPR_LIBRARY cpr)
	include_directories(${CPR_INCLUDE_DIRS})
	link_libraries(${CPR_LIBRARY})

	find_package(zlib)
	find_library(ZLIB_LIBRARY zlib)
	include_directories(${ZLIB_INCLUDE_DIRS})
	link_libraries(${ZLIB_LIBRARY})

	if(NOT DISABLE_MBEDTLS)
		find_library(MBEDTLS_X509_LIBRARY mbedx509)
		find_library(MBEDTLS_TLS_LIBRARY mbedtls)
		find_library(MBEDTLS_CRYPTO_LIBRARY mbedcrypto)
		include_directories(${MBEDTLS_INCLUDE_DIRS})
		link_libraries(${MBEDTLS_TLS_LIBRARY})
		link_libraries(${MBEDTLS_X509_LIBRARY})
		link_libraries(${MBEDTLS_CRYPTO_LIBRARY})
	endif()

	find_package(nlohmann_json 3.2.0 REQUIRED)

	find_path(IXWEBSOCKET_INCLUDE_DIRS ixwebsocket/ixwebsocket.h)
	find_library(IXWEBSOCKET_LIBRARY ixwebsocket)
	link_libraries(${IXWEBSOCKET_LIBRARY})
	include_directories(${IXWEBSOCKET_LIBRARY}})
else()
	find_package(PkgConfig)
	
	set(BUILD_CPR_TESTS OFF)
	set(BUILD_CPR_TESTS_SSL OFF)

	include(FetchContent)
	FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/whoshuu/cpr.git)
	FetchContent_MakeAvailable(cpr)
	include_directories(${CPR_INCLUDE_DIRS})
	link_libraries(cpr::cpr)

	pkg_check_modules(NLOHMANN_JSON REQUIRED nlohmann_json)
	include_directories(${NLOHMANN_JSON_INCLUDE_DIRS})

	pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
	link_directories(${GTKMM_LIBRARY_DIRS})
	include_directories(${GTKMM_INCLUDE_DIRS})

	pkg_check_modules(ATKMM REQUIRED atkmm-1.6)
	link_directories(${ATKMM_LIBRARY_DIRS})
	include_directories(${ATKMM_INCLUDE_DIRS})

	pkg_check_modules(GDKMM REQUIRED gdkmm-3.0)
	link_directories(${GDKMM_LIBRARY_DIRS})
	include_directories(${GDKMM_INCLUDE_DIRS})

	pkg_check_modules(GIOMM REQUIRED giomm-2.4)
	link_directories(${GIOMM_LIBRARY_DIRS})
	include_directories(${GIOMM_INCLUDE_DIRS})

	pkg_check_modules(SIGC REQUIRED sigc++-2.0)
	link_directories(${SIGC_LIBRARY_DIRS})
	include_directories(${SIGC_INCLUDE_DIRS})

	pkg_check_modules(GLIBMM REQUIRED glibmm-2.4)
	link_directories(${GLIBMM_LIBRARY_DIRS})
	include_directories(${GLIBMM_INCLUDE_DIRS})

	set(USE_TLS TRUE)
	set(USE_OPEN_SSL TRUE)
	add_subdirectory(thirdparty/IXWebSocket)
	include_directories("thirdparty/IXWebSocket")
	link_libraries(ixwebsocket)

	include_directories("thirdparty/simpleini")
endif()

if(mingw)
	link_libraries(ws2_32 wsock2)
endif()

file(GLOB ABADDON_SOURCES
	"*.h"
	"*.hpp"
	"*.cpp"
	"discord/*.hpp"
	"discord/*.cpp"
	"components/*.hpp"
	"components/*.cpp"
	"windows/*.hpp"
	"windows/*.cpp"
	"dialogs/*.hpp"
	"dialogs/*.cpp"
)

add_executable(abaddon ${ABADDON_SOURCES})
target_link_libraries(abaddon ${GTKMM_LIBRARIES})
target_link_libraries(abaddon ${GDKMM_LIBRARIES})
target_link_libraries(abaddon ${GLIBMM_LIBRARIES})
target_link_libraries(abaddon ${SIGC_LIBRARIES})
target_link_libraries(abaddon ${ATKMM_LIBRARIES})
target_link_libraries(abaddon ${GIOMM_LIBRARIES})
target_link_libraries(abaddon ${GDK_LIBRARIES})
