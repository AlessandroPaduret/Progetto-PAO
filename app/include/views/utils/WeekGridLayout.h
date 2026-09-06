#ifndef APP_WEEK_GRID_LAYOUT_H
#define APP_WEEK_GRID_LAYOUT_H

#include <QDate>
#include <QRect>

#include <vector>

#include "events/core/Occurrence.h"

namespace app {

/** @brief Misure geometriche di una griglia settimanale/giornaliera, in
 *  pixel, condivise tra il calcolo del layout e il disegno della griglia. */
struct WeekGridGeometry {
    int gutterWidth;         ///< larghezza della colonna ore (a sinistra)
    int headerHeight;        ///< altezza dell'intestazione giorni
    int allDayRowHeight;     ///< altezza di UNA riga della striscia "tutto il giorno"
    int dayWidth;            ///< larghezza corrente di una colonna giorno
    int hourHeight;          ///< altezza corrente di un'ora
    int minOccurrenceHeight; ///< altezza minima di un chip (durata zero)
};

/** @brief Geometria calcolata per una singola occorrenza dopo il layout. */
struct OccurrencePlacement {
    QRect rect;      ///< geometria da applicare al widget (invalida se !visible)
    bool visible;    ///< false se l'occorrenza cade fuori dai giorni mostrati
};

/** @brief Esito del calcolo di layout: una geometria per occorrenza (stesso
 *  ordine/indice del vettore passato a `place`) piu' l'altezza totale
 *  raggiunta dalla striscia "tutto il giorno" (righe impilate). */
struct WeekGridResult {
    std::vector<OccurrencePlacement> placements;
    int allDayHeight;
};

/** @brief Calcolo puro (nessun widget/QPainter coinvolto) del posizionamento
 *  delle occorrenze in una griglia stile Google Calendar: le "tutto il
 *  giorno" vanno impilate su righe nella striscia in alto (spanning su piu'
 *  giorni), le altre affiancate in colonne quando si sovrappongono nello
 *  stesso giorno. `viewStart` e' il primo giorno mostrato (il lunedi' per la
 *  vista settimanale, il giorno stesso per la vista giornaliera); `dayCount`
 *  e' generico (1 per la vista giorno, 7 di default per la settimana) quindi
 *  i giorni sono indicizzati per offset da `viewStart`, non con un enum
 *  Lun..Dom fisso a 7 valori. */
namespace WeekGridLayout {

WeekGridResult place(const std::vector<events::Occurrence>& occurrences,
                     const QDate& viewStart, int dayCount,
                     const WeekGridGeometry& geometry);

} // namespace WeekGridLayout

} // namespace app

#endif // APP_WEEK_GRID_LAYOUT_H
