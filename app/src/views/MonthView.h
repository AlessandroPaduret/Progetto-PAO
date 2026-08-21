#ifndef APP_MONTHVIEW_H
#define APP_MONTHVIEW_H

#include <QDate>
#include <QDateTime>
#include <QWidget>

#include <optional>
#include <vector>

#include "events/events.h"

namespace app {

/** @brief Vista "mese": griglia settimanale del mese (5-6 righe x 7 giorni).
 *
 *  Ogni giorno mostra un massimo di 3 attivita' come chip colorati (le
 *  rimanenti sono indicate con "+N"); il numero del giorno e' evidenziato
 *  se e' oggi. I giorni fuori dal mese sono in grigio.
 *
 *  Interazione: clic sinistro seleziona, doppio clic su un'occorrenza
 *  modifica (con la scelta serie/istanza per i ricorrenti, come nella vista
 *  settimana), doppio clic su una cella vuota crea una nuova attivita' alle
 *  09:00, menu contestuale per Info/Modifica/Elimina/Nuova attivita'.
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
    /** @brief Clic sulla spunta: inverte lo stato evaso/da fare dell'occorrenza. */
    void doneToggled(const events::Occurrence& occurrence);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    /** @brief Numero di righe settimanali del mese corrente (5 o 6). */
    int rows() const;
    /** @brief Lunedi' di partenza della griglia (anche fuori dal mese). */
    QDate gridStart() const;
    /** @brief Rettangolo di una cella della griglia. */
    QRect cellRect(int row, int day) const;
    /** @brief Data della cella sotto il punto (solo griglia). */
    std::optional<QDate> dateAt(const QPoint& pos) const;
    /** @brief Indice dell'occorrenza il cui chip contiene il punto. */
    int hitTest(const QPoint& pos) const;
    /** @brief Calcola i rettangoli dei chip (stacking per giorno). */
    void ensureRects();

    std::vector<events::Occurrence> m_occurrences;
    std::vector<QRect> m_chipRects;   // chip corrente (parallelo a m_occurrences)
    std::vector<QRect> m_checkRects;  // spunta di ogni chip
    std::vector<int> m_extraCounts;   // occorrenze oltre i chip mostrati, per cella
    QDate m_month;
    int m_selected = -1;
};

} // namespace app

#endif // APP_MONTHVIEW_H
