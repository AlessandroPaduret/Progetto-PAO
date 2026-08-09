#ifndef SERVER_MAPPERS_H
#define SERVER_MAPPERS_H

#include <memory>

#include <nlohmann/json.hpp>

#include "db/EventRepository.h"
#include "events/domain/Event.h"
#include "events/domain/RecurrentEvent.h"

namespace server {

/** @brief Converte un EventRecord semplice nel modello events::Event.
 *  @param record Record di persistenza
 *  @return Evento del modello
 */
std::unique_ptr<events::Event> toSimpleEvent(const db::EventRecord& record);

/** @brief Converte un EventRecord ricorrente nel modello events::RecurrentEvent.
 *
 *  Ricostruisce il generatore (FixedIntervalGenerator/YearlyGenerator da
 *  kind/interval/end) e le eccezioni nel set del modello.
 *
 *  @param record Record di persistenza
 *  @return Evento ricorrente del modello
 */
std::unique_ptr<events::RecurrentEvent> toRecurrentEvent(const db::EventRecord& record);

/** @brief Serializza un'occorrenza in JSON.
 *  @param eventId Id dell'evento di appartenenza
 *  @param occurrence Occorrenza da serializzare
 *  @return JSON: {"event_id", "title", "start", "end"} con date ISO-8601
 */
nlohmann::json occurrenceToJson(long long eventId, const events::Event& occurrence);

} // namespace server

#endif // SERVER_MAPPERS_H
