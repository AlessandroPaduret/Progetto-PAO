#include <iostream>
#include <memory>
#include <vector>

#include "events/events.h"

using namespace events;

int main() {
    // 1. Setup Date: Inizio oggi, ricorrenza ogni 7 giorni (settimanale)
    TimePoint start = std::chrono::time_point_cast<Duration>(Clock::now());
    Duration week = Duration(7 * 24 * 3600);
    TimePoint endRange = start + (week * 4); // Range di 4 settimane

    // 2. Creazione componenti della strategia
    auto gen = std::make_shared<FixedIntervalGenerator>(start, week, endRange); // Generatore di ricorrenze settimanali
    auto base = std::make_shared<NullProvider<Event>>(); // Provider base che non fornisce modifiche

    // 3. Creazione Strategia e Evento Ricorrente
    RecurrenceStrategy<Event> strategy(gen, base);
    Event templateEvent("Riunione Settimanale", start, Duration(3600));
    RecurrentEvent weeklyMeeting(std::move(strategy), templateEvent);

    // 4. Aggiunta di un'eccezione (salta la seconda settimana)
    TimePoint secondWeek = start + week;
    weeklyMeeting.addException(secondWeek);
    std::cout << "--- Eccezione aggiunta per la data: " << secondWeek.time_since_epoch().count() << " ---\n";

    // 5. Aggiunta di una modifica (cambia titolo alla terza settimana)
    TimePoint thirdWeek = start + (week * 2);
    auto specialEvent = std::make_unique<Event>("Sessione Straordinaria", thirdWeek, Duration(7200));
    weeklyMeeting.addModification(thirdWeek, std::move(specialEvent));
    std::cout << "--- Modifica aggiunta per la data: " << thirdWeek.time_since_epoch().count() << " ---\n";

    // 6. Generazione e verifica
    std::cout << "\nGenerazione eventi per le prossime 4 settimane:\n";
    auto occurrences = weeklyMeeting.getSchedulable(start, endRange - Duration(1));

    for (const auto& ev : occurrences) {
        std::cout << *ev << std::endl;
    }

    return 0;
}