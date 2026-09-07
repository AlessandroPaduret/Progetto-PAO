#include "dialog/utils/ActivitySeriesBuilder.h"

#include <QDateTime>
#include <QTimeZone>

#include <utility>

#include "generators/FixedIntervalGenerator.h"
#include "generators/MonthlyGenerator.h"
#include "generators/YearlyGenerator.h"
#include "dialog/ActivityTypeWidget.h"
#include "dialog/utils/RecurrenceBuilder.h"

namespace app {

namespace {

events::TimePoint toTimePoint(const QDateTime& local) {
    return events::TimePoint(std::chrono::seconds(local.toSecsSinceEpoch()));
}

// Generatore per le unita' a intervallo singolo (Giorni/Mesi/Anni): la
// settimana su piu' giorni ha una regola diversa (una serie per giorno
// scelto, vedi ActivitySeriesBuilder::buildWeekly) e non passa da qui.
std::shared_ptr<const events::DateGenerator> makeIntervalGenerator(
    RecurrenceFormWidget::Unit unit, int every) {
    switch (unit) {
    case RecurrenceFormWidget::Months:
        return std::make_shared<events::MonthlyGenerator>(every);
    case RecurrenceFormWidget::Years:
        return std::make_shared<events::YearlyGenerator>(every);
    case RecurrenceFormWidget::Days:
    case RecurrenceFormWidget::Weeks:
    default:
        return std::make_shared<events::FixedIntervalGenerator>(
            events::Duration(events::Days(every)));
    }
}

} // namespace

ActivitySeriesBuilder::ActivitySeriesBuilder(std::string title, QDate startDate,
                                             QTime startTime, int durationMinutes)
    : m_title(std::move(title)), m_startDate(startDate), m_startTime(startTime),
      m_durationMinutes(durationMinutes) {}

ActivitySeriesBuilder& ActivitySeriesBuilder::setRecurrence(RecurrenceRule rule) {
    m_rule = std::move(rule);
    return *this;
}

ActivitySeriesBuilder& ActivitySeriesBuilder::setTypeWidget(const ActivityTypeWidget* typeWidget) {
    m_typeWidget = typeWidget;
    return *this;
}

std::vector<std::unique_ptr<events::Activity>> ActivitySeriesBuilder::build() const {
    if (m_title.empty() || m_typeWidget == nullptr) {
        return {};
    }
    if (!m_rule.repeating) {
        return buildSingle();
    }
    if (m_rule.unit == RecurrenceFormWidget::Weeks) {
        return buildWeekly();
    }
    return buildRecurrent();
}

std::unique_ptr<events::Activity>
ActivitySeriesBuilder::createTypedActivity(events::ActivityConfig config) const {
    m_typeWidget->applyToConfig(config);
    return m_typeWidget->createActivity(std::move(config));
}

events::TimePoint ActivitySeriesBuilder::resolveStart() const {
    // "Tutto il giorno": inizio a mezzanotte UTC (coerente con le query
    // della griglia, che usano UTC) per non far slittare il giorno: in
    // locale 00:00 di Lun = Dom 22:00 UTC, che cadrebbe nella settimana
    // precedente.
    return m_rule.allDay ? toTimePoint(QDateTime(m_startDate, QTime(0, 0), QTimeZone(0)))
                         : toTimePoint(QDateTime(m_startDate, m_startTime));
}

events::TimePoint ActivitySeriesBuilder::resolveSeriesEnd() const {
    // "Mai"/"Dopo N" -> resta max() qui (il secondo viene poi sovrascritto
    // da chi chiama con RecurrenceBuilder::calculateEndAfterCount/
    // calculateNthWeeklyDate); "Fino al" -> fine giornata della data scelta.
    if (m_rule.endMode != RecurrenceFormWidget::EndMode::OnDate) {
        return events::TimePoint::max();
    }
    return toTimePoint(QDateTime(m_rule.endDate.addDays(1), QTime(0, 0)).addSecs(-1));
}

events::Duration ActivitySeriesBuilder::seriesDuration() const {
    return m_rule.allDay ? std::chrono::seconds(86399)
                         : std::chrono::minutes(m_durationMinutes);
}

std::vector<std::unique_ptr<events::Activity>> ActivitySeriesBuilder::buildSingle() const {
    // "Tutto il giorno" senza ripetizione dura ESATTAMENTE 24h (86400s, non
    // gli 86399 usati per le serie, vedi seriesDuration()): la striscia in
    // alto riconosce entrambe come "copre un giorno intero".
    const events::Duration duration =
        m_rule.allDay ? std::chrono::seconds(86400) : seriesDuration();
    std::vector<std::unique_ptr<events::Activity>> result;
    result.push_back(createTypedActivity(events::ActivityConfig{
        .title = m_title, .start = resolveStart(), .duration = duration}));
    return result;
}

std::vector<std::unique_ptr<events::Activity>> ActivitySeriesBuilder::buildRecurrent() const {
    const events::TimePoint start = resolveStart();
    auto generator = makeIntervalGenerator(m_rule.unit, m_rule.every);

    events::TimePoint end = resolveSeriesEnd();
    if (m_rule.endMode == RecurrenceFormWidget::EndMode::AfterCount) {
        end = RecurrenceBuilder::calculateEndAfterCount(*generator, start, m_rule.endCount);
    }

    std::vector<std::unique_ptr<events::Activity>> result;
    result.push_back(createTypedActivity(events::ActivityConfig{
        .title = m_title,
        .start = start,
        .duration = seriesDuration(),
        .end = end,
        .generator = std::move(generator)}));
    return result;
}

std::vector<std::unique_ptr<events::Activity>> ActivitySeriesBuilder::buildWeekly() const {
    // Una o piu' serie ricorrenti, una per giorno della settimana scelto.
    const int baseDow = m_startDate.dayOfWeek();
    const QTime time = m_rule.allDay ? QTime(0, 0) : m_startTime;
    const events::Duration duration = seriesDuration();

    // Giorni selezionati (id Qt: 1=Lun..7=Dom), fallback: il giorno dell'inizio.
    std::vector<int> selected = m_rule.selectedWeekdays;
    if (selected.empty()) {
        selected.push_back(baseDow);
    }

    // Il limite "dopo N occorrenze" vale sul CALENDARIO COMBINATO, non per
    // singola serie (vedi RecurrenceBuilder::calculateNthWeeklyDate).
    events::TimePoint end = resolveSeriesEnd();
    if (m_rule.endMode == RecurrenceFormWidget::EndMode::AfterCount) {
        const QDate nthDate = RecurrenceBuilder::calculateNthWeeklyDate(
            m_startDate, baseDow, selected, m_rule.every, m_rule.endCount);
        end = toTimePoint(QDateTime(nthDate.addDays(1), QTime(0, 0)).addSecs(-1));
    }

    std::vector<std::unique_ptr<events::Activity>> result;
    for (int dow : selected) {
        const int offset = (dow - baseDow + 7) % 7;
        // Anche le serie settimanali "tutto il giorno" partono a mezzanotte
        // UTC (come l'evento singolo), per la stessa ragione di allineamento.
        const events::TimePoint anchor =
            m_rule.allDay
                ? toTimePoint(QDateTime(m_startDate.addDays(offset), QTime(0, 0), QTimeZone(0)))
                : toTimePoint(QDateTime(m_startDate.addDays(offset), time));
        result.push_back(createTypedActivity(events::ActivityConfig{
            .title = m_title,
            .start = anchor,
            .duration = duration,
            .end = end,
            .generator = std::make_shared<events::FixedIntervalGenerator>(
                events::Duration(events::Days(7 * m_rule.every)))}));
    }
    return result;
}

} // namespace app
