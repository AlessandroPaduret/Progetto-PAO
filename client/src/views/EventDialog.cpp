#include "views/EventDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "api/dto.h"

namespace client {

EventDialog::EventDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Nuovo evento"));
    setModal(true);

    m_title = new QLineEdit(this);
    m_title->setPlaceholderText(tr("Titolo"));

    m_start = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_start->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
    m_start->setCalendarPopup(true);

    m_durationMinutes = new QSpinBox(this);
    m_durationMinutes->setRange(1, 60 * 24 * 365);
    m_durationMinutes->setValue(60);
    m_durationMinutes->setSuffix(tr(" min"));

    m_type = new QComboBox(this);
    m_type->addItem(tr("Singolo"), QStringLiteral("single"));
    m_type->addItem(tr("Ricorrente (intervallo)"), QStringLiteral("fixed"));
    m_type->addItem(tr("Ricorrente (annuale)"), QStringLiteral("yearly"));

    m_intervalDays = new QSpinBox(this);
    m_intervalDays->setRange(1, 3650);
    m_intervalDays->setValue(7);
    m_intervalDays->setSuffix(tr(" giorni"));

    m_hasEnd = new QCheckBox(tr("Data di fine"), this);
    m_end = new QDateTimeEdit(QDateTime::currentDateTime().addYears(1), this);
    m_end->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
    m_end->setCalendarPopup(true);
    m_end->setEnabled(false);

    auto* buttons = new QHBoxLayout;
    auto* okButton = new QPushButton(tr("Crea"), this);
    auto* cancelButton = new QPushButton(tr("Annulla"), this);
    buttons->addStretch();
    buttons->addWidget(okButton);
    buttons->addWidget(cancelButton);

    auto* form = new QFormLayout;
    form->addRow(tr("Titolo"), m_title);
    form->addRow(tr("Inizio"), m_start);
    form->addRow(tr("Durata"), m_durationMinutes);
    form->addRow(tr("Tipo"), m_type);
    form->addRow(tr("Intervallo"), m_intervalDays);
    form->addRow(m_hasEnd, m_end);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(buttons);

    connect(m_type, &QComboBox::currentIndexChanged, this, &EventDialog::onTypeChanged);
    connect(m_hasEnd, &QCheckBox::toggled, m_end, &QWidget::setEnabled);
    connect(okButton, &QPushButton::clicked, this, &EventDialog::onAccept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    onTypeChanged();
}

void EventDialog::setStart(const QDateTime& start) {
    m_start->setDateTime(start.toLocalTime());
}

void EventDialog::setOccurrence(const Occurrence& occurrence) {
    setWindowTitle(tr("Modifica occorrenza"));
    m_title->setText(occurrence.title);
    m_start->setDateTime(occurrence.start.toLocalTime());
    const qint64 minutes =
        qMax<qint64>(1, occurrence.start.secsTo(occurrence.end) / 60);
    m_durationMinutes->setValue(static_cast<int>(
        qMin<qint64>(minutes, m_durationMinutes->maximum())));
    // La modifica di una singola istanza diventa un evento singolo.
    m_type->setCurrentIndex(0);
    m_type->setEnabled(false);
    m_hasEnd->setChecked(false);
}

void EventDialog::onTypeChanged() {
    const bool isFixed = m_type->currentData().toString() ==
                         QStringLiteral("fixed");
    m_intervalDays->setEnabled(isFixed);
}

CreateEventRequest EventDialog::request() const {
    CreateEventRequest request;
    request.title = m_title->text().trimmed();
    request.start = m_start->dateTime().toUTC();
    request.durationSec = static_cast<qint64>(m_durationMinutes->value()) * 60;
    eventTypeFromString(m_type->currentData().toString(), request.type);
    request.intervalSec = static_cast<qint64>(m_intervalDays->value()) * 86400;
    if (m_hasEnd->isChecked()) {
        request.end = m_end->dateTime().toUTC();
    }
    return request;
}

void EventDialog::onAccept() {
    if (m_title->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Evento non valido"),
                             tr("Inserisci un titolo."));
        return;
    }
    if (m_type->currentData().toString() == QStringLiteral("fixed") &&
        m_intervalDays->value() <= 0) {
        QMessageBox::warning(this, tr("Evento non valido"),
                             tr("L'intervallo deve essere maggiore di zero."));
        return;
    }
    accept();
}

} // namespace client
