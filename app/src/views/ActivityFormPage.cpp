#include "views/ActivityFormPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimeEdit>
#include <QVBoxLayout>

#include <memory>

#include "CalendarController.h"
#include "events/domain/ActivityFactory.h"
#include "events/domain/AllDayEvent.h"
#include "events/domain/Anniversary.h"
#include "events/domain/Event.h"
#include "events/domain/Meeting.h"
#include "events/domain/RecurrentEvent.h"
#include "events/domain/Task.h"
#include "events/generators/FixedIntervalGenerator.h"

namespace app {

namespace {

// Indici dei pannelli nello QStackedWidget
constexpr int kEventPanel = 0;
constexpr int kRecurrentPanel = 1;
constexpr int kMeetingPanel = 2;
constexpr int kTaskPanel = 3;
constexpr int kAllDayPanel = 4;
constexpr int kAnniversaryPanel = 5;
constexpr int kPanelCount = 6;

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

QDateEdit* makeDay(QWidget* parent) {
    auto* edit = new QDateEdit(QDate::currentDate(), parent);
    edit->setCalendarPopup(true);
    edit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
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
    m_typeCombo->addItem(tr("Riunione"));
    m_typeCombo->addItem(tr("Compito"));
    m_typeCombo->addItem(tr("Tutto il giorno"));
    m_typeCombo->addItem(tr("Anniversario"));

    m_forms = new QStackedWidget(this);
    m_forms->addWidget(buildEventPanel());
    m_forms->addWidget(buildRecurrentPanel());
    m_forms->addWidget(buildMeetingPanel());
    m_forms->addWidget(buildTaskPanel());
    m_forms->addWidget(buildAllDayPanel());
    m_forms->addWidget(buildAnniversaryPanel());

    auto* saveButton = new QPushButton(tr("Salva"), this);
    auto* cancelButton = new QPushButton(tr("Annulla"), this);
    m_saveButton = saveButton;
    // Elimina: visibile solo in modifica (in creazione non c'e' nulla da
    // eliminare); si nasconde con startCreate()
    m_deleteButton = new QPushButton(tr("Elimina"), this);
    m_deleteButton->setVisible(false);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #c5221f;"));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_typeCombo);
    layout->addWidget(m_forms, 1);
    layout->addWidget(m_errorLabel);
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_deleteButton);
    buttons->addStretch(1);
    buttons->addWidget(saveButton);
    buttons->addWidget(cancelButton);
    layout->addLayout(buttons);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ActivityFormPage::onTypeChanged);
    connect(m_hasEndCheck, &QCheckBox::toggled,
            this, &ActivityFormPage::onRecurrenceEndToggled);
    // L'orario della scadenza segue la fine dell'evento ripetuto quando
    // cambia l'orario dell'inizio o la durata della serie.
    connect(m_startR, &QDateTimeEdit::dateTimeChanged,
            this, [this] { syncRecurrenceEndTime(); });
    connect(m_durationR, &QTimeEdit::timeChanged,
            this, [this] { syncRecurrenceEndTime(); });
    connect(saveButton, &QPushButton::clicked, this, &ActivityFormPage::onSave);
    connect(m_deleteButton, &QPushButton::clicked, this, &ActivityFormPage::onDelete);
    connect(cancelButton, &QPushButton::clicked, this, &ActivityFormPage::backRequested);

    // Partecipanti della riunione
    connect(m_attendeeEdit, &QLineEdit::returnPressed,
            this, &ActivityFormPage::onAddAttendee);
    connect(m_attendeesList, &QListWidget::itemDoubleClicked,
            this, &ActivityFormPage::onRemoveAttendee);

