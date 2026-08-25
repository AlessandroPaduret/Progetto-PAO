# Progetto "Attivita" — applicazione Qt standalone
# Compila con qmake (QMake 3.1, Qt 6.x): `qmake6 && make`
# Il modello logico, la persistenza JSON e la GUI sono un unico eseguibile.

QT += core gui widgets
CONFIG += c++21
CONFIG -= app_bundle
TEMPLATE = app
TARGET = attivita

INCLUDEPATH += model/include model/persistence/include app/src

SOURCES += \
    app/src/main.cpp \
    app/src/CalendarController.cpp \
    app/src/views/ActivityDetailDialog.cpp \
    app/src/views/ActivityFormPage.cpp \
    app/src/views/ActivityFormDialog.cpp \
    app/src/views/ActivityListPage.cpp \
    app/src/views/ActivityViewHelpers.cpp \
    app/src/views/MainWindow.cpp \
    app/src/views/RecurrenceChoiceDialog.cpp \
    app/src/views/WeekView.cpp \
    app/src/views/DayView.cpp \
    app/src/views/MonthView.cpp \
    app/src/views/YearView.cpp \
    model/src/events/Activity.cpp \
    model/src/events/ActivityBuilder.cpp \
    model/src/events/ActivityFactory.cpp \
    model/src/events/Calendar.cpp \
    model/src/events/DateGenerator.cpp \
    model/src/events/FixedIntervalGenerator.cpp \
    model/src/events/Meeting.cpp \
    model/src/events/MonthlyGenerator.cpp \
    model/src/events/MoveGeneratorVisitor.cpp \
    model/src/events/SingleGenerator.cpp \
    model/src/events/Task.cpp \
    model/src/events/YearlyGenerator.cpp \
    model/persistence/src/persistence/JsonPersistence.cpp

HEADERS += \
    app/src/CalendarController.h \
    app/src/views/ActivityDetailDialog.h \
    app/src/views/ActivityFormPage.h \
    app/src/views/ActivityFormDialog.h \
    app/src/views/ActivityListPage.h \
    app/src/views/ActivityViewHelpers.h \
    app/src/views/MainWindow.h \
    app/src/views/RecurrenceChoiceDialog.h \
    app/src/views/WeekView.h \
    app/src/views/DayView.h \
    app/src/views/MonthView.h \
    app/src/views/YearView.h \
    app/src/views/ViewShared.h \
    model/include/events/events.h \
    model/include/events/builders/ActivityBuilder.h \
    model/include/events/core/Activity.h \
    model/include/events/core/ActivityVisitor.h \
    model/include/events/core/CommonTypes.h \
    model/include/events/core/DateGenerator.h \
    model/include/events/core/DateGeneratorVisitor.h \
    model/include/events/core/Format.h \
    model/include/events/core/Occurrence.h \
    model/include/events/domain/ActivityFactory.h \
    model/include/events/domain/Calendar.h \
    model/include/events/domain/Meeting.h \
    model/include/events/domain/Task.h \
    model/include/events/generators/FixedIntervalGenerator.h \
    model/include/events/generators/MonthlyGenerator.h \
    model/include/events/generators/MoveGeneratorVisitor.h \
    model/include/events/generators/SingleGenerator.h \
    model/include/events/generators/YearlyGenerator.h \
    model/persistence/include/persistence/JsonPersistence.h
