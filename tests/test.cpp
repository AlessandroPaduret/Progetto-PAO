#include <iostream>
#include <memory>
#include <vector>

#include "events/events.h"

using namespace std::chrono_literals;

using namespace events;

int main() {
    // 1. Setup Date: Inizio oggi, ricorrenza ogni 7 giorni (settimanale)
    TimePoint start = std::chrono::time_point_cast<Duration>(Clock::now());
    Duration week = Duration(7 * 24 * 3600);
    TimePoint endRange = start + (week * 4); // Range di 4 settimane

    std::unique_ptr<RecurrentEvent> weeklyMeeting = EventFactory::createSimpleWeekly("Riunione Settimanale", start,std::chrono::hours(1), endRange);
    std::cout << "Evento ricorrente creato: " << *weeklyMeeting << "\n";

    // 4. Aggiunta di un'eccezione (salta la seconda settimana)
    TimePoint secondWeek = start + week;
    weeklyMeeting->addException(secondWeek);
    std::cout << "--- Eccezione aggiunta per la data: " << secondWeek.time_since_epoch().count() << " ---\n";

    // 5. Aggiunta di una modifica (cambia titolo alla terza settimana)
    TimePoint thirdWeek = start + (week * 2);
    auto specialEvent = std::make_unique<Event>("Sessione Straordinaria", thirdWeek, Duration(7200));
    weeklyMeeting->addModification(thirdWeek, std::move(specialEvent));
    std::cout << "--- Modifica aggiunta per la data: " << thirdWeek.time_since_epoch().count() << " ---\n";

    // 6. Generazione e verifica
    std::cout << "\nGenerazione eventi per le prossime 4 settimane:\n";
    auto occurrences = weeklyMeeting->getSchedulable(start, endRange - Duration(1));

    for (const auto& ev : occurrences) {
        std::cout << *ev << std::endl;
    }


    // Test compleanno
    auto birthday = EventFactory::createBirthday("Mario Rossi", 2026y/2/28);
    std::cout << "\nEvento ricorrente compleanno creato: " << *birthday << "\n";

    // Generazione compleanni per i prossimi 5 anni
    TimePoint endBirthdayRange = start + Duration(365 * 5 * 24 * 3600);
    std::cout << "\nGenerazione compleanni per i prossimi 5 anni:\n";
    auto birthdayOccurrences = birthday->getSchedulable(start, endBirthdayRange);

    for (const auto& ev : birthdayOccurrences) {
        std::cout << *ev << std::endl;
    }

    std::cout << "\n" << 2026y/2/28;

    return 0;
}