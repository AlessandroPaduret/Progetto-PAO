#include <iostream>
#include <memory>
#include <vector>

#include "events/events.h"

using namespace std::chrono_literals;

using namespace events;

int main() {
    // 1. Setup Date: 
    TimePoint start = std::chrono::time_point_cast<Duration>(Clock::now());
    Duration week = Duration(7 * 24 * 3600);
    TimePoint endRange = start + (week * 4); // Range di 4 settimane

    // 2. Creazione evento ricorrente settimanale
    std::unique_ptr<RecurrentEvent> weeklyMeeting = EventFactory::createSimpleWeekly("Riunione Settimanale", start, std::chrono::hours(1), endRange);
    std::cout << "Evento ricorrente creato: " << *weeklyMeeting << "\n";

    // 3. Aggiunta di un'eccezione (salta la seconda settimana)
    TimePoint secondWeek = start + week;
    weeklyMeeting->addException(secondWeek);
    std::cout << "--- Eccezione aggiunta per la data: " << secondWeek << " ---\n";

    // 4. Aggiunta di una modifica (anticipa di unora la riunione della terza settimana, aumenta durata e cambia titolo)
    TimePoint thirdWeek = start + (week * 2);
    std::unique_ptr<Event> specialEvent = std::make_unique<Event>("Sessione Straordinaria", thirdWeek - std::chrono::hours(1), Duration(7200));
    weeklyMeeting->addModification(thirdWeek, std::move(specialEvent));
    std::cout << "--- Modifica aggiunta per la data: " << thirdWeek << " ---\n";

    // 5. Generazione e verifica
    std::cout << "\nGenerazione eventi per le prossime 4 settimane:\n";
    std::vector<std::unique_ptr<Event>> occurrences = weeklyMeeting->getSchedulable(start, endRange - Duration(1));
    for (const auto& ev : occurrences) {
        std::cout << *ev << std::endl;
    }


    // Test compleanno
    auto birthday = EventFactory::createBirthday("Mario Rossi", 2026y/2/28);
    std::cout << "\nEvento ricorrente compleanno creato:\n" << *birthday << "\n";

    // Generazione compleanni per i prossimi 5 anni
    TimePoint endBirthdayRange = start + Duration(365 * 5 * 24 * 3600);
    std::cout << "\nGenerazione compleanni per i prossimi 5 anni:\n";
    std::vector<std::unique_ptr<Event>> birthdayOccurrences = birthday->getSchedulable(start, endBirthdayRange);
    for (const auto& ev : birthdayOccurrences) {
        std::cout << *ev << std::endl;
    }

    // Literals
    std::cout << 2026y/2/28 << "\n";

    return 0;
}