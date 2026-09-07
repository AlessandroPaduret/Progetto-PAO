#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <memory>

#include "events.h"

namespace persistence {

/** @brief Serializza un'attivita' in JSON (Visitor, double dispatch).
 *  @return L'oggetto JSON, con campo "type" tra event|task|meeting.
 */
QJsonObject activityToJson(const events::Activity& activity);

/** @brief Serializza l'intero calendario.
 *  @return La radice del documento: {"version": 1, "activities": [...]}.
 */
QJsonObject calendarToJson(const events::Calendar& calendar);

/** @brief Ricostruisce un'attivita' da un oggetto JSON.
 *  @param error Se non nullo, riceve il messaggio in caso di fallimento.
 *  @return L'attivita', oppure nullptr se il JSON non e' valido.
 */
std::unique_ptr<events::Activity> activityFromJson(const QJsonObject& json, QString* error = nullptr);

/** @brief Aggiunge le attivita' dell'array a un calendario.
 *  @return false se almeno una voce non e' valida (le voci gia' aggiunte restano).
 */
bool calendarFromJsonArray(events::Calendar& calendar, const QJsonArray& array, QString* error = nullptr);

/** @brief Salva il calendario su file (JSON indentato, UTF-8). */
bool saveToFile(const events::Calendar& calendar, const QString& filePath, QString* error = nullptr);

/** @brief Carica il calendario da file, SOSTITUENDO il contenuto attuale. */
bool loadFromFile(events::Calendar& calendar, const QString& filePath, QString* error = nullptr);

} // namespace persistence
