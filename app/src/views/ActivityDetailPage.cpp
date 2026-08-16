#include "views/ActivityDetailPage.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "CalendarController.h"
#include "events/core/ActivityVisitor.h"
#include "events/domain/Deadline.h"
#include "events/domain/Event.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Reminder.h"
#include "views/ActivityViewHelpers.h"

namespace app {

namespace {

QString localDateTime(const events::TimePoint tp) {
  return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count())
      .toString(QStringLiteral("dd/MM/yyyy HH:mm"));
}

// --- Visitor: costruisce le righe "campo: valore" specifiche per tipo --------
class FieldsVisitor : public events::ActivityVisitor {
public:
  QStringList fields;

  void visit(const events::Event& event) override {
    fields << QObject::tr("Inizio: %1").arg(localDateTime(event.getStart()))
           << QObject::tr("Fine: %1").arg(localDateTime(event.getEnd()))
           << QObject::tr("Durata: %1")
                  .arg(ActivityViewHelpers::durationLabel(event.getDuration()));
  }

  void visit(const events::RecurrentEvent& event) override {
    fields << QObject::tr("Regola: %1")
                  .arg(ActivityViewHelpers::recurrenceRuleLabel(event))
           << QObject::tr("Prima occorrenza: %1")
                  .arg(localDateTime(event.getTemplateEvent().getStart()))
           << QObject::tr("Durata: %1")
                  .arg(ActivityViewHelpers::durationLabel(
                      event.getTemplateEvent().getDuration()))
           << QObject::tr("Eccezioni: %1").arg(event.getExceptions().size());
  }

  void visit(const events::Deadline& deadline) override {
    fields << QObject::tr("Scadenza: %1").arg(localDateTime(deadline.getDue()))
           << QObject::tr("Priorita': %1")
                  .arg(QString::fromStdString(
                      events::Deadline::priorityLabel(deadline.getPriority())))
           << QObject::tr("Stato: %1")
                  .arg(deadline.isDone() ? QObject::tr("evasa")
                                         : QObject::tr("in corso"));
  }

  void visit(const events::Reminder& reminder) override {
    fields << QObject::tr("Attivazione: %1")
                  .arg(localDateTime(reminder.getTrigger()))
           << QObject::tr("Messaggio: %1")
                  .arg(QString::fromStdString(reminder.getMessage()))
           << QObject::tr("Ripetizione: %1")
                  .arg(reminder.isRepeating()
                           ? QObject::tr("%1 giorni")
                                 .arg(reminder.getRepeatInterval().count() / 86400)
                           : QObject::tr("una tantum"));
  }
};

} // namespace

ActivityDetailPage::ActivityDetailPage(CalendarController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    m_fieldsLabel = new QLabel(this);
    m_fieldsLabel->setWordWrap(true);

    auto* backButton = new QPushButton(tr("Indietro"), this);
    m_editButton = new QPushButton(tr("Modifica"), this);
    m_deleteButton = new QPushButton(tr("Elimina"), this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_fieldsLabel, 1);
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(backButton);
    buttons->addStretch(1);
    buttons->addWidget(m_editButton);
    buttons->addWidget(m_deleteButton);
    layout->addLayout(buttons);

    connect(backButton, &QPushButton::clicked, this, &ActivityDetailPage::backRequested);
    connect(m_editButton, &QPushButton::clicked, this, &ActivityDetailPage::onEdit);
    connect(m_deleteButton, &QPushButton::clicked, this, &ActivityDetailPage::onDelete);
}

void ActivityDetailPage::showActivity(const events::Activity* activity) {
    m_activity = activity;
    m_titleLabel->setText(QString::fromStdString(activity->getTitle()));

    FieldsVisitor visitor;
    activity->accept(visitor);
    m_fieldsLabel->setText(visitor.fields.join(QLatin1Char('\n')));
}

const events::Activity* ActivityDetailPage::currentActivity() const {
    return m_activity;
}

void ActivityDetailPage::onEdit() {
    if (m_activity) {
        emit editRequested(m_activity);
    }
}

void ActivityDetailPage::onDelete() {
    if (!m_activity) {
        return;
    }
    if (QMessageBox::question(this, tr("Elimina attivita'"),
                              tr("Eliminare '%1'?").arg(
                                  QString::fromStdString(m_activity->getTitle()))) !=
        QMessageBox::Yes) {
        return;
    }
    m_controller->removeActivity(m_activity);
    m_activity = nullptr;
    emit backRequested();
}

} // namespace app
