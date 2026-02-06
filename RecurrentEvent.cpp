
#include "RecurrentEvent.h"
#include "CommonTypes.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <chrono>

RecurrentEvent::RecurrentEvent(const String& title, 
                               const TimePoint start, 
                               const Duration duration,
                               const Duration mainInterval,
                               const TimePoint end,
                               const std::vector<Duration>& offsets)
    : Event(title, start, duration), m_mainInterval(mainInterval), m_end(end), m_offsets(offsets) {}

RecurrentEvent::RecurrentEvent(const RecurrentEvent& other)
    : Event(other), m_mainInterval(other.m_mainInterval), m_end(other.m_end), 
      m_offsets(other.m_offsets), m_exceptions(other.m_exceptions), 
      m_modifications(other.m_modifications) {}

RecurrentEvent& RecurrentEvent::operator=(const RecurrentEvent& other) {
    if (this != &other) {
        Event::operator=(other);
        m_mainInterval = other.m_mainInterval;
        m_end = other.m_end;
        m_offsets = other.m_offsets;
        m_exceptions = other.m_exceptions;
        m_modifications = other.m_modifications;
    }
    return *this;
}

void RecurrentEvent::addOffset(const Duration offset) {
    std::vector<Duration>::iterator it = std::find(m_offsets.begin(), m_offsets.end(), offset);
    if (it == m_offsets.end()) { // se non è già presente
        m_offsets.push_back(offset);
    }
}


void RecurrentEvent::deleteOffset(const Duration offset) {
    std::remove(m_offsets.begin(), m_offsets.end(), offset); // sposta alla fine gli elementi da rimuovere
    m_offsets.erase(m_offsets.end());   
}

std::ostream& operator<<(std::ostream& os, const RecurrentEvent& event) {
    os << static_cast<const Event&>(event);

    std::time_t end_time = Clock::to_time_t(event.m_end);

    if (event.m_mainInterval.count() % 86400 == 0) {
        os << "Recurs every: " << event.m_mainInterval.count() / 86400 << " days\n";
    } else {
        os << "Recurs every: " << event.m_mainInterval.count() << " seconds\n";
    }
    os << "End: " << (event.m_end == TimePoint::max() ? "Never" : std::ctime(&end_time)) << "\n";
    os << "Offsets: ";
    for (const auto& offset : event.m_offsets) {
        os << offset.count() << "s ";
    }
    os << "\nExceptions: ";
    for (const auto& exception : event.m_exceptions) {
        os << Clock::to_time_t(exception) << " ";
    }
    os << "\nModifications: ";
    for (const auto& modPair : event.m_modifications) {
        os << "[" << Clock::to_time_t(modPair.first) << ": " << modPair.second.getTitle() << "] ";
    }
    os << "\n";
    return os;
}

std::unique_ptr<Event> RecurrentEvent::clone() const {
        return std::make_unique<RecurrentEvent>(*this);
}

TimePoint RecurrentEvent::getEnd() const {
    return m_end;
}
bool RecurrentEvent::isIn(const TimePoint from, const TimePoint to) const {
    std::vector<std::unique_ptr<Event>> occurrences = getOccurrences(from, to);
    return !occurrences.empty();
}

bool RecurrentEvent::overlapsWith(const Schedulable& other) const {
    std::vector<std::unique_ptr<Event>> occurrences = getOccurrences(other.getStart(), other.getEnd());
    for (const std::unique_ptr<Event>& occ : occurrences) {
        if (occ->overlapsWith(other)) {
            return true;
        }
    }
    return false;
}

void RecurrentEvent::addException(const TimePoint exception) {
    m_exceptions.insert(exception);
}

void RecurrentEvent::deleteException(const TimePoint exception) {
    m_exceptions.erase(exception);
}

void RecurrentEvent::addModification(const Event& modifiedEvent) {
    // Sovrascrive se esiste già
    m_modifications[modifiedEvent.getStart()] = modifiedEvent;
}

void RecurrentEvent::deleteModification(const TimePoint modifiedEvent) {
    m_modifications.erase(modifiedEvent);
}

bool RecurrentEvent::isException(const TimePoint time) const {
    return m_exceptions.find(time) != m_exceptions.end();
}

bool RecurrentEvent::isModified(const TimePoint time) const {
    return m_modifications.find(time) != m_modifications.end();
}

std::vector<std::unique_ptr<Event>> RecurrentEvent::getOccurrences(const TimePoint from, const TimePoint to) const {
    std::vector<std::unique_ptr<Event>> occurrences;
    TimePoint searchEnd = std::min(m_end, to);
    
    // 1. Calcoliamo la distanza tra l'inizio del range e l'inizio assoluto della ricorrenza
    // Se from è prima di getStart(), partiamo da 0.
    auto distanceFromStart = (from > getStart()) ? (from - getStart()) : Duration::zero();

    // 2. Troviamo quante iterazioni di m_mainInterval sono passate
    long long int numIntervals = distanceFromStart / m_mainInterval;
    
    // 3. Calcoliamo il punto di inizio del ciclo (il "battito" del metronomo)
    TimePoint currentCycleStart = getStart() + (numIntervals * m_mainInterval);

    // 4. Ciclo principale: scorriamo i cicli finché non superiamo searchEnd
    while (currentCycleStart < searchEnd) {
        
        // Per ogni ciclo, controlliamo tutti gli offset
        for (const auto& offset : m_offsets) {
            TimePoint occStart = currentCycleStart + offset;
            TimePoint occEnd = occStart + getDuration();

            // Controllo sovrapposizione con il range richiesto [from, to]
            if (occEnd > from && occStart < searchEnd) {
                
                // 5. Controllo Eccezioni e Modifiche
                if (isModified(occStart)) {
                    occurrences.push_back(std::make_unique<Event>((m_modifications.at(occStart)))); //make_unique alloca nell heap tramite costruttore
                } else if (!isException(occStart)) {
                    std::unique_ptr<Event> newOcc = std::make_unique<Event>(static_cast<const Event&>(*this)); // copia dell'evento base
                    newOcc->setStart(occStart);
                    occurrences.push_back(std::move(newOcc));
                }
            }
        }
        
        currentCycleStart += m_mainInterval;
    }

    return occurrences;
}
