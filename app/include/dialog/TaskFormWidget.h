#pragma once

#include "dialog/ActivityTypeWidget.h"

class QCheckBox;
class QComboBox;

namespace app {

/** @brief Sezione specifica del Compito (priorita' + stato evaso), estratta
 *  da ActivitySidebarWidget in un widget figlio autonomo che implementa
 *  ActivityTypeWidget: sa da solo svuotarsi, popolarsi da un
 *  `events::Task` esistente e costruire il proprio `events::Task`
 *  (polimorfismo al posto dello switch in
 *  ActivitySidebarWidget::makeTypedActivity). */
class TaskFormWidget : public ActivityTypeWidget {
    Q_OBJECT
public:
    explicit TaskFormWidget(QWidget* parent = nullptr);

    events::Priority priority() const;
    void setPriority(events::Priority priority);

    bool isDone() const;
    void setDone(bool done);

    /** @brief Guardia usata da createActivity(): se disabilitata, l'evaso
     *  visivo non viene scritto sul Task costruito (resta il default "da
     *  fare"). Nessun chiamante la disabilita oggi (sempre true sia in
     *  clear() sia in populateFrom()), ma resta il punto d'estensione
     *  corretto per un futuro stato "non applicabile". */
    void setDoneEnabled(bool enabled);
    bool isDoneEnabled() const;

    // ActivityTypeWidget
    void clear() override;
    void populateFrom(const events::Activity& activity) override;
    void applyToConfig(events::ActivityConfig& config) const override;
    std::unique_ptr<events::Activity> createActivity(events::ActivityConfig config) const override;

private:
    QComboBox* m_priorityCombo = nullptr;
    QCheckBox* m_doneCheck = nullptr;
};

} // namespace app
