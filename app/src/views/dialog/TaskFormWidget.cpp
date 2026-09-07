#include "views/dialog/TaskFormWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>

namespace app {

TaskFormWidget::TaskFormWidget(QWidget* parent) : ActivityTypeWidget(parent) {
    m_priorityCombo = new QComboBox(this);
    m_priorityCombo->addItem(tr("Bassa"));
    m_priorityCombo->addItem(tr("Media"));
    m_priorityCombo->addItem(tr("Alta"));
    m_priorityCombo->setCurrentIndex(1);
    m_doneCheck = new QCheckBox(tr("Evaso"), this);

    auto* form = new QFormLayout(this);
    form->setContentsMargins(0, 0, 0, 0);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->addRow(tr("Priorita'"), m_priorityCombo);
    form->addRow(m_doneCheck);
}

events::Priority TaskFormWidget::priority() const {
    switch (m_priorityCombo->currentIndex()) {
    case 0: return events::Priority::Low;
    case 2: return events::Priority::High;
    default: return events::Priority::Medium;
    }
}

void TaskFormWidget::setPriority(events::Priority priority) {
    switch (priority) {
    case events::Priority::Low:
        m_priorityCombo->setCurrentIndex(0);
        break;
    case events::Priority::High:
        m_priorityCombo->setCurrentIndex(2);
        break;
    case events::Priority::Medium:
    default:
        m_priorityCombo->setCurrentIndex(1);
        break;
    }
}

bool TaskFormWidget::isDone() const { return m_doneCheck->isChecked(); }
void TaskFormWidget::setDone(bool done) { m_doneCheck->setChecked(done); }
void TaskFormWidget::setDoneEnabled(bool enabled) { m_doneCheck->setEnabled(enabled); }
bool TaskFormWidget::isDoneEnabled() const { return m_doneCheck->isEnabled(); }

void TaskFormWidget::clear() {
    m_priorityCombo->setCurrentIndex(1);
    m_doneCheck->setChecked(false);
    m_doneCheck->setEnabled(true);
}

void TaskFormWidget::populateFrom(const events::Activity& activity) {
    const auto& task = static_cast<const events::Task&>(activity);
    setPriority(task.getPriority());
    setDone(task.isDone());
    setDoneEnabled(true);
}

void TaskFormWidget::applyToConfig(events::ActivityConfig& /*config*/) const {
    // Priorita'/evaso non fanno parte della ActivityConfig comune (sono in
    // TaskConfig): createActivity() li aggiunge direttamente.
}

std::unique_ptr<events::Activity>
TaskFormWidget::createActivity(events::ActivityConfig config) const {
    auto task = events::makeTask(events::TaskConfig(std::move(config), priority()));
    if (isDoneEnabled()) {
        task->setDone(isDone());
    }
    return task;
}

} // namespace app
