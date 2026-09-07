#pragma once

#include <QDate>
#include <QWidget>

#include <array>
#include <vector>

#include "events.h"

class QCalendarWidget;
class QGridLayout;

namespace app {

/** @brief Vista "anno": 12 QCalendarWidget nativi in griglia 3x4, giorni con
 *  attivita' evidenziati (colore della prima non evasa, grigio se solo
 *  Compiti evasi). Delega a Qt calcolo calendario/nomi/evidenziazione "oggi".
 *  Doppio clic (o Invio) su un giorno emette daySelected. */
class YearView : public QWidget {
    Q_OBJECT
public:
    explicit YearView(QWidget* parent = nullptr);

    void setOccurrences(const std::vector<events::Occurrence>& occurrences);

    void setYear(const QDate& januaryFirst);

    int baseWidth() const;
    int baseHeight() const;

signals:
    void daySelected(const QDate& date);

private:
    static constexpr int kCols = 3;
    static constexpr int kRows = 4;

    QGridLayout* m_grid = nullptr;
    std::array<QCalendarWidget*, kCols * kRows> m_calendars = {};
    QDate m_year;
};

} // namespace app
