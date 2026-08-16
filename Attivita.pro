# Progetto "Attivita" — applicazione Qt standalone
# Compila con qmake (QMake 3.1, Qt 6.x): `qmake6 && make`
# Il modello logico, la persistenza JSON e la GUI sono un unico eseguibile.

QT += core gui widgets
CONFIG += c++20
CONFIG -= app_bundle
TEMPLATE = app
TARGET = attivita

INCLUDEPATH += model/include model/persistence/include app/src

SOURCES += \
    app/src/main.cpp \
    app/src/CalendarController.cpp \
    app/src/views/ActivityDetailPage.cpp \
    app/src/views/ActivityFormPage.cpp \
    app/src/views/ActivityFormDialog.cpp \
    app/src/views/ActivityListPage.cpp \
    app/src/views/ActivityViewHelpers.cpp \
    app/src/views/MainWindow.cpp \
    app/src/views/RecurrenceChoiceDialog.cpp \
    app/src/views/WeekView.cpp \
    model/src/events/Activity.cpp \
    model/src/events/ActivityFactory.cpp \
    model/src/events/Calendar.cpp \
    model/src/events/Deadline.cpp \
    model/src/events/Event.cpp \
    model/src/events/FixedIntervalGenerator.cpp \
    model/src/events/RecurrentEvent.cpp \
    model/src/events/Reminder.cpp \
    model/src/events/YearlyGenerator.cpp \
    model/persistence/src/persistence/JsonPersistence.cpp

HEADERS += \
    app/src/CalendarController.h \
    app/src/views/ActivityDetailPage.h \
    app/src/views/ActivityFormPage.h \
    app/src/views/ActivityFormDialog.h \
    app/src/views/ActivityListPage.h \
    app/src/views/ActivityViewHelpers.h \
    app/src/views/MainWindow.h \
    app/src/views/RecurrenceChoiceDialog.h \
    app/src/views/WeekView.h \
    model/include/events/events.h \
    model/include/events/core/Activity.h \
    model/include/events/core/ActivityVisitor.h \
    model/include/events/core/CommonTypes.h \
    model/include/events/core/DateGenerator.h \
    model/include/events/core/DateGeneratorVisitor.h \
    model/include/events/core/Format.h \
    model/include/events/core/Occurrence.h \
    model/include/events/domain/ActivityFactory.h \
    model/include/events/domain/Calendar.h \
    model/include/events/domain/Deadline.h \
    model/include/events/domain/Event.h \
    model/include/events/domain/RecurrentEvent.h \
    model/include/events/domain/Reminder.h \
    model/include/events/generators/FixedIntervalGenerator.h \
    model/include/events/generators/NullGenerator.h \
    model/include/events/generators/YearlyGenerator.h \
    model/persistence/include/persistence/JsonPersistence.h
