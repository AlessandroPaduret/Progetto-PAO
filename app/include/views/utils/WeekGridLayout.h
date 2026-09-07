#pragma once

#include <QDate>
#include <QRect>

#include <vector>

#include "core/Occurrence.h"

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

/** @brief Una "tutto il giorno" impilata su una riga della striscia in alto:
 *  occupa le colonne [firstDay, lastDay] (offset da viewStart, gia' clampati
 *  a [0, dayCount-1]) sulla riga indicata. `index` e' la posizione
 *  dell'occorrenza nel vettore passato a layoutAllDayRows. */
struct AllDayItem {
    int index;
    int firstDay;
    int lastDay;
    int row;
};

namespace WeekGridLayout {

/** @brief Impila le occorrenze "tutto il giorno" (coversFullDay) su righe:
 *  ogni item occupa la prima riga libera per tutta la sua estensione
 *  [firstDay, lastDay]. Usata da AllDayAreaWidget per un QGridLayout con
 *  column-span (colonna = 1+giorno, la colonna 0 e' riservata al gutter;
 *  rowSpan sempre 1, colSpan = lastDay-firstDay+1). Le occorrenze che non
 *  coprono un giorno intero sono ignorate (non compaiono nel risultato). */
std::vector<AllDayItem> layoutAllDayRows(const std::vector<events::Occurrence>& occurrences,
                                         const QDate& viewStart, int dayCount);

/** @brief Geometria (QRect) delle occorrenze NON "tutto il giorno" di UN
 *  SINGOLO giorno, in coordinate LOCALI a quella colonna (0,0 = mezzanotte,
 *  x in [0, columnWidth)): le sovrapposte vengono affiancate in colonne come
 *  Google Calendar (interval-graph greedy coloring). Il risultato e'
 *  parallelo a `dayOccurrences` (gia' filtrate su un solo giorno). Usata da
 *  DayColumnWidget, che possiede solo le occorrenze del proprio giorno. */
std::vector<QRect> layoutDayColumn(const std::vector<events::Occurrence>& dayOccurrences,
                                   int columnWidth);

} // namespace WeekGridLayout

} // namespace app
