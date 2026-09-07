#pragma once

#include <chrono>
#include <memory>
#include <vector>
#include <cstddef>

#include "core/Activity.h"
#include "core/CommonTypes.h"
#include "core/DateGenerator.h"

namespace events {

/**
 * @class Meeting
 * @brief Riunione: sotto-classe di Activity con luogo e partecipanti.
 */
class Meeting : public Activity {
private:
    String m_location;               ///< Luogo o link della riunione
    std::vector<String> m_attendees;  ///< Elenco dei partecipanti (senza duplicati)

public:
    /**
     * @brief Costruttore della riunione.
     * @param title Titolo della riunione (obbligatorio)
     * @param start Data e ora d'inizio (obbligatorio)
     * @param duration Durata della riunione (default: 0)
     * @param location Luogo o link (default: vuoto)
     * @param generator Regola di ricorrenza (default: nullptr = SingleGenerator)
     * @param end Data limite di fine serie (default: max)
     * @param maxOccurrences Limite massimo di occorrenze (default: 0 = illimitate)
     */
    explicit Meeting(String title,
                     TimePoint start,
                     Duration duration = Duration::zero(),
                     String location = "",
                     std::shared_ptr<const DateGenerator> generator = nullptr,
                     TimePoint end = TimePoint::max());

    ~Meeting() override = default;

    /** @return Il luogo o link della riunione */
    String getLocation() const { return m_location; }

    /** @brief Imposta il luogo o link della riunione */
    void setLocation(const String& location) { m_location = location; }

    /** @return Il numero di partecipanti */
    std::size_t attendeeCount() const { return m_attendees.size(); }

    /** @return L'elenco dei partecipanti (riferimento read-only) */
    const std::vector<String>& getAttendees() const { return m_attendees; }

    /** 
     * @brief Aggiunge un partecipante (i duplicati vengono rifiutati).
     * @return true se il partecipante e' stato aggiunto.
     */
    bool addAttendee(const String& attendee);

    /** 
     * @brief Rimuove un partecipante.
     * @return true se il partecipante era presente ed e' stato rimosso.
     */
    bool removeAttendee(const String& attendee);

    /// @inheritdoc
    String describe() const override;

    /// @inheritdoc
    void accept(ActivityVisitor& visitor) const override;

    /// @inheritdoc
    [[nodiscard]] std::unique_ptr<Activity> clone() const override;
};

} // namespace events