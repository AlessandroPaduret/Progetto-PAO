#ifndef JSON_PERSISTENCE_H
#define JSON_PERSISTENCE_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <memory>

#include "events/events.h"

namespace persistence {

/** @brief Serializza un'attivita' in un oggetto JSON (doppio dispatch via Visitor).
 *  @param activity L'attivita' da serializzare
 *  @return L'oggetto JSON, con campo "type" tra event|recurrent|deadline|reminder
 */
QJsonObject activityToJson(const events::Activity& activity);

/** @brief Serializza l'intero calendario in un documento JSON.
 *  @param calendar Il calendario da serializzare
 *  @return La radice del documento: {"version": 1, "activities": [...]}
 */
QJsonObject calendarToJson(const events::Calendar& calendar);

/** @brief Ricostruisce un'attivita' da un oggetto JSON.
 *  @param json L'oggetto JSON prodotto da activityToJson
 *  @param error Se non nullo, riceve il messaggio di errore in caso di fallimento
 *  @return L'attivita' ricostruita, oppure nullptr se il JSON non e' valido
 */
std::unique_ptr<events::Activity> activityFromJson(const QJsonObject& json, QString* error = nullptr);

/** @brief Aggiunge le attivita' contenute nell'array a un calendario.
 *  @param calendar Il calendario da riempire
 *  @param array L'array di attivita' JSON
 *  @param error Se non nullo, riceve il messaggio di errore in caso di fallimento
 *  @return false se almeno una voce non e' valida (le voci gia' aggiunte restano)
 */
bool calendarFromJsonArray(events::Calendar& calendar, const QJsonArray& array, QString* error = nullptr);

/** @brief Salva il calendario su file (JSON indentato, UTF-8).
 *  @param calendar Il calendario da salvare
 *  @param filePath Percorso del file (scelto dall'utente tramite dialog, mai cablato)
 *  @param error Se non nullo, riceve il messaggio di errore in caso di fallimento
 *  @return true se il salvataggio e' riuscito
 */
bool saveToFile(const events::Calendar& calendar, const QString& filePath, QString* error = nullptr);

/** @brief Carica il calendario da file, SOSTITUENDO il contenuto attuale.
 *  @param calendar Il calendario da popolare (viene svuotato)
 *  @param filePath Percorso del file
 *  @param error Se non nullo, riceve il messaggio di errore in caso di fallimento
 *  @return true se il caricamento e' riuscito
 */
bool loadFromFile(events::Calendar& calendar, const QString& filePath, QString* error = nullptr);

} // namespace persistence

#endif // JSON_PERSISTENCE_H
