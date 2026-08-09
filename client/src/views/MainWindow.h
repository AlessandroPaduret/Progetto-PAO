#ifndef CLIENT_MAINWINDOW_H
#define CLIENT_MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

class QDateEdit;
class QTableWidget;

namespace client {

class EventsController;
struct Occurrence;

/** @brief Finestra principale: tabella delle occorrenze nel range selezionato. */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(EventsController* events, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void onPreviousMonth();
    void onNextMonth();
    void onCreateEvent();
    void onDeleteEvent();
    void onAddException();

private:
    void populateTable(const QVector<Occurrence>& occurrences);
    bool selectedOccurrence(Occurrence& out) const;
    void setRangeToCurrentMonth();

    EventsController* m_events;
    QDateEdit* m_fromDate;
    QDateEdit* m_toDate;
    QTableWidget* m_table;
};

} // namespace client

#endif // CLIENT_MAINWINDOW_H
