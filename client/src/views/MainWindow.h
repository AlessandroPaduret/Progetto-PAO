#ifndef CLIENT_MAINWINDOW_H
#define CLIENT_MAINWINDOW_H

#include <QDateTime>
#include <QMainWindow>
#include <QVector>

class QDateEdit;

namespace client {

class EventsController;
class WeekView;
struct Occurrence;

/** @brief Finestra principale: vista settimanale degli eventi. */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(EventsController* events, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onPreviousWeek();
    void onNextWeek();
    void onCreateEvent(const QDateTime& start = QDateTime());
    void showEventInfo(const Occurrence& occurrence);
    void confirmModifyEvent(const Occurrence& occurrence);
    void confirmDeleteEvent(const Occurrence& occurrence);

private:
    void setRangeToCurrentWeek();
    bool selectedOccurrence(Occurrence& out) const;

    EventsController* m_events;
    QDateEdit* m_fromDate;
    QDateEdit* m_toDate;
    WeekView* m_weekView;
};

} // namespace client

#endif // CLIENT_MAINWINDOW_H
