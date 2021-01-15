cmake_minimum_required(VERSION 3.16)

project(abaddon)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(USE_TLS TRUE)
set(USE_OPEN_SSL TRUE)

find_package(nlohmann_json REQUIRED)
find_package(CURL)
find_package(cpr REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(gtkmm REQUIRED)

find_path(IXWEBSOCKET_INCLUDE_DIRS ixwebsocket/IXWebSocket.h)
find_library(IXWEBSOCKET_LIBRARY ixwebsocket)
if (NOT IXWEBSOCKET_LIBRARY)
	add_subdirectory(thirdparty/IXWebSocket)
	include_directories(IXWEBSOCKET_INCLUDE_DIRS)
endif()

include_directories(thirdparty/simpleini)

if(MINGW OR WIN32)
	link_libraries(ws2_32)
endif()

if(WIN32)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	add_compile_definitions(NOMINMAX)
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
	"windows/guildsettings/*.hpp"
	"windows/guildsettings/*.cpp"
	"dialogs/*.hpp"
	"dialogs/*.cpp"
)

add_executable(abaddon ${ABADDON_SOURCES})
target_include_directories(abaddon PUBLIC ${GTKMM_INCLUDE_DIRS})
target_include_directories(abaddon PUBLIC ${CPR_INCLUDE_DIRS})
target_include_directories(abaddon PUBLIC ${ZLIB_INCLUDE_DIRS})
target_include_directories(abaddon PUBLIC ${SQLite3_INCLUDE_DIRS})

if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
	(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
	((CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_CXX_COMPILER_VERSION LESS 9))))
	target_link_libraries(abaddon stdc++fs)
endif()

if (IXWEBSOCKET_LIBRARY)
	target_link_libraries(abaddon ${IXWEBSOCKET_LIBRARY})
	find_library(MBEDTLS_X509_LIBRARY mbedx509)
	find_library(MBEDTLS_TLS_LIBRARY mbedtls)
	find_library(MBEDTLS_CRYPTO_LIBRARY mbedcrypto)
	if (MBEDTLS_TLS_LIBRARY)
		target_link_libraries(abaddon ${MBEDTLS_TLS_LIBRARY})
	endif()
	if (MBEDTLS_X509_LIBRARY)
		target_link_libraries(abaddon ${MBEDTLS_X509_LIBRARY})
	endif()
	if (MBEDTLS_CRYPTO_LIBRARY)
		target_link_libraries(abaddon ${MBEDTLS_CRYPTO_LIBRARY})
	endif()
else()
	target_link_libraries(abaddon $<BUILD_INTERFACE:ixwebsocket>)
endif()

find_package(Threads)
if (Threads_FOUND)
	target_link_libraries(abaddon Threads::Threads)
endif()

target_link_libraries(abaddon ${SQLite3_LIBRARIES})
target_link_libraries(abaddon ${GTKMM_LIBRARIES})
target_link_libraries(abaddon ${CURL_LIBRARIES})
target_link_libraries(abaddon ${CPR_LIBRARY})
target_link_libraries(abaddon ${ZLIB_LIBRARY})
target_link_libraries(abaddon ${NLOHMANN_JSON_LIBRARIES})
