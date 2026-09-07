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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dipendenze -------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Test QUIET)

# Modello logico + persistenza JSON (gestisce da solo Qt Core e Catch2 opzionali)
add_subdirectory(../model model)

# --- Libreria core ----------------------------------------------------------
add_library(pao_core STATIC
    src/controller/CalendarController.cpp
    include/controller/CalendarController.h
)
target_include_directories(pao_core PUBLIC src include)
target_link_libraries(pao_core PUBLIC events events_persistence Qt6::Core)

# --- Applicazione Qt --------------------------------------------------------
add_executable(pao_app
    resources/app.qrc
    src/main.cpp
    src/menu/AppMenuBar.cpp
    include/menu/AppMenuBar.h
    include/menu/MenuShortcutStyle.h

    # Finestra principale e helper di livello top: include/, src/
    src/MainWindow.cpp
    include/MainWindow.h
    src/NavigationBar.cpp
    include/NavigationBar.h
    include/Theme.h

    # Viste principali (elenco, griglie mensile/annuale): views/
    src/views/DayView.cpp
    include/views/DayView.h
    src/views/MonthView.cpp
    include/views/MonthView.h
    src/views/YearView.cpp
    include/views/YearView.h
    src/views/ActivityListPage.cpp
    include/views/ActivityListPage.h

    # Griglia settimanale (composizione di WeekView): views/weekly/
    src/views/weekly/WeekView.cpp
    include/views/weekly/WeekView.h
    src/views/weekly/HeaderWidget.cpp
    include/views/weekly/HeaderWidget.h
    src/views/weekly/AllDayAreaWidget.cpp
    include/views/weekly/AllDayAreaWidget.h
    src/views/weekly/TimeGutterWidget.cpp
    include/views/weekly/TimeGutterWidget.h
    src/views/weekly/DayColumnWidget.cpp
    include/views/weekly/DayColumnWidget.h
    src/views/weekly/OccurrenceWidget.cpp
    include/views/weekly/OccurrenceWidget.h

    # Pannello di creazione/modifica (widget principali): dialog/
    src/dialog/ActivitySidebarWidget.cpp
    include/dialog/ActivitySidebarWidget.h
    include/dialog/ActivityTypeWidget.h
    src/dialog/RecurrenceFormWidget.cpp
    include/dialog/RecurrenceFormWidget.h
    src/dialog/MeetingFormWidget.cpp
    include/dialog/MeetingFormWidget.h
    src/dialog/TaskFormWidget.cpp
    include/dialog/TaskFormWidget.h
    src/dialog/RecurrenceChoiceDialog.cpp
    include/dialog/RecurrenceChoiceDialog.h

    # Logica di dominio del pannello, non Qt Widgets: dialog/utils/
    src/dialog/utils/ActivitySeriesBuilder.cpp
    include/dialog/utils/ActivitySeriesBuilder.h
    src/dialog/utils/RecurrenceBuilder.cpp
    include/dialog/utils/RecurrenceBuilder.h

    # Helper condivisi da piu' viste (nessuna vista/widget principale): views/utils/
    include/views/utils/ViewShared.h
    include/views/utils/WidgetUtils.h
    src/views/utils/WeekGridLayout.cpp
    include/views/utils/WeekGridLayout.h
    src/views/utils/ActivityViewHelpers.cpp
    include/views/utils/ActivityViewHelpers.h
    src/views/utils/ActivityVisitors.cpp
    include/views/utils/ActivityVisitors.h
)
target_link_libraries(pao_app PRIVATE pao_core Qt6::Widgets)

# --- Tests -----------00----------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    if(TARGET Qt6::Test)
        add_executable(app_controller_tests tests/test_controller.cpp)
        target_link_libraries(app_controller_tests PRIVATE pao_core Qt6::Test Qt6::Gui)
        add_test(NAME AppControllerTest COMMAND app_controller_tests)
    else()
        message(STATUS "Qt Test non trovato: test dell'app disabilitati")
    endif()
endif()

# --- Indentazione -----------------------------------------------------------
find_program(CLANG_FORMAT_BIN NAMES clang-format clang-format-15 clang-format-14)

if(CLANG_FORMAT_BIN)
    # Corretto: raggruppa tutti i sorgenti e header nelle sottocartelle
    file(GLOB_RECURSE ALL_SOURCE_FILES 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    )

    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT_BIN} -i -style=file ${ALL_SOURCE_FILES}
        COMMENT "Esecuzione di clang-format sui file sorgente..."
    )
else()
    message(WARNING "clang-format non trovato! Il target 'format' non sarà disponibile.")
endif()