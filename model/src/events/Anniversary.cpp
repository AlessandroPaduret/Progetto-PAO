#include <chrono>
#include <memory>
#include <vector>

#include "events/core/ActivityVisitor.h"
#include "events/core/CommonTypes.h"
#include "events/core/Format.h"
#include "events/domain/Anniversary.h"
#include "events/generators/YearlyGenerator.h"

namespace events {

// L'anniversario occupa l'intero giorno (mezzanotte - 1 secondo), come il
// vecchio "compleanno": evita sovrapposizioni con gli eventi del giorno.
namespace {
const Duration kAllDay = std::chrono::hours(24) - std::chrono::seconds(1);
}

Anniversary::Anniversary(const String &title, const TimePoint date,
                         const TimePoint end)
    : Activity(title), m_date(date), m_end(end) {}

Anniversary *Anniversary::clone_impl() const {
  return new Anniversary(*this);
}

std::unique_ptr<Anniversary> Anniversary::clone() const {
  return std::unique_ptr<Anniversary>(clone_impl());
}

TimePoint Anniversary::getStart() const { return m_date; }

void Anniversary::setDate(const TimePoint date) { m_date = date; }

TimePoint Anniversary::getEnd() const { return m_end; }

void Anniversary::setEnd(const TimePoint end) { m_end = end; }

const std::unordered_set<TimePoint, TimePointHasher> &
Anniversary::getDoneOccurrences() const {
  return m_doneOccurrences;
}

bool Anniversary::isDoneAt(const TimePoint occurrenceStart) const {
  return m_doneOccurrences.find(occurrenceStart) != m_doneOccurrences.end();
}

void Anniversary::setDoneAt(const TimePoint occurrenceStart, const bool done) {
  if (done) {
    m_doneOccurrences.insert(occurrenceStart);
  } else {
    m_doneOccurrences.erase(occurrenceStart);
  }
}

std::vector<Occurrence> Anniversary::occurrencesIn(const TimePoint from,
                                                   const TimePoint to) const {
  // Riusa YearlyGenerator per la gestione degli anni bisestili (29/2 -> 28/2)
  YearlyGenerator generator(m_date, m_end);
  std::vector<Occurrence> result;
  for (const TimePoint tp : generator.generateDates(from, to)) {
    result.push_back(Occurrence{this, tp, kAllDay});
  }
  return result;
}

void Anniversary::moveTo(const TimePoint newStart) { m_date = newStart; }

String Anniversary::describe() const {
  return "Anniversario: " + getTitle() + " - ogni anno il " +
         formatDateTime(m_date);
}

void Anniversary::accept(ActivityVisitor &visitor) const {
  visitor.visit(*this);
}

} // namespace events