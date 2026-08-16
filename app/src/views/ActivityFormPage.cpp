#include "views/ActivityFormPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimeEdit>
#include <QVBoxLayout>

#include <memory>

#include "CalendarController.h"
#include "events/domain/ActivityFactory.h"
#include "events/domain/Deadline.h"
#include "events/domain/Event.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Reminder.h"
#include "events/generators/FixedIntervalGenerator.h"

namespace app {

namespace {

// Indici dei pannelli nello QStackedWidget
constexpr int kEventPanel = 0;
constexpr int kRecurrentPanel = 1;
constexpr int kDeadlinePanel = 2;
constexpr int kReminderPanel = 3;

// Riga del form: etichetta a sinistra, box di input affiancata a destra
void addRow(QGridLayout* grid, int row, const QString& label, QWidget* field) {
    auto* caption = new QLabel(label);
    caption->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(caption, row, 0);
    grid->addWidget(field, row, 1);
}

QLineEdit* makeTitle(QWidget* parent) {
    // Nessun placeholder: alla creazione la box deve essere semplicemente vuota
    return new QLineEdit(parent);
}

QDateTimeEdit* makeDate(QWidget* parent) {
    auto* edit = new QDateTimeEdit(QDateTime::currentDateTime(), parent);
    edit->setCalendarPopup(true);
    edit->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
    return edit;
}

QTimeEdit* makeDuration(QWidget* parent) {
    auto* edit = new QTimeEdit(QTime(1, 0), parent);
    edit->setDisplayFormat(QStringLiteral("HH:mm"));
    return edit;
}

} // namespace

ActivityFormPage::ActivityFormPage(CalendarController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Evento"));
    m_typeCombo->addItem(tr("Evento ricorrente"));
    m_typeCombo->addItem(tr("Scadenza"));
    m_typeCombo->addItem(tr("Promemoria"));

    m_forms = new QStackedWidget(this);
    m_forms->addWidget(buildEventPanel());
    m_forms->addWidget(buildRecurrentPanel());
    m_forms->addWidget(buildDeadlinePanel());
    m_forms->addWidget(buildReminderPanel());

    auto* saveButton = new QPushButton(tr("Salva"), this);
    auto* cancelButton = new QPushButton(tr("Annulla"), this);
    m_saveButton = saveButton;

    m_errorLabel = new QLabel(this);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #c5221f;"));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_typeCombo);
    layout->addWidget(m_forms, 1);
    layout->addWidget(m_errorLabel);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(saveButton);
    buttons->addWidget(cancelButton);
    layout->addLayout(buttons);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ActivityFormPage::onTypeChanged);
    connect(m_hasEndCheck, &QCheckBox::toggled,
            this, &ActivityFormPage::onRecurrenceEndToggled);
    connect(saveButton, &QPushButton::clicked, this, &ActivityFormPage::onSave);
    connect(cancelButton, &QPushButton::clicked, this, &ActivityFormPage::backRequested);
}

QWidget* ActivityFormPage::buildEventPanel() {
    auto* panel = new QWidget(this);
    m_titleE = makeTitle(panel);
    m_startE = makeDate(panel);
    m_durationE = makeDuration(panel);

    auto* grid = new QGridLayout(panel);
    grid->setColumnStretch(1, 1);
    addRow(grid, 0, tr("Titolo"), m_titleE);
    addRow(grid, 1, tr("Data"), m_startE);
    addRow(grid, 2, tr("Durata"), m_durationE);
    return panel;
}

QWidget* ActivityFormPage::buildRecurrentPanel() {
    auto* panel = new QWidget(this);
    m_titleR = makeTitle(panel);
    m_startR = makeDate(panel);
    m_durationR = makeDuration(panel);
    m_intervalDays = new QSpinBox(panel);
    m_intervalDays->setRange(1, 3650);
    m_intervalDays->setValue(7);
    m_intervalDays->setSuffix(tr(" giorni"));
    m_hasEndCheck = new QCheckBox(tr("Scade il"), panel);
    m_endEdit = makeDate(panel);
    m_endEdit->setEnabled(false);

    auto* grid = new QGridLayout(panel);
    grid->setColumnStretch(1, 1);
    addRow(grid, 0, tr("Titolo"), m_titleR);
    addRow(grid, 1, tr("Data"), m_startR);
    addRow(grid, 2, tr("Durata"), m_durationR);
    addRow(grid, 3, tr("Ogni"), m_intervalDays);
    grid->addWidget(m_hasEndCheck, 4, 0, Qt::AlignRight);
    grid->addWidget(m_endEdit, 4, 1);
    return panel;
}

