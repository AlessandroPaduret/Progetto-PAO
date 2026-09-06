#ifndef APP_WEEKVIEW_H
#define APP_WEEKVIEW_H

#include <QDate>
#include <QDateTime>
#include <QWidget>

#include <optional>
#include <vector>

#include "events/events.h"

class QRubberBand;
class QLabel;

namespace app {

class OccurrenceWidget;

/** @brief Griglia settimanale stile Google Calendar.
 *
 *  Intestazione con giorni della settimana e numeri, ore sul bordo sinistro,
 *  attivita' rappresentate da widget Qt reali (`OccurrenceWidget`, non
 *  rettangoli disegnati a mano): posizione = ora di inizio, altezza =
 *  durata; durata zero = chip di altezza minima. Il tooltip al passaggio del
 *  mouse, il menu contestuale (tasto destro) e la spunta dei Compiti sono
 *  quelli nativi del widget di ciascuna occorrenza.
 *
 *  In alto, sotto i giorni, c'e' una STRISCIA dedicata alle attivita'
 *  "tutto il giorno" (chip che coprono una o piu' date).
 *
 *  Le occorrenze che si sovrappongono temporalmente nello stesso giorno
 *  vengono AFFIANCATE in colonne (come in Google Calendar), cosi' possono
 *  coesistere nella stessa casella.
 *
 *  Trascinamento: tenendo premuto il tasto sinistro su un'occorrenza e
 *  spostando il mouse oltre la soglia di sistema (QApplication::
 *  startDragDistance) parte un drag&drop nativo Qt (QDrag/QMimeData); al
 *  rilascio su una nuova cella viene emesso `activityMoved` con la nuova
 *  data/ora.
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
    static constexpr int kAllDayHeight = 22;  // altezza striscia "tutto il giorno"
    static constexpr int kDayWidth = 120;     // larghezza base colonna giorno
    static constexpr int kHourHeight = 60;    // pixel base per ora
    static constexpr int kDaysPerWeek = 7;
    static constexpr int kMinOccurrenceHeight = 18;  // chip per durata zero

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
    /** @brief Clic sulla spunta di un COMPITO: inverte lo stato evaso/da fare. */
    void doneToggled(const events::Occurrence& occurrence);

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
    /** @brief Inizio verticale della griglia oraria (sotto la striscia all-day). */
    int gridTop() const;
    int dayWidth() const;   // larghezza corrente di una colonna giorno
    int hourHeight() const; // altezza corrente di un'ora

    /** @brief Ricrea i widget delle occorrenze (uno per occorrenza). */
    void rebuildWidgets();
    /** @brief Ricalcola e applica la geometria di ogni OccurrenceWidget
     *  (striscia "tutto il giorno" impilata + colonne nella griglia oraria). */
    void relayout();

    /** @brief Cella (giorno/ora) dalle coordinate locali, o nullopt se fuori
     *  dalla griglia oraria (bordo/intestazione/striscia all-day). */
    std::optional<QDateTime> cellAt(const QPoint& pos) const;

    /** @brief Rettangolo occupato da un'occorrenza ipotetica che iniziasse a
     *  `localStart` con la durata indicata (per l'anteprima live). */
    QRect slotRect(const QDateTime& localStart, const events::Duration duration) const;

    void setSelected(int index);

    std::vector<events::Occurrence> m_occurrences;
    std::vector<OccurrenceWidget*> m_widgets;  // parallelo a m_occurrences
    int m_allDayHeight = kAllDayHeight;        // altezza corrente striscia "tutto il giorno"
    QDate m_monday;
    int m_dayCount = kDaysPerWeek;
    int m_selected = -1;
    std::optional<Preview> m_preview;          // anteprima evento in modifica
    QLabel* m_previewLabel = nullptr;          // widget dell'anteprima (nascosto se assente)

    QRubberBand* m_dropIndicator = nullptr;    // evidenzia la cella di destinazione durante il drag
};

} // namespace app

#endif // APP_WEEKVIEW_H
