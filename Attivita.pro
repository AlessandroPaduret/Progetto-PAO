# Progetto "Attivita" — applicazione Qt standalone
# Compila con qmake (QMake 3.1, Qt 6.x): `qmake6 && make`
# Il modello logico, la persistenza JSON e la GUI sono un unico eseguibile.

QT += core gui widgets
CONFIG += c++21
CONFIG -= app_bundle
TEMPLATE = app
TARGET = attivita

INCLUDEPATH += model/include model/persistence/include app/src app/include

RESOURCES += app/resources/app.qrc

SOURCES += \
    app/src/main.cpp \
    app/src/controller/CalendarController.cpp \
    app/src/menu/AppMenuBar.cpp \
    app/src/views/dialog/ActivityDetailDialog.cpp \
    app/src/views/dialog/ActivityFormPage.cpp \
    app/src/views/dialog/ActivityFormDialog.cpp \
    app/src/views/ActivityListPage.cpp \
    app/src/views/dialog/ActivityViewHelpers.cpp \
    app/src/views/MainWindow.cpp \
    app/src/views/dialog/RecurrenceChoiceDialog.cpp \
    app/src/views/WeekView.cpp \
    app/src/views/DayView.cpp \
    app/src/views/MonthView.cpp \
    app/src/views/YearView.cpp \
    app/src/views/OccurrenceWidget.cpp \
    model/src/events/Activity.cpp \
    model/src/events/ActivityBuilder.cpp \
    model/src/events/Calendar.cpp \
    model/src/events/FixedIntervalGenerator.cpp \
    model/src/events/GeneratorBuilder.cpp \
    model/src/events/MaxOccurencesDecorator.cpp \
    model/src/events/Meeting.cpp \
    model/src/events/MonthlyGenerator.cpp \
    model/src/events/SingleGenerator.cpp \
    model/src/events/Task.cpp \
    model/src/events/YearlyGenerator.cpp \
    model/persistence/src/persistence/JsonPersistence.cpp

HEADERS += \
    app/include/controller/CalendarController.h \
    app/include/menu/AppMenuBar.h \
    app/include/menu/MenuShortcutStyle.h \
    app/include/views/dialog/ActivityDetailDialog.h \
    app/include/views/OccurrenceWidget.h \
    app/include/views/utils/Theme.h \
    app/include/views/dialog/ActivityFormPage.h \
    app/include/views/dialog/ActivityFormDialog.h \
    app/include/views/ActivityListPage.h \
    app/include/views/dialog/ActivityViewHelpers.h \
    app/include/views/MainWindow.h \
    app/include/views/dialog/RecurrenceChoiceDialog.h \
    app/include/views/WeekView.h \
    app/include/views/DayView.h \
    app/include/views/MonthView.h \
    app/include/views/YearView.h \
    app/include/views/utils/ViewShared.h \
    app/include/views/utils/WidgetUtils.h \
    model/include/events/events.h \
    model/include/events/builders/ActivityBuilder.h \
    model/include/events/builders/GeneratorBuilder.h \
    model/include/events/core/Activity.h \
    model/include/events/core/ActivityVisitor.h \
    model/include/events/core/CommonTypes.h \
    model/include/events/core/DateGenerator.h \
    model/include/events/core/DateGeneratorVisitor.h \
    model/include/events/core/Format.h \
    model/include/events/core/Occurrence.h \
    model/include/events/domain/Calendar.h \
    model/include/events/domain/Meeting.h \
    model/include/events/domain/Task.h \
    model/include/events/generators/FixedIntervalGenerator.h \
    model/include/events/generators/MaxOccurrencesDecorator.h \
    model/include/events/generators/MonthlyGenerator.h \
    model/include/events/generators/SingleGenerator.h \
    model/include/events/generators/YearlyGenerator.h \
    model/persistence/include/persistence/JsonPersistence.h
