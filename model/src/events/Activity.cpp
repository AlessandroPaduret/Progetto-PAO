#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include "events/core/Activity.h"
#include "events/core/ActivityVisitor.h"
#include "events/core/Format.h"
#include "events/generators/MoveGeneratorVisitor.h"
#include "events/generators/NullGenerator.h"
#include "events/generators/SingleGenerator.h"

namespace events {

Activity::Activity(String title, TimePoint start, Duration duration,
                   std::shared_ptr<DateGenerator> generator)
    : m_title(std::move(title)),
      m_duration(duration),
      m_generator(generator ? std::move(generator) : std::make_shared<SingleGenerator>(start)) {
  if (duration < Duration::zero()) {
    throw std::invalid_argument("La durata non puo' essere negativa.");
  }
  // L'istante di riferimento vive nel generatore (immutabile): se ne viene
  // fornito uno esplicito, il suo inizio fa fede su `start`.
}

Activity *Activity::clone_impl() const { return new Activity(*this); }

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

TimePoint Activity::getEnd() const { return getStart() + m_duration; }

void Activity::setEnd(TimePoint end) { setDuration(end - getStart()); }

const std::shared_ptr<DateGenerator> &Activity::getGenerator() const {
  return m_generator;
}

const std::unordered_set<TimePoint, TimePointHasher> &
Activity::getExceptions() const {
  return m_exceptions;
}

void Activity::addException(TimePoint tp) { m_exceptions.insert(tp); }

void Activity::deleteExceptions(TimePoint tp) { m_exceptions.erase(tp); }

void Activity::truncateBefore(TimePoint tp) {
  // Ricostruisce il generatore con la nuova fine (tp escluso).
  MoveGeneratorVisitor visitor(std::nullopt, tp - Duration(1));
  m_generator->accept(visitor);
  m_generator = visitor.result;
}

bool Activity::isRecurrent() const {
  return !(std::dynamic_pointer_cast<SingleGenerator>(m_generator) ||
           std::dynamic_pointer_cast<NullGenerator>(m_generator));
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

std::vector<std::unique_ptr<Activity>>
Activity::getSchedulable(const TimePoint from, const TimePoint to) const {
  // API legacy (usata dal REST server): restituisce una Activity singola
  // per ogni occorrenza della serie nell'intervallo.
  std::vector<std::unique_ptr<Activity>> result;
  for (const TimePoint tp : occurrenceDates(from, to)) {
    result.push_back(std::make_unique<Activity>(m_title, tp, m_duration));
  }
  return result;
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
  // Il generatore e' immutabile: MoveGeneratorVisitor ne ricostruisce uno
  // nuovo traslato. La fine NON slitta (resta quella che era); le eccezioni
  // (date assolute) non vengono traslate: la serie spostata e' INTONSA.
  MoveGeneratorVisitor visitor(newStart, std::nullopt);
  m_generator->accept(visitor);
  m_generator = visitor.result;
  m_exceptions.clear();
}

String Activity::describe() const {
  return "Evento: " + m_title + " - " + m_generator->describe();
}

void Activity::accept(ActivityVisitor &visitor) const {
  visitor.visit(*this);
}

std::unique_ptr<Activity> Activity::clone() const {
  return std::unique_ptr<Activity>(clone_impl());
}

} // namespace events