#ifndef EVENT_H  // Header Guard
#define EVENT_H  

#include "Schedulable.h"

#include <string>
#include <chrono>
#include <iostream>

/**
 * @class Event
 * @brief Classe base astratta (o concreta in questo caso) per la gestione di un evento temporale.
 * * Rappresenta l'entità principale del tuo schema "Eventi".
 */


using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock, std::chrono::seconds>;
using Duration = std::chrono::seconds;
using String = std::string;

class Event : public Schedulable {
private:
    String m_title;    // Titolo o descrizione dell'evento
    TimePoint m_start;      // Data e ora di inizio
    Duration m_duration;    // Durata dell'evento (es. 1h, 30min)
    
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

    /** @return Il titolo dell'evento */
    String getTitle() const;

    /** @return Il punto temporale (data/ora) di inizio */
    TimePoint getStart() const override;
    
    /** @return La durata dell'evento */
    Duration getDuration() const override;
    
    /** @return Il punto temporale (data/ora) di fine */
    TimePoint getEnd() const override;
    
    /** @param from Inizio del range 
     *  @param to Fine del range
     *  @return Se l'evento è compreso nel range specificato */
    bool isIn(const TimePoint from, const TimePoint to) const override;
    
    /** @param other Altro evento
     *  @return Se l'evento si sovrappone con un altro evento */
    bool overlapsWith(const Schedulable& other) const override;

    /** @param delta Intervallo di tempo
     *  @brief Posticipa l'evento di un certo intervallo di tempo */
    void postpone(const Duration delta) override;

    /** @param delta Intervallo di tempo
     *  @brief Anticipa l'evento di un certo intervallo di tempo */
    void advance(const Duration delta) override;

    /** @brief Operatore di output per stampare i dettagli dell'evento */
    friend std::ostream& operator<<(std::ostream& os, const Event& event);

};

#endif // EVENT_H