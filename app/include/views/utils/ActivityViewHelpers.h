#pragma once

#include <QString>
#include <QStringList>

#include "events.h"

namespace app {

/** @brief Etichette calcolate con i Visitor: niente stringa di tipo dal
 *  modello per il controllo di flusso, solo per il display. */
namespace ActivityViewHelpers {

/** @brief "Evento", "Ricorrente", "Scadenza", "Promemoria". */
QString typeLabel(const events::Activity& activity);

/** @brief Riga sintetica (data/ora + regola) per le liste. */
QString summaryLabel(const events::Activity& activity);

/** @brief "ogni 7 giorni", "ogni anno", ... */
QString recurrenceRuleLabel(const events::Activity& activity);

/** @brief "1 h 30 min". */
QString durationLabel(const events::Duration duration);

} // namespace ActivityViewHelpers

} // namespace app
