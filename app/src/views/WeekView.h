#ifndef APP_WEEKVIEW_H
#define APP_WEEKVIEW_H

#include <QDate>
#include <QDateTime>
#include <QWidget>

#include <vector>

#include "events/events.h"

namespace app {

/** @brief Griglia settimanale stile Google Calendar.
 *
 *  Intestazione con giorni della settimana e numeri, ore sul bordo sinistro,
 *  attivita' disegnate come blocchi colorati (posizione = ora di inizio,
 *  altezza = durata; durata zero = chip di altezza minima).
 *
 *  La griglia si adatta al ridimensionamento: sotto le dimensioni base mostra
 *  le scrollbar (dimensione minima), sopra scala giorno/ora (e font) per
 *  riempire la finestra. Un clic seleziona un'occorrenza, il tasto destro
 *  apre un menu contestuale, un doppio clic su una cella vuota emette
 *  `emptySlotClicked` con l'orario locale della cella.
 */
class WeekView : public QWidget {
    Q_OBJECT
public:
    static constexpr int kGutterWidth = 56;   // larghezza colonna ore
    static constexpr int kHeaderHeight = 48;  // altezza intestazione giorni
    static constexpr int kDayWidth = 120;     // larghezza base colonna giorno
    static constexpr int kHourHeight = 60;    // pixel base per ora
    static constexpr int kDaysPerWeek = 7;
    static constexpr int kMinOccurrenceHeight = 18;  // chip per durata zero

    explicit WeekView(QWidget* parent = nullptr);

    /** @brief Imposta le occorrenze da mostrare. */
    void setOccurrences(const std::vector<events::Occurrence>& occurrences);

    /** @brief Imposta il lunedi' della settimana visualizzata. */
    void setWeekStart(const QDate& monday);

    /** @brief Occorrenza selezionata (clic sinistro), o nullptr se assente. */
    const events::Occurrence* selectedOccurrence() const;

    /** @brief Larghezza minima (griglia alle dimensioni base). */
    int baseWidth() const;
    /** @brief Altezza minima (griglia alle dimensioni base). */
    int baseHeight() const;

signals:
    /** @brief Doppio clic (o menu) su una cella vuota: orario locale della cella. */
    void emptySlotClicked(const QDateTime& start);
    /** @brief Doppio clic su un'occorrenza: modifica dell'ATTIVITA' sorgente
     *  (per i ricorrenti la serie intera, con la sua regola di ricorrenza). */
    void activityEditRequested(const events::Occurrence& occurrence);
    /** @brief Menu contestuale: mostra le informazioni dell'occorrenza. */
    void infoRequested(const events::Occurrence& occurrence);
    /** @brief Menu contestuale: modifica la singola istanza. */
    void modifyEventRequested(const events::Occurrence& occurrence);
    /** @brief Menu contestuale: elimina l'occorrenza/attivita'. */
    void deleteEventRequested(const events::Occurrence& occurrence);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    int dayWidth() const;   // larghezza corrente di una colonna giorno
    int hourHeight() const; // altezza corrente di un'ora
    QRect occurrenceRect(const events::Occurrence& occurrence) const;
    int hitTest(const QPoint& pos) const;

    std::vector<events::Occurrence> m_occurrences;
    QDate m_monday;
    int m_selected = -1;
};

} // namespace app

#endif // APP_WEEKVIEW_H
