#ifndef MEETING_H
#define MEETING_H

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/Occurrence.h"

namespace events {

/**
 * @class Meeting
 * @brief Riunione/appuntamento con luogo e partecipanti: un intervallo
 *        temporale come un Evento, piu' il posto (fisico o link) e l'elenco
 *        delle persone coinvolte.
 *
 * Rispetto a Event aggiunge dati e interazioni proprie: addAttendee /
 * removeAttendee / getAttendees e getLocation. La spunta (evasa) e' ereditata
 * dalla radice.
 */
class Meeting : public Activity {
private:
    TimePoint m_start;              ///< Data e ora di inizio
    Duration m_duration;            ///< Durata della riunione
    String m_location;              ///< Luogo (o link) della riunione
    std::vector<String> m_attendees;  ///< Partecipanti

protected:
    Meeting* clone_impl() const override;

public:
    /** @brief Costruttore.
     *  @param title Titolo della riunione
     *  @param start Inizio (default: ora attuale)
     *  @param duration Durata (default: zero)
     *  @param location Luogo/link (default: vuoto)
     *  @throws std::invalid_argument se duration e' negativa
     */
    explicit Meeting(const String& title = "",
                     const TimePoint start =
                         std::chrono::time_point_cast<std::chrono::seconds>(
                             Clock::now()),
                     const Duration duration = Duration::zero(),
                     const String& location = "");

    /** @return Il punto temporale di inizio */
    TimePoint getStart() const override;

    /** @return La durata della riunione */
    Duration getDuration() const;

    /** @return Il punto temporale di fine */
    TimePoint getEnd() const;

    /** @brief Imposta l'orario di inizio */
    void setStart(TimePoint start);

    /** @brief Imposta la durata @throws std::invalid_argument se negativa */
    void setDuration(Duration duration);

    /** @return Il luogo (o link) della riunione */
    String getLocation() const;

    /** @brief Imposta il luogo (o link) della riunione */
    void setLocation(const String& location);

    /** @return Il numero di partecipanti */
    size_t attendeeCount() const;

    /** @return L'elenco dei partecipanti */
    const std::vector<String>& getAttendees() const;

    /** @brief Aggiunge un partecipante (se non gia' presente)
     *  @param attendee Nome del partecipante
     *  @return true se aggiunto, false se gia' presente
     */
    bool addAttendee(const String& attendee);

    /** @brief Rimuove un partecipante
     *  @param attendee Nome del partecipante
     *  @return true se rimosso, false se non presente
     */
    bool removeAttendee(const String& attendee);

    /// Implementazione dei metodi virtuali di Activity

    /** @return La singola occorrenza se l'inizio e' in [from, to] */
    std::vector<Occurrence> occurrencesIn(TimePoint from, TimePoint to) const override;

    /** @brief Sposta la riunione al nuovo inizio (durata invariata). */
    void moveTo(TimePoint newStart) override;

    /** @return Descrizione testuale della riunione (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const Meeting&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia della riunione */
    std::unique_ptr<Meeting> clone() const;
};

} // namespace events

#endif // MEETING_H