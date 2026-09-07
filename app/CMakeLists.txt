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
    src/views/MainWindow.cpp
    include/views/MainWindow.h
    src/views/WeekView.cpp
    include/views/WeekView.h
    src/views/HeaderWidget.cpp
    include/views/HeaderWidget.h
    src/views/AllDayAreaWidget.cpp
    include/views/AllDayAreaWidget.h
    src/views/TimeGutterWidget.cpp
    include/views/TimeGutterWidget.h
    src/views/DayColumnWidget.cpp
    include/views/DayColumnWidget.h
    src/views/DayView.cpp
    include/views/DayView.h
    src/views/MonthView.cpp
    include/views/MonthView.h
    src/views/NavigationBar.cpp
    include/views/NavigationBar.h
    src/views/YearView.cpp
    include/views/YearView.h
    src/views/ActivityListPage.cpp
    include/views/ActivityListPage.h
    src/views/dialog/ActivitySidebarWidget.cpp
    include/views/dialog/ActivitySidebarWidget.h
    src/views/dialog/RecurrenceFormWidget.cpp
    include/views/dialog/RecurrenceFormWidget.h
    src/views/dialog/MeetingFormWidget.cpp
    include/views/dialog/MeetingFormWidget.h
    src/views/dialog/TaskFormWidget.cpp
    include/views/dialog/TaskFormWidget.h
    src/views/dialog/ActivityViewHelpers.cpp
    include/views/dialog/ActivityViewHelpers.h
    src/views/dialog/RecurrenceChoiceDialog.cpp
    include/views/dialog/RecurrenceChoiceDialog.h
    src/views/OccurrenceWidget.cpp
    include/views/OccurrenceWidget.h
    include/views/utils/ViewShared.h
    include/views/utils/WidgetUtils.h
    include/views/utils/Theme.h
    src/views/utils/WeekGridLayout.cpp
    include/views/utils/WeekGridLayout.h
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