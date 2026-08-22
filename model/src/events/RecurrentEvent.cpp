#include <iostream>
#include <memory>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/domain/RecurrentEvent.h"

namespace events {

// Costruttore: inizializza il generatore e l'evento "stampino" (template);
// il titolo dell'attivita' e' quello del template
RecurrentEvent::RecurrentEvent(std::shared_ptr<DateGenerator> generator,
                               Event templateEvent)
    : Activity(templateEvent.getTitle()),
      m_generator(std::move(generator)),
      m_templateEvent(std::move(templateEvent)) {}

RecurrentEvent *RecurrentEvent::clone_impl() const {
  return new RecurrentEvent(*this);
}

std::unique_ptr<RecurrentEvent> RecurrentEvent::clone() const {
  return std::unique_ptr<RecurrentEvent>(clone_impl());
}

// Operatore di output per debug/stampa
std::ostream &operator<<(std::ostream &os,
                         const events::RecurrentEvent &event) {
  os << "[Recurrent Event]\n"
     << event.getTitle() << "\n"
     << event.m_generator->describe() << "\nwith "
     << event.m_exceptions.size() << " exceptions";
  return os;
}

const std::shared_ptr<DateGenerator> &RecurrentEvent::getGenerator() const {
  return m_generator;
}

const Event &RecurrentEvent::getTemplateEvent() const { return m_templateEvent; }

const std::unordered_set<TimePoint, TimePointHasher> &
RecurrentEvent::getExceptions() const {
  return m_exceptions;
}

// Date delle occorrenze in [from, to], escluse le eccezioni
static std::vector<TimePoint>
occurrenceDates(const DateGenerator &generator,
                const std::unordered_set<TimePoint, TimePointHasher> &exceptions,
                const TimePoint from, const TimePoint to) {
  std::vector<TimePoint> dates;
  for (const TimePoint tp : generator.generateDates(from, to)) {
    if (exceptions.find(tp) == exceptions.end()) {
      dates.push_back(tp);
    }
  }
  return dates;
}

std::vector<std::unique_ptr<Event>>
RecurrentEvent::getSchedulable(const TimePoint from, const TimePoint to) const {

  std::vector<std::unique_ptr<Event>> result;

  for (const TimePoint tp : occurrenceDates(*m_generator, m_exceptions, from, to)) {
    // Crea un'istanza standard basata sull'evento template
    std::unique_ptr<Event> standardOccurrence = m_templateEvent.clone();
    standardOccurrence->setTitle(getTitle());
    standardOccurrence->setStart(tp);
    result.push_back(std::move(standardOccurrence));
  }

  return result;
}

std::vector<Occurrence>
RecurrentEvent::occurrencesIn(const TimePoint from, const TimePoint to) const {

  std::vector<Occurrence> result;
  for (const TimePoint tp : occurrenceDates(*m_generator, m_exceptions, from, to)) {
    result.push_back(Occurrence{this, tp, m_templateEvent.getDuration()});
  }
  return result;
}

TimePoint RecurrentEvent::getStart() const { return m_templateEvent.getStart(); }

void RecurrentEvent::moveTo(const TimePoint newStart) {
  m_generator->setStart(newStart);
  // La data di scadenza della serie NON slitta con lo spostamento: resta
  // quella che era. Unica eccezione: la fine non puo' mai diventare
  // antecedente al nuovo inizio, quindi in quel caso viene portata al
  // nuovo inizio (vincolo: la fine non precede l'inizio).
  const TimePoint end = m_generator->getEnd();
  if (end != TimePoint::max() && end < newStart) {
    m_generator->setEnd(newStart);
  }
  m_templateEvent.setStart(newStart);
  // La serie spostata viene ricreata INTONSA: le eccezioni (date assolute
  // delle occorrenze escluse, es. eventi staccati) non vengono traslate,
  // altrimenti produrrebbero buchi nelle nuove date. Gli eventuali eventi
  // staccati restano nel calendario come eventi singoli indipendenti.
  m_exceptions.clear();
}

String RecurrentEvent::describe() const {
  return "Evento ricorrente: " + getTitle() + " - " + m_generator->describe();
}

void RecurrentEvent::accept(ActivityVisitor &visitor) const {
  visitor.visit(*this);
}

void RecurrentEvent::addException(TimePoint tp) { m_exceptions.insert(tp); }

void RecurrentEvent::deleteExceptions(TimePoint tp) { m_exceptions.erase(tp); }

void RecurrentEvent::truncateBefore(TimePoint tp) {
  m_generator->setEnd(tp - Duration(1));
}

} // namespace events
