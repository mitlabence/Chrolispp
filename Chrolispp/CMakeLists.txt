cmake_minimum_required(VERSION 3.10)
project(Chrolispp)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/external)

# Add the source files
set(SOURCES
    src/Chrolispp.cpp
    src/LEDFunctions.cpp
    src/Logger.cpp
    src/ProtocolStep.cpp
    src/Utils.cpp
)

# Add the executable
add_executable(Chrolispp ${SOURCES})

# Link external libraries
target_link_libraries(Chrolispp 
    ${CMAKE_SOURCE_DIR}/lib/external/TL6WL_32.dll
    ${CMAKE_SOURCE_DIR}/lib/external/TL6WL_64.dll
    ${CMAKE_SOURCE_DIR}/lib/external/TL6WL_64.lib
    ${CMAKE_SOURCE_DIR}/lib/external/TLUP_64.dll
    ${CMAKE_SOURCE_DIR}/lib/external/TLUP_64.lib
    ${CMAKE_SOURCE_DIR}/lib/external/TLUP_service_64.lib
)
