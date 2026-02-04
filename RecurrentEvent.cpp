
#include "RecurrentEvent.h"

RecurrentEvent::RecurrentEvent(const std::string& title, 
                               const TimePoint start, 
                               const Duration duration,
                               const Duration recurrenceInterval)
    : Event(title, start, duration), m_end(start + duration) {
    if( recurrenceInterval != Duration::zero() ){
        m_recurrenceInterval.push_back(recurrenceInterval);
    }
}

void RecurrentEvent::addRecurrenceInterval(const Duration interval) {
    m_recurrenceInterval.push_back(interval);
}

void RecurrentEvent::addException(const TimePoint exception) {
    m_exceptions.insert(exception);
}

void RecurrentEvent::addModification(const Event& modifiedEvent) {
    m_modifications[modifiedEvent.getStart()] = modifiedEvent;
}

bool RecurrentEvent::isException(const TimePoint time) const {
    return m_exceptions.find(time) != m_exceptions.end();
}

bool RecurrentEvent::isModified(const TimePoint time) const {
    return m_modifications.find(time) != m_modifications.end();
}

std::vector<Duration> RecurrentEvent::getRecurrenceIntervals() const {
    return m_recurrenceInterval;
}

std::vector<Event> RecurrentEvent::getOccurrences(
    const TimePoint from,
    const TimePoint to) const 
{
    std::vector<Event> occurrences;
    auto searchEnd = std::min(m_end, to);
    auto eventStart = getStart();
    auto eventDuration = getDuration();

    for (const auto& interval : m_recurrenceInterval) {
        
        auto current = eventStart;
        if (current < from) {
            // Calcoliamo l'inizio del primo evento dopo from
            auto diff = from - current;
            auto steps = diff / interval;
            current += (steps * interval);
            
            // Se il salto ci ha portato ancora prima di 'from', facciamo un passo avanti
            if (current < from) current += interval;
        }

        // 3. Generazione effettiva
        while (current <= searchEnd) {

            if(isModified(current)) //se 
            {
                occurrences.push_back(m_modifications.at(current)); // at() è l'equivalente costante di operator[]
            }else if(!isException(current) )
            {
                occurrences.emplace_back(getTitle(), current, eventDuration);
            }
            current += interval;
            
            // Sicurezza: evita cicli infiniti se interval è <= 0
            if (interval <= Duration::zero()) break;
        }
    }

    return occurrences;
}