#pragma once

#include "dialog/ActivityTypeWidget.h"

class QCheckBox;
class QComboBox;

namespace app {

/** @brief Sezione specifica del Compito (priorita' + stato evaso):
 *  implementa ActivityTypeWidget, sa da sola svuotarsi/popolarsi/costruire
 *  il proprio events::Task. */
class TaskFormWidget : public ActivityTypeWidget {
    Q_OBJECT
public:
    explicit TaskFormWidget(QWidget* parent = nullptr);

    events::Priority priority() const;
    void setPriority(events::Priority priority);

    bool isDone() const;
    void setDone(bool done);

    /** @brief Guardia usata da createActivity(): se disabilitata, l'evaso
     *  non viene scritto sul Task costruito. Nessuno la disabilita oggi, ma
     *  resta il punto d'estensione per un futuro stato "non applicabile". */
    void setDoneEnabled(bool enabled);
    bool isDoneEnabled() const;

    void clear() override;
    void populateFrom(const events::Activity& activity) override;
    void applyToConfig(events::ActivityConfig& config) const override;
    std::unique_ptr<events::Activity> createActivity(events::ActivityConfig config) const override;

private:
    QComboBox* m_priorityCombo = nullptr;
    QCheckBox* m_doneCheck = nullptr;
};

} // namespace app
