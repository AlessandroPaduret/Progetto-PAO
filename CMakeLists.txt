cmake_minimum_required(VERSION 3.16)

project(pao_events VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# Libreria con l'implementazione del dominio (include/ + src/events/)
file(GLOB_RECURSE EVENTS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/events/*.cpp")
add_library(events STATIC ${EVENTS_SOURCES})
target_include_directories(events PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# --- Test con Catch2 --------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    find_package(Catch2 3 REQUIRED)

    add_executable(events_tests tests/test.cpp)
    target_link_libraries(events_tests PRIVATE events Catch2::Catch2)

    include(Catch)
    catch_discover_tests(events_tests)
endif()
