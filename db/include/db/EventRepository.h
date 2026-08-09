#ifndef DB_EVENT_REPOSITORY_H
#define DB_EVENT_REPOSITORY_H

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "db/ConnectionPool.h"

namespace db {

/** @brief Tipo di ricorrenza di un evento persistito.
 *
 *  Corrisponde al campo `tipo` della tabella `eventi`:
 *  `Single` -> 'SINGLE', `Fixed` -> 'FIXED', `Yearly` -> 'YEARLY'.
 */
enum class RecurrenceKind { Single, Fixed, Yearly };

/** @brief Rappresentazione neutrale di un evento persistito.
 *
 *  I timestamp sono memorizzati con precisione al secondo, come
 *  `events::TimePoint` del modello (epoch seconds nel DB).
 */
struct EventRecord {
    long long id = 0;                       /**< id dell'evento nel DB (0 se non persistito) */
    long long userId = 0;                   /**< id del proprietario (utente) */
    std::string title;                      /**< titolo dell'evento */
    std::chrono::system_clock::time_point start; /**< inizio (precisione al secondo) */
    std::chrono::seconds duration;          /**< durata in secondi (>= 0) */
    RecurrenceKind kind = RecurrenceKind::Single; /**< tipo di ricorrenza */
    std::chrono::seconds interval{};        /**< intervallo in secondi, solo per Fixed */
    std::optional<std::chrono::system_clock::time_point> end; /**< fine ricorrenza; nullopt = illimitata */
    std::vector<std::chrono::system_clock::time_point> exceptions; /**< eccezioni (EXDATE) */
};

/** @brief Repository degli eventi (CRUD + eccezioni) su PostgreSQL. */
class EventRepository {
public:
    /** @brief Costruttore
     *  @param pool Pool di connessioni da cui acquisire le transazioni
     */
    explicit EventRepository(std::shared_ptr<ConnectionPool> pool);

    /** @brief Inserisce un evento (semplice o ricorrente secondo `kind`) con le sue eccezioni.
     *  @param userId Proprietario dell'evento
     *  @param record Dati dell'evento da persistere
     *  @return L'id generato dal database
     */
    long long createEvent(long long userId, const EventRecord& record);

    /** @brief Carica tutti gli eventi dell'utente, eccezioni incluse.
     *  @param userId Proprietario degli eventi
     *  @return Gli eventi ordinati per data di inizio
     */
    std::vector<EventRecord> getAllEvents(long long userId);

    /** @brief Carica gli eventi dell'utente che possono produrre occorrenze in [from, to].
     *
     *  Filtro applicato in SQL:
     *  - `Single`: l'inizio è in [from, to];
     *  - `Fixed`/`Yearly`: `inizio <= to` e (`fine` NULL oppure `fine >= from`),
     *    cioè la ricorrenza può cadere nel range (il calcolo preciso delle
     *    occorrenze è delegato al modello con `getSchedulable`).
     *
     *  @param userId Proprietario degli eventi
     *  @param from Inizio del range (inclusivo)
     *  @param to Fine del range (inclusivo)
     *  @return Gli eventi candidati, ordinati per data di inizio, eccezioni incluse
     */
    std::vector<EventRecord> getEvents(long long userId,
                                       std::chrono::system_clock::time_point from,
                                       std::chrono::system_clock::time_point to);

    /** @brief Aggiunge un'eccezione (EXDATE) a un evento.
     *  @param eventId Evento a cui aggiungere l'eccezione
     *  @param date Occorrenza da escludere
     *  @return true se l'eccezione è stata aggiunta (false se già presente)
     */
    bool addException(long long eventId, std::chrono::system_clock::time_point date);

    /** @brief Rimuove un'eccezione (EXDATE) da un evento.
     *  @param eventId Evento da cui rimuovere l'eccezione
     *  @param date Occorrenza da ripristinare
     *  @return true se l'eccezione esisteva ed è stata rimossa
     */
    bool removeException(long long eventId, std::chrono::system_clock::time_point date);

    /** @brief Elimina un evento, solo se appartiene all'utente.
     *  @param eventId Evento da eliminare
     *  @param userId Proprietario richiesto
     *  @return true se l'evento esisteva ed è stato eliminato
     */
    bool deleteEvent(long long eventId, long long userId);

    /** @brief Verifica che l'evento appartenga all'utente.
     *  @param eventId Evento da verificare
     *  @param userId Proprietario richiesto
     *  @return true se l'evento esiste ed è di proprietà dell'utente
     */
    bool belongsToUser(long long eventId, long long userId);

private:
    std::shared_ptr<ConnectionPool> m_pool; /**< pool di connessioni */

    /** @brief Carica le eccezioni (EXDATE) degli eventi dell'utente e le aggancia ai record.
     *  @param tx Transazione attiva su cui interrogare
     *  @param events Record da arricchire con le eccezioni
     *  @param userId Proprietario degli eventi
     */
    void attachExceptions(pqxx::work& tx, std::vector<EventRecord>& events,
                          long long userId);

    static std::string kindToString(RecurrenceKind kind);   /**< RecurrenceKind -> testo SQL */
    static RecurrenceKind stringToKind(const std::string& s); /**< testo SQL -> RecurrenceKind */
};

} // namespace db

#endif // DB_EVENT_REPOSITORY_H
