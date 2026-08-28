#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include "events/core/Activity.h"
#include "events/core/ActivityVisitor.h"
#include "events/core/Format.h"
#include "events/generators/SingleGenerator.h"

namespace events {

Activity::Activity(String title, Duration duration,
                   std::unique_ptr<DateGenerator> generator)
    : m_title(std::move(title)),
      m_duration(duration),
      m_generator(generator ? std::move(generator)
                            : std::make_unique<SingleGenerator>(
                                  std::chrono::time_point_cast<std::chrono::seconds>(
                                      Clock::now()))) {
  if (duration < Duration::zero()) {
    throw std::invalid_argument("La durata non puo' essere negativa.");
  }
  // L'istante di riferimento vive nel generatore: la Activity non ha un
  // proprio inizio; il fallback e' un SingleGenerator(now).
}

std::unique_ptr<Activity> Activity::clone() const {
  return std::make_unique<Activity>(m_title, m_duration, m_generator->clone());
}

String Activity::getTitle() const { return m_title; }

void Activity::setTitle(const String &title) { m_title = title; }

TimePoint Activity::getStart() const { return m_generator->getStart(); }

Duration Activity::getDuration() const { return m_duration; }

void Activity::setDuration(Duration duration) {
  if (duration < Duration::zero()) {
    throw std::invalid_argument("La durata non puo' essere negativa.");
  }
  m_duration = duration;
}

TimePoint Activity::getEnd() const { return m_generator->getEnd(); }

void Activity::setStart(TimePoint start) { m_generator->setStart(start); }

void Activity::setEnd(TimePoint end) {
  // La fine della ricorrenza vive nel generatore: si tronca con setEnd.
  m_generator->setEnd(end);
}

const DateGenerator &Activity::getGenerator() const {
  return *m_generator;
}

void Activity::setGenerator(std::unique_ptr<DateGenerator> generator) {
  m_generator = generator ? std::move(generator)
                          : std::make_unique<SingleGenerator>(
                                std::chrono::time_point_cast<std::chrono::seconds>(
                                    Clock::now()));
}

const std::unordered_set<TimePoint, TimePointHasher> &
Activity::getExceptions() const {
  return m_exceptions;
}

bool Activity::addException(TimePoint tp) {
  // L'eccezione deve essere una data effettivamente generata dalla serie:
  // niente occorrenze arbitrarie che non appartengono al generatore.
  if (!m_generator->isIn(tp)) {
    return false;
  }
  return m_exceptions.insert(tp).second;
}

void Activity::truncateBefore(TimePoint tp) {
  // Conserva le occorrenze strettamente precedenti a tp (tp escluso).
  m_generator->setEnd(tp - Duration(1));
}

std::vector<TimePoint> Activity::occurrenceDates(const TimePoint from,
                                                 const TimePoint to) const {
  std::vector<TimePoint> dates;
  for (const TimePoint tp : m_generator->generateDates(from, to)) {
    if (m_exceptions.find(tp) == m_exceptions.end()) {
      dates.push_back(tp);
    }
  }
  return dates;
}

std::vector<Occurrence> Activity::occurrencesIn(const TimePoint from,
                                                const TimePoint to) const {
  std::vector<Occurrence> result;
  for (const TimePoint tp : occurrenceDates(from, to)) {
    result.push_back(Occurrence{this, tp, m_duration});
  }
  return result;
}

void Activity::moveTo(TimePoint newStart) {
  // Sposta la serie con setStart (la fine NON slitta; se il nuovo inizio
  // supera la fine, il generatore allinea la fine al nuovo inizio). Le
  // eccezioni (date assolute) non vengono traslate: la serie spostata e'
  // INTONSA.
  m_generator->setStart(newStart);
  m_exceptions.clear();
}

String Activity::describe() const {
  return "Evento: " + m_title + " - " + m_generator->describe();
}

void Activity::accept(ActivityVisitor &visitor) const {
  visitor.visit(*this);
}

} // namespace events