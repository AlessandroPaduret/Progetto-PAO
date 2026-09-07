#pragma once

#include <QDate>
#include <QWidget>

#include <vector>

#include "core/Occurrence.h"
#include "views/weekly/OccurrenceWidget.h"

class QGridLayout;

namespace app {

/** @brief Striscia "tutto il giorno" sopra la griglia oraria: QGridLayout con
 *  column-span, colonna 0 riservata al gutter, una colonna per giorno, una
 *  riga per livello di sovrapposizione (WeekGridLayout::layoutAllDayRows
 *  decide riga/colonne). I chip non sono trascinabili. */
class AllDayAreaWidget : public QWidget {
    Q_OBJECT
public:
    explicit AllDayAreaWidget(QWidget* parent = nullptr);

    void setDays(const QDate& viewStart, int dayCount);

    /** @brief Le occorrenze non "tutto il giorno" sono filtrate e ignorate. */
    void setOccurrences(const std::vector<events::Occurrence>& occurrences);

signals:
    void chipPressed(OccurrenceWidget* chip, const events::Occurrence& occurrence);
    void doneToggled(const events::Occurrence& occurrence);
    void activityEditRequested(const events::Occurrence& occurrence);
    void occurrenceEditChoiceRequested(const events::Occurrence& occurrence);
    void infoRequested(const events::Occurrence& occurrence);
    void modifyEventRequested(const events::Occurrence& occurrence);
    void deleteEventRequested(const events::Occurrence& occurrence);
    /** @brief Clic su area vuota: azzera la selezione corrente. */
    void backgroundClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QGridLayout* m_grid = nullptr;
    QDate m_viewStart;
    int m_dayCount = 0;
    std::vector<OccurrenceWidget*> m_chips;
};

} // namespace app