QWidget* ActivityFormPage::buildDeadlinePanel() {
    auto* panel = new QWidget(this);
    m_titleD = makeTitle(panel);
    m_dueEdit = makeDate(panel);
    m_priorityCombo = new QComboBox(panel);
    m_priorityCombo->addItem(tr("Bassa"));
    m_priorityCombo->addItem(tr("Media"));
    m_priorityCombo->addItem(tr("Alta"));
    m_priorityCombo->setCurrentIndex(1);
    m_doneCheck = new QCheckBox(tr("Evasa"), panel);

    auto* grid = new QGridLayout(panel);
    grid->setColumnStretch(1, 1);
    addRow(grid, 0, tr("Titolo"), m_titleD);
    addRow(grid, 1, tr("Data"), m_dueEdit);
    addRow(grid, 2, tr("Priorita'"), m_priorityCombo);
    grid->addWidget(m_doneCheck, 3, 0, 1, 2);
    return panel;
}

QWidget* ActivityFormPage::buildReminderPanel() {
    auto* panel = new QWidget(this);
    m_titleM = makeTitle(panel);
    m_triggerEdit = makeDate(panel);
    m_messageEdit = new QLineEdit(panel);
    m_messageEdit->setPlaceholderText(tr("Dettagli del promemoria"));
    m_repeatDays = new QSpinBox(panel);
    m_repeatDays->setRange(0, 3650);
    m_repeatDays->setSpecialValueText(tr("Una tantum"));
    m_repeatDays->setSuffix(tr(" giorni"));

    auto* grid = new QGridLayout(panel);
    grid->setColumnStretch(1, 1);
    addRow(grid, 0, tr("Titolo"), m_titleM);
    addRow(grid, 1, tr("Data"), m_triggerEdit);
    addRow(grid, 2, tr("Dettagli"), m_messageEdit);
    addRow(grid, 3, tr("Ripeti ogni"), m_repeatDays);
    return panel;
}

events::TimePoint ActivityFormPage::toTimePoint(const QDateTime& local) {
  return events::TimePoint(std::chrono::seconds(local.toSecsSinceEpoch()));
}

QDateTime ActivityFormPage::toLocal(const events::TimePoint tp) {
  return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count()).toLocalTime();
}

QLineEdit* ActivityFormPage::titleOf(int panel) const {
  switch (panel) {
  case kEventPanel:
    return m_titleE;
  case kRecurrentPanel:
    return m_titleR;
  case kDeadlinePanel:
    return m_titleD;
  case kReminderPanel:
  default:
    return m_titleM;
  }
}

QDateTimeEdit* ActivityFormPage::dateOf(int panel) const {
  switch (panel) {
  case kEventPanel:
    return m_startE;
  case kRecurrentPanel:
    return m_startR;
  case kDeadlinePanel:
    return m_dueEdit;
  case kReminderPanel:
  default:
    return m_triggerEdit;
  }
}

QTimeEdit* ActivityFormPage::durationOf(int panel) const {
  switch (panel) {
  case kEventPanel:
    return m_durationE;
  case kRecurrentPanel:
    return m_durationR;
  case kDeadlinePanel:
  case kReminderPanel:
  default:
    return nullptr;
  }
}

void ActivityFormPage::onTypeChanged(int index) {
  const int from = m_forms->currentIndex();
  if (from != index) {
    syncCommonFields(from, index);
  }
  m_forms->setCurrentIndex(index);
}

void ActivityFormPage::syncCommonFields(int fromPanel, int toPanel) {
  if (fromPanel < 0 || fromPanel > kReminderPanel || toPanel < 0 ||
      toPanel > kReminderPanel || fromPanel == toPanel) {
    return;
  }
  // Conserva quello che l'utente ha gia' scritto quando cambia tipo
  titleOf(toPanel)->setText(titleOf(fromPanel)->text());
  dateOf(toPanel)->setDateTime(dateOf(fromPanel)->dateTime());
  if (QTimeEdit* fromDur = durationOf(fromPanel)) {
    if (QTimeEdit* toDur = durationOf(toPanel)) {
      toDur->setTime(fromDur->time());
    }
  }
}

