#include "views/ActivityFormPage.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimeEdit>
#include <QTimeZone>
#include <QVBoxLayout>

#include <memory>

#include <algorithm>

#include "CalendarController.h"
#include "events/builders/ActivityConfig.h"
#include "events/domain/Meeting.h"
#include "events/domain/Task.h"
#include "events/generators/FixedIntervalGenerator.h"
#include "events/generators/MonthlyGenerator.h"
#include "events/generators/YearlyGenerator.h"
#include "views/ViewShared.h"

namespace app {

namespace {

// Indici dei pannelli nello QStackedWidget
constexpr int kEventPanel = 0;
constexpr int kMeetingPanel = 1;
constexpr int kTaskPanel = 2;
constexpr int kAnniversaryPanel = 3;
constexpr int kPanelCount = 4;

// Unita' di ricorrenza (indici della combo)
constexpr int kUnitDays = 0;
constexpr int kUnitWeeks = 1;
constexpr int kUnitMonths = 2;
constexpr int kUnitYears = 3;

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
    // La creazione non chiede il tipo: "Evento" e' il pannello "a domande"
    // che istanzia un'attivita' singola o una serie in base alle risposte.
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Evento"));
    m_typeCombo->addItem(tr("Riunione"));
    m_typeCombo->addItem(tr("Compito"));
    m_typeCombo->addItem(tr("Anniversario"));

    m_forms = new QStackedWidget(this);
    m_forms->addWidget(buildEventPanel());
    m_forms->addWidget(buildMeetingPanel());
    m_forms->addWidget(buildTaskPanel());
    m_forms->addWidget(buildAnniversaryPanel());

    auto* saveButton = new QPushButton(tr("Salva"), this);
    auto* cancelButton = new QPushButton(tr("Annulla"), this);
    m_saveButton = saveButton;
    // Elimina: visibile solo in modifica (in creazione non c'e' nulla da
    // eliminare); si nasconde con startCreate()
    m_deleteButton = new QPushButton(tr("Elimina"), this);
    m_deleteButton->setVisible(false);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("formErrorLabel"));
    m_errorLabel->setWordWrap(true);

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
    // Ricorrenza "a domande"
    connect(m_repeatCheck, &QCheckBox::toggled,
            this, &ActivityFormPage::onRepeatToggled);
    connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ActivityFormPage::onUnitChanged);
    connect(m_endDateRadio, &QRadioButton::toggled, this, [this](bool on) {
        m_endDate->setEnabled(on);
    });
    connect(m_endCountRadio, &QRadioButton::toggled, this, [this](bool on) {
        m_countSpin->setEnabled(on);
    });
    // "Tutto il giorno" nasconde gli slot Ora e Durata (si puo' combinare
    // con "Si ripete": una serie che ricorre a giornate intere).
    connect(m_allDayCheck, &QCheckBox::toggled, this, [this](bool on) {
        m_timeRow->setVisible(!on);
        m_durationRow->setVisible(!on);
        emitPreview();
    });
    // Partecipanti della riunione
    connect(m_attendeeEdit, &QLineEdit::returnPressed,
            this, &ActivityFormPage::onAddAttendee);
    connect(m_attendeesList, &QListWidget::itemDoubleClicked,
            this, &ActivityFormPage::onRemoveAttendee);

    connect(saveButton, &QPushButton::clicked, this, &ActivityFormPage::onSave);
    connect(m_deleteButton, &QPushButton::clicked, this, &ActivityFormPage::onDelete);
    connect(cancelButton, &QPushButton::clicked, this, &ActivityFormPage::backRequested);

    // Anteprima live: ogni modifica dei campi aggiorna la griglia
    const auto refreshPreview = [this] { emitPreview(); };
    connect(m_titleE, &QLineEdit::textChanged, this, refreshPreview);
    connect(m_startDateE, &QDateEdit::dateChanged, this, refreshPreview);
    connect(m_startTimeE, &QTimeEdit::timeChanged, this, refreshPreview);
    connect(m_durationE, &QTimeEdit::timeChanged, this, refreshPreview);
    connect(m_titleMt, &QLineEdit::textChanged, this, refreshPreview);
    connect(m_startMt, &QDateTimeEdit::dateTimeChanged, this, refreshPreview);
    connect(m_durationMt, &QTimeEdit::timeChanged, this, refreshPreview);
    connect(m_titleT, &QLineEdit::textChanged, this, refreshPreview);
    connect(m_dueT, &QDateTimeEdit::dateTimeChanged, this, refreshPreview);
}

