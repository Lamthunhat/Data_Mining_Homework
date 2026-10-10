cmake_minimum_required(VERSION 3.20)
project(DataMining_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags for Release mode
if(MSVC)
    add_compile_options(/O2 /Oi /Ot /GL /W3 /MP)
    add_link_options(/LTCG /OPT:REF /OPT:ICF)
else()
    add_compile_options(-O3 -march=native -Wall -Wextra)
endif()

# Set output directories so executables go directly into bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/DataUtils.cpp
    src/Apriori.cpp
    src/Eclat.cpp
    src/FPGrowth.cpp
    src/CompactFPGrowth.cpp
    src/main.cpp
)

add_executable(DataMining_Project ${SOURCES})

# Windows psapi for memory tracking
if(WIN32)
    target_link_libraries(DataMining_Project PRIVATE psapi)
endif()