void ActivityFormPage::startCreate(const QDateTime& suggestedStart) {
  m_mode = Mode::Create;
  m_editingActivity = nullptr;
  m_editingOccurrence.reset();
  m_errorLabel->clear();

  // Box del titolo vuota (nessun testo residuo da creazioni precedenti)
  m_titleE->clear();
  m_titleR->clear();
  m_titleD->clear();
  m_titleM->clear();
  m_messageEdit->clear();
  m_durationE->setTime(QTime(1, 0));
  m_durationR->setTime(QTime(1, 0));
  m_intervalDays->setValue(7);
  m_repeatDays->setValue(0);
  m_hasEndCheck->setChecked(false);
  m_priorityCombo->setCurrentIndex(1);
  m_doneCheck->setChecked(false);

  // Data/ora suggerita (doppio clic su una cella): precompila ogni pannello
  const QDateTime value = suggestedStart.isValid()
                              ? suggestedStart
                              : QDateTime::currentDateTime();
  m_startE->setDateTime(value);
  m_startR->setDateTime(value);
  m_dueEdit->setDateTime(value);
  m_triggerEdit->setDateTime(value);

  m_typeCombo->setEnabled(true);
  m_typeCombo->setCurrentIndex(kEventPanel);
  m_doneCheck->setEnabled(true);
  m_saveButton->setText(tr("Salva"));
  m_forms->setCurrentIndex(kEventPanel);
}

void ActivityFormPage::startEditActivity(const events::Activity* activity) {
  m_mode = Mode::EditActivity;
  m_editingActivity = activity;
  m_editingOccurrence.reset();
  m_errorLabel->clear();

  if (auto* event = dynamic_cast<const events::Event*>(activity)) {
    populateEvent(*event);
  } else if (auto* recurrent = dynamic_cast<const events::RecurrentEvent*>(activity)) {
    populateRecurrent(*recurrent);
  } else if (auto* deadline = dynamic_cast<const events::Deadline*>(activity)) {
    populateDeadline(*deadline);
  } else if (auto* reminder = dynamic_cast<const events::Reminder*>(activity)) {
    populateReminder(*reminder);
  }
  m_typeCombo->setEnabled(false);
  m_doneCheck->setEnabled(true);
  m_saveButton->setText(tr("Salva"));
}

void ActivityFormPage::startEditOccurrence(const events::Occurrence& occurrence) {
  m_mode = Mode::EditOccurrence;
  m_editingActivity = nullptr;
  m_editingOccurrence = occurrence;
  m_errorLabel->clear();

  m_typeCombo->setEnabled(false);
  m_typeCombo->setCurrentIndex(kEventPanel);
  m_titleE->setText(QString::fromStdString(occurrence.source->getTitle()));
  m_startE->setDateTime(toLocal(occurrence.start));
  m_durationE->setTime(QTime(0, 0).addSecs(
      static_cast<int>(occurrence.duration.count())));
  m_saveButton->setText(tr("Salva"));
  m_forms->setCurrentIndex(kEventPanel);
}

void ActivityFormPage::populateEvent(const events::Event& event) {
  m_titleE->setText(QString::fromStdString(event.getTitle()));
  m_startE->setDateTime(toLocal(event.getStart()));
  m_durationE->setTime(QTime(0, 0).addSecs(
      static_cast<int>(event.getDuration().count())));
  m_typeCombo->setCurrentIndex(kEventPanel);
  m_forms->setCurrentIndex(kEventPanel);
}

void ActivityFormPage::populateRecurrent(const events::RecurrentEvent& event) {
  const events::Event& templ = event.getTemplateEvent();
  m_titleR->setText(QString::fromStdString(event.getTitle()));
  m_startR->setDateTime(toLocal(templ.getStart()));
  m_durationR->setTime(QTime(0, 0).addSecs(
      static_cast<int>(templ.getDuration().count())));

  if (auto* fixed = dynamic_cast<const events::FixedIntervalGenerator*>(
          event.getGenerator().get())) {
    m_intervalDays->setValue(
        static_cast<int>(fixed->getInterval().count() / 86400));
    if (fixed->getEnd() != events::TimePoint::max()) {
      m_hasEndCheck->setChecked(true);
      m_endEdit->setDateTime(toLocal(fixed->getEnd()));
    } else {
      m_hasEndCheck->setChecked(false);
    }
  } else {
    m_intervalDays->setValue(365);
    m_hasEndCheck->setChecked(false);
  }
  m_typeCombo->setCurrentIndex(kRecurrentPanel);
  m_forms->setCurrentIndex(kRecurrentPanel);
}

