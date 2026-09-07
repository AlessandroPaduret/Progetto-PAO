#pragma once

#include <QDate>

#include <vector>

#include "events.h"

namespace app::RecurrenceBuilder {

/** @brief Calcola la fine di una serie DOPO N occorrenze avanzando il
 *  generatore (align + next ripetuto): il modello non ha un limite di
 *  conteggio nativo (generatori stateless), quindi "dopo N occorrenze" si
 *  traduce qui in un `end`, sfruttando il clamping di calendario che
 *  avanzare il generatore a mano gia' garantisce (fine mese, bisestili...).
 *  @param generator generatore della serie.
 *  @param seriesStart inizio della serie.
 *  @param count numero di occorrenze desiderato (>= 1).
 *  @return la data dell'ultima (N-esima) occorrenza. */
events::TimePoint calculateEndAfterCount(const events::DateGenerator& generator,
                                          events::TimePoint seriesStart, int count);

/** @brief Calcola la data dell'N-esima occorrenza COMBINATA di una serie
 *  settimanale su piu' giorni (es. lun+mer+ven): non avanzabile con un solo
 *  generatore, perche' ogni giorno scelto e' in realta' una
 *  FixedIntervalGenerator separata che condivide solo la fine. Si calcola
 *  con l'aritmetica sugli offset rispetto al giorno d'inizio (es. lun+mar+mer
 *  con N=5 -> sett1 lun/mar/mer + sett2 lun/mar, l'ultima e' il mar sett. 2).
 *  @param selectedWeekdays mai vuoto (il chiamante mette almeno baseDow).
 *  @param every "ogni N settimane".
 *  @return la data dell'ultima (N-esima) occorrenza combinata. */
QDate calculateNthWeeklyDate(QDate startDate, int baseDow,
                              const std::vector<int>& selectedWeekdays,
                              int every, int count);

} // namespace app::RecurrenceBuilder
