
#include <vector>
#include <chrono>

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock, std::chrono::seconds>;
using Duration = std::chrono::seconds;

class Schedulable {
public:
    /** @return Il punto temporale (data/ora) di inizio */
    virtual TimePoint getStart() const = 0;

    /** @return La durata dell'evento */
    virtual Duration getDuration() const = 0;

    /** @return Il punto temporale (data/ora) di fine */
    virtual TimePoint getEnd() const = 0;

    /** @return Se l'evento è compreso nel range specificato */
    virtual bool isIn(const TimePoint from, const TimePoint to) const = 0;

    /** @return Se l'evento si sovrappone con un altro evento */
    virtual bool overlapsWith(const Schedulable& other) const = 0;

    /** @brief Posticipa l'evento di un certo intervallo di tempo */
    virtual void postpone(const Duration delta) = 0;

    /** @brief Anticipa l'evento di un certo intervallo di tempo */
    virtual void advance(const Duration delta) = 0;
};