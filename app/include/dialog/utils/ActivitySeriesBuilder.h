#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QDate>
#include <QTime>

#include "builders/ActivityConfig.h"
#include "core/Activity.h"
#include "dialog/RecurrenceFormWidget.h"

namespace app {

class ActivityTypeWidget;

/** @brief Dati grezzi di ricorrenza estratti dal form (value-type puro,
 *  niente riferimenti a widget Qt): raccolti qui cosi' ActivitySeriesBuilder
 *  non deve interrogare il widget campo per campo. */
struct RecurrenceRule {
    bool allDay = false;
    bool repeating = false;
    RecurrenceFormWidget::Unit unit = RecurrenceFormWidget::Days;
    int every = 1;
    std::vector<int> selectedWeekdays;
    RecurrenceFormWidget::EndMode endMode = RecurrenceFormWidget::EndMode::Never;
    QDate endDate;
    int endCount = 0;
};

/** @brief Costruisce le attivita' (una singola, o una o piu' serie
 *  ricorrenti) per il pannello di creazione/modifica: builder API (set...
 *  concatenabili + build() finale) cosi' il coordinatore si limita a
 *  raccogliere i valori dal form senza conoscere la logica di costruzione. */
class ActivitySeriesBuilder {
public:
    ActivitySeriesBuilder(std::string title, QDate startDate, QTime startTime,
                          int durationMinutes);

    ActivitySeriesBuilder& setRecurrence(RecurrenceRule rule);
    ActivitySeriesBuilder& setTypeWidget(const ActivityTypeWidget* typeWidget);

    /** @return vuoto se il titolo e' vuoto o non e' stato impostato un ActivityTypeWidget. */
    std::vector<std::unique_ptr<events::Activity>> build() const;

private:
    /** @brief Applica i campi specifici del tipo (polimorfismo via
     *  m_typeWidget) e costruisce l'attivita' concreta. */
    std::unique_ptr<events::Activity> createTypedActivity(events::ActivityConfig config) const;

    // Un caso ciascuno di build(), a seconda di m_rule (SRP)
    std::vector<std::unique_ptr<events::Activity>> buildSingle() const;
    std::vector<std::unique_ptr<events::Activity>> buildRecurrent() const;
    std::vector<std::unique_ptr<events::Activity>> buildWeekly() const;

    // Letture comuni a piu' sotto-passi
    events::TimePoint resolveStart() const;
    events::TimePoint resolveSeriesEnd() const;
    events::Duration seriesDuration() const;

    std::string m_title;
    QDate m_startDate;
    QTime m_startTime;
    int m_durationMinutes;
    RecurrenceRule m_rule;
    const ActivityTypeWidget* m_typeWidget = nullptr;
};

} // namespace app
