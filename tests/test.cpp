#include <iostream>
#include <memory>
#include <vector>

#include "events/events.h"

using namespace std::chrono_literals;

using namespace events;

int main() {
    // 1. Setup Date: 
    TimePoint start = std::chrono::time_point_cast<Duration>(Clock::now());
    TimePoint endRange = start + std::chrono::weeks(4);
    std::cout << "Data di partenza: " << start << "\n";
    std::cout << "Data di fine range: " << endRange << "\n";

    // 2. Creazione evento ricorrente settimanale
    std::unique_ptr<RecurrentEvent> weeklyMeeting = EventFactory::createSimpleWeekly("Riunione Settimanale", start, std::chrono::hours(1), endRange);
    std::cout << "Evento ricorrente creato: " << *weeklyMeeting << "\n";

    // 3. Aggiunta di un'eccezione (salta la seconda settimana)
    TimePoint secondWeek = start + std::chrono::weeks(1);
    weeklyMeeting->addException(secondWeek);
    std::cout << "--- Eccezione aggiunta per la data: " << secondWeek << " ---\n";

    // 4. Aggiunta di una modifica (anticipa di unora la riunione della terza settimana, aumenta durata e cambia titolo)
    TimePoint thirdWeek = start + std::chrono::weeks(2);
    std::unique_ptr<Event> specialEvent = std::make_unique<Event>("Sessione Straordinaria", thirdWeek - std::chrono::hours(1), Duration(7200));
    weeklyMeeting->addModification(thirdWeek, std::move(specialEvent));
    std::cout << "--- Modifica aggiunta per la data: " << thirdWeek << " ---\n";

    // 4b. Modifica di un evento già modificato usando il suo nuovo orario di inizio
    weeklyMeeting->addModification(thirdWeek - std::chrono::hours(1), std::make_unique<Event>("Sessione Super Straordinaria", thirdWeek - std::chrono::hours(1), Duration(10800)));
    std::cout << "--- Modifica aggiornata per la data: " << thirdWeek << " ---\n";

    // 5. Generazione e verifica
    std::cout << "\nGenerazione eventi per le prossime 4 settimane:\n";
    for (const auto& ev : weeklyMeeting->getSchedulable(start, start + std::chrono::weeks(4))) {
        std::cout << *ev << std::endl;
    }

    // Test compleanno
    auto birthday = EventFactory::createBirthday("Mario Rossi", 2026y/2/28);
    std::cout << "\nEvento ricorrente compleanno creato:\n" << *birthday << "\n";

    // Generazione compleanni per i prossimi 5 anni
    std::cout << "\nGenerazione compleanni per i prossimi 5 anni:\n";

    for (const auto& ev : birthday->getSchedulable(start, start + std::chrono::years(5))) {
        std::cout << *ev << std::endl;
    }

    // Literals
    std::cout << 2026y/2/28 << "\n";

    return 0;
}