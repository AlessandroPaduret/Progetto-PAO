#include <memory>
#include <string>

#include "events/core/CommonTypes.h"
#include "events/domain/Event.h"
#include "events/domain/EventFactory.h"
#include "events/domain/RecurrentEvent.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "events/providers/NullProvider.h"
#include "events/generators/YearlyGenerator.h"

namespace events {
    
    std::unique_ptr<Event> EventFactory::createSimpleEvent(const String& title, TimePoint start, Duration duration) {
        return std::make_unique<Event>(title, start, duration);
    }

    std::unique_ptr<RecurrentEvent> EventFactory::createRecurrentEvent(const String& title, TimePoint start, Duration duration, Duration interval) {
        auto gen = std::make_shared<FixedIntervalGenerator>(start, interval);
        auto provider = std::make_shared<NullProvider<Event>>();
        
        RecurrenceStrategy<Event> strategy(gen, provider);
        return std::make_unique<RecurrentEvent>(std::move(strategy), Event(title, start, duration));
    }

    std::unique_ptr<RecurrentEvent> EventFactory::createBirthday(const String& name, std::chrono::year_month_day date) {

        std::chrono::sys_days midnightDays = std::chrono::sys_days{date};

        // 2. Converti nel TimePoint usato dal tuo sistema (es. precisione secondi)
        TimePoint midnightStart = std::chrono::time_point_cast<Duration>(midnightDays);

        // 3. Usa midnightStart per il generatore
        auto gen = std::make_shared<YearlyGenerator>(midnightStart);

        auto provider = std::make_shared<NullProvider<Event>>();
        RecurrenceStrategy<Event> strategy(gen, provider);

        // 4. Anche il template deve avere la stessa mezzanotte
        Event templateEvent(name + " - Compleanno", midnightStart, std::chrono::hours(24) - std::chrono::seconds(1)); // Durata di un giorno meno 1 secondo per evitare sovrapposizioni

        return std::make_unique<RecurrentEvent>(std::move(strategy), std::move(templateEvent));
    }

    std::unique_ptr<RecurrentEvent> EventFactory::createSimpleWeekly(const String& title, TimePoint start, Duration duration, TimePoint end) {
        auto gen = std::make_shared<FixedIntervalGenerator>(start, std::chrono::hours(24 * 7), end);
        auto provider = std::make_shared<NullProvider<Event>>();
        
        RecurrenceStrategy<Event> strategy(gen, provider);
        return std::make_unique<RecurrentEvent>(std::move(strategy), Event(title, start, duration));
    }





} // namespace events
