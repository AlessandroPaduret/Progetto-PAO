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

# --- Dipendenze -------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Modello logico + persistenza JSON (gestisce da solo Qt Core e Catch2 opzionali)
add_subdirectory(../model model)

# --- Libreria core (controller: logica di applicazione senza Widgets) -------
# L'header e' elencato esplicitamente tra i sorgenti (oltre al .cpp): AUTOMOC
# nella maggior parte dei casi accoppia un .cpp al proprio header cercandolo
# nella STESSA cartella; siccome qui gli header vivono in src/include/ (per
# separare header/implementazione come in model/), va reso un sorgente del
# target anche lui, altrimenti Q_OBJECT non verrebbe visto e il link fallisce
# per vtable/staticMetaObject mancanti.
add_library(pao_core STATIC
    src/controller/CalendarController.cpp
    src/include/controller/CalendarController.h
)
target_include_directories(pao_core PUBLIC src src/include)
target_link_libraries(pao_core PUBLIC events events_persistence Qt6::Core)

# --- Applicazione Qt (menu + views + main) -----------------------------------
# Stesso motivo per gli header sotto src/include/{menu,views}/: elencati
# esplicitamente perche' non sono nella stessa cartella del .cpp associato.
add_executable(pao_app
    resources/app.qrc
    src/main.cpp
    src/menu/AppMenuBar.cpp
    src/include/menu/AppMenuBar.h
    src/include/menu/MenuShortcutStyle.h
    src/views/MainWindow.cpp
    src/include/views/MainWindow.h
    src/views/WeekView.cpp
    src/include/views/WeekView.h
    src/views/DayView.cpp
    src/include/views/DayView.h
    src/views/MonthView.cpp
    src/include/views/MonthView.h
    src/views/YearView.cpp
    src/include/views/YearView.h
    src/views/ActivityListPage.cpp
    src/include/views/ActivityListPage.h
    src/views/ActivityDetailDialog.cpp
    src/include/views/ActivityDetailDialog.h
    src/views/ActivityFormPage.cpp
    src/include/views/ActivityFormPage.h
    src/views/ActivityFormDialog.cpp
    src/include/views/ActivityFormDialog.h
    src/views/ActivityViewHelpers.cpp
    src/include/views/ActivityViewHelpers.h
    src/views/RecurrenceChoiceDialog.cpp
    src/include/views/RecurrenceChoiceDialog.h
    src/views/OccurrenceWidget.cpp
    src/include/views/OccurrenceWidget.h
    src/include/views/ViewShared.h
    src/include/views/WidgetUtils.h
    src/include/views/Theme.h
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
