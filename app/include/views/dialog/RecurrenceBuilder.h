#pragma once

#include <QDate>

#include <vector>

#include "events.h"

namespace app::RecurrenceBuilder {

/** @brief Calcola la fine di una serie DOPO N occorrenze, avanzando il
 *  generatore a partire da `seriesStart` (align + next ripetuto): il
 *  modello non ha un limite di conteggio nativo (nessun "maxOccurrences"
 *  sul generatore, stateless per progetto), quindi "dopo N occorrenze" si
 *  traduce qui in un `end` compatibile con `events::Activity`, mantenendo
 *  corretto il clamping di calendario (fine mese, anni bisestili, ...) che
 *  avanzare a mano il generatore stesso garantisce automaticamente.
 *  @param generator generatore della serie (Days/Months/Years: un solo
 *  generatore descrive l'intera sequenza).
 *  @param seriesStart inizio della serie.
 *  @param count numero di occorrenze desiderato (>= 1).
 *  @return la data dell'ultima (N-esima) occorrenza. */
events::TimePoint calculateEndAfterCount(const events::DateGenerator& generator,
                                          events::TimePoint seriesStart, int count);

/** @brief Calcola la data dell'N-esima occorrenza COMBINATA di una serie
 *  settimanale su piu' giorni della settimana (es. lun+mer+ven "ogni 1
 *  settimana"): non e' rappresentabile avanzando un solo generatore, perche'
 *  ogni giorno scelto e' in realta' una serie `FixedIntervalGenerator`
 *  separata (una per giorno) che condividono solo la fine. Si calcola quindi
 *  con l'aritmetica sugli offset dei giorni scelti rispetto al giorno
 *  d'inizio: es. lun+mar+mer con N=5 -> sett1 lun/mar/mer + sett2 lun/mar
 *  = 5 occorrenze totali, l'ultima e' il mar della settimana 2.
 *  @param startDate data di inizio della serie.
 *  @param baseDow giorno della settimana di `startDate` (QDate::dayOfWeek(),
 *  1=Lun..7=Dom).
 *  @param selectedWeekdays giorni scelti (stessa convenzione di `baseDow`);
 *  mai vuoto (il chiamante ci mette almeno `baseDow` come fallback).
 *  @param every "ogni N settimane".
 *  @param count numero di occorrenze combinate desiderato (>= 1).
 *  @return la data dell'ultima (N-esima) occorrenza combinata. */
QDate calculateNthWeeklyDate(QDate startDate, int baseDow,
                              const std::vector<int>& selectedWeekdays,
                              int every, int count);

} // namespace app::RecurrenceBuilder
