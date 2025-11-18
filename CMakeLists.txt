cmake_minimum_required(VERSION 3.16)

project(Stremato VERSION "5.0.20")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(DEBUG_LOG "Allow debug logs" ON)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "VCPKG toolchain file")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MPV_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/libmpv/x86_64/include")
    set(MPV_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/deps/libmpv/x86_64/mpv.lib")
    set(MPV_DLL "${CMAKE_CURRENT_SOURCE_DIR}/deps/libmpv/x86_64/libmpv-2.dll")
else()
    # 32-bit architecture
endif()

include_directories(${MPV_INCLUDE_DIR})

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-webview2 CONFIG REQUIRED)
find_package(wil CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED) 

add_subdirectory(resources/discord-rpc-src)


set(SOURCES
        src/app/app_manager.cpp
        src/app/app_manager.h
        src/crashlog/crashlog.cpp
        src/crashlog/crashlog.h
        src/discord/discord_manager.cpp
        src/discord/discord_manager.h
        src/extensions/extensions_manager.cpp
        src/extensions/extensions_manager.h
        src/globals/globals.cpp
        src/globals/globals.h
        src/helpers/helpers.cpp
        src/helpers/helpers.h
        src/logger/logger.cpp
        src/logger/logger.h
        src/mpv/mpv_manager.cpp
        src/mpv/mpv_manager.h
        src/server/server_manager.cpp
        src/server/server_manager.h
        src/settings/settings_manager.cpp
        src/settings/settings_manager.h
        src/updater/updater_manager.cpp
        src/updater/updater_manager.h
        src/webview/webview_manager.cpp
        src/webview/webview_manager.h
        src/webview_protocol/command_handler/command_handler.cpp
        src/webview_protocol/command_handler/command_handler.h
        src/webview_protocol/event_emitter/event_emitter.cpp
        src/webview_protocol/event_emitter/event_emitter.h
        src/webview_protocol/transport_constants.h
        src/webview_protocol/types.h
        src/window/window_manager.cpp
        src/window/window_manager.h
        src/main.cpp
        src/resource.h
)


add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE
        gdiplus.lib
        dwmapi.lib
        shcore.lib
        Msimg32.lib
        winhttp.lib
        shlwapi.lib
        ws2_32.lib 
        nlohmann_json::nlohmann_json
        unofficial::webview2::webview2
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
        WIL::WIL
        ${MPV_LIBRARY}
        discord-rpc-static 
)

if(DEBUG_LOG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_LOG)
endif()

set(NODE_RUNTIME_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/stremato/stremio-runtime.exe")
set(NODE_SERVER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/stremato/server.js")
set(PORTABLE_CONFIG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/portable_config")
set(DEST_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
set(PORTABLE_CONFIG_DEST "${DEST_DIR}/portable_config")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MPV_DLL}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PORTABLE_CONFIG_DEST}"
    COMMENT "Copying resource files to output directory and ensuring config folders exist..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NODE_RUNTIME_SRC}" "${DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NODE_SERVER_SRC}" "${DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PORTABLE_CONFIG_SRC}" "${PORTABLE_CONFIG_DEST}"
)
