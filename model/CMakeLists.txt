cmake_minimum_required(VERSION 3.16)

project(pao_events VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# AUTOMOC e' necessario per elaborare le macro Q_OBJECT nei file di test QTest
set(CMAKE_AUTOMOC ON)
# compile_commands.json in build/: usato da clangd (vedi .clangd) per
# risolvere i veri include path del progetto invece di indovinarli.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# --- Sorgenti del Dominio e Utility -----------------------------------------
file(GLOB_RECURSE EVENTS_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/events/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.cpp"
)

add_library(events STATIC ${EVENTS_SOURCES})

target_include_directories(events PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# --- Moduli Qt (Core e Test) ------------------------------------------------
find_package(Qt6 COMPONENTS Core Test QUIET)

if(NOT Qt6_FOUND)
    # Fallback per ambienti con Qt5
    find_package(Qt5 COMPONENTS Core Test QUIET)
endif()

# --- Persistenza JSON (Qt Core) ----------------------------------------------
if(TARGET Qt::Core OR TARGET Qt5::Core)
    add_subdirectory(persistence)
else()
    message(STATUS "Qt Core non trovato: modulo di persistenza JSON disabilitato")
endif()

# --- Test con QTest (Suite nativa Qt) --------------------------------------
include(CTest)
if(BUILD_TESTING)
    if(TARGET Qt::Test OR TARGET Qt5::Test)
        # Determina il target Qt corretto (Qt6 o Qt5)
        set(QT_TEST_LIB Qt::Test)
        if(NOT TARGET Qt::Test)
            set(QT_TEST_LIB Qt5::Test)
        endif()

        # Test del Modello di Dominio
        add_executable(events_tests tests/test.cpp)
        target_link_libraries(events_tests PRIVATE events ${QT_TEST_LIB})
        add_test(NAME EventsTest COMMAND events_tests)

        # Test della Persistenza JSON
        if(TARGET events_persistence)
            add_executable(persistence_tests tests/test_persistence.cpp)
            target_link_libraries(persistence_tests PRIVATE events_persistence ${QT_TEST_LIB})
            add_test(NAME PersistenceTest COMMAND persistence_tests)
        endif()

        message(STATUS "Test abilitati con QTest nativo")
    else()
        message(STATUS "Qt Test non trovato: compilazione dei test disabilitata")
    endif()
endif()