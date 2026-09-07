#pragma once

#include <QDate>
#include <QWidget>

#include <vector>

#include "core/Occurrence.h"
#include "views/OccurrenceWidget.h"

class QGridLayout;

namespace app {

class CalendarController;

/** @brief Striscia "tutto il giorno" sopra la griglia oraria: QGridLayout con
 *  column-span, colonna 0 riservata al gutter (allineata a TimeGutterWidget),
 *  una colonna per giorno, una riga per ogni "livello" di sovrapposizione
 *  (WeekGridLayout::layoutAllDayRows decide riga/colonne di ogni occorrenza
 *  che copre piu' giorni). Le occorrenze non "tutto il giorno" sono ignorate.
 *  I chip non sono trascinabili (come nella vecchia WeekView). */
class AllDayAreaWidget : public QWidget {
    Q_OBJECT
public:
    /** @param controller Usato SOLO per risolvere il colore delle occorrenze
     *  (CalendarController::colorFor), vedi DayColumnWidget. */
    explicit AllDayAreaWidget(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Configura le colonne per i giorni [viewStart, viewStart+dayCount). */
    void setDays(const QDate& viewStart, int dayCount);

    /** @brief Occorrenze mostrate (quelle non "tutto il giorno" sono filtrate
     *  internamente e ignorate). */
    void setOccurrences(const std::vector<events::Occurrence>& occurrences);

signals:
    void chipPressed(OccurrenceWidget* chip, const events::Occurrence& occurrence);
    void doneToggled(const events::Occurrence& occurrence);
    void activityEditRequested(const events::Occurrence& occurrence);
    void occurrenceEditChoiceRequested(const events::Occurrence& occurrence);
    void infoRequested(const events::Occurrence& occurrence);
    void modifyEventRequested(const events::Occurrence& occurrence);
    void deleteEventRequested(const events::Occurrence& occurrence);
    /** @brief Clic sinistro su un'area vuota della striscia: azzera la
     *  selezione corrente (in questa striscia o in una colonna). */
    void backgroundClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    CalendarController* m_controller;
    QGridLayout* m_grid = nullptr;
    QDate m_viewStart;
    int m_dayCount = 0;
    std::vector<OccurrenceWidget*> m_chips;
};

} // namespace app
