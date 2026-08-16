#ifndef APP_ACTIVITY_VIEW_HELPERS_H
#define APP_ACTIVITY_VIEW_HELPERS_H

#include <QString>

#include "events/events.h"

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
QString recurrenceRuleLabel(const events::RecurrentEvent& event);

/** @brief Durata leggibile ("1 h 30 min"). */
QString durationLabel(const events::Duration duration);

} // namespace ActivityViewHelpers

} // namespace app

#endif // APP_ACTIVITY_VIEW_HELPERS_H
