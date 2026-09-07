cmake_minimum_required(VERSION 3.16)
project(pao_events VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON) # necessario per elaborare le macro Q_OBJECT nei file di test QTest
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra)

# --- Libreria Core ---
file(GLOB_RECURSE EVENTS_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_library(events STATIC ${EVENTS_SOURCES})
target_include_directories(events PUBLIC include)

# --- Moduli ---
find_package(Qt6 COMPONENTS Core Test QUIET)
add_subdirectory(persistence)

# --- Test ---
include(CTest)
if(BUILD_TESTING AND TARGET Qt6::Test)
    add_executable(events_tests tests/test.cpp)
    target_link_libraries(events_tests PRIVATE events Qt6::Test)
    add_test(NAME EventsTest COMMAND events_tests)

    if(TARGET events_persistence)
        add_executable(persistence_tests tests/test_persistence.cpp)
        target_link_libraries(persistence_tests PRIVATE events_persistence Qt6::Test)
        add_test(NAME PersistenceTest COMMAND persistence_tests)
    endif()
endif()