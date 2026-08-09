#ifndef CLIENT_WEEKVIEW_H
#define CLIENT_WEEKVIEW_H

#include <QDate>
#include <QDateTime>
#include <QVector>
#include <QWidget>

#include <optional>

#include "api/dto.h"

namespace client {

/** @brief Griglia settimanale stile Google Calendar.
 *
 *  Intestazione con giorni della settimana e numeri, ore sul bordo sinistro,
 *  eventi disegnati come blocchi colorati (posizione = ora di inizio,
 *  altezza = durata).
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

    explicit WeekView(QWidget* parent = nullptr);

    /** @brief Imposta gli eventi da mostrare. */
    void setOccurrences(const QVector<Occurrence>& occurrences);

    /** @brief Imposta il lunedì della settimana visualizzata. */
    void setWeekStart(const QDate& monday);

    /** @brief Occorrenza selezionata (clic sinistro), se presente. */
    std::optional<Occurrence> selectedOccurrence() const;

    /** @brief Larghezza minima (griglia alle dimensioni base). */
    int baseWidth() const;
    /** @brief Altezza minima (griglia alle dimensioni base). */
    int baseHeight() const;

signals:
    /** @brief Doppio clic (o menu) su una cella vuota: orario locale della cella. */
    void emptySlotClicked(const QDateTime& start);
    /** @brief Menu contestuale: mostra le informazioni dell'occorrenza. */
    void infoRequested(const Occurrence& occurrence);
    /** @brief Menu contestuale: modifica la singola istanza. */
    void modifyEventRequested(const Occurrence& occurrence);
    /** @brief Menu contestuale: elimina l'occorrenza/evento. */
    void deleteEventRequested(const Occurrence& occurrence);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    int dayWidth() const;   // larghezza corrente di una colonna giorno
    int hourHeight() const; // altezza corrente di un'ora
    QRect occurrenceRect(const Occurrence& occurrence) const;
    int hitTest(const QPoint& pos) const;

    QVector<Occurrence> m_occurrences;
    QDate m_monday;
    int m_selected = -1;
};

} // namespace client

#endif // CLIENT_WEEKVIEW_H