void ActivityFormPage::populateDeadline(const events::Deadline& deadline) {
  m_titleD->setText(QString::fromStdString(deadline.getTitle()));
  m_dueEdit->setDateTime(toLocal(deadline.getDue()));
  switch (deadline.getPriority()) {
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
  m_doneCheck->setChecked(deadline.isDone());
  m_typeCombo->setCurrentIndex(kDeadlinePanel);
  m_forms->setCurrentIndex(kDeadlinePanel);
}

void ActivityFormPage::populateReminder(const events::Reminder& reminder) {
  m_titleM->setText(QString::fromStdString(reminder.getTitle()));
  m_triggerEdit->setDateTime(toLocal(reminder.getTrigger()));
  m_messageEdit->setText(QString::fromStdString(reminder.getMessage()));
  m_repeatDays->setValue(
      static_cast<int>(reminder.getRepeatInterval().count() / 86400));
  m_typeCombo->setCurrentIndex(kReminderPanel);
  m_forms->setCurrentIndex(kReminderPanel);
}

void ActivityFormPage::onRecurrenceEndToggled(bool checked) {
  m_endEdit->setEnabled(checked);
}

std::unique_ptr<events::Activity> ActivityFormPage::buildActivity() const {
  const int panelIndex = m_forms->currentIndex();
  const QString title = titleOf(panelIndex)->text().trimmed();
  if (title.isEmpty()) {
    return nullptr;
  }

  const events::TimePoint start = toTimePoint(dateOf(panelIndex)->dateTime());

  switch (panelIndex) {
  case kEventPanel: {
    const events::Duration duration = std::chrono::seconds(
        m_durationE->time().msecsSinceStartOfDay() / 1000);
    return events::ActivityFactory::createSimpleEvent(title.toStdString(), start,
                                                      duration);
  }

  case kRecurrentPanel: {
    const events::Duration duration = std::chrono::seconds(
        m_durationR->time().msecsSinceStartOfDay() / 1000);
    const events::Duration interval = events::Days(m_intervalDays->value());
    events::TimePoint end = events::TimePoint::max();
    if (m_hasEndCheck->isChecked()) {
      end = toTimePoint(m_endEdit->dateTime());
    }
    auto generator = std::make_shared<events::FixedIntervalGenerator>(
        start, interval, end);
    return std::make_unique<events::RecurrentEvent>(
        generator, events::Event(title.toStdString(), start, duration));
  }

  case kDeadlinePanel: {
    events::Priority priority = events::Priority::Medium;
    switch (m_priorityCombo->currentIndex()) {
    case 0:
      priority = events::Priority::Low;
      break;
    case 2:
      priority = events::Priority::High;
      break;
    default:
      break;
    }
    auto deadline = events::ActivityFactory::createDeadline(
        title.toStdString(), toTimePoint(m_dueEdit->dateTime()), priority);
    if (m_doneCheck->isEnabled()) {
      deadline->setDone(m_doneCheck->isChecked());
    }
    return deadline;
  }

  case kReminderPanel: {
    const events::Duration repeat = events::Days(m_repeatDays->value());
    return events::ActivityFactory::createReminder(
        title.toStdString(), toTimePoint(m_triggerEdit->dateTime()),
        m_messageEdit->text().toStdString(), repeat);
  }

  default:
    return nullptr;
  }
}

void ActivityFormPage::onSave() {
  std::unique_ptr<events::Activity> activity = buildActivity();
  if (!activity) {
    m_errorLabel->setText(tr("Inserire un titolo non vuoto."));
    return;
  }

  bool ok = false;
  switch (m_mode) {
  case Mode::Create:
    ok = m_controller->addActivity(std::move(activity));
    break;
  case Mode::EditActivity:
    ok = m_controller->updateActivity(m_editingActivity, std::move(activity));
    break;
  case Mode::EditOccurrence: {
    auto* event = dynamic_cast<events::Event*>(activity.get());
    if (!event) {
      m_errorLabel->setText(tr("Errore interno nella costruzione dell'evento."));
      return;
    }
    std::unique_ptr<events::Event> replacement(
        static_cast<events::Event*>(activity.release()));
    ok = m_controller->modifyOccurrence(*m_editingOccurrence,
                                        std::move(replacement));
    break;
  }
  }

  if (ok) {
    m_errorLabel->clear();
    emit backRequested();
  } else {
    m_errorLabel->setText(tr("Operazione non riuscita."));
  }
}

} // namespace app
