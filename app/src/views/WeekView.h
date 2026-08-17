#ifndef APP_WEEKVIEW_H
#define APP_WEEKVIEW_H

#include <QDate>
#include <QDateTime>
#include <QWidget>

#include <optional>
#include <vector>

#include "events/events.h"

namespace app {

/** @brief Griglia settimanale stile Google Calendar.
 *
 *  Intestazione con giorni della settimana e numeri, ore sul bordo sinistro,
 *  attivita' disegnate come blocchi colorati (posizione = ora di inizio,
 *  altezza = durata; durata zero = chip di altezza minima).
 *
 *  Le occorrenze che si sovrappongono temporalmente nello stesso giorno
 *  vengono AFFIANCATE in colonne (come in Google Calendar), cosi' possono
 *  coesistere nella stessa casella.
 *
 *  Trascinamento: tenendo premuto il tasto sinistro su un'occorrenza e
 *  spostando il mouse, l'occorrenza diventa trascinabile; al rilascio su
 *  una nuova cella viene emesso `activityMoved` con la nuova data/ora.
 *
 *  La griglia si adatta al ridimensionamento: sotto le dimensioni base mostra
 *  le scrollbar (dimensione minima), sopra scala giorno/ora (e font) per
 *  riempire la finestra.
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
    static constexpr int kDragThresholdPx = 8;       // soglia per avviare il drag

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

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    int dayWidth() const;   // larghezza corrente di una colonna giorno
    int hourHeight() const; // altezza corrente di un'ora

    // Layout a colonne: rect di ogni occorrenza (eventi sovrapposti affiancati)
    void ensureRects();
    QTime localTimeOf(const events::Occurrence& occurrence) const;
    int minuteOf(const QTime& time) const;

    /** @brief Riga di una cella (giorno/ora) dalle coordinate locali. */
    std::optional<QDateTime> cellAt(const QPoint& pos) const;

    /** @brief Rettangolo di un'occorrenza "gettonata" (drag) in una data/ora. */
    QRect dragGhostRect(const QDateTime& localStart,
                        const events::Duration duration) const;

    int hitTest(const QPoint& pos) const;

    std::vector<events::Occurrence> m_occurrences;
    std::vector<QRect> m_rects;           // layout corrente (parallelo a m_occurrences)
    QDate m_monday;
    int m_dayCount = kDaysPerWeek;
    int m_selected = -1;
    std::optional<Preview> m_preview;     // anteprima evento in modifica

    // Stato del drag&drop
    bool m_dragActive = false;
    bool m_dragMoved = false;
    int m_dragIndex = -1;
    QPoint m_dragPressPos;
    std::optional<QDateTime> m_dropCell;
};

} // namespace app

#endif // APP_WEEKVIEW_H
