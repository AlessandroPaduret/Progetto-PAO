#ifndef APP_WEEK_GRID_PAINTER_H
#define APP_WEEK_GRID_PAINTER_H

#include <QDate>
#include <QRect>

#include "views/utils/WeekGridLayout.h"

class QPainter;

namespace app {

/** @brief Disegna lo "sfondo" della griglia settimanale/giornaliera
 *  (intestazione giorni, striscia "tutto il giorno", linee della griglia,
 *  ore sul bordo sinistro): nessuna occorrenza, solo la cornice su cui
 *  `WeekGridLayout` posiziona i widget delle occorrenze.
 *
 *  Puro disegno (nessuno stato, nessun evento): la vista chiama `paint` dal
 *  proprio `paintEvent`, passando la stessa `WeekGridGeometry` usata per il
 *  layout piu' l'altezza gia' calcolata della striscia "tutto il giorno". */
namespace WeekGridPainter {

void paint(QPainter& painter, const QRect& viewport, const QDate& viewStart,
          int dayCount, int allDayHeight, const WeekGridGeometry& geometry,
          int headerFontSize, int smallFontSize);

} // namespace WeekGridPainter

} // namespace app

#endif // APP_WEEK_GRID_PAINTER_H
