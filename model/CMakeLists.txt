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

# --- Persistenza JSON (Qt Core, consentito dalla specifica PAO) --------------
# Qt Core e' presente sia sul host di sviluppo sia nel container del professore;
# il server (Dockerfile dedicato) non ha Qt: il modulo viene saltato.
find_package(Qt6 COMPONENTS Core QUIET)
if(Qt6Core_FOUND)
    add_subdirectory(persistence)
else()
    message(STATUS "Qt6 Core non trovato: modulo di persistenza JSON disabilitato")
endif()

# --- Test con Catch2 --------------------------------------------------------
# Catch2 non e' presente nel container di valutazione: i test vengono
# compilati solo se il pacchetto e' disponibile (build di sviluppo).
include(CTest)
if(BUILD_TESTING)
    find_package(Catch2 3 QUIET)

    if(Catch2_FOUND)
        add_executable(events_tests tests/test.cpp)
        target_link_libraries(events_tests PRIVATE events Catch2::Catch2)

        include(Catch)
        catch_discover_tests(events_tests)

        if(TARGET events_persistence)
            add_executable(persistence_tests tests/test_persistence.cpp)
            target_link_libraries(persistence_tests PRIVATE events_persistence Catch2::Catch2)
            catch_discover_tests(persistence_tests)
        endif()
    else()
        message(STATUS "Catch2 non trovato: compilazione dei test disabilitata")
    endif()
endif()
