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
 *  Ogni DayColumnWidget e' autonoma per interazione (clic su cella vuota,
 *  doppio clic, menu contestuale, drag&drop, anteprima live): WeekView si
 *  limita a distribuire le occorrenze per giorno e a inoltrare i segnali,
 *  esponendo verso l'esterno la stessa interfaccia pubblica di sempre.
 *
 *  Il numero di giorni e' configurabile (`setDayCount`, default 7; usato da
 *  DayView con 1). La selezione (clic sinistro su un'occorrenza) e'
 *  ESCLUSIVA sull'intera griglia, a prescindere da quale colonna la
 *  possiede (stesso schema di MonthView/MonthDayCell: WeekView tiene solo il
 *  puntatore al chip selezionato, senza sapere a quale colonna appartiene).
 */
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

    /** @brief Numero di giorni mostrati nella griglia (1 = vista giorno,
     *  default 7). Il riferimento resta il lunedi' passato a setWeekStart. */
    void setDayCount(int days);
    int dayCount() const;

    /** @brief Imposta le occorrenze da mostrare. */
    void setOccurrences(const std::vector<events::Occurrence>& occurrences);

    /** @brief Imposta il lunedi' della settimana visualizzata. */
    void setWeekStart(const QDate& monday);

    /** @brief Mostra/nasconde l'anteprima dell'evento in fase di modifica. */
    void setPreview(const std::optional<Preview>& preview);

    /** @return L'anteprima corrente (per test/strumenti). */
    const std::optional<Preview>& preview() const;

    /** @brief Occorrenza selezionata (clic sinistro), o nullptr se assente. */
    const events::Occurrence* selectedOccurrence() const;

signals:
    /** @brief Doppio clic (o menu) su una cella vuota: orario locale della cella. */
    void emptySlotClicked(const QDateTime& start);
    /** @brief Doppio clic su un'occorrenza: modifica dell'ATTIVITA' sorgente
     *  (per i ricorrenti la serie intera, con la sua regola di ricorrenza). */
    void activityEditRequested(const events::Occurrence& occurrence);
    /** @brief Doppio clic su un'occorrenza successiva alla prima di una serie:
     *  chiedi se modificare la serie o la singola occorrenza. */
    void occurrenceEditChoiceRequested(const events::Occurrence& occurrence);
    /** @brief Menu contestuale: mostra le informazioni dell'occorrenza. */
    void infoRequested(const events::Occurrence& occurrence);
    /** @brief Menu contestuale: modifica la singola istanza. */
    void modifyEventRequested(const events::Occurrence& occurrence);
    /** @brief Menu contestuale: elimina l'occorrenza/attivita'. */
    void deleteEventRequested(const events::Occurrence& occurrence);
    /** @brief Drag&drop: l'occorrenza e' stata rilasciata nella nuova data/ora. */
    void activityMoved(const events::Occurrence& occurrence, const QDateTime& newStart);
    /** @brief Drag di un'occorrenza successiva alla prima di una serie: chiedi se
     *  spostare la serie o la singola occorrenza. */
    void occurrenceDragChoiceRequested(const events::Occurrence& occurrence,
                                       const QDateTime& newStart);
    /** @brief Clic sulla spunta di un COMPITO: inverte lo stato evaso/da fare. */
    void doneToggled(const events::Occurrence& occurrence);

private:
    /** @brief Ricrea le DayColumnWidget (quando cambia dayCount). */
    void rebuildColumns();
    /** @brief Ripartisce m_occurrences fra AllDayAreaWidget e le colonne
     *  giorno per giorno (per data locale), e ridistribuisce l'anteprima. */
    void distributeOccurrences();
    /** @brief Selezione esclusiva su tutta la griglia (schema MonthView). */
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
