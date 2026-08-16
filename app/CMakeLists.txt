cmake_minimum_required(VERSION 3.16)

project(pao_app VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

set(CMAKE_AUTOMOC ON)

# --- Dipendenze -------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Modello logico + persistenza JSON (gestisce da solo Qt Core e Catch2 opzionali)
add_subdirectory(../model model)

# --- Libreria core (controller: logica di applicazione senza Widgets) -------
add_library(pao_core STATIC src/CalendarController.cpp)
target_include_directories(pao_core PUBLIC src)
target_link_libraries(pao_core PUBLIC events events_persistence Qt6::Core)

# --- Applicazione Qt (views + main) -----------------------------------------
add_executable(pao_app
    src/main.cpp
    src/views/MainWindow.cpp
    src/views/WeekView.cpp
    src/views/ActivityListPage.cpp
    src/views/ActivityDetailPage.cpp
    src/views/ActivityFormPage.cpp
    src/views/ActivityFormDialog.cpp
    src/views/ActivityViewHelpers.cpp
    src/views/RecurrenceChoiceDialog.cpp
)
target_link_libraries(pao_app PRIVATE pao_core Qt6::Widgets)

# --- Test del controller (Catch2, headless, opzionali) ----------------------
include(CTest)
if(BUILD_TESTING)
    find_package(Catch2 3 QUIET)
    if(Catch2_FOUND)
        add_executable(app_controller_tests tests/test_controller.cpp)
        target_link_libraries(app_controller_tests PRIVATE pao_core Catch2::Catch2)

        include(Catch)
        catch_discover_tests(app_controller_tests)
    else()
        message(STATUS "Catch2 non trovato: test dell'app disabilitati")
    endif()
endif()
