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

/** @brief Vista "mese": griglia di 6x7 celle widget (una per giorno, sempre
 *  6 settimane come Google Calendar), non piu' un canvas dipinto a mano.
 *
 *  Ogni cella (`MonthDayCell`) e' un widget reale che possiede il proprio
 *  numero del giorno e fino a 3 chip (`OccurrenceWidget`, condiviso con
 *  WeekView) per le attivita' del giorno, piu' un'etichetta "+N"; tooltip,
 *  menu contestuale e spunta dei Compiti sono quelli nativi dei rispettivi
 *  widget. Interazione: clic sinistro su un chip seleziona, doppio clic su
 *  un'occorrenza modifica (con la scelta serie/istanza per i ricorrenti),
 *  doppio clic su una cella vuota crea alle 09:00, menu contestuale.
 */
class MonthView : public QWidget {
    Q_OBJECT
public:
    explicit MonthView(QWidget* parent = nullptr);

    /** @brief Imposta le occorrenze da mostrare (gia' filtrate sul mese). */
    void setOccurrences(const std::vector<events::Occurrence>& occurrences);

    /** @brief Imposta il mese visualizzato (primo giorno del mese). */
    void setMonth(const QDate& firstOfMonth);

    /** @brief Occorrenza selezionata (clic sinistro), o nullptr se assente. */
    const events::Occurrence* selectedOccurrence() const;

    /** @brief Larghezza minima (griglia alle dimensioni base). */
    int baseWidth() const;
    /** @brief Altezza minima (griglia alle dimensioni base). */
    int baseHeight() const;

signals:
    /** @brief Doppio clic su una cella vuota: data/ora (09:00) della cella. */
    void emptySlotClicked(const QDateTime& start);
    /** @brief Doppio clic su un'occorrenza: modifica dell'ATTIVITA' sorgente. */
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
    /** @brief Clic sulla spunta di un COMPITO: inverte lo stato evaso/da fare. */
    void doneToggled(const events::Occurrence& occurrence);

private:
    static constexpr int kRows = 6;
    static constexpr int kCols = 7;

    /** @brief Lunedi' di partenza della griglia (anche fuori dal mese). */
    QDate gridStart() const;
    void setSelectedChip(OccurrenceWidget* chip, const events::Occurrence& occurrence);

    QGridLayout* m_grid = nullptr;
    MonthDayCell* m_cells[kRows * kCols] = {};
    QDate m_month;

    OccurrenceWidget* m_selectedChip = nullptr;
    std::optional<events::Occurrence> m_selectedOccurrence;
};

} // namespace app
