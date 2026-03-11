
#include <iostream>
#include <memory>

#include "events/events.h"

using namespace Events;

int main(){

    // Test 1: Creazione di un evento semplice
    Event* ev = new Event("Evento Semplice", now(), Duration(3600));
    std::cout << "Evento creato: " << *ev << "\n";

    // Test 2: Creazione di un insieme di eventi disordinati
    Events* evs = new Events();
    evs->addEvent(new Event("Evento 1", now() + 2h, 2h));
    evs->addEvent(ev);

    // ritorna gli eventi di oggi
    std::cout << "Eventi di oggi:\n";
    Events* evsToday = evs->interval(today(), today() + 1d);

    evsToday->ForEach([](const Event& e) {
        std::cout << e << "\n";
    });

    //oppure anche semplicemente
    std::cout << evs << "\n";

    // Test 3: Creazione di un evento ricorrente settimanale
    std::cout << "\nCreazione di un evento ricorrente settimanale:\n";

    // Test 4: Creazione di un evento ricorrente settimanale con eccezioni e modifiche
    
    // RecEvents(StartEvent, Frequency, EndDate)
    Events* weekly = new RecurrentEvents( new Event("Evento Ricorrente", today() + 10h, 2h),
                                        RecurrentEvents::Frequency::WEEKLY,
                                        today() + 1y);
    // Aggiunta di un'eccezione (salta la seconda settimana)
    weekly->addException(today() + 1w);

    // Aggiunta di una modifica (anticipa di unora la riunione della terza settimana, aumenta durata e cambia titolo)
    Event* specialEvent = new Event("Sessione Straordinaria", today() + 2w - 1h, Duration(7200));
    weekly->addModification(today() + 2w, specialEvent);

    // Modifica evento modificato
    Event* modifiedEvent = new Event("Sessione Straordinaria Modificata", today() + 2w - 2h, Duration(10800));
    weekly->addModification(today() + 2w, modifiedEvent);

    // Genera una sacco di modifiche che non hanno senso (fuori dai tempi dell'evento ricorrente per vedere se vengono ignorate correttamente)
    for (int i = 0; i < 1000; ++i) {
        Event* nonsenseEvent = new Event("Evento Nonsense", today() + 2y + i * 1d, Duration(3600));
        weekly->addModification(today() + 2y + i * 1d, nonsenseEvent);
    }

    // Test 5: Creazione di un evento ricorrente compleanno
    std::cout << "\nCreazione di un evento ricorrente compleanno:\n";
    Events* birthday = new Birthday("Mario Rossi", today() - 30y);
    std::cout << *birthday << "\n";

    // Test 6: Generazione di eventi ricorrenti mensili per i prossimi 6 mesi
    std::cout << "\nGenerazione di eventi ricorrenti mensili per i prossimi 6 mesi:\n";
    Events* monthly = new RecurrentEvents( new Event("Evento Mensile", today() + 7h, 2h),
                                        RecurrentEvents::Frequency::MONTHLY,
                                        today() + 6m);

    // Test 7: salva gli eventi da file json e caricali da file json
    std::cout << "\nSalvataggio e caricamento da file JSON:\n";

    EventVisitor* jsonSaver = new JSONSaver("events.json");
    weekly->accept(jsonSaver);
    evs->accept(jsonSaver);
    birthday->accept(jsonSaver);

    EventVisitor* jsonLoader = new JSONLoader("events.json");
    Events* jsonLoaded = jsonLoader->load();

    //Test 8: confronta gli eventi caricati da file con quelli originali (dovrebbero essere uguali)

    // Unione di tutti gli eventi caricati in un unico insieme
    Events* all = new Events();

    all->addEvents(weekly);
    all->addEvents(evs);
    all->addEvents(birthday);

    // Confronta l'insiem di eventi

    std::cout << "\nEventi caricati da JSON:\n";
    all->ForEach([](const Event& e) {
        std::cout << e << "\n";
    });

    all == loaded ? std::cout << "Caricamento riuscito, gli eventi sono uguali\n" : std::cout << "Caricamento fallito, gli eventi sono diversi\n";
    
    // Test 9: confronta eventi con date e orari diversi (dovrebbero essere diversi)
    Event* differentEvent = new Event("Evento Diverso", today() + 1d, Duration(3600));
    std::cout << "\nConfronto di eventi con date e orari diversi:\n";
    *ev == *differentEvent ? std::cout << "Gli eventi sono uguali\n" : std::cout << "Gli eventi sono diversi\n";

    // Test 10: confronta eventi con titoli diversi ma date e orari uguali (dovrebbero essere diversi)
    Event* sameTimeEvent = new Event("Evento Diverso", today() + 10h, Duration(2h));
    std::cout << "\nConfronto di eventi con titoli diversi ma date e orari uguali:\n";
    *ev == *sameTimeEvent ? std::cout << "Gli eventi sono uguali\n" : std::cout << "Gli eventi sono diversi\n";

    // Test 11: salvataggio e caricamento da XML
    std::cout << "\nSalvataggio e caricamento da file XML:\n";
    EventVisitor* xmlSaver = new XMLSaver("events.xml");
    weekly->accept(xmlSaver);
    evs->accept(xmlSaver);
    birthday->accept(xmlSaver);

    EventVisitor* xmlLoader = new XMLLoader("events.xml");
    Events* xmlLoaded = xmlLoader->load();

    std::cout << "\nEventi caricati da XML:\n";
    xmlLoaded->ForEach([](const Event& e) {
        std::cout << e << "\n";
    });

    xmlLoaded == all ? std::cout << "Caricamento riuscito, gli eventi sono uguali\n" : std::cout << "Caricamento fallito, gli eventi sono diversi\n";



}