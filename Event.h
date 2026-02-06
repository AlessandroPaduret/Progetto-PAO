#ifndef EVENT_H  // Header Guard
#define EVENT_H  

#include "Schedulable.h"

#include <string>
#include <chrono>
#include <iostream>
#include <memory>

/**
 * @class Event
 * @brief Classe base astratta (o concreta in questo caso) per la gestione di un evento temporale.
 * * Rappresenta l'entità principale del tuo schema "Eventi".
 */

class Event : public Schedulable {
private:
    String m_title;         // Titolo o descrizione dell'evento
    TimePoint m_start;      // Data e ora di inizio
    Duration m_duration;    // Durata dell'evento
    
public:
    /**
     * @brief Costruttore con parametri opzionali.
     * @param title Nome dell'evento (default: stringa vuota).
     * @param start Orario di inizio (default: ora attuale).
     * @param duration Durata (default: zero).
     */
    Event(const String& title = "", 
          const TimePoint start = std::chrono::time_point_cast<std::chrono::seconds>(Clock::now()), 
          const Duration duration = Duration::zero());

    /** @brief Imposta il titolo dell'evento */
    void setTitle(const String& title);
    
    /** @return Il titolo dell'evento */
    String getTitle() const;

    /** @brief Operatore di output per stampare i dettagli dell'evento */
    friend std::ostream& operator<<(std::ostream& os, const Event& event);

    /** @brief Crea una copia dell'evento */
    virtual std::unique_ptr<Event> clone() const;

    /// Implementazione dei metodi virtuali di Schedulable

    /** @return Il punto temporale (data/ora) di inizio */
    TimePoint getStart() const override;
    
    /** @return La durata dell'evento */
    Duration getDuration() const override;
    
    /** @return Il punto temporale (data/ora) di fine */
    TimePoint getEnd() const override;

    /** @brief Imposta l'orario di inizio dell'evento */
    void setStart(const TimePoint start) override;

    /** @brief Imposta la durata dell'evento */
    void setDuration(const Duration duration) override;

    /** @brief Imposta l'orario di fine dell'evento quindi modifica durata */
    void setEnd(const TimePoint end) override;
    
    /** @param from Inizio del range 
     *  @param to Fine del range
     *  @return Se l'evento è compreso nel range specificato */
    bool isIn(const TimePoint from, const TimePoint to) const override;
    
    /** @param other Altro evento
     *  @return Se l'evento si sovrappone con un altro evento */
    bool overlapsWith(const Schedulable& other) const override;

};

#endif // EVENT_H