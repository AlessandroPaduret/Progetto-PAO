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
    app/src/views/HeaderWidget.cpp \
    app/src/views/AllDayAreaWidget.cpp \
    app/src/views/TimeGutterWidget.cpp \
    app/src/views/DayColumnWidget.cpp \
    app/src/views/DayView.cpp \
    app/src/views/MonthView.cpp \
    app/src/views/NavigationBar.cpp \
    app/src/views/YearView.cpp \
    app/src/views/OccurrenceWidget.cpp \
    app/src/views/utils/WeekGridLayout.cpp \
    model/src/core/Activity.cpp \
    model/src/core/DateGenerator.cpp \
    model/src/events/ActivityBuilder.cpp \
    model/src/domain/Calendar.cpp \
    model/src/generators/FixedIntervalGenerator.cpp \
    model/src/events/GeneratorBuilder.cpp \
    model/src/events/MaxOccurencesDecorator.cpp \
    model/src/domain/Meeting.cpp \
    model/src/generators/MonthlyGenerator.cpp \
    model/src/generators/SingleGenerator.cpp \
    model/src/domain/Task.cpp \
    model/src/generators/YearlyGenerator.cpp \
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
    app/include/views/HeaderWidget.h \
    app/include/views/AllDayAreaWidget.h \
    app/include/views/TimeGutterWidget.h \
    app/include/views/DayColumnWidget.h \
    app/include/views/DayView.h \
    app/include/views/MonthView.h \
    app/include/views/NavigationBar.h \
    app/include/views/YearView.h \
    app/include/views/utils/ViewShared.h \
    app/include/views/utils/WidgetUtils.h \
    app/include/views/utils/WeekGridLayout.h \
    model/include/events.h \
    model/include/builders/ActivityBuilder.h \
    model/include/builders/GeneratorBuilder.h \
    model/include/core/Activity.h \
    model/include/core/ActivityVisitor.h \
    model/include/core/CommonTypes.h \
    model/include/core/DateGenerator.h \
    model/include/core/DateGeneratorVisitor.h \
    model/include/core/Format.h \
    model/include/core/Occurrence.h \
    model/include/domain/Calendar.h \
    model/include/domain/Meeting.h \
    model/include/domain/Task.h \
    model/include/generators/FixedIntervalGenerator.h \
    model/include/generators/MaxOccurrencesDecorator.h \
    model/include/generators/MonthlyGenerator.h \
    model/include/generators/SingleGenerator.h \
    model/include/generators/YearlyGenerator.h \
    model/persistence/include/persistence/JsonPersistence.h