// ---------------------------------------------------------------------------
// Pannello Evento "a domande"
// ---------------------------------------------------------------------------
QWidget* ActivityFormPage::buildEventPanel() {
    auto* panel = new QWidget(this);
    m_titleE = makeTitle(panel);
    // Data e Ora in DUE slot separati (l'ora sparisce se "Tutto il giorno")
    m_startDateE = makeDay(panel);
    m_startTimeE = new QTimeEdit(QTime(9, 0), panel);
    m_startTimeE->setDisplayFormat(QStringLiteral("HH:mm"));
    m_durationE = makeDuration(panel);
    m_allDayCheck = new QCheckBox(tr("Tutto il giorno"), panel);
    m_repeatCheck = new QCheckBox(tr("Si ripete"), panel);

    // --- Sotto-pannello di ricorrenza (visibile se "Si ripete") ------------
    m_repeatBox = new QWidget(panel);
    m_repeatBox->setVisible(false);

    m_unitCombo = new QComboBox(m_repeatBox);
    m_unitCombo->addItem(tr("giorni"));
    m_unitCombo->addItem(tr("settimane"));
    m_unitCombo->addItem(tr("mesi"));
    m_unitCombo->addItem(tr("anno"));

    m_everySpin = new QSpinBox(m_repeatBox);
    m_everySpin->setRange(1, 3650);
    m_everySpin->setValue(1);
    m_everySpin->setSuffix(tr(" giorni"));

    // Pulsanti dei giorni della settimana (per la ricorrenza settimanale)
    m_dayRow = new QWidget(m_repeatBox);
    auto* dayLayout = new QHBoxLayout(m_dayRow);
    dayLayout->setContentsMargins(0, 0, 0, 0);
    const char* kDayLabels[] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};
    for (int i = 0; i < 7; ++i) {
        auto* button = new QPushButton(QString::fromLatin1(kDayLabels[i]), m_dayRow);
        button->setCheckable(true);
        button->setMinimumWidth(40);
        dayLayout->addWidget(button);
        m_dayButtons.append(button);
    }
    m_dayRow->setVisible(false);

    // Fine della ricorrenza: mai / fino a / dopo N occorrenze
    auto* endGroup = new QGroupBox(tr("Fine"), m_repeatBox);
    m_endNever = new QRadioButton(tr("Mai"), endGroup);
    m_endDateRadio = new QRadioButton(tr("Fino al"), endGroup);
    m_endDate = makeDay(endGroup);
    m_endDate->setEnabled(false);
    m_endCountRadio = new QRadioButton(tr("Dopo"), endGroup);
    m_countSpin = new QSpinBox(endGroup);
    m_countSpin->setRange(1, 10000);
    m_countSpin->setValue(5);
    m_countSpin->setSuffix(tr(" occorrenze"));
    m_countSpin->setEnabled(false);
    auto* endGroupLayout = new QGridLayout(endGroup);
    endGroupLayout->addWidget(m_endNever, 0, 0, 1, 2);
    endGroupLayout->addWidget(m_endDateRadio, 1, 0);
    endGroupLayout->addWidget(m_endDate, 1, 1);
    endGroupLayout->addWidget(m_endCountRadio, 2, 0);
    endGroupLayout->addWidget(m_countSpin, 2, 1);

    auto* repeatLayout = new QGridLayout(m_repeatBox);
    repeatLayout->setContentsMargins(0, 0, 0, 0);
    addRow(repeatLayout, 0, tr("Unita'"), m_unitCombo);
    addRow(repeatLayout, 1, tr("Ogni"), m_everySpin);
    addRow(repeatLayout, 2, tr("Giorni"), m_dayRow);
    repeatLayout->addWidget(endGroup, 3, 0, 1, 2);

    auto* grid = new QGridLayout(panel);
    grid->setColumnStretch(1, 1);
    addRow(grid, 0, tr("Titolo"), m_titleE);
    addRow(grid, 1, tr("Data"), m_startDateE);

    // Righe "Ora" e "Durata": contenitori nascosti quando "Tutto il giorno"
    m_timeRow = new QWidget(panel);
    auto* timeLayout = new QHBoxLayout(m_timeRow);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    auto* timeCaption = new QLabel(tr("Ora"), m_timeRow);
    timeCaption->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    timeLayout->addWidget(timeCaption);
    timeLayout->addWidget(m_startTimeE, 1);
    grid->addWidget(m_timeRow, 2, 0, 1, 2);

    m_durationRow = new QWidget(panel);
    auto* durLayout = new QHBoxLayout(m_durationRow);
    durLayout->setContentsMargins(0, 0, 0, 0);
    auto* durCaption = new QLabel(tr("Durata"), m_durationRow);
    durCaption->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    durLayout->addWidget(durCaption);
    durLayout->addWidget(m_durationE, 1);
    grid->addWidget(m_durationRow, 3, 0, 1, 2);

    auto* checksRow = new QHBoxLayout;
    checksRow->addWidget(m_allDayCheck);
    checksRow->addWidget(m_repeatCheck);
    checksRow->addStretch(1);
    grid->addLayout(checksRow, 4, 0, 1, 2);
    grid->addWidget(m_repeatBox, 5, 0, 1, 2);
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
  case kMeetingPanel:
    return m_titleMt;
  case kTaskPanel:
    return m_titleT;
  case kAnniversaryPanel:
    return m_titleAn;
  default:
    return nullptr;
  }
}

