#pragma once

#include <QDate>
#include <QDateTime>
#include <QRect>
#include <QWidget>

#include <optional>
#include <vector>

#include "core/Occurrence.h"
#include "views/OccurrenceWidget.h"
#include "views/WeekView.h"

class QRubberBand;
class QLabel;

namespace app {

class CalendarController;

/** @brief Una colonna-giorno della griglia oraria: possiede le proprie
 *  occorrenze (posizionate assolutamente al proprio interno, coordinate
 *  locali = minuti dalla mezzanotte tramite WeekGridLayout::layoutDayColumn),
 *  il proprio drag&drop (sorgente E destinazione), il clic su cella vuota
 *  (crea), il doppio clic su un'occorrenza (modifica) e l'anteprima live.
 *
 *  Larghezza elastica (stretch nel QHBoxLayout del genitore), altezza fissa
 *  24*kWeekHourHeight: scorre dentro la QScrollArea di WeekView, non scala
 *  piu' con la finestra. */
class DayColumnWidget : public QWidget {
    Q_OBJECT
public:
    /** @param controller Usato SOLO per risolvere il colore delle occorrenze
     *  (CalendarController::colorFor): il colore non e' un campo del modello,
     *  vive li'. Nessun'altra dipendenza da CalendarController. */
    explicit DayColumnWidget(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Giorno rappresentato da questa colonna. */
    void setDate(const QDate& date);

    /** @brief Occorrenze di QUESTO giorno, gia' filtrate escludendo quelle
     *  "tutto il giorno" (mostrate da AllDayAreaWidget, non qui). */
    void setOccurrences(const std::vector<events::Occurrence>& dayOccurrences);

    /** @brief Anteprima dell'evento in fase di creazione/modifica: mostrata
     *  solo se la sua data coincide col giorno di questa colonna. */
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
    /** @brief Clic sinistro su un'area vuota della colonna: azzera la
     *  selezione corrente. */
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
    /** @brief Data/ora locale (giorno di questa colonna) dalla coordinata Y. */
    QDateTime timeAt(int y) const;
    /** @brief Rettangolo occupato da un'occorrenza ipotetica che iniziasse a
     *  `localStart` con la durata indicata (per anteprima e drop indicator). */
    QRect slotRect(const QDateTime& localStart, events::Duration duration) const;
    /** @brief Riapplica il layout (WeekGridLayout::layoutDayColumn + anteprima)
     *  ai widget correnti: chiamata da setOccurrences e ad ogni resize
     *  (la larghezza e' elastica). */
    void relayout();

    CalendarController* m_controller;
    QDate m_date;
    std::vector<events::Occurrence> m_occurrences;
    std::vector<OccurrenceWidget*> m_widgets;  // parallelo a m_occurrences
    std::optional<WeekView::Preview> m_preview;
    QLabel* m_previewLabel = nullptr;

    QRubberBand* m_dropIndicator = nullptr;  // evidenzia la cella di destinazione durante il drag
};

} // namespace app
