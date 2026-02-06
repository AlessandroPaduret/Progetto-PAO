#include "Event.h"
#include "RecurrentEvent.h"
#include "CommonTypes.h"
#include <iostream>

int main(){

    TimePoint adesso = std::chrono::time_point_cast<std::chrono::seconds>(Clock::now());
    Duration ora = Duration(3600); // 1 hour
    Duration giorno = Duration(86400); // 1 day
    

    Event* meeting = new Event("Team Meeting", adesso, ora);
    Event* appuntamento = new Event("Doctor Appointment", adesso + ora*3/2 , ora);

    std::cout << *meeting << "\n" << *appuntamento << "\n";
    
    std::cout << "Gli eventi "<< meeting->getTitle() <<" e " << appuntamento->getTitle();
    if (! meeting->overlapsWith(*appuntamento))
        std::cout << " non";

    std::cout << " si sovrappongono." << "\n\n";

    //test dei metodi
    

    //test recurrent event overlapsWith
    RecurrentEvent* recEvent = new RecurrentEvent("Yoga Class", adesso, ora, giorno, adesso + giorno * 7); // Ogni giorno per una settimana

    //test addException e isException
    TimePoint exceptionDate = adesso + giorno * 2; // Secondo giorno
    recEvent->addException(exceptionDate);
    if(recEvent->isException(exceptionDate)){
        std::cout << "Eccezione aggiunta correttamente per il " << Clock::to_time_t(exceptionDate) << "\n";
    } else {
        std::cout << "Errore nell'aggiunta dell'eccezione per il " << Clock::to_time_t(exceptionDate) << "\n";
    }

    //test addModification e isModified
    TimePoint modificationDate = adesso + giorno * 3; // Terzo giorno
    Event modifiedEvent("Special Yoga Class", modificationDate, ora + Duration(1800)); // 1.5 hours
    recEvent->addModification(modifiedEvent);

    if(recEvent->isModified(modificationDate)){
        std::cout << "Modifica aggiunta correttamente per il " << Clock::to_time_t(modificationDate) << "\n";
    } else {
        std::cout << "Errore nell'aggiunta della modifica per il " << Clock::to_time_t(modificationDate) << "\n";
    }

    std::cout << *recEvent << std::endl;

    if (recEvent->overlapsWith(*meeting)) {
        std::cout << "Il meeting si sovrappone con la classe di yoga." << std::endl;
    } else {
        std::cout << "Il meeting non si sovrappone con la classe di yoga." << std::endl;
    }

    //test getOccurrences
    TimePoint from = adesso;
    TimePoint to = adesso + giorno * 4; // Prossimi 4

    std::vector<std::unique_ptr<Event>> occurrences = recEvent->getOccurrences(from, to);
    std::cout << "Occorrenze di '" << recEvent->getTitle() << "' da " << Clock::to_time_t(from)
              << " a " << Clock::to_time_t(to) << ":\n";
    
    for (const auto& occ : occurrences) {
        std::cout << *occ << std::endl;
    }

    std::cout << "lunghezza occurrences: " << occurrences.size() << "\n";

    std::cout << "Fine del test." << std::endl;

    delete meeting;
    delete appuntamento;
    delete recEvent;

    return 0;
}
