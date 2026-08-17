#ifndef APP_YEARVIEW_H
#define APP_YEARVIEW_H

#include <QDate>
#include <QWidget>

#include <optional>
#include <vector>

#include "events/events.h"

namespace app {

/** @brief Vista "anno": 12 mini-calendari disposti in una griglia 4x3, un
 *  mese ciascuno; i giorni con attivita' mostrano un pallino colorato
 *  (uno per attivita' diversa, nel limite del riquadro).
 *
 *  Doppio clic su un giorno: passa alla vista giorno di quella data
 *  (segnale `daySelected`). Tooltip con l'elenco delle attivita' del giorno.
 */
class YearView : public QWidget {
    Q_OBJECT
public:
    explicit YearView(QWidget* parent = nullptr);

    /** @brief Imposta le occorrenze da mostrare (gia' filtrate sull'anno). */
    void setOccurrences(const std::vector<events::Occurrence>& occurrences);

    /** @brief Imposta l'anno visualizzato (1 gennaio dell'anno). */
    void setYear(const QDate& januaryFirst);

    /** @brief Larghezza minima (griglia alle dimensioni base). */
    int baseWidth() const;
    /** @brief Altezza minima (griglia alle dimensioni base). */
    int baseHeight() const;

signals:
    /** @brief Doppio clic su un giorno: la data scelta. */
    void daySelected(const QDate& date);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private:
    /** @brief Rettangolo del pannello del mese (0..11). */
    QRect monthRect(int monthIndex) const;
    /** @brief Data del giorno sotto il punto, se nel mese. */
    std::optional<QDate> dateAt(const QPoint& pos) const;

    std::vector<events::Occurrence> m_occurrences;
    QDate m_year;
};

} // namespace app

#endif // APP_YEARVIEW_H
