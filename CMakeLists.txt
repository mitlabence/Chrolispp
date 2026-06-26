cmake_minimum_required(VERSION 3.23)

project(Chrolispp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CHROLISPP_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Chrolispp")
set(CHROLISPP_INCLUDE_DIR "${CHROLISPP_PROJECT_DIR}/include")
set(CHROLISPP_EXTERNAL_INCLUDE_DIR "${CHROLISPP_INCLUDE_DIR}/external")
set(CHROLISPP_EXTERNAL_LIB_DIR "${CHROLISPP_PROJECT_DIR}/lib/external")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CHROLISPP_PLATFORM_BITS 64)
    set(CHROLISPP_TL6WL_LIB_NAME TL6WL_64)
    set(CHROLISPP_TLUP_DLL_NAME TLUP_64.dll)
    set(CHROLISPP_VISA_BIN_DEFAULT "C:/Program Files/IVI Foundation/VISA/Win64/Bin")
    set(CHROLISPP_VISA_LIB_DEFAULT "C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc")
else()
    set(CHROLISPP_PLATFORM_BITS 32)
    set(CHROLISPP_TL6WL_LIB_NAME TL6WL_32)
    set(CHROLISPP_TLUP_DLL_NAME TLUP_32.dll)
    set(CHROLISPP_VISA_BIN_DEFAULT "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/Bin")
    set(CHROLISPP_VISA_LIB_DEFAULT "C:/Program Files (x86)/IVI Foundation/VISA/WinNT/lib/msc")
endif()

set(CHROLISPP_VISA_BIN_DIR "${CHROLISPP_VISA_BIN_DEFAULT}" CACHE PATH "Path to VISA Bin directory")
set(CHROLISPP_VISA_LIB_DIR "${CHROLISPP_VISA_LIB_DEFAULT}" CACHE PATH "Path to VISA library directory")

set(CHROLISPP_SOURCES
    "${CHROLISPP_PROJECT_DIR}/src/ArduinoCommands.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/InitialBreakBatch.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/Chrolispp.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/COMFunctions.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/LEDFunctions.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/Logger.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/ProtocolPlanner.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/ProtocolStep.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/PulseChainBatch.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/Timing.cpp"
    "${CHROLISPP_PROJECT_DIR}/src/Utils.cpp"
)

add_executable(Chrolispp ${CHROLISPP_SOURCES})

target_include_directories(Chrolispp PRIVATE
    "${CHROLISPP_INCLUDE_DIR}"
    "${CHROLISPP_EXTERNAL_INCLUDE_DIR}"
)

if(MSVC)
    target_compile_definitions(Chrolispp PRIVATE UNICODE _UNICODE)
    target_compile_options(Chrolispp PRIVATE /W3 /MP /permissive- /std:c++latest)
endif()

find_library(CHROLISPP_TL6WL_LIB
    NAMES ${CHROLISPP_TL6WL_LIB_NAME}
    PATHS
        "${CHROLISPP_EXTERNAL_LIB_DIR}"
        "${CHROLISPP_VISA_LIB_DIR}"
    REQUIRED
)

target_link_libraries(Chrolispp PRIVATE "${CHROLISPP_TL6WL_LIB}")

set(CHROLISPP_TLUP_DLL "${CHROLISPP_VISA_BIN_DIR}/${CHROLISPP_TLUP_DLL_NAME}")
if(EXISTS "${CHROLISPP_TLUP_DLL}")
    add_custom_command(TARGET Chrolispp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CHROLISPP_TLUP_DLL}"
            "$<TARGET_FILE_DIR:Chrolispp>"
    )
endif()