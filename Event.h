#ifndef EVENT_H  // Header Guard
#define EVENT_H  

#include <string>
#include <chrono>

/**
 * @class Event
 * @brief Classe base astratta (o concreta in questo caso) per la gestione di un evento temporale.
 * * Rappresenta l'entità principale del tuo schema "Eventi".
 */


using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock, std::chrono::seconds>;
using Duration = std::chrono::seconds;

class Event {
private:
    std::string m_title;    // Titolo o descrizione dell'evento
    TimePoint m_start;      // Data e ora di inizio
    Duration m_duration;    // Durata dell'evento (es. 1h, 30min)
    
public:
    /**
     * @brief Costruttore con parametri opzionali.
     * @param title Nome dell'evento (default: stringa vuota).
     * @param start Orario di inizio (default: ora attuale).
     * @param duration Durata (default: zero).
     */
    Event(const std::string& title = "", 
          const TimePoint start = std::chrono::time_point_cast<std::chrono::seconds>(Clock::now()), 
          const Duration duration = Duration::zero());

    // --- Getter (Metodi di accesso) ---

    /** @return Il titolo dell'evento */
    std::string getTitle() const;

    /** @return Il punto temporale (data/ora) di inizio */
    TimePoint getStart() const;
    /** @return La durata dell'evento */
    Duration getDuration() const;

    /** * @brief Calcola il momento della fine dell'evento.
     * @return time_point risultante dalla somma di inizio e durata.
     */
    TimePoint getEnd() const;
};

#endif // EVENT_H