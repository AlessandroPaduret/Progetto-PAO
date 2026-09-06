cmake_minimum_required(VERSION 3.16)

project(pao_app VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# compile_commands.json in build/: usato da clangd (vedi .clangd) per
# risolvere i veri include path del progetto invece di indovinarli.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dipendenze -------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Modello logico + persistenza JSON (gestisce da solo Qt Core e Catch2 opzionali)
add_subdirectory(../model model)

# --- Libreria core (controller: logica di applicazione senza Widgets) -------
# L'header e' elencato esplicitamente tra i sorgenti (oltre al .cpp): AUTOMOC
# nella maggior parte dei casi accoppia un .cpp al proprio header cercandolo
# nella STESSA cartella; siccome qui gli header vivono in include/ (per
# separare header/implementazione come in model/), va reso un sorgente del
# target anche lui, altrimenti Q_OBJECT non verrebbe visto e il link fallisce
# per vtable/staticMetaObject mancanti.
add_library(pao_core STATIC
    src/controller/CalendarController.cpp
    include/controller/CalendarController.h
)
target_include_directories(pao_core PUBLIC src include)
target_link_libraries(pao_core PUBLIC events events_persistence Qt6::Core)

# --- Applicazione Qt (menu + views + main) -----------------------------------
# Stesso motivo per gli header sotto include/{menu,views}/: elencati
# esplicitamente perche' non sono nella stessa cartella del .cpp associato.
add_executable(pao_app
    resources/app.qrc
    src/main.cpp
    src/menu/AppMenuBar.cpp
    include/menu/AppMenuBar.h
    include/menu/MenuShortcutStyle.h
    src/views/MainWindow.cpp
    include/views/MainWindow.h
    src/views/WeekView.cpp
    include/views/WeekView.h
    src/views/DayView.cpp
    include/views/DayView.h
    src/views/MonthView.cpp
    include/views/MonthView.h
    src/views/YearView.cpp
    include/views/YearView.h
    src/views/ActivityListPage.cpp
    include/views/ActivityListPage.h
    src/views/ActivityDetailDialog.cpp
    include/views/ActivityDetailDialog.h
    src/views/ActivityFormPage.cpp
    include/views/ActivityFormPage.h
    src/views/ActivityFormDialog.cpp
    include/views/ActivityFormDialog.h
    src/views/ActivityViewHelpers.cpp
    include/views/ActivityViewHelpers.h
    src/views/RecurrenceChoiceDialog.cpp
    include/views/RecurrenceChoiceDialog.h
    src/views/OccurrenceWidget.cpp
    include/views/OccurrenceWidget.h
    include/views/ViewShared.h
    include/views/WidgetUtils.h
    include/views/Theme.h
)
target_link_libraries(pao_app PRIVATE pao_core Qt6::Widgets)

# --- Test del controller (Catch2, headless, opzionali) ----------------------
include(CTest)
if(BUILD_TESTING)
    find_package(Catch2 3 QUIET)
    if(Catch2_FOUND)
        add_executable(app_controller_tests tests/test_controller.cpp)
        target_link_libraries(app_controller_tests PRIVATE pao_core Catch2::Catch2 Qt6::Gui)

        include(Catch)
        catch_discover_tests(app_controller_tests)
    else()
        message(STATUS "Catch2 non trovato: test dell'app disabilitati")
    endif()
endif()
