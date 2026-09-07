#pragma once

#include <QRect>
#include <QtGlobal>

#include <vector>

namespace app {

// Geometria fissa (pixel) della griglia settimanale/giornaliera, condivisa da
// HeaderWidget/AllDayAreaWidget/TimeGutterWidget/DayColumnWidget cosi' che le
// colonne restino allineate tra loro senza ricalcoli duplicati. A differenza
// della vecchia WeekGridGeometry non e' piu' "scalata" a runtime in base al
// ridimensionamento: l'altezza di un'ora e' sempre la stessa, la griglia
// oraria semmai SCORRE (QScrollArea) invece di rimpicciolire il testo.
inline constexpr int kWeekGutterWidth = 56;          // larghezza colonna ore
inline constexpr int kWeekHeaderHeight = 48;         // altezza intestazione giorni
inline constexpr int kWeekAllDayRowHeight = 22;      // altezza di una riga "tutto il giorno"
inline constexpr int kWeekHourHeight = 60;           // altezza di un'ora
inline constexpr int kWeekMinOccurrenceHeight = 18;  // altezza minima chip (durata zero)
inline constexpr int kWeekDaysPerWeek = 7;

/** @brief [startDay, endDay] (offset da viewStart, INCLUSO, clampati a [0,
 *  dayCount-1]) di una "tutto il giorno"; il chiamante la calcola da
 *  un'Occurrence, questo modulo non conosce ne' QDate ne' Occurrence. */
struct DaySpan {
    int startDay;
    int endDay;
};

/** @brief Riga/colonne di una "tutto il giorno" nella striscia in alto;
 *  `index` e' la posizione del DaySpan nel vettore passato a
 *  layoutAllDayRows (1:1, nessun filtro qui). */
struct AllDayPlacement {
    int index;
    int row;
    int startDay;
    int endDay;
};

/** @brief [startMinutes, endMinutes) dalla mezzanotte del giorno di una
 *  colonna; il chiamante lo ricava da un'Occurrence gia' RITAGLIATA
 *  sull'intervallo visibile (inizio a 0 se iniziata il giorno prima, fine a
 *  1440 se finisce dopo). */
struct TimeSlot {
    int startMinutes;
    int endMinutes;
};

namespace WeekGridLayout {

/** @brief Impila le "tutto il giorno" su righe (prima riga libera per tutta
 *  la loro estensione); usata da AllDayAreaWidget per un QGridLayout con
 *  column-span (colonna = 1+giorno, colonna 0 riservata al gutter). Puro
 *  calcolo geometrico, nessuna nozione di data o Occurrence. */
std::vector<AllDayPlacement> layoutAllDayRows(const std::vector<DaySpan>& spans, int dayCount);

/** @brief Affianca in colonne gli slot che si sovrappongono (interval-graph
 *  greedy coloring, come Google Calendar), coordinate LOCALI (0,0 =
 *  mezzanotte). Risultato parallelo a `timeSlots`. Usata da
 *  DayColumnWidget::relayout() (vedi WeekView::distributeOccurrences per la
 *  duplicazione a cavallo di mezzanotte). */
std::vector<QRect> layoutDayColumn(const std::vector<TimeSlot>& timeSlots, int columnWidth);

} // namespace WeekGridLayout

} // namespace app
