#pragma once

#include <QString>
#include <QStringList>

#include "events.h"

namespace app {

/** @brief Helper di visualizzazione per le attivita': etichette calcolate
 *  con i Visitor (nessuna stringa di tipo dal modello per il controllo di
 *  flusso; le stringhe qui sono solo per il display). */
namespace ActivityViewHelpers {

/** @brief Etichetta del tipo ("Evento", "Ricorrente", "Scadenza", "Promemoria"). */
QString typeLabel(const events::Activity& activity);

/** @brief Riga descrittiva sintetica (data/ora + regola) per le liste. */
QString summaryLabel(const events::Activity& activity);

/** @brief Regola di ricorrenza leggibile ("ogni 7 giorni", "ogni anno", ...). */
QString recurrenceRuleLabel(const events::Activity& activity);

/** @brief Durata leggibile ("1 h 30 min"). */
QString durationLabel(const events::Duration duration);

/** @brief Righe "campo: valore" specifiche per tipo (per il dettaglio). */
QStringList fieldLines(const events::Activity& activity);

} // namespace ActivityViewHelpers

} // namespace app
