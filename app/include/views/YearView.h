#pragma once

#include <QDate>
#include <QWidget>

#include <array>
#include <vector>

#include "events.h"

class QCalendarWidget;
class QGridLayout;

namespace app {

/** @brief Vista "anno": 12 QCalendarWidget nativi disposti in una griglia
 *  3x4 (uno per mese, senza barra di navigazione: il mese mostrato resta
 *  fisso), con i giorni che hanno attivita' evidenziati da uno sfondo
 *  colorato (colore della prima attivita' non evasa del giorno, grigio se
 *  ci sono solo Compiti evasi). Delega a Qt il calcolo del calendario, i
 *  nomi dei giorni/mesi (localizzati) e l'evidenziazione di "oggi".
 *
 *  Doppio clic (o Invio) su un giorno: passa alla vista giorno di quella
 *  data (segnale `daySelected`, dal nativo `QCalendarWidget::activated`).
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
    /** @brief Doppio clic (o Invio) su un giorno: la data scelta. */
    void daySelected(const QDate& date);

private:
    static constexpr int kCols = 3;
    static constexpr int kRows = 4;

    QGridLayout* m_grid = nullptr;
    std::array<QCalendarWidget*, kCols * kRows> m_calendars = {};
    QDate m_year;
};

} // namespace app
