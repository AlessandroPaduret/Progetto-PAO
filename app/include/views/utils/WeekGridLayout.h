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

/** @brief Estensione in giorni [startDay, endDay] (offset da viewStart,
 *  INCLUSO, gia' clampati a [0, dayCount-1]) di una "tutto il giorno": il
 *  chiamante la calcola da un'Occurrence, WeekGridLayout non conosce ne'
 *  QDate ne' Occurrence. */
struct DaySpan {
    int startDay;
    int endDay;
};

/** @brief Riga/colonne assegnate a una "tutto il giorno" nella striscia in
 *  alto. `index` e' la posizione del DaySpan corrispondente nel vettore
 *  passato a layoutAllDayRows (stesso ordine 1:1, nessun filtro qui: lo fa
 *  gia' il chiamante prima di costruire i DaySpan). */
struct AllDayPlacement {
    int index;
    int row;
    int startDay;
    int endDay;
};

/** @brief Intervallo [startMinutes, endMinutes) dalla mezzanotte del giorno
 *  rappresentato da una colonna: il chiamante lo ricava da un'Occurrence gia'
 *  RITAGLIATA sull'intervallo visibile in quel giorno (inizio a 0 se iniziata
 *  il giorno prima, fine a 1440 se finisce il giorno dopo). */
struct TimeSlot {
    int startMinutes;
    int endMinutes;
};

namespace WeekGridLayout {

/** @brief Impila le "tutto il giorno" su righe: ognuna occupa la prima riga
 *  libera per tutta la sua estensione [startDay, endDay]. Usata da
 *  AllDayAreaWidget per un QGridLayout con column-span (colonna = 1+giorno,
 *  la colonna 0 e' riservata al gutter; rowSpan sempre 1, colSpan =
 *  endDay-startDay+1). Puro calcolo geometrico: nessuna nozione di data o di
 *  Occurrence, il chiamante filtra e converte prima di chiamarla. */
std::vector<AllDayPlacement> layoutAllDayRows(const std::vector<DaySpan>& spans, int dayCount);

/** @brief Geometria (QRect) di slot orari che si sovrappongono, affiancati in
 *  colonne come Google Calendar (interval-graph greedy coloring), in
 *  coordinate LOCALI a quella colonna (0,0 = mezzanotte, x in [0,
 *  columnWidth)). Il risultato e' parallelo a `timeSlots`. Usata da
 *  DayColumnWidget::relayout(), che ricava ogni TimeSlot dalla propria
 *  Occurrence gia' ritagliata sul giorno di questa colonna (vedi
 *  WeekView::distributeOccurrences per la duplicazione a cavallo di
 *  mezzanotte). */
std::vector<QRect> layoutDayColumn(const std::vector<TimeSlot>& timeSlots, int columnWidth);

} // namespace WeekGridLayout

} // namespace app
