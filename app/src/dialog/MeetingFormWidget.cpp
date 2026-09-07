#include "dialog/MeetingFormWidget.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QListWidget>

namespace app {

MeetingFormWidget::MeetingFormWidget(QWidget* parent) : ActivityTypeWidget(parent) {
    m_locationEdit = new QLineEdit(this);
    m_locationEdit->setPlaceholderText(tr("Aula, sede o link"));

    m_attendeeEdit = new QLineEdit(this);
    m_attendeeEdit->setPlaceholderText(tr("Nome partecipante + Invio"));

    m_attendeesList = new QListWidget(this);
    m_attendeesList->setMaximumHeight(110);

    auto* form = new QFormLayout(this);
    form->setContentsMargins(0, 0, 0, 0);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->addRow(tr("Luogo"), m_locationEdit);
    form->addRow(tr("Aggiungi"), m_attendeeEdit);
    form->addRow(tr("Partecipanti"), m_attendeesList);

    connect(m_attendeeEdit, &QLineEdit::returnPressed,
            this, &MeetingFormWidget::onAddAttendee);
    connect(m_attendeesList, &QListWidget::itemDoubleClicked,
            this, &MeetingFormWidget::onRemoveAttendee);
}

QString MeetingFormWidget::location() const { return m_locationEdit->text().trimmed(); }

QStringList MeetingFormWidget::attendees() const {
    QStringList names;
    for (int i = 0; i < m_attendeesList->count(); ++i) {
        names.append(m_attendeesList->item(i)->text());
    }
    return names;
}

void MeetingFormWidget::setLocation(const QString& location) {
    m_locationEdit->setText(location);
}

void MeetingFormWidget::setAttendees(const QStringList& attendees) {
    m_attendeesList->clear();
    for (const QString& name : attendees) {
        new QListWidgetItem(name, m_attendeesList);
    }
}

void MeetingFormWidget::clear() {
    m_locationEdit->clear();
    m_attendeeEdit->clear();
    m_attendeesList->clear();
}

void MeetingFormWidget::populateFrom(const events::Activity& activity) {
    const auto& meeting = static_cast<const events::Meeting&>(activity);
    setLocation(QString::fromStdString(meeting.getLocation()));
    QStringList attendees;
    for (const auto& name : meeting.getAttendees()) {
        attendees.append(QString::fromStdString(name));
    }
    setAttendees(attendees);
}

void MeetingFormWidget::applyToConfig(events::ActivityConfig& /*config*/) const {
    // Luogo/partecipanti non fanno parte della ActivityConfig comune (sono
    // in MeetingConfig): createActivity() li aggiunge direttamente.
}

std::unique_ptr<events::Activity>
MeetingFormWidget::createActivity(events::ActivityConfig config) const {
    auto meeting = events::makeMeeting(
        events::MeetingConfig(std::move(config), location().toStdString()));
    for (const QString& name : attendees()) {
        meeting->addAttendee(name.toStdString());
    }
    return meeting;
}

void MeetingFormWidget::onAddAttendee() {
    const QString name = m_attendeeEdit->text().trimmed();
    if (name.isEmpty()) {
        return;
    }
    for (int i = 0; i < m_attendeesList->count(); ++i) {
        if (m_attendeesList->item(i)->text() == name) {
            return;  // gia' presente
        }
    }
    new QListWidgetItem(name, m_attendeesList);
    m_attendeeEdit->clear();
}

void MeetingFormWidget::onRemoveAttendee() {
    if (QListWidgetItem* item = m_attendeesList->currentItem()) {
        delete item;
    }
}

} // namespace app
