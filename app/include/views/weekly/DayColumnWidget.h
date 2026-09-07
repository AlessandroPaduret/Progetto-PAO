#pragma once

#include <QDate>
#include <QDateTime>
#include <QRect>
#include <QWidget>

#include <optional>
#include <vector>

#include "core/Occurrence.h"
#include "views/weekly/OccurrenceWidget.h"
#include "views/weekly/WeekView.h"

class QRubberBand;
class QLabel;

namespace app {

/** @brief Una colonna-giorno della griglia oraria: possiede le proprie
 *  occorrenze (coordinate locali = minuti dalla mezzanotte, via
 *  WeekGridLayout::layoutDayColumn), il proprio drag&drop (sorgente E
 *  destinazione), click/doppio click e l'anteprima live.
 *
 *  Larghezza elastica, altezza fissa 24*kWeekHourHeight: scorre dentro la
 *  QScrollArea di WeekView invece di scalare con la finestra. */
class DayColumnWidget : public QWidget {
    Q_OBJECT
public:
    explicit DayColumnWidget(QWidget* parent = nullptr);

    void setDate(const QDate& date);

    /** @brief Occorrenze di QUESTO giorno; quelle "tutto il giorno" sono gia'
     *  escluse (le mostra AllDayAreaWidget). */
    void setOccurrences(const std::vector<events::Occurrence>& dayOccurrences);

    /** @brief Mostrata solo se la data dell'anteprima coincide col giorno di
     *  questa colonna. */
    void setPreview(const std::optional<WeekView::Preview>& preview);

signals:
    void emptySlotClicked(const QDateTime& start);
    void activityEditRequested(const events::Occurrence& occurrence);
    void occurrenceEditChoiceRequested(const events::Occurrence& occurrence);
    void infoRequested(const events::Occurrence& occurrence);
    void modifyEventRequested(const events::Occurrence& occurrence);
    void deleteEventRequested(const events::Occurrence& occurrence);
    void activityMoved(const events::Occurrence& occurrence, const QDateTime& newStart);
    void occurrenceDragChoiceRequested(const events::Occurrence& occurrence,
                                       const QDateTime& newStart);
    void doneToggled(const events::Occurrence& occurrence);
    void chipPressed(OccurrenceWidget* chip, const events::Occurrence& occurrence);
    /** @brief Clic su area vuota: azzera la selezione corrente. */
    void backgroundClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    /** @brief Data/ora locale dalla coordinata Y. */
    QDateTime timeAt(int y) const;
    /** @brief Rettangolo di un'occorrenza ipotetica che iniziasse a
     *  `localStart` con quella durata (per anteprima e drop indicator). */
    QRect slotRect(const QDateTime& localStart, events::Duration duration) const;
    /** @brief Riapplica il layout ai widget correnti: chiamata da
     *  setOccurrences e ad ogni resize (la larghezza e' elastica). */
    void relayout();

    QDate m_date;
    std::vector<events::Occurrence> m_occurrences;
    std::vector<OccurrenceWidget*> m_widgets;  // parallelo a m_occurrences
    std::optional<WeekView::Preview> m_preview;
    QLabel* m_previewLabel = nullptr;

    QRubberBand* m_dropIndicator = nullptr;  // evidenzia la cella di destinazione durante il drag
};

} // namespace app
