#include <memory>
#include <utility>

#include "events/generators/FixedIntervalGenerator.h"
#include "events/generators/MonthlyGenerator.h"
#include "events/generators/MoveGeneratorVisitor.h"
#include "events/generators/SingleGenerator.h"
#include "events/generators/YearlyGenerator.h"

namespace events {

MoveGeneratorVisitor::MoveGeneratorVisitor(std::optional<TimePoint> newStart,
                                           std::optional<TimePoint> newEnd)
    : m_newStart(newStart), m_newEnd(newEnd) {}

void MoveGeneratorVisitor::visit(const FixedIntervalGenerator &generator) {
  TimePoint start = m_newStart.value_or(generator.getStart());
  TimePoint end = m_newEnd.value_or(generator.getEnd());
  // La fine NON slitta con lo spostamento; se il nuovo inizio la supera,
  // la fine viene portata al nuovo inizio (vincolo: end >= start).
  if (end != TimePoint::max() && end < start) {
    end = start;
  }
  result = std::make_shared<FixedIntervalGenerator>(
      start, generator.getInterval(), end, generator.getMaxOccurrences());
}

void MoveGeneratorVisitor::visit(const MonthlyGenerator &generator) {
  TimePoint start = m_newStart.value_or(generator.getStart());
  TimePoint end = m_newEnd.value_or(generator.getEnd());
  if (end != TimePoint::max() && end < start) {
    end = start;
  }
  result = std::make_shared<MonthlyGenerator>(
      start, generator.getMonths(), end, generator.getMaxOccurrences());
}

void MoveGeneratorVisitor::visit(const YearlyGenerator &generator) {
  TimePoint start = m_newStart.value_or(generator.getStart());
  TimePoint end = m_newEnd.value_or(generator.getEnd());
  if (end != TimePoint::max() && end < start) {
    end = start;
  }
  result = std::make_shared<YearlyGenerator>(start, end,
                                             generator.getMaxOccurrences());
}

void MoveGeneratorVisitor::visit(const SingleGenerator &generator) {
  result = std::make_shared<SingleGenerator>(
      m_newStart.value_or(generator.getStart()));
}

} // namespace events