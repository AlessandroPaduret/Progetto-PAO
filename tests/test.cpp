#include <iostream>
#include <memory>
#include <vector>

#include "events/events.h"

using namespace std::chrono_literals;

using namespace events;

int main() {
    // 1. Setup Date: 
    TimePoint start = std::chrono::time_point_cast<Duration>(Clock::now());
    TimePoint endRange = start + 4_weeks;
    std::cout << "Data di partenza: " << start << "\n";
    std::cout << "Data di fine range: " << endRange << "\n";

    // 2. Creazione evento ricorrente settimanale
    std::unique_ptr<RecurrentEvent> weeklyMeeting = EventFactory::createSimpleWeekly("Riunione Settimanale", start, 1h, endRange);
    std::cout << "Evento ricorrente creato: " << *weeklyMeeting << "\n";

    // 3. Aggiunta di un'eccezione (salta la seconda settimana)
    TimePoint secondWeek = start + 1_weeks;
    weeklyMeeting->addException(secondWeek);
    std::cout << "--- Eccezione aggiunta per la data: " << secondWeek << " ---\n";

    // 4. Aggiunta di una modifica (anticipa di unora la riunione della terza settimana, aumenta durata e cambia titolo)
    TimePoint thirdWeek = start + std::chrono::weeks(2);
    std::unique_ptr<Event> specialEvent = std::make_unique<Event>("Sessione Straordinaria", thirdWeek - 1h, 2h);
    weeklyMeeting->addModification(thirdWeek, std::move(specialEvent));
    std::cout << "--- Modifica aggiunta per la data: " << thirdWeek << " ---\n";

    // 5. Generazione e verifica
    std::cout << "\nGenerazione eventi per le prossime 4 settimane:\n";
    for (const auto& ev : weeklyMeeting->getSchedulable(start, start + 4_weeks)) {
        std::cout << *ev << std::endl;
    }

    // Test compleanno
    auto birthday = EventFactory::createBirthday("Mario Rossi", 2026y/2/28);
    std::cout << "\nEvento ricorrente compleanno creato:\n" << *birthday << "\n";

    // Generazione compleanni per i prossimi 5 anni
    std::cout << "\nGenerazione compleanni per i prossimi 5 anni:\n";

    for (const auto& ev : birthday->getSchedulable(start, start + 5_years)) {
        std::cout << *ev << std::endl;
    }

    // Literals
    std::cout << 2026y/2/28 << "\n";

    return 0;
}