QDateTimeEdit* ActivityFormPage::dateOf(int panel) const {
  switch (panel) {
  case kEventPanel:  // data e ora sono in DUE slot separati
    return nullptr;
  case kMeetingPanel:
    return m_startMt;
  case kTaskPanel:
    return m_dueT;
  case kAnniversaryPanel:
  default:
    return nullptr;
  }
}

QTimeEdit* ActivityFormPage::durationOf(int panel) const {
  switch (panel) {
  case kEventPanel:
    return m_durationE;
  case kMeetingPanel:
    return m_durationMt;
  case kTaskPanel:
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

  // Data: il pannello Evento usa due slot separati (Data + Ora)
  if (fromPanel == kEventPanel && toPanel == kEventPanel) {
    // niente da fare
  } else if (fromPanel == kEventPanel) {
    if (QDateTimeEdit* toDate = dateOf(toPanel)) {
      toDate->setDateTime(
          QDateTime(m_startDateE->date(), m_startTimeE->time()));
    }
  } else if (toPanel == kEventPanel) {
    if (QDateTimeEdit* fromDate = dateOf(fromPanel)) {
      m_startDateE->setDate(fromDate->date());
      m_startTimeE->setTime(fromDate->time());
    }
  } else {
    if (QDateTimeEdit* fromDate = dateOf(fromPanel)) {
      if (QDateTimeEdit* toDate = dateOf(toPanel)) {
        toDate->setDateTime(fromDate->dateTime());
      }
    }
  }

  if (QTimeEdit* fromDur = durationOf(fromPanel)) {
    if (QTimeEdit* toDur = durationOf(toPanel)) {
      toDur->setTime(fromDur->time());
    }
  }
}

void ActivityFormPage::onRepeatToggled(bool checked) {
  m_repeatBox->setVisible(checked);
  // Alla prima attivazione con unita' settimane, preseleziona il giorno
  // dell'inizio se non ne e' selezionato nessuno
  if (checked && m_unitCombo->currentIndex() == kUnitWeeks) {
    bool any = false;
    for (auto* button : m_dayButtons) {
      if (button->isChecked()) {
        any = true;
        break;
      }
    }
    if (!any) {
      m_dayButtons[m_startDateE->date().dayOfWeek() - 1]->setChecked(true);
    }
  }
  emitPreview();
}

void ActivityFormPage::onUnitChanged(int index) {
  const bool weeks = index == kUnitWeeks;
  const bool years = index == kUnitYears;
  m_dayRow->setVisible(weeks);
  m_everySpin->setEnabled(!years);
  switch (index) {
  case kUnitDays:
    m_everySpin->setSuffix(tr(" giorni"));
    break;
  case kUnitWeeks:
    m_everySpin->setSuffix(tr(" settimane"));
    break;
  case kUnitMonths:
    m_everySpin->setSuffix(tr(" mesi"));
    break;
  default:
    m_everySpin->setSuffix(tr(" anno"));
    break;
  }
  if (weeks) {
    bool any = false;
    for (auto* button : m_dayButtons) {
      if (button->isChecked()) {
        any = true;
        break;
      }
    }
    if (!any && m_startDateE) {
      m_dayButtons[m_startDateE->date().dayOfWeek() - 1]->setChecked(true);
    }
  }
}

void ActivityFormPage::startCreate(const QDateTime& suggestedStart) {
  startCreateType(kEventPanel, suggestedStart);
}

void ActivityFormPage::startCreateType(int typeIndex,
                                       const QDateTime& suggestedStart) {
  m_mode = Mode::Create;
  m_editingActivity = nullptr;
  m_editingOccurrence.reset();
  m_errorLabel->clear();

  // Box del titolo vuota (nessun testo residuo da creazioni precedenti)
  m_titleE->clear();
  m_titleMt->clear();
  m_titleT->clear();
  m_titleAn->clear();
  m_locationMt->clear();
  m_attendeeEdit->clear();
  m_attendeesList->clear();
  m_durationE->setTime(QTime(1, 0));
  m_durationMt->setTime(QTime(1, 0));
  m_priorityCombo->setCurrentIndex(1);
  m_doneCheck->setChecked(false);

  // Reset della ricorrenza "a domande"
  m_allDayCheck->setChecked(false);
  m_allDayCheck->setEnabled(true);
  m_repeatCheck->setChecked(false);
  m_repeatCheck->setEnabled(true);
  m_repeatBox->setVisible(false);
  m_unitCombo->setCurrentIndex(kUnitDays);
  m_everySpin->setValue(1);
  for (auto* button : m_dayButtons) {
    button->setChecked(false);
  }
  m_endNever->setChecked(true);
  m_countSpin->setValue(5);
  m_deleteButton->setVisible(false);

  // Data/ora suggerita (doppio clic su una cella): precompila ogni pannello
  const QDateTime value = suggestedStart.isValid()
                              ? suggestedStart
                              : QDateTime::currentDateTime();
  m_startDateE->setDate(value.date());
  m_startTimeE->setTime(value.time());
  m_startMt->setDateTime(value);
  m_dueT->setDateTime(value);
  m_dateAn->setDate(value.date());
  m_endDate->setDate(value.date());

  m_typeCombo->setEnabled(true);
  m_typeCombo->setCurrentIndex(qBound(0, typeIndex, kPanelCount - 1));
  m_doneCheck->setEnabled(true);
  m_saveButton->setText(tr("Salva"));
  m_forms->setCurrentIndex(qBound(0, typeIndex, kPanelCount - 1));
  emitPreview();
}

void ActivityFormPage::startEditActivity(const events::Activity* activity) {
  m_mode = Mode::EditActivity;
  m_editingActivity = activity;
  m_editingOccurrence.reset();
  m_errorLabel->clear();

  // Il tipo dinamico e' Activity/Task/Meeting; la ricorrenza si deduce dal
  // generatore. Un anniversario e' un'Activity annuale "tutto il giorno".
  if (auto* task = dynamic_cast<const events::Task*>(activity)) {
    populateTask(*task);
  } else if (auto* meeting = dynamic_cast<const events::Meeting*>(activity)) {
    populateMeeting(*meeting);
  } else if (isAnniversary(activity)) {
    populateAnniversary(*activity);
  } else {
    populateEventLike(*activity);
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

  // L'istanza singola diventa un Evento: niente ricorrenza, niente all-day
  m_allDayCheck->setChecked(false);
  m_repeatCheck->setChecked(false);
  m_repeatBox->setVisible(false);
  m_typeCombo->setEnabled(false);
  m_typeCombo->setCurrentIndex(kEventPanel);
  m_titleE->setText(QString::fromStdString(occurrence.source->getTitle()));
  const QDateTime occStart = toLocal(occurrence.start);
  m_startDateE->setDate(occStart.date());
  m_startTimeE->setTime(occStart.time());
  m_durationE->setTime(QTime(0, 0).addSecs(
      static_cast<int>(occurrence.duration.count())));
  m_saveButton->setText(tr("Salva"));
  m_forms->setCurrentIndex(kEventPanel);
  m_deleteButton->setVisible(true);
  emitPreview();
}

void ActivityFormPage::populateEventLike(const events::Activity& activity) {
  m_titleE->setText(QString::fromStdString(activity.getTitle()));
  const QDateTime start = toLocal(activity.getStart());
  m_startDateE->setDate(start.date());
  m_startTimeE->setTime(start.time());
  m_durationE->setTime(QTime(0, 0).addSecs(
      static_cast<int>(activity.getDuration().count())));
  // "Tutto il giorno" se copre un giorno intero. La durata e' l'unico
  // criterio affidabile: l'evento all-day e' salvato a mezzanotte UTC, quindi
  // il suo inizio in ORA LOCALE non e' necessariamente alle 00:00.
  const bool allDay = activity.getDuration().count() >= 86399;
  m_allDayCheck->setChecked(allDay);
  const bool recurrent = isRecurrent(&activity);
  m_repeatCheck->setChecked(recurrent);
  m_repeatBox->setVisible(recurrent);

  for (auto* button : m_dayButtons) {
    button->setChecked(false);
  }
  const int startDow = m_startDateE->date().dayOfWeek();

  // La fine della serie vive sull'Activity (i generatori sono stateless e
  // non hanno piu' un proprio "end"). Il limite "dopo N occorrenze" non e'
  // rappresentabile nel modello attuale (rimosso col MaxOccurrencesDecorator):
  // in modifica una serie limitata si vede sempre come "Fino al" [end].
  const events::TimePoint end = activity.getEnd();
  const events::DateGenerator* gen = &activity.getGenerator();
  if (const auto* fixed =
          dynamic_cast<const events::FixedIntervalGenerator*>(gen)) {
    const qint64 intervalDays = fixed->getInterval().count() / 86400;
    if (intervalDays % 7 == 0) {
      m_unitCombo->setCurrentIndex(kUnitWeeks);
      m_everySpin->setValue(static_cast<int>(intervalDays / 7));
      m_dayButtons[startDow - 1]->setChecked(true);
    } else {
      m_unitCombo->setCurrentIndex(kUnitDays);
      m_everySpin->setValue(static_cast<int>(intervalDays));
    }
  } else if (const auto* monthly =
                 dynamic_cast<const events::MonthlyGenerator*>(gen)) {
    m_unitCombo->setCurrentIndex(kUnitMonths);
    m_everySpin->setValue(monthly->getMonths());
  } else if (dynamic_cast<const events::YearlyGenerator*>(gen)) {
    m_unitCombo->setCurrentIndex(kUnitYears);
    m_everySpin->setValue(1);
  }
  onUnitChanged(m_unitCombo->currentIndex());

  if (end != events::TimePoint::max()) {
    m_endDateRadio->setChecked(true);
    m_endDate->setDate(toLocal(end).date());
  } else {
    m_endNever->setChecked(true);
  }

  m_typeCombo->setCurrentIndex(kEventPanel);
  m_forms->setCurrentIndex(kEventPanel);
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

void ActivityFormPage::populateAnniversary(const events::Activity& activity) {
  m_titleAn->setText(QString::fromStdString(activity.getTitle()));
  m_dateAn->setDate(toLocal(activity.getStart()).date());
  m_typeCombo->setCurrentIndex(kAnniversaryPanel);
  m_forms->setCurrentIndex(kAnniversaryPanel);
}

void ActivityFormPage::emitPreview() {
  const int panel = m_forms->currentIndex();
  const QString title = titleOf(panel)->text();
  // Niente anteprima per i tipi senza data/ora nella griglia (l'all-day va
  // nella striscia in alto, non nella griglia oraria)
  if (panel == kAnniversaryPanel ||
      (panel == kEventPanel && m_allDayCheck->isChecked())) {
    emit previewChanged(title, QDateTime(), 0, false);
    return;
  }
  const QDateTime start =
      panel == kEventPanel
          ? QDateTime(m_startDateE->date(), m_startTimeE->time())
          : (dateOf(panel) ? dateOf(panel)->dateTime() : QDateTime());
  qint64 durationSeconds = 0;
  if (QTimeEdit* dur = durationOf(panel)) {
    durationSeconds = dur->time().msecsSinceStartOfDay() / 1000;
  }
  emit previewChanged(title, start, durationSeconds, true);
}

std::vector<std::unique_ptr<events::Activity>>
ActivityFormPage::buildEventActivities() const {
  std::vector<std::unique_ptr<events::Activity>> result;
  const QString title = m_titleE->text().trimmed();
  if (title.isEmpty()) {
    return result;
  }
  const bool allDay = m_allDayCheck->isChecked();
  // "Tutto il giorno": inizio a mezzanotte UTC (coerente con le query della
  // griglia, che usano UTC) per non far slittare il giorno: in locale 00:00
  // di Lun = Dom 22:00 UTC, che cadrebbe nel giorno/settimana precedente.
  const events::TimePoint start =
      allDay ? toTimePoint(
                   QDateTime(m_startDateE->date(), QTime(0, 0), QTimeZone(0)))
             : toTimePoint(
                   QDateTime(m_startDateE->date(), m_startTimeE->time()));
  const events::Duration duration = allDay
                                        ? std::chrono::seconds(86399)
                                        : std::chrono::seconds(
                                              m_durationE->time().msecsSinceStartOfDay() /
                                              1000);

  // "Tutto il giorno" SENZA ripetizione -> un Event dalle 00:00 di 24h
  if (allDay && !m_repeatCheck->isChecked()) {
    // Evento che parte alle 00:00 e dura 24h (fino alle 00:00 del giorno
    // dopo): la striscia in alto lo riconosce perche' copre un giorno intero.
    result.push_back(events::makeActivity(events::ActivityConfig{
        .title = title.toStdString(),
        .start = start,
        .duration = std::chrono::seconds(86400)}));
    return result;
  }

  // Niente ripetizione -> un semplice Event
  if (!m_repeatCheck->isChecked()) {
    result.push_back(events::makeActivity(events::ActivityConfig{
        .title = title.toStdString(), .start = start, .duration = duration}));
    return result;
  }

  // Fine della ricorrenza scelta dall'utente ("Mai" -> resta max())
  events::TimePoint end = events::TimePoint::max();
  if (m_endDateRadio->isChecked()) {
    end = toTimePoint(QDateTime(m_endDate->date().addDays(1), QTime(0, 0)).addSecs(-1));
  }
  const int countLimit = m_endCountRadio->isChecked() ? m_countSpin->value() : 0;

  // Il modello non ha piu' un limite di conteggio nel generatore (rimosso
  // col MaxOccurrencesDecorator): "dopo N occorrenze" si traduce qui in un
  // `end` pari alla data dell'N-esima occorrenza, calcolata avanzando il
  // generatore stesso (align + next), cosi' il clamping di calendario
  // (fine mese, anni bisestili, ...) resta corretto.
  auto endAfterCount = [](const events::DateGenerator& generator,
                          events::TimePoint seriesStart, int count) {
    events::TimePoint current = generator.align(seriesStart, seriesStart);
    for (int i = 1; i < count && current != events::TimePoint::max(); ++i) {
      current = generator.next(current);
    }
    return current;
  };

  auto pushRecurrent = [&](std::shared_ptr<const events::DateGenerator> generator,
                           events::TimePoint seriesStart,
                           events::TimePoint seriesEnd) {
    if (countLimit > 0) {
      seriesEnd = endAfterCount(*generator, seriesStart, countLimit);
    }
    result.push_back(events::makeActivity(events::ActivityConfig{
        .title = title.toStdString(),
        .start = seriesStart,
        .duration = duration,
        .end = seriesEnd,
        .generator = std::move(generator)}));
  };

  const int unit = m_unitCombo->currentIndex();
  const int every = m_everySpin->value();
  if (unit == kUnitDays) {
    pushRecurrent(std::make_shared<events::FixedIntervalGenerator>(
                      events::Duration(events::Days(every))),
                  start, end);
  } else if (unit == kUnitWeeks) {
    // Una o piu' serie ricorrenti, una per giorno della settimana scelto.
    // Il limite "dopo N occorrenze" vale sul CALENDARIO COMBINATO, non per
    // singola serie: es. lun+mar+mer con N=5 -> sett1 lun/mar/mer + sett2
    // lun/mar = 5 eventi totali. La fine e' quindi la data della N-esima
    // occorrenza complessiva (non e' rappresentabile avanzando un solo
    // generatore, quindi si calcola con l'aritmetica sui giorni scelti).
    const int baseDow = m_startDateE->date().dayOfWeek();
    const QDate startDate = m_startDateE->date();
    const QTime time = allDay ? QTime(0, 0) : m_startTimeE->time();

    // Giorni selezionati (fallback: il giorno dell'inizio)
    QList<int> selected;
    for (int dow = 1; dow <= 7; ++dow) {
      if (m_dayButtons[dow - 1]->isChecked()) {
        selected.append(dow);
      }
    }
    if (selected.isEmpty()) {
      selected.append(baseDow);
    }

    // Fine: fino-a (data) / dopo-N (data della N-esima occorrenza combinata)
    events::TimePoint effectiveEnd = end;
    if (countLimit > 0) {
      const int n = countLimit;
      QList<int> offsets;
      for (int dow : selected) {
        offsets.append((dow - baseDow + 7) % 7);
      }
      std::sort(offsets.begin(), offsets.end());
      const int period = (n - 1) / offsets.size();
      const int idx = (n - 1) % offsets.size();
      const QDate nthDate =
          startDate.addDays(offsets[idx] + period * every * 7);
      effectiveEnd = toTimePoint(
          QDateTime(nthDate.addDays(1), QTime(0, 0)).addSecs(-1));
    }

    for (int dow : selected) {
      const int offset = (dow - baseDow + 7) % 7;
      // Anche le serie settimanali "tutto il giorno" partono a mezzanotte
      // UTC (come l'evento singolo), per la stessa ragione di allineamento.
      const events::TimePoint anchor =
          allDay ? toTimePoint(QDateTime(startDate.addDays(offset), QTime(0, 0),
                                         QTimeZone(0)))
                 : toTimePoint(QDateTime(startDate.addDays(offset), time));
      result.push_back(events::makeActivity(events::ActivityConfig{
          .title = title.toStdString(),
          .start = anchor,
          .duration = duration,
          .end = effectiveEnd,
          .generator = std::make_shared<events::FixedIntervalGenerator>(
              events::Duration(events::Days(7 * every)))}));
    }
  } else if (unit == kUnitMonths) {
    pushRecurrent(std::make_shared<events::MonthlyGenerator>(every), start, end);
  } else {  // anno ("ogni N" e' disabilitato in UI per questa unita': passo 1)
    pushRecurrent(std::make_shared<events::YearlyGenerator>(1), start, end);
  }
  return result;
}

std::unique_ptr<events::Activity> ActivityFormPage::buildActivity() const {
  const int panelIndex = m_forms->currentIndex();
  const QString title = titleOf(panelIndex)->text().trimmed();
  if (title.isEmpty()) {
    return nullptr;
  }

  switch (panelIndex) {
  case kMeetingPanel: {
    const events::Duration duration = std::chrono::seconds(
        m_durationMt->time().msecsSinceStartOfDay() / 1000);
    auto meeting = events::makeMeeting(events::MeetingConfig(
        events::ActivityConfig{.title = title.toStdString(),
                               .start = toTimePoint(m_startMt->dateTime()),
                               .duration = duration},
        m_locationMt->text().trimmed().toStdString()));
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
    auto task = events::makeTask(events::TaskConfig(
        events::ActivityConfig{.title = title.toStdString(),
                               .start = toTimePoint(m_dueT->dateTime())},
        priority));
    if (m_doneCheck->isEnabled()) {
      task->setDone(m_doneCheck->isChecked());
    }
    return task;
  }

  case kAnniversaryPanel: {
    const events::TimePoint date =
        toTimePoint(QDateTime(m_dateAn->date(), QTime(0, 0)));
    return events::makeActivity(events::ActivityConfig{
        .title = title.toStdString(),
        .start = date,
        .duration = std::chrono::hours(24) - std::chrono::seconds(1),
        .generator = std::make_shared<events::YearlyGenerator>(1)});
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

void ActivityFormPage::onSave() {
  // Pannello Evento "a domande": attivita' singola o una o piu' serie
  if (m_forms->currentIndex() == kEventPanel) {
    auto activities = buildEventActivities();
    if (activities.empty()) {
      m_errorLabel->setText(tr("Inserire un titolo non vuoto."));
      return;
    }
    bool ok = false;
    switch (m_mode) {
    case Mode::Create:
      ok = m_controller->addActivities(std::move(activities));
      break;
    case Mode::EditActivity:
      ok = m_controller->updateActivity(m_editingActivity,
                                        std::move(activities[0]));
      break;
    case Mode::EditOccurrence:
      ok = m_controller->modifyOccurrence(*m_editingOccurrence,
                                          std::move(activities[0]));
      break;
    }
    if (ok) {
      m_errorLabel->clear();
      emit backRequested();
    } else {
      m_errorLabel->setText(tr("Operazione non riuscita."));
    }
    return;
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
  case Mode::EditOccurrence:
    ok = m_controller->modifyOccurrence(*m_editingOccurrence,
                                        std::move(activity));
    break;
  }

  if (ok) {
    m_errorLabel->clear();
    emit backRequested();
  } else {
    m_errorLabel->setText(tr("Operazione non riuscita."));
  }
}

} // namespace app