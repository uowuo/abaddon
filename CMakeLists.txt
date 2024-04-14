cmake_minimum_required(VERSION 3.16)

project(abaddon)

set(ABADDON_RESOURCE_DIR "/usr/share/abaddon" CACHE PATH "Fallback directory for resources on Linux")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

option(USE_LIBHANDY "Enable features that require libhandy (default)" ON)
option(ENABLE_VOICE "Enable voice suppport" ON)
option(USE_KEYCHAIN "Store the token in the keychain (default)" ON)
option(ENABLE_NOTIFICATION_SOUNDS "Enable notification sounds (default)" ON)
option(ENABLE_RNNOISE "Enable RNNoise for voice activity detection (default)" ON)
option(ENABLE_QRCODE_LOGIN "Enable QR code login (default)" ON)

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

configure_file(${PROJECT_SOURCE_DIR}/src/config.h.in ${PROJECT_BINARY_DIR}/config.h)

file(GLOB_RECURSE ABADDON_SOURCES
        "src/*.h"
        "src/*.hpp"
        "src/*.cpp"
)

list(FILTER ABADDON_SOURCES EXCLUDE REGEX ".*notifier_gio\\.cpp$")
list(FILTER ABADDON_SOURCES EXCLUDE REGEX ".*notifier_fallback\\.cpp$")

add_executable(abaddon ${ABADDON_SOURCES})
target_include_directories(abaddon PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_include_directories(abaddon PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories(abaddon PUBLIC ${GTKMM_INCLUDE_DIRS})
target_include_directories(abaddon PUBLIC ${ZLIB_INCLUDE_DIRS})
target_include_directories(abaddon PUBLIC ${SQLite3_INCLUDE_DIRS})
target_include_directories(abaddon PUBLIC ${NLOHMANN_JSON_INCLUDE_DIRS})

if (ENABLE_QRCODE_LOGIN)
    add_library(qrcodegen subprojects/qrcodegen/cpp/qrcodegen.hpp subprojects/qrcodegen/cpp/qrcodegen.cpp)
    target_include_directories(qrcodegen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/qrcodegen/cpp")
    target_link_libraries(abaddon qrcodegen)

    target_include_directories(abaddon PUBLIC "subprojects/qrcodegen/cpp")
    target_compile_definitions(abaddon PRIVATE WITH_QRLOGIN)
endif ()

target_precompile_headers(abaddon PRIVATE <gtkmm.h> src/abaddon.hpp src/util.hpp)

if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
((CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_CXX_COMPILER_VERSION LESS 9))))
    target_link_libraries(abaddon stdc++fs)
endif ()

if (NOT WIN32)
    target_sources(abaddon PRIVATE src/notifications/notifier_gio.cpp)
else ()
    target_sources(abaddon PRIVATE src/notifications/notifier_fallback.cpp)
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

find_package(spdlog REQUIRED)
target_link_libraries(abaddon spdlog::spdlog)

target_link_libraries(abaddon ${SQLite3_LIBRARIES})
target_link_libraries(abaddon ${GTKMM_LIBRARIES})
target_link_libraries(abaddon ${CURL_LIBRARIES})
target_link_libraries(abaddon ${ZLIB_LIBRARY})
target_link_libraries(abaddon ${NLOHMANN_JSON_LIBRARIES})
target_link_libraries(abaddon ${CMAKE_DL_LIBS})

include(CheckAtomic)
if (NOT HAVE_CXX_ATOMICS_WITHOUT_LIB OR NOT HAVE_CXX_ATOMICS64_WITHOUT_LIB)
    target_link_libraries(abaddon atomic)
endif ()

if (USE_LIBHANDY)
    find_package(libhandy REQUIRED)
    target_include_directories(abaddon PUBLIC ${libhandy_INCLUDE_DIRS})
    target_link_libraries(abaddon ${libhandy_LIBRARIES})
    target_compile_definitions(abaddon PRIVATE WITH_LIBHANDY)
endif ()

if (USE_KEYCHAIN)
    find_package(keychain QUIET)
    if (NOT keychain_FOUND)
        message("keychain was not found and will be included as a submodule")
        add_subdirectory(subprojects/keychain)
        target_link_libraries(abaddon keychain)
        target_compile_definitions(abaddon PRIVATE WITH_KEYCHAIN)
    endif ()
endif ()

set(USE_MINIAUDIO FALSE)

if (APPLE)
    target_link_libraries(abaddon "-framework CoreFoundation")
    target_link_libraries(abaddon "-framework CoreAudio")
    target_link_libraries(abaddon "-framework AudioToolbox")
endif ()

if (ENABLE_VOICE)
    target_compile_definitions(abaddon PRIVATE WITH_VOICE)

    find_package(PkgConfig)

    set(USE_MINIAUDIO TRUE)
    pkg_check_modules(Opus REQUIRED IMPORTED_TARGET opus)
    target_link_libraries(abaddon PkgConfig::Opus)

    pkg_check_modules(libsodium REQUIRED IMPORTED_TARGET libsodium)
    target_link_libraries(abaddon PkgConfig::libsodium)

    target_link_libraries(abaddon ${CMAKE_DL_LIBS})

    if (ENABLE_RNNOISE)
        target_compile_definitions(abaddon PRIVATE WITH_RNNOISE)

        find_package(rnnoise QUIET)
        if (NOT rnnoise_FOUND)
            message("rnnoise was not found and will be included as a submodule")
            # This is potentially really stupid
            add_library(rnnoise STATIC
                    subprojects/rnnoise/src/arch.h
                    subprojects/rnnoise/src/celt_lpc.c
                    subprojects/rnnoise/src/celt_lpc.h
                    subprojects/rnnoise/src/common.h
                    subprojects/rnnoise/src/denoise.c
                    subprojects/rnnoise/src/kiss_fft.c
                    subprojects/rnnoise/src/kiss_fft.h
                    subprojects/rnnoise/src/opus_types.h
                    subprojects/rnnoise/src/pitch.c
                    subprojects/rnnoise/src/pitch.h
                    subprojects/rnnoise/src/rnn_data.c
                    subprojects/rnnoise/src/rnn_data.h
                    subprojects/rnnoise/src/rnn_reader.c
                    subprojects/rnnoise/src/rnn.c
                    subprojects/rnnoise/src/rnn.h
                    subprojects/rnnoise/src/tansig_table.h
                    subprojects/rnnoise/src/_kiss_fft_guts.h
                    subprojects/rnnoise/include/rnnoise.h)
            target_include_directories(rnnoise PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/rnnoise/include")
            target_link_libraries(abaddon rnnoise)
        else ()
            target_link_libraries(abaddon rnnoise::rnnoise)
        endif ()
    endif ()
endif ()

if (${ENABLE_NOTIFICATION_SOUNDS})
    set(USE_MINIAUDIO TRUE)
    target_compile_definitions(abaddon PRIVATE ENABLE_NOTIFICATION_SOUNDS)
endif ()

if (USE_MINIAUDIO)
    find_path(MINIAUDIO_INCLUDE_DIR
            NAMES miniaudio.h
            HINTS subprojects
            PATH_SUFFIXES miniaudio
            REQUIRED)

    target_include_directories(abaddon PUBLIC ${MINIAUDIO_INCLUDE_DIR})
    target_compile_definitions(abaddon PRIVATE WITH_MINIAUDIO)
endif ()

set(ABADDON_COMPILER_DEFS "" CACHE STRING "Additional compiler definitions")
foreach (COMPILER_DEF IN LISTS ABADDON_COMPILER_DEFS)
    target_compile_definitions(abaddon PRIVATE "${COMPILER_DEF}")
endforeach ()
