cmake_minimum_required(VERSION 3.16)

project(abaddon)

set(ABADDON_RESOURCE_DIR "/usr/share/abaddon" CACHE PATH "Fallback directory for resources on Linux")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

option(USE_LIBHANDY "Enable features that require libhandy (default)" ON)

find_package(nlohmann_json REQUIRED)
find_package(CURL)
find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(gtkmm REQUIRED)

set(USE_TLS TRUE)
set(USE_OPEN_SSL TRUE)
find_package(IXWebSocket QUIET)
if (NOT IXWebSocket_FOUND)
    message("ixwebsocket was not found and will be included as a submodule")
    add_subdirectory(subprojects/ixwebsocket)
    include_directories(IXWEBSOCKET_INCLUDE_DIRS)
endif ()

if (MINGW OR WIN32)
    link_libraries(ws2_32)
endif ()

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(NOMINMAX)
endif ()

include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    add_compile_definitions(ABADDON_IS_BIG_ENDIAN)
endif ()

configure_file(${PROJECT_SOURCE_DIR}/src/config.h.in ${PROJECT_BINARY_DIR}/config.h)

file(GLOB_RECURSE ABADDON_SOURCES
        "src/*.h"
        "src/*.hpp"
        "src/*.cpp"
        )

add_executable(abaddon ${ABADDON_SOURCES})
target_include_directories(abaddon PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_include_directories(abaddon PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories(abaddon PUBLIC ${GTKMM_INCLUDE_DIRS})
target_include_directories(abaddon PUBLIC ${ZLIB_INCLUDE_DIRS})
target_include_directories(abaddon PUBLIC ${SQLite3_INCLUDE_DIRS})
target_include_directories(abaddon PUBLIC ${NLOHMANN_JSON_INCLUDE_DIRS})

if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
((CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_CXX_COMPILER_VERSION LESS 9))))
    target_link_libraries(abaddon stdc++fs)
endif ()

if (IXWebSocket_LIBRARIES)
    target_link_libraries(abaddon ${IXWebSocket_LIBRARIES})
    find_library(MBEDTLS_X509_LIBRARY mbedx509)
    find_library(MBEDTLS_TLS_LIBRARY mbedtls)
    find_library(MBEDTLS_CRYPTO_LIBRARY mbedcrypto)
    if (MBEDTLS_TLS_LIBRARY)
        target_link_libraries(abaddon ${MBEDTLS_TLS_LIBRARY})
    endif ()
    if (MBEDTLS_X509_LIBRARY)
        target_link_libraries(abaddon ${MBEDTLS_X509_LIBRARY})
    endif ()
    if (MBEDTLS_CRYPTO_LIBRARY)
        target_link_libraries(abaddon ${MBEDTLS_CRYPTO_LIBRARY})
    endif ()
else ()
    target_link_libraries(abaddon $<BUILD_INTERFACE:ixwebsocket>)
endif ()

find_package(Threads)
if (Threads_FOUND)
    target_link_libraries(abaddon Threads::Threads)
endif ()

find_package(Fontconfig QUIET)
if (Fontconfig_FOUND)
    target_link_libraries(abaddon Fontconfig::Fontconfig)
endif ()

target_link_libraries(abaddon ${SQLite3_LIBRARIES})
target_link_libraries(abaddon ${GTKMM_LIBRARIES})
target_link_libraries(abaddon ${CURL_LIBRARIES})
target_link_libraries(abaddon ${ZLIB_LIBRARY})
target_link_libraries(abaddon ${NLOHMANN_JSON_LIBRARIES})

if (USE_LIBHANDY)
    find_package(libhandy)
    if (NOT libhandy_FOUND)
        message("libhandy could not be found. features requiring it have been disabled")
        set(USE_LIBHANDY OFF)
    else ()
        target_include_directories(abaddon PUBLIC ${libhandy_INCLUDE_DIRS})
        target_link_libraries(abaddon ${libhandy_LIBRARIES})
        target_compile_definitions(abaddon PRIVATE WITH_LIBHANDY)
    endif ()
endif ()