    // Anteprima live: ogni modifica dei campi aggiorna la griglia
    const auto refreshPreview = [this] { emitPreview(); };
    connect(m_titleE, &QLineEdit::textChanged, this, refreshPreview);
    connect(m_startE, &QDateTimeEdit::dateTimeChanged, this, refreshPreview);
    connect(m_durationE, &QTimeEdit::timeChanged, this, refreshPreview);
    connect(m_titleR, &QLineEdit::textChanged, this, refreshPreview);
    connect(m_durationR, &QTimeEdit::timeChanged, this, refreshPreview);
    connect(m_titleMt, &QLineEdit::textChanged, this, refreshPreview);
    connect(m_startMt, &QDateTimeEdit::dateTimeChanged, this, refreshPreview);
    connect(m_durationMt, &QTimeEdit::timeChanged, this, refreshPreview);
    connect(m_titleT, &QLineEdit::textChanged, this, refreshPreview);
    connect(m_dueT, &QDateTimeEdit::dateTimeChanged, this, refreshPreview);
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

QWidget* ActivityFormPage::buildMeetingPanel() {
    auto* panel = new QWidget(this);
    m_titleMt = makeTitle(panel);
    m_startMt = makeDate(panel);
    m_durationMt = makeDuration(panel);
    m_locationMt = new QLineEdit(panel);
    m_locationMt->setPlaceholderText(tr("Aula, sede o link"));

    m_attendeeEdit = new QLineEdit(panel);
    m_attendeeEdit->setPlaceholderText(tr("Nome partecipante + Invio"));
    m_attendeesList = new QListWidget(panel);
    m_attendeesList->setMaximumHeight(110);

    auto* grid = new QGridLayout(panel);
    grid->setColumnStretch(1, 1);
    addRow(grid, 0, tr("Titolo"), m_titleMt);
    addRow(grid, 1, tr("Data"), m_startMt);
    addRow(grid, 2, tr("Durata"), m_durationMt);
    addRow(grid, 3, tr("Luogo"), m_locationMt);
    addRow(grid, 4, tr("Aggiungi"), m_attendeeEdit);
    addRow(grid, 5, tr("Partecipanti"), m_attendeesList);
    return panel;
}

QWidget* ActivityFormPage::buildTaskPanel() {
    auto* panel = new QWidget(this);
    m_titleT = makeTitle(panel);
    m_dueT = makeDate(panel);
    m_priorityCombo = new QComboBox(panel);
    m_priorityCombo->addItem(tr("Bassa"));
    m_priorityCombo->addItem(tr("Media"));
    m_priorityCombo->addItem(tr("Alta"));
    m_priorityCombo->setCurrentIndex(1);
    m_doneCheck = new QCheckBox(tr("Evaso"), panel);

    auto* grid = new QGridLayout(panel);
    grid->setColumnStretch(1, 1);
    addRow(grid, 0, tr("Titolo"), m_titleT);
    addRow(grid, 1, tr("Scadenza"), m_dueT);
    addRow(grid, 2, tr("Priorita'"), m_priorityCombo);
    grid->addWidget(m_doneCheck, 3, 0, 1, 2);
    return panel;
}

QWidget* ActivityFormPage::buildAllDayPanel() {
    auto* panel = new QWidget(this);
    m_titleA = makeTitle(panel);
    m_startDateA = makeDay(panel);
    m_endDateA = makeDay(panel);

    auto* grid = new QGridLayout(panel);
    grid->setColumnStretch(1, 1);
    addRow(grid, 0, tr("Titolo"), m_titleA);
    addRow(grid, 1, tr("Dal"), m_startDateA);
    addRow(grid, 2, tr("Al"), m_endDateA);
    return panel;
}

QWidget* ActivityFormPage::buildAnniversaryPanel() {
    auto* panel = new QWidget(this);
    m_titleAn = makeTitle(panel);
    m_dateAn = makeDay(panel);

    auto* grid = new QGridLayout(panel);
    grid->setColumnStretch(1, 1);
    addRow(grid, 0, tr("Titolo"), m_titleAn);
    addRow(grid, 1, tr("Data"), m_dateAn);
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
  case kMeetingPanel:
    return m_titleMt;
  case kTaskPanel:
    return m_titleT;
  case kAllDayPanel:
    return m_titleA;
  case kAnniversaryPanel:
    return m_titleAn;
  default:
    return nullptr;
  }
}

