cmake_minimum_required(VERSION 3.22)
project(trgkASIO)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(
        _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
        WIN32_LEAN_AND_MEAN
        VC_EXTRALEAN
        NOMINMAX  # disable min/max macros
)

#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug mode")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Build-Debug)
    set(TRACY_ENABLE ON)
    set(TRACY_ON_DEMAND ON)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    message("Release mode")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Build-Release)
    set_property(GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set(TRACY_ENABLE OFF)
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("RelWithDebInfo mode")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Build-RelWithDebInfo)
    set_property(GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set(TRACY_ENABLE OFF)
else ()
    message(FATAL_ERROR "Error: Only Debug/Release mode supported, but got ${CMAKE_BUILD_TYPE}")
endif ()

add_subdirectory(Source/tracy)


# Main driver
add_library(trgkASIO SHARED
        Source/lib/r8brain_free_src/r8bbase.cpp

        Source/dllentry/dllmain.cpp
        Source/dllentry/register.cpp
        Source/dllentry/COMBaseClasses.cpp

        Source/utils/logger.cpp
        Source/utils/WASAPIUtils.cpp
        Source/utils/utf8convert.cpp
        Source/utils/homeDirFilePath.cpp
        Source/utils/accurateTime.cpp
        Source/utils/hexdump.cpp
        Source/utils/ResourceLoad.cpp

        Source/MessageWindow/MessageWindow.cpp
        Source/MessageWindow/TrayHandler.cpp
        Source/MessageWindow/KeyDownListener.cpp

        Source/pref/UserPref.cpp
        Source/pref/UserPrefGUI.cpp
        Source/TrgkASIO.cpp
        Source/TrgkASIOImpl.cpp
        Source/PreparedState.cpp
        Source/RunningState.cpp
        Source/WASAPIOutput/ClapRenderer.cpp
        Source/WASAPIOutput/WASAPIOutputEvent.cpp

        Source/res/resource.rc
        Source/TrgkASIO.def
        Source/WASAPIOutput/iidConstant.cpp
        Source/WASAPIOutput/createIAudioClient.cpp
        Source/utils/WaveLoad.cpp
        Source/utils/dlgGetText.cpp
)

target_compile_definitions(trgkASIO PRIVATE UNICODE _UNICODE)
target_include_directories(trgkASIO PRIVATE
        Source/lib/ASIOSDK/common
        Source/include
        Source/tracy/public
)
target_link_libraries(trgkASIO PUBLIC Avrt.lib winmm.lib)
if (TRACY_ENABLE)
    target_link_libraries(trgkASIO PUBLIC Tracy::TracyClient)
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit mode
    message("x64 target")
    set_target_properties(trgkASIO
            PROPERTIES
            OUTPUT_NAME "trgkASIO64"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/x64"
    )
endif ()


# Unit testing
add_executable(test_main
        Tests/test_ringbuffer.cpp
        Tests/test_main.cpp
)

# gui testing
add_executable(test_pref_gui WIN32
        Source/res/resource.rc
        Source/pref/UserPrefGUI.cpp

        Source/utils/dlgGetText.cpp
        Source/pref/UserPref.cpp
        Source/utils/homeDirFilePath.cpp
        Source/utils/logger.cpp
        Source/utils/utf8convert.cpp
        Source/utils/WASAPIUtils.cpp
)
target_include_directories(test_pref_gui PRIVATE
        Source/include
)
target_compile_definitions(test_pref_gui PUBLIC
        TRGKASIO_PREFGUI_TEST_MAIN
)
