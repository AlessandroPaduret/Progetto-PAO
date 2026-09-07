#pragma once

#include <QDate>
#include <QDateTime>
#include <QWidget>

#include <optional>
#include <vector>

#include "events.h"

class QGridLayout;

namespace app {

class MonthDayCell;
class OccurrenceWidget;

/** @brief Vista "mese": griglia 6x7 di MonthDayCell (sempre 6 settimane, come
 *  Google Calendar), ognuna con fino a 3 chip (OccurrenceWidget, condiviso con
 *  WeekView) + etichetta "+N". Clic seleziona, doppio clic modifica (scelta
 *  serie/istanza per i ricorrenti), doppio clic su cella vuota crea alle 09:00. */
class MonthView : public QWidget {
    Q_OBJECT
public:
    explicit MonthView(QWidget* parent = nullptr);

    void setOccurrences(const std::vector<events::Occurrence>& occurrences);

    void setMonth(const QDate& firstOfMonth);

    const events::Occurrence* selectedOccurrence() const;

    int baseWidth() const;
    int baseHeight() const;

signals:
    void emptySlotClicked(const QDateTime& start);
    void activityEditRequested(const events::Occurrence& occurrence);
    /** @brief Occorrenza successiva alla prima di una serie: chiedi serie o istanza. */
    void occurrenceEditChoiceRequested(const events::Occurrence& occurrence);
    void infoRequested(const events::Occurrence& occurrence);
    void modifyEventRequested(const events::Occurrence& occurrence);
    void deleteEventRequested(const events::Occurrence& occurrence);
    /** @brief Solo i Compiti hanno la spunta. */
    void doneToggled(const events::Occurrence& occurrence);

private:
    static constexpr int kRows = 6;
    static constexpr int kCols = 7;

    /** @brief Puo' ricadere nel mese precedente. */
    QDate gridStart() const;
    void setSelectedChip(OccurrenceWidget* chip, const events::Occurrence& occurrence);

    QGridLayout* m_grid = nullptr;
    MonthDayCell* m_cells[kRows * kCols] = {};
    QDate m_month;

    OccurrenceWidget* m_selectedChip = nullptr;
    std::optional<events::Occurrence> m_selectedOccurrence;
};

} // namespace app
