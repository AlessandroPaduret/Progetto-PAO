#pragma once

#include <QDate>
#include <QDateTime>
#include <QWidget>

#include <optional>
#include <vector>

#include "core/Occurrence.h"

class QScrollArea;

namespace app {

class AllDayAreaWidget;
class DayColumnWidget;
class HeaderWidget;
class OccurrenceWidget;
class TimeGutterWidget;

/** @brief Griglia settimanale stile Google Calendar, composta da widget Qt
 *  reali invece che da un unico canvas dipinto a mano:
 *
 *    WeekView (QVBoxLayout)
 *    +-- HeaderWidget          intestazione dei giorni (fissa in alto)
 *    +-- AllDayAreaWidget      striscia "tutto il giorno" (fissa)
 *    +-- QScrollArea           SOLO verticale, sulla griglia oraria
 *         +-- QWidget (contenuto)
 *              +-- QHBoxLayout
 *                   +-- TimeGutterWidget    colonna ore 00:00..23:00
 *                   +-- DayColumnWidget x N una per giorno, elastica in
 *                                            larghezza, proprietaria delle
 *                                            proprie occorrenze/drag&drop
 *
 *  Ogni DayColumnWidget e' autonoma per interazione: WeekView si limita a
 *  distribuire le occorrenze per giorno e a inoltrare i segnali, esponendo
 *  verso l'esterno la stessa interfaccia pubblica di sempre.
 *
 *  Numero di giorni configurabile (setDayCount, default 7; DayView usa 1).
 *  La selezione (clic sinistro) e' ESCLUSIVA sull'intera griglia a
 *  prescindere da quale colonna la possiede (stesso schema di
 *  MonthView/MonthDayCell). */
class WeekView : public QWidget {
    Q_OBJECT
public:
    static constexpr int kDaysPerWeek = 7;

    /** @brief Anteprima di un evento in fase di creazione/modifica. */
    struct Preview {
        QString title;               ///< Titolo digitato nel form
        QDateTime start;             ///< Data/ora (locale) dal form
        events::Duration duration;   ///< Durata (zero per attivita' puntuali)
    };

    explicit WeekView(QWidget* parent = nullptr);

    /** @brief Giorni mostrati (1 = vista giorno, default 7); il riferimento
     *  resta il lunedi' passato a setWeekStart. */
    void setDayCount(int days);
    int dayCount() const;

    void setOccurrences(const std::vector<events::Occurrence>& occurrences);

    void setWeekStart(const QDate& monday);

    void setPreview(const std::optional<Preview>& preview);

    const std::optional<Preview>& preview() const;

    const events::Occurrence* selectedOccurrence() const;

signals:
    void emptySlotClicked(const QDateTime& start);
    /** @brief Modifica dell'ATTIVITA' sorgente (per i ricorrenti, la serie intera). */
    void activityEditRequested(const events::Occurrence& occurrence);
    /** @brief Occorrenza successiva alla prima di una serie: ambiguo, chiedi
     *  se modificare la serie o la singola occorrenza. */
    void occurrenceEditChoiceRequested(const events::Occurrence& occurrence);
    void infoRequested(const events::Occurrence& occurrence);
    void modifyEventRequested(const events::Occurrence& occurrence);
    void deleteEventRequested(const events::Occurrence& occurrence);
    void activityMoved(const events::Occurrence& occurrence, const QDateTime& newStart);
    /** @brief Stesso caso ambiguo di occurrenceEditChoiceRequested, ma per il drag. */
    void occurrenceDragChoiceRequested(const events::Occurrence& occurrence,
                                       const QDateTime& newStart);
    void doneToggled(const events::Occurrence& occurrence);

private:
    /** @brief Ricrea le DayColumnWidget quando cambia dayCount. */
    void rebuildColumns();
    /** @brief Ripartisce m_occurrences fra AllDayAreaWidget e le colonne per
     *  data locale, e ridistribuisce l'anteprima. */
    void distributeOccurrences();
    void setSelectedChip(OccurrenceWidget* chip, const events::Occurrence& occurrence);
    void clearSelection();

    HeaderWidget* m_header = nullptr;
    AllDayAreaWidget* m_allDayArea = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_gridContent = nullptr;
    TimeGutterWidget* m_gutter = nullptr;
    std::vector<DayColumnWidget*> m_columns;

    std::vector<events::Occurrence> m_occurrences;
    QDate m_monday;
    int m_dayCount = kDaysPerWeek;
    std::optional<Preview> m_preview;

    OccurrenceWidget* m_selectedChip = nullptr;
    std::optional<events::Occurrence> m_selectedOccurrence;
};

} // namespace app