QDateTimeEdit* ActivityFormPage::dateOf(int panel) const {
  switch (panel) {
  case kEventPanel:
    return m_startE;
  case kRecurrentPanel:
    return m_startR;
  case kMeetingPanel:
    return m_startMt;
  case kTaskPanel:
    return m_dueT;
  case kAllDayPanel:
  case kAnniversaryPanel:
  default:
    return nullptr;
  }
}

QTimeEdit* ActivityFormPage::durationOf(int panel) const {
  switch (panel) {
  case kEventPanel:
    return m_durationE;
  case kRecurrentPanel:
    return m_durationR;
  case kMeetingPanel:
    return m_durationMt;
  case kTaskPanel:
  case kAllDayPanel:
  case kAnniversaryPanel:
  default:
    return nullptr;
  }
}

void ActivityFormPage::onTypeChanged(int index) {
  const int from = m_forms->currentIndex();
  // In CREAZIONE i campi comuni (titolo/data) si conservano cambiando tipo;
  // in MODIFICA il cambio combo e' programmatico (populate*): i campi sono
  // gia' stati compilati e non vanno sovrascritti.
  if (from != index && m_mode == Mode::Create) {
    syncCommonFields(from, index);
  }
  m_forms->setCurrentIndex(index);
  emitPreview();
}

void ActivityFormPage::syncCommonFields(int fromPanel, int toPanel) {
  if (fromPanel < 0 || fromPanel >= kPanelCount || toPanel < 0 ||
      toPanel >= kPanelCount || fromPanel == toPanel) {
    return;
  }
  // Conserva quello che l'utente ha gia' scritto quando cambia tipo
  titleOf(toPanel)->setText(titleOf(fromPanel)->text());
  if (QDateTimeEdit* fromDate = dateOf(fromPanel)) {
    if (QDateTimeEdit* toDate = dateOf(toPanel)) {
      toDate->setDateTime(fromDate->dateTime());
    }
  }
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
  m_titleMt->clear();
  m_titleT->clear();
  m_titleA->clear();
  m_titleAn->clear();
  m_locationMt->clear();
  m_attendeeEdit->clear();
  m_attendeesList->clear();
  m_durationE->setTime(QTime(1, 0));
  m_durationR->setTime(QTime(1, 0));
  m_durationMt->setTime(QTime(1, 0));
  m_intervalDays->setValue(7);
  m_hasEndCheck->setChecked(false);
  m_priorityCombo->setCurrentIndex(1);
  m_doneCheck->setChecked(false);
  // In creazione non c'e' nulla da eliminare: il bottone resta nascosto
  m_deleteButton->setVisible(false);

  // Data/ora suggerita (doppio clic su una cella): precompila ogni pannello
  const QDateTime value = suggestedStart.isValid()
                              ? suggestedStart
                              : QDateTime::currentDateTime();
  m_startE->setDateTime(value);
  m_startR->setDateTime(value);
  m_startMt->setDateTime(value);
  m_dueT->setDateTime(value);
  m_startDateA->setDate(value.date());
  m_endDateA->setDate(value.date());
  m_dateAn->setDate(value.date());

  m_typeCombo->setEnabled(true);
  m_typeCombo->setCurrentIndex(kEventPanel);
  m_doneCheck->setEnabled(true);
  m_saveButton->setText(tr("Salva"));
  m_forms->setCurrentIndex(kEventPanel);
  emitPreview();
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
  } else if (auto* meeting = dynamic_cast<const events::Meeting*>(activity)) {
    populateMeeting(*meeting);
  } else if (auto* task = dynamic_cast<const events::Task*>(activity)) {
    populateTask(*task);
  } else if (auto* allday = dynamic_cast<const events::AllDayEvent*>(activity)) {
    populateAllDay(*allday);
  } else if (auto* anniversary = dynamic_cast<const events::Anniversary*>(activity)) {
    populateAnniversary(*anniversary);
  }
  m_typeCombo->setEnabled(false);
  m_doneCheck->setEnabled(true);
  m_saveButton->setText(tr("Salva"));
  m_deleteButton->setVisible(true);
  emitPreview();
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
  m_deleteButton->setVisible(true);
  emitPreview();
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

