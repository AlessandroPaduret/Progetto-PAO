#ifndef MEETING_H
#define MEETING_H

#include <chrono>
#include <memory>
#include <vector>

#include "events/core/Activity.h"
#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"
#include "events/core/Occurrence.h"

namespace events {

/**
 * @class Meeting
 * @brief Riunione: sotto-classe di Activity con luogo e partecipanti.
 *
 * Eredita da Activity l'intervallo temporale (inizio + durata) e la
 * regola di ricorrenza (per default un evento singolo).
 */
class Meeting : public Activity {
private:
    String m_location;                    ///< Luogo o link
    std::vector<String> m_attendees;      ///< Partecipanti (senza duplicati)

protected:
    Meeting* clone_impl() const override;

public:
    /** @brief Costruttore.
     *  @param title Titolo della riunione
     *  @param duration Durata (default: zero)
     *  @param location Luogo o link (default: vuoto)
     *  @param generator Regola di ricorrenza (default: nullptr = SingleGenerator(now))
     *  @throws std::invalid_argument se la durata e' negativa
     */
    Meeting(const String& title = "",
            const Duration duration = Duration::zero(),
            const String& location = "",
            std::shared_ptr<DateGenerator> generator = nullptr);

    /** @return Il luogo o link della riunione */
    String getLocation() const;

    /** @brief Imposta il luogo o link della riunione */
    void setLocation(const String& location);

    /** @return Il numero di partecipanti */
    size_t attendeeCount() const;

    /** @return L'elenco dei partecipanti (riferimento read-only) */
    const std::vector<String>& getAttendees() const;

    /** @brief Aggiunge un partecipante (i duplicati sono rifiutati)
     *  @return true se il partecipante e' stato aggiunto
     */
    bool addAttendee(const String& attendee);

    /** @brief Rimuove un partecipante
     *  @return true se il partecipante era presente ed e' stato rimosso
     */
    bool removeAttendee(const String& attendee);

    /// Implementazione dei metodi virtuali di Activity

    /** @return Descrizione testuale della riunione (solo visualizzazione) */
    String describe() const override;

    /** @brief Doppio dispatch verso ActivityVisitor::visit(const Meeting&) */
    void accept(ActivityVisitor& visitor) const override;

    /** @brief Crea una copia della riunione */
    std::unique_ptr<Meeting> clone() const;
};

} // namespace events

#endif // MEETING_H