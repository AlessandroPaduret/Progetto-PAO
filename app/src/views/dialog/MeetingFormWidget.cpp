#include "views/dialog/MeetingFormWidget.h"

#include <QCompleter>
#include <QFormLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QStringListModel>

namespace app {

MeetingFormWidget::MeetingFormWidget(QWidget* parent) : QWidget(parent) {
    m_locationEdit = new QLineEdit(this);
    m_locationEdit->setPlaceholderText(tr("Aula, sede o link"));

    m_attendeeEdit = new QLineEdit(this);
    m_attendeeEdit->setPlaceholderText(tr("Nome partecipante + Invio"));
    // Suggerisce nomi gia' usati in altre Riunioni del calendario corrente
    // (aggiornato dal genitore via setAttendeeSuggestions): qui si crea solo
    // il completer vuoto e lo si aggancia al campo.
    m_attendeeCompleter = new QCompleter(this);
    m_attendeeCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_attendeeEdit->setCompleter(m_attendeeCompleter);

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

void MeetingFormWidget::setAttendeeSuggestions(const QStringList& names) {
    m_attendeeCompleter->setModel(new QStringListModel(names, m_attendeeCompleter));
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
