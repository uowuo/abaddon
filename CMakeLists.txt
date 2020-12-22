cmake_minimum_required(VERSION 3.16)

project(abaddon)

set(CMAKE_CXX_STANDARD 17)

option(DISABLE_MBEDTLS "Disable mbedtls" OFF)

add_compile_definitions(NOMINMAX)

find_path(GTKMM_INCLUDE_DIRS gtkmm.h REQUIRED)
find_library(GTKMM_LIBRARY gtkmm)
include_directories(${GTKMM_INCLUDE_DIRS})

find_library(GLIB_LIBRARY glib-2.0)
find_library(GTK_LIBRARY gtk-3.0)

find_path(GLIBMM_INCLUDE_DIRS glibmm.h)
find_library(GLIBMM_LIBRARY glibmm)
include_directories(${GLIBMM_INCLUDE_DIRS})

find_path(SIGC_INCLUDE_DIRS sigc++/sigc++.h)
find_library(SIGC_LIBRARY sigc-2.0)
include_directories(${SIGC_INCLUDE_DIRS})

find_path(GIOMM_INCLUDE_DIRS giomm.h)
find_library(GIOMM_LIBRARY giomm)
include_directories(${GIOMM_INCLUDE_DIRS})

find_path(GDKMM_INCLUDE_DIRS gdkmm.h)
find_library(GDKMM_LIBRARY gdkmm)
include_directories(${GDKMM_INCLUDE_DIRS})

find_path(ATKMM_INCLUDE_DIRS atkmm.h)
find_library(ATKMM_LIBRARY atkmm)
include_directories(${ATKMM_INCLUDE_DIRS})

find_package(curl)
find_library(CURL_LIBRARY libcurl)

find_path(CPR_INCLUDE_DIRS cpr/cpr.h)
find_library(CPR_LIBRARY cpr)
include_directories(${CPR_INCLUDE_DIRS})

find_package(zlib)
find_library(ZLIB_LIBRARY zlib)
include_directories(${ZLIB_INCLUDE_DIRS})

if(NOT DISABLE_MBEDTLS)
	find_library(MBEDTLS_X509_LIBRARY mbedx509)
	find_library(MBEDTLS_TLS_LIBRARY mbedtls)
	find_library(MBEDTLS_CRYPTO_LIBRARY mbedcrypto)
	include_directories(${MBEDTLS_INCLUDE_DIRS})
endif()

find_package(nlohmann_json 3.2.0 REQUIRED)

find_package(unofficial-sqlite3 CONFIG REQUIRED)

find_path(IXWEBSOCKET_INCLUDE_DIRS ixwebsocket/ixwebsocket.h)
find_library(IXWEBSOCKET_LIBRARY ixwebsocket)
include_directories(${IXWEBSOCKET_INCLUDE_DIRS}})

if(mingw)
	link_libraries(ws2_32 wsock2)
endif()

if(WIN32)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
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
target_link_libraries(abaddon ${IXWEBSOCKET_LIBRARY})
target_link_libraries(abaddon unofficial::sqlite3::sqlite3)
target_link_libraries(abaddon ${GTKMM_LIBRARY})
target_link_libraries(abaddon ${GLIB_LIBRARY})
target_link_libraries(abaddon ${GLIBMM_LIBRARY})
target_link_libraries(abaddon ${SIGC_LIBRARY})
target_link_libraries(abaddon ${GIOMM_LIBRARY})
target_link_libraries(abaddon ${GDKMM_LIBRARY})
target_link_libraries(abaddon ${ATKMM_LIBRARY})
target_link_libraries(abaddon ${CURL_LIBRARY})
target_link_libraries(abaddon ${CPR_LIBRARY})
target_link_libraries(abaddon ${ZLIB_LIBRARY})
target_link_libraries(abaddon ${GTK_LIBRARY})
if(NOT DISABLE_MBEDTLS)
	target_link_libraries(abaddon ${MBEDTLS_TLS_LIBRARY})
	target_link_libraries(abaddon ${MBEDTLS_X509_LIBRARY})
	target_link_libraries(abaddon ${MBEDTLS_CRYPTO_LIBRARY})
endif()