void ActivityFormPage::populateMeeting(const events::Meeting& meeting) {
  m_titleMt->setText(QString::fromStdString(meeting.getTitle()));
  m_startMt->setDateTime(toLocal(meeting.getStart()));
  m_durationMt->setTime(QTime(0, 0).addSecs(
      static_cast<int>(meeting.getDuration().count())));
  m_locationMt->setText(QString::fromStdString(meeting.getLocation()));
  m_attendeesList->clear();
  for (const auto& name : meeting.getAttendees()) {
    new QListWidgetItem(QString::fromStdString(name), m_attendeesList);
  }
  m_typeCombo->setCurrentIndex(kMeetingPanel);
  m_forms->setCurrentIndex(kMeetingPanel);
}

void ActivityFormPage::populateTask(const events::Task& task) {
  m_titleT->setText(QString::fromStdString(task.getTitle()));
  m_dueT->setDateTime(toLocal(task.getDue()));
  switch (task.getPriority()) {
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
  m_doneCheck->setChecked(task.isDone());
  m_typeCombo->setCurrentIndex(kTaskPanel);
  m_forms->setCurrentIndex(kTaskPanel);
}

void ActivityFormPage::populateAllDay(const events::AllDayEvent& event) {
  m_titleA->setText(QString::fromStdString(event.getTitle()));
  m_startDateA->setDate(QDateTime::fromSecsSinceEpoch(
      event.getStart().time_since_epoch().count()).date());
  m_endDateA->setDate(QDateTime::fromSecsSinceEpoch(
      event.getEnd().time_since_epoch().count()).addDays(-1).date());
  m_typeCombo->setCurrentIndex(kAllDayPanel);
  m_forms->setCurrentIndex(kAllDayPanel);
}

void ActivityFormPage::populateAnniversary(const events::Anniversary& anniversary) {
  m_titleAn->setText(QString::fromStdString(anniversary.getTitle()));
  m_dateAn->setDate(QDateTime::fromSecsSinceEpoch(
      anniversary.getStart().time_since_epoch().count()).date());
  m_typeCombo->setCurrentIndex(kAnniversaryPanel);
  m_forms->setCurrentIndex(kAnniversaryPanel);
}

void ActivityFormPage::onRecurrenceEndToggled(bool checked) {
  if (checked) {
    // Valore di default: stessa data dell'inizio, orario = fine dell'evento
    // ripetuto (inizio + durata)
    m_endEdit->setDateTime(
        QDateTime(m_startR->date(), recurrenceEndTime()));
  }
  m_endEdit->setEnabled(checked);
}

QTime ActivityFormPage::recurrenceEndTime() const {
  return m_startR->time().addSecs(
      m_durationR->time().msecsSinceStartOfDay() / 1000);
}

void ActivityFormPage::syncRecurrenceEndTime() {
  if (!m_hasEndCheck->isChecked()) {
    return;
  }
  // La data di scadenza resta quella scelta; l'orario e' la fine dell'evento
  m_endEdit->setTime(recurrenceEndTime());
}

void ActivityFormPage::emitPreview() {
  const int panel = m_forms->currentIndex();
  const QString title = titleOf(panel)->text();
  // Niente anteprima per i tipi senza data/ora nella griglia
  if (panel == kAllDayPanel || panel == kAnniversaryPanel) {
    emit previewChanged(title, QDateTime(), 0, false);
    return;
  }
  const QDateTime start =
      dateOf(panel) ? dateOf(panel)->dateTime() : QDateTime();
  qint64 durationSeconds = 0;
  if (QTimeEdit* dur = durationOf(panel)) {
    durationSeconds = dur->time().msecsSinceStartOfDay() / 1000;
  }
  emit previewChanged(title, start, durationSeconds, true);
}

std::unique_ptr<events::Activity> ActivityFormPage::buildActivity() const {
  const int panelIndex = m_forms->currentIndex();
  const QString title = titleOf(panelIndex)->text().trimmed();
  if (title.isEmpty()) {
    return nullptr;
  }

  switch (panelIndex) {
  case kEventPanel: {
    const events::Duration duration = std::chrono::seconds(
        m_durationE->time().msecsSinceStartOfDay() / 1000);
    auto event = events::ActivityFactory::createSimpleEvent(
        title.toStdString(), toTimePoint(m_startE->dateTime()), duration);
    event->setDone(false);
    return event;
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
        toTimePoint(m_startR->dateTime()), interval, end);
    return std::make_unique<events::RecurrentEvent>(
        generator, events::Event(title.toStdString(),
                                 toTimePoint(m_startR->dateTime()), duration));
  }

  case kMeetingPanel: {
    const events::Duration duration = std::chrono::seconds(
        m_durationMt->time().msecsSinceStartOfDay() / 1000);
    auto meeting = events::ActivityFactory::createMeeting(
        title.toStdString(), toTimePoint(m_startMt->dateTime()), duration,
        m_locationMt->text().trimmed().toStdString());
    for (int i = 0; i < m_attendeesList->count(); ++i) {
      meeting->addAttendee(m_attendeesList->item(i)->text().toStdString());
    }
    return meeting;
  }

  case kTaskPanel: {
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
    auto task = events::ActivityFactory::createTask(
        title.toStdString(), toTimePoint(m_dueT->dateTime()), priority);
    if (m_doneCheck->isEnabled()) {
      task->setDone(m_doneCheck->isChecked());
    }
    return task;
  }

  case kAllDayPanel: {
    const events::TimePoint start =
        toTimePoint(QDateTime(m_startDateA->date(), QTime(0, 0)));
    const events::TimePoint end =
        toTimePoint(QDateTime(m_endDateA->date().addDays(1), QTime(0, 0)));
    return events::ActivityFactory::createAllDayEvent(
        title.toStdString(), start, end);
  }

  case kAnniversaryPanel: {
    const events::TimePoint date =
        toTimePoint(QDateTime(m_dateAn->date(), QTime(0, 0)));
    return events::ActivityFactory::createAnniversary(title.toStdString(), date);
  }

  default:
    return nullptr;
  }
}

void ActivityFormPage::onAddAttendee() {
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

void ActivityFormPage::onRemoveAttendee() {
  if (QListWidgetItem* item = m_attendeesList->currentItem()) {
    delete item;
  }
}

void ActivityFormPage::onDelete() {
  if (m_mode == Mode::Create) {
    return;
  }
  const QString what = m_mode == Mode::EditOccurrence
                           ? tr("questa occorrenza")
                           : tr("l'attivita'");
  if (QMessageBox::question(this, tr("Elimina"),
                            tr("Eliminare %1?").arg(what)) != QMessageBox::Yes) {
    return;
  }

  bool ok = false;
  if (m_mode == Mode::EditActivity) {
    ok = m_controller->removeActivity(m_editingActivity);
  } else if (m_mode == Mode::EditOccurrence && m_editingOccurrence) {
    // Per un'occorrenza di una serie: la serie continua senza quel giorno
    // (eccezione interna); per un'attivita' singola la rimuove del tutto.
    ok = m_controller->deleteOccurrence(*m_editingOccurrence);
  }
  if (ok) {
    emit backRequested();
  }
}

void ActivityFormPage::onSave() {  // Vincolo sulla serie ricorrente: la data di scadenza non puo' essere
  // antecedente (o uguale) alla data di inizio.
  if (m_forms->currentIndex() == kRecurrentPanel && m_hasEndCheck->isChecked()) {
    const events::TimePoint start = toTimePoint(m_startR->dateTime());
    const events::TimePoint end = toTimePoint(m_endEdit->dateTime());
    if (end <= start) {
      m_errorLabel->setText(
          tr("La data di scadenza deve essere successiva all'inizio "
             "dell'evento ricorrente."));
      return;
    }
  }

  // Vincolo "tutto il giorno": la fine deve essere >= l'inizio
  if (m_forms->currentIndex() == kAllDayPanel) {
    if (m_endDateA->date() < m_startDateA->date()) {
      m_errorLabel->setText(
          tr("La data finale non puo' precedere quella iniziale."));
      return;
    }
  }

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