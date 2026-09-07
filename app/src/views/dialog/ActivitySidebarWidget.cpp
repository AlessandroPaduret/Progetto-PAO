#include "views/dialog/ActivitySidebarWidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStringListModel>
#include <QTimeZone>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <memory>

#include "controller/CalendarController.h"
#include "builders/ActivityConfig.h"
#include "domain/Meeting.h"
#include "domain/Task.h"
#include "generators/FixedIntervalGenerator.h"
#include "generators/MonthlyGenerator.h"
#include "generators/YearlyGenerator.h"
#include "views/utils/ViewShared.h"

namespace app {

namespace {

// Indici del tipo (combo "Tipo" e sezioni rivelate sotto ai campi comuni)
constexpr int kEvent = 0;
constexpr int kMeeting = 1;
constexpr int kTask = 2;
constexpr int kTypeCount = 3;

// Unita' di ricorrenza (indici della combo)
constexpr int kUnitDays = 0;
constexpr int kUnitWeeks = 1;
constexpr int kUnitMonths = 2;
constexpr int kUnitYears = 3;

// Nomi brevi dei giorni della settimana: indice 0 = Lunedi', coerente con
// l'id 1..7 assegnato ai pulsanti di m_dayGroup (QDate::dayOfWeek()).
constexpr std::array<const char*, 7> kDayLabels = {
    "Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};

QLineEdit* makeTitle(QWidget* parent) {
    // Nessun placeholder: alla creazione la box deve essere semplicemente vuota
    return new QLineEdit(parent);
}

QDateTimeEdit* makeDateTime(QWidget* parent) {
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

// Durata in minuti: un QSpinBox unico e' piu' onesto di un QTimeEdit "HH:mm"
// (che non rappresenta correttamente durate >= 24h e lascia intendere un
// orario, non un intervallo). Range fino a 7 giorni: copre anche le riunioni
// o gli eventi multi-giorno inseriti a mano.
QSpinBox* makeDurationSpin(QWidget* parent, int defaultMinutes = 60) {
    auto* spin = new QSpinBox(parent);
    spin->setRange(5, 7 * 24 * 60);
    spin->setSingleStep(5);
    spin->setSuffix(QStringLiteral(" min"));
    spin->setValue(defaultMinutes);
    return spin;
}

} // namespace

ActivitySidebarWidget::ActivitySidebarWidget(CalendarController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    setMinimumWidth(340);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Evento"));
    m_typeCombo->addItem(tr("Riunione"));
    m_typeCombo->addItem(tr("Compito"));

    // Campi comuni a tutti i tipi: un solo set di widget, non uno per tipo
    // (Titolo/Data/Durata sono gli stessi campi che Activity possiede sempre).
    m_titleEdit = makeTitle(this);
    m_startEdit = makeDateTime(this);
    m_durationEdit = makeDurationSpin(this);

    auto* content = new QWidget(this);
    m_commonForm = new QFormLayout;
    m_commonForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_commonForm->addRow(tr("Titolo"), m_titleEdit);
    m_commonForm->addRow(tr("Data"), m_startEdit);
    m_commonForm->addRow(tr("Durata"), m_durationEdit);

    m_eventSection = buildEventSection();
    m_meetingSection = buildMeetingSection();
    m_taskSection = buildTaskSection();

    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->addLayout(m_commonForm);
    contentLayout->addWidget(m_eventSection);
    contentLayout->addWidget(m_meetingSection);
    contentLayout->addWidget(m_taskSection);
    contentLayout->addStretch(1);

    // Il contenuto sta in una scroll area: la sidebar puo' essere piu' stretta
    // di un dialog, il contenuto resta comunque raggiungibile.
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("formErrorLabel"));
    m_errorLabel->setWordWrap(true);

    auto* saveButton = new QPushButton(tr("Salva"), this);
    auto* cancelButton = new QPushButton(tr("Annulla"), this);
    // Elimina: visibile solo in modifica (in creazione non c'e' nulla da
    // eliminare); si nasconde con showCreate()/showCreateType()
    m_deleteButton = new QPushButton(tr("Elimina"), this);
    m_deleteButton->setVisible(false);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_deleteButton);
    buttons->addStretch(1);
    buttons->addWidget(saveButton);
    buttons->addWidget(cancelButton);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_typeCombo);
    layout->addWidget(scroll, 1);
    layout->addWidget(m_errorLabel);
    layout->addLayout(buttons);

    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ActivitySidebarWidget::onTypeChanged);
    // Ricorrenza (solo per il tipo Evento)
    connect(m_repeatCheck, &QCheckBox::toggled,
            this, &ActivitySidebarWidget::onRepeatToggled);
    connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ActivitySidebarWidget::onUnitChanged);
    connect(m_endDateRadio, &QRadioButton::toggled, this, [this](bool on) {
        m_endDate->setEnabled(on);
    });
    connect(m_endCountRadio, &QRadioButton::toggled, this, [this](bool on) {
        m_countSpin->setEnabled(on);
    });
    // "Tutto il giorno": l'ora sparisce cambiando il displayFormat dello
    // stesso QDateTimeEdit (niente widget separato da nascondere), e la riga
    // "Durata" si nasconde con QFormLayout::setRowVisible (l'attivita' dura
    // sempre 24h esatte). Si puo' combinare con "Si ripete": una serie che
    // ricorre a giornate intere.
    connect(m_allDayCheck, &QCheckBox::toggled, this, [this](bool on) {
        m_startEdit->setDisplayFormat(on ? QStringLiteral("dd/MM/yyyy")
                                          : QStringLiteral("dd/MM/yyyy HH:mm"));
        m_commonForm->setRowVisible(m_durationEdit, !on);
        emitPreview();
    });
    // Partecipanti della riunione
    connect(m_attendeeEdit, &QLineEdit::returnPressed,
            this, &ActivitySidebarWidget::onAddAttendee);
    connect(m_attendeesList, &QListWidget::itemDoubleClicked,
            this, &ActivitySidebarWidget::onRemoveAttendee);

    connect(saveButton, &QPushButton::clicked, this, &ActivitySidebarWidget::onSave);
    connect(m_deleteButton, &QPushButton::clicked, this, &ActivitySidebarWidget::onDelete);
    connect(cancelButton, &QPushButton::clicked, this, &ActivitySidebarWidget::closeRequested);

    // Anteprima live: ogni modifica dei campi comuni aggiorna la griglia
    // (un solo set di segnali, non uno per pannello: i campi sono condivisi)
    connect(m_titleEdit, &QLineEdit::textChanged, this, &ActivitySidebarWidget::emitPreview);
    connect(m_startEdit, &QDateTimeEdit::dateTimeChanged, this, &ActivitySidebarWidget::emitPreview);
    connect(m_durationEdit, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ActivitySidebarWidget::emitPreview);

    showSection(kEvent);
}

// ---------------------------------------------------------------------------
// Sezione Evento: "tutto il giorno" + ricorrenza
// ---------------------------------------------------------------------------
QWidget* ActivitySidebarWidget::buildEventSection() {
    auto* panel = new QWidget(this);
    m_allDayCheck = new QCheckBox(tr("Tutto il giorno"), panel);
    m_repeatCheck = new QCheckBox(tr("Si ripete"), panel);

    auto* checksRow = new QHBoxLayout;
    checksRow->addWidget(m_allDayCheck);
    checksRow->addWidget(m_repeatCheck);
    checksRow->addStretch(1);

    // --- Sotto-pannello di ricorrenza (visibile se "Si ripete") ------------
    m_repeatBox = new QWidget(panel);
    m_repeatBox->setVisible(false);

    m_unitCombo = new QComboBox(m_repeatBox);
    m_unitCombo->addItem(tr("giorni"));
    m_unitCombo->addItem(tr("settimane"));
    m_unitCombo->addItem(tr("mesi"));
    m_unitCombo->addItem(tr("anni"));

    m_everySpin = new QSpinBox(m_repeatBox);
    m_everySpin->setRange(1, 3650);
    m_everySpin->setValue(1);
    m_everySpin->setSuffix(tr(" giorni"));

    // Giorni della settimana: QButtonGroup NON esclusivo (piu' giorni
    // selezionabili insieme), id del pulsante = giorno Qt (1=Lun..7=Dom,
    // QDate::dayOfWeek()): elimina la necessita' di scandire a mano una
    // lista di pulsanti per sapere "quale" giorno rappresentano.
    m_dayRow = new QWidget(m_repeatBox);
    auto* dayLayout = new QHBoxLayout(m_dayRow);
    dayLayout->setContentsMargins(0, 0, 0, 0);
    m_dayGroup = new QButtonGroup(m_dayRow);
    m_dayGroup->setExclusive(false);
    for (int i = 0; i < 7; ++i) {
        auto* button = new QPushButton(QString::fromLatin1(kDayLabels[i]), m_dayRow);
        button->setCheckable(true);
        button->setMinimumWidth(36);
        dayLayout->addWidget(button);
        m_dayGroup->addButton(button, i + 1);
    }

    // Fine della ricorrenza: mai / fino a / dopo N occorrenze. Ogni radio
    // fa da "etichetta" della propria riga (QFormLayout::addRow accetta
    // qualunque QWidget come label, non solo QLabel).
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
    auto* endForm = new QFormLayout(endGroup);
    endForm->addRow(m_endNever);
    endForm->addRow(m_endDateRadio, m_endDate);
    endForm->addRow(m_endCountRadio, m_countSpin);

    m_repeatForm = new QFormLayout(m_repeatBox);
    m_repeatForm->setContentsMargins(0, 0, 0, 0);
    m_repeatForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_repeatForm->addRow(tr("Unita'"), m_unitCombo);
    m_repeatForm->addRow(tr("Ogni"), m_everySpin);
    m_repeatForm->addRow(tr("Giorni"), m_dayRow);
    m_repeatForm->addRow(endGroup);

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(checksRow);
    layout->addWidget(m_repeatBox);
    return panel;
}

QWidget* ActivitySidebarWidget::buildMeetingSection() {
    auto* panel = new QWidget(this);
    m_locationEdit = new QLineEdit(panel);
    m_locationEdit->setPlaceholderText(tr("Aula, sede o link"));

    m_attendeeEdit = new QLineEdit(panel);
    m_attendeeEdit->setPlaceholderText(tr("Nome partecipante + Invio"));
    // Suggerisce nomi gia' usati in altre Riunioni del calendario corrente
    // (aggiornato in showCreateType/showEditActivity, vedi
    // refreshAttendeeCompleter): il modello e' sostituito li', qui si crea
    // solo il completer vuoto e lo si aggancia al campo.
    m_attendeeCompleter = new QCompleter(this);
    m_attendeeCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_attendeeEdit->setCompleter(m_attendeeCompleter);

    m_attendeesList = new QListWidget(panel);
    m_attendeesList->setMaximumHeight(110);

    auto* form = new QFormLayout(panel);
    form->setContentsMargins(0, 0, 0, 0);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->addRow(tr("Luogo"), m_locationEdit);
    form->addRow(tr("Aggiungi"), m_attendeeEdit);
    form->addRow(tr("Partecipanti"), m_attendeesList);
    return panel;
}

QWidget* ActivitySidebarWidget::buildTaskSection() {
    auto* panel = new QWidget(this);
    m_priorityCombo = new QComboBox(panel);
    m_priorityCombo->addItem(tr("Bassa"));
    m_priorityCombo->addItem(tr("Media"));
    m_priorityCombo->addItem(tr("Alta"));
    m_priorityCombo->setCurrentIndex(1);
    m_doneCheck = new QCheckBox(tr("Evaso"), panel);

    auto* form = new QFormLayout(panel);
    form->setContentsMargins(0, 0, 0, 0);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->addRow(tr("Priorita'"), m_priorityCombo);
    form->addRow(m_doneCheck);
    return panel;
}

void ActivitySidebarWidget::showSection(int type) {
    m_eventSection->setVisible(type == kEvent);
    m_meetingSection->setVisible(type == kMeeting);
    m_taskSection->setVisible(type == kTask);
}

events::TimePoint ActivitySidebarWidget::toTimePoint(const QDateTime& local) {
  return events::TimePoint(std::chrono::seconds(local.toSecsSinceEpoch()));
}

QDateTime ActivitySidebarWidget::toLocal(const events::TimePoint tp) {
  return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count()).toLocalTime();
}

void ActivitySidebarWidget::onTypeChanged(int index) {
  showSection(index);
  emitPreview();
}

void ActivitySidebarWidget::onRepeatToggled(bool checked) {
  m_repeatBox->setVisible(checked);
  // Alla prima attivazione con unita' settimane, preseleziona il giorno
  // dell'inizio se non ne e' selezionato nessuno.
  if (checked && m_unitCombo->currentIndex() == kUnitWeeks &&
      !std::ranges::any_of(m_dayGroup->buttons(), &QAbstractButton::isChecked)) {
    m_dayGroup->button(m_startEdit->date().dayOfWeek())->setChecked(true);
  }
  emitPreview();
}

void ActivitySidebarWidget::onUnitChanged(int index) {
  const bool weeks = index == kUnitWeeks;
  m_repeatForm->setRowVisible(m_dayRow, weeks);
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
    m_everySpin->setSuffix(tr(" anni"));
    break;
  }
  if (weeks && !std::ranges::any_of(m_dayGroup->buttons(), &QAbstractButton::isChecked)) {
    m_dayGroup->button(m_startEdit->date().dayOfWeek())->setChecked(true);
  }
}

void ActivitySidebarWidget::showCreate(const QDateTime& suggestedStart) {
  showCreateType(kEvent, suggestedStart);
}

void ActivitySidebarWidget::showCreateType(int typeIndex,
                                           const QDateTime& suggestedStart) {
  m_mode = Mode::Create;
  m_editingActivity = nullptr;
  m_editingOccurrence.reset();
  m_errorLabel->clear();

  m_titleEdit->clear();
  m_locationEdit->clear();
  m_attendeeEdit->clear();
  m_attendeesList->clear();
  m_durationEdit->setValue(60);
  m_priorityCombo->setCurrentIndex(1);
  m_doneCheck->setChecked(false);
  m_doneCheck->setEnabled(true);

  // Reset della ricorrenza
  m_allDayCheck->setChecked(false);
  m_allDayCheck->setEnabled(true);
  // Esplicito (non solo affidato al segnale "toggled", che non scatta se il
  // valore precedente era gia' false): la riga Durata torna visibile e il
  // formato torna a includere l'ora, qualunque fosse lo stato precedente.
  m_startEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
  m_commonForm->setRowVisible(m_durationEdit, true);
  m_repeatCheck->setChecked(false);
  m_repeatCheck->setEnabled(true);
  m_repeatBox->setVisible(false);
  m_unitCombo->setCurrentIndex(kUnitDays);
  m_everySpin->setValue(1);
  for (QAbstractButton* button : m_dayGroup->buttons()) {
    button->setChecked(false);
  }
  m_endNever->setChecked(true);
  m_countSpin->setValue(5);
  m_deleteButton->setVisible(false);

  // Data/ora suggerita (doppio clic su una cella)
  const QDateTime value = suggestedStart.isValid()
                              ? suggestedStart
                              : QDateTime::currentDateTime();
  m_startEdit->setDateTime(value);
  m_endDate->setDate(value.date());

  m_typeCombo->setEnabled(true);
  const int clamped = std::clamp(typeIndex, 0, kTypeCount - 1);
  m_typeCombo->setCurrentIndex(clamped);
  // Esplicito: se il tipo non e' cambiato rispetto a prima, currentIndexChanged
  // non scatta e la sezione mostrata resterebbe quella dell'uso precedente.
  showSection(clamped);
  refreshAttendeeCompleter();
  emitPreview();
}

void ActivitySidebarWidget::showEditActivity(const events::Activity* activity) {
  m_mode = Mode::EditActivity;
  m_editingActivity = activity;
  m_editingOccurrence.reset();
  m_errorLabel->clear();
  refreshAttendeeCompleter();

  // Il tipo dinamico e' Activity/Task/Meeting; la ricorrenza si deduce dal
  // generatore.
  if (auto* task = dynamic_cast<const events::Task*>(activity)) {
    populateTask(*task);
  } else if (auto* meeting = dynamic_cast<const events::Meeting*>(activity)) {
    populateMeeting(*meeting);
  } else {
    populateEventLike(*activity);
  }
  m_typeCombo->setEnabled(false);
  m_doneCheck->setEnabled(true);
  m_deleteButton->setVisible(true);
  emitPreview();
}

void ActivitySidebarWidget::showEditOccurrence(const events::Occurrence& occurrence) {
  m_mode = Mode::EditOccurrence;
  m_editingActivity = nullptr;
  m_editingOccurrence = occurrence;
  m_errorLabel->clear();

  // L'istanza singola diventa un Evento: niente ricorrenza, niente all-day
  m_allDayCheck->setChecked(false);
  m_startEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
  m_commonForm->setRowVisible(m_durationEdit, true);
  m_repeatCheck->setChecked(false);
  m_repeatBox->setVisible(false);
  m_typeCombo->setEnabled(false);
  m_typeCombo->setCurrentIndex(kEvent);
  showSection(kEvent);
  m_titleEdit->setText(QString::fromStdString(occurrence.source->getTitle()));
  m_startEdit->setDateTime(toLocal(occurrence.start));
  m_durationEdit->setValue(std::max(1, static_cast<int>(occurrence.duration.count() / 60)));
  m_deleteButton->setVisible(true);
  emitPreview();
}

void ActivitySidebarWidget::populateEventLike(const events::Activity& activity) {
  m_titleEdit->setText(QString::fromStdString(activity.getTitle()));
  m_startEdit->setDateTime(toLocal(activity.getStart()));
  m_durationEdit->setValue(std::max(1, static_cast<int>(activity.getDuration().count() / 60)));
  // "Tutto il giorno" se copre un giorno intero. La durata e' l'unico
  // criterio affidabile: l'evento all-day e' salvato a mezzanotte UTC, quindi
  // il suo inizio in ORA LOCALE non e' necessariamente alle 00:00.
  const bool allDay = activity.getDuration().count() >= 86399;
  m_allDayCheck->setChecked(allDay);
  m_startEdit->setDisplayFormat(allDay ? QStringLiteral("dd/MM/yyyy")
                                       : QStringLiteral("dd/MM/yyyy HH:mm"));
  m_commonForm->setRowVisible(m_durationEdit, !allDay);
  const bool recurrent = isRecurrent(&activity);
  m_repeatCheck->setChecked(recurrent);
  m_repeatBox->setVisible(recurrent);

  for (QAbstractButton* button : m_dayGroup->buttons()) {
    button->setChecked(false);
  }
  const int startDow = m_startEdit->date().dayOfWeek();

  // La fine della serie vive sull'Activity (i generatori sono stateless e
  // non hanno un proprio "end"). Il limite "dopo N occorrenze" non e'
  // rappresentabile nel modello attuale: in modifica una serie limitata si
  // vede sempre come "Fino al" [end].
  const events::TimePoint end = activity.getEnd();
  const events::DateGenerator* gen = &activity.getGenerator();
  if (const auto* fixed =
          dynamic_cast<const events::FixedIntervalGenerator*>(gen)) {
    const qint64 intervalDays = fixed->getInterval().count() / 86400;
    if (intervalDays % 7 == 0) {
      m_unitCombo->setCurrentIndex(kUnitWeeks);
      m_everySpin->setValue(static_cast<int>(intervalDays / 7));
      m_dayGroup->button(startDow)->setChecked(true);
    } else {
      m_unitCombo->setCurrentIndex(kUnitDays);
      m_everySpin->setValue(static_cast<int>(intervalDays));
    }
  } else if (const auto* monthly =
                 dynamic_cast<const events::MonthlyGenerator*>(gen)) {
    m_unitCombo->setCurrentIndex(kUnitMonths);
    m_everySpin->setValue(monthly->getMonths());
  } else if (const auto* yearly =
                 dynamic_cast<const events::YearlyGenerator*>(gen)) {
    m_unitCombo->setCurrentIndex(kUnitYears);
    m_everySpin->setValue(yearly->getYears());
  }
  onUnitChanged(m_unitCombo->currentIndex());

  if (end != events::TimePoint::max()) {
    m_endDateRadio->setChecked(true);
    m_endDate->setDate(toLocal(end).date());
  } else {
    m_endNever->setChecked(true);
  }

  m_typeCombo->setCurrentIndex(kEvent);
  showSection(kEvent);
}

void ActivitySidebarWidget::populateMeeting(const events::Meeting& meeting) {
  m_titleEdit->setText(QString::fromStdString(meeting.getTitle()));
  m_startEdit->setDateTime(toLocal(meeting.getStart()));
  m_startEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
  m_commonForm->setRowVisible(m_durationEdit, true);
  m_durationEdit->setValue(std::max(1, static_cast<int>(meeting.getDuration().count() / 60)));
  m_locationEdit->setText(QString::fromStdString(meeting.getLocation()));
  m_attendeesList->clear();
  for (const auto& name : meeting.getAttendees()) {
    new QListWidgetItem(QString::fromStdString(name), m_attendeesList);
  }
  m_typeCombo->setCurrentIndex(kMeeting);
  showSection(kMeeting);
}

void ActivitySidebarWidget::populateTask(const events::Task& task) {
  m_titleEdit->setText(QString::fromStdString(task.getTitle()));
  m_startEdit->setDateTime(toLocal(task.getDue()));
  m_startEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
  m_commonForm->setRowVisible(m_durationEdit, true);
  m_durationEdit->setValue(std::max(1, static_cast<int>(task.getDuration().count() / 60)));
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
  m_typeCombo->setCurrentIndex(kTask);
  showSection(kTask);
}

void ActivitySidebarWidget::emitPreview() {
  const QString title = m_titleEdit->text();
  // Niente anteprima per l'Evento "tutto il giorno" (va nella striscia in
  // alto, non nella griglia oraria); tutti gli altri casi usano gli stessi
  // campi comuni (Data/Durata), quindi non serve piu' un dispatch per tipo.
  if (m_typeCombo->currentIndex() == kEvent && m_allDayCheck->isChecked()) {
    emit previewChanged(title, QDateTime(), 0, false);
    return;
  }
  const qint64 durationSeconds = static_cast<qint64>(m_durationEdit->value()) * 60;
  emit previewChanged(title, m_startEdit->dateTime(), durationSeconds, true);
}

void ActivitySidebarWidget::refreshAttendeeCompleter() {
  QStringList names;
  for (const auto& activity : m_controller->calendar()) {
    if (const auto* meeting = dynamic_cast<const events::Meeting*>(activity.get())) {
      for (const auto& name : meeting->getAttendees()) {
        names.append(QString::fromStdString(name));
      }
    }
  }
  std::ranges::sort(names);
  const auto duplicates = std::ranges::unique(names);
  names.erase(duplicates.begin(), duplicates.end());
  m_attendeeCompleter->setModel(new QStringListModel(names, m_attendeeCompleter));
}

std::vector<std::unique_ptr<events::Activity>>
ActivitySidebarWidget::buildEventActivities() const {
  std::vector<std::unique_ptr<events::Activity>> result;
  const QString title = m_titleEdit->text().trimmed();
  if (title.isEmpty()) {
    return result;
  }
  const bool allDay = m_allDayCheck->isChecked();
  // "Tutto il giorno": inizio a mezzanotte UTC (coerente con le query della
  // griglia, che usano UTC) per non far slittare il giorno: in locale 00:00
  // di Lun = Dom 22:00 UTC, che cadrebbe nel giorno/settimana precedente.
  const events::TimePoint start =
      allDay ? toTimePoint(QDateTime(m_startEdit->date(), QTime(0, 0), QTimeZone(0)))
             : toTimePoint(m_startEdit->dateTime());
  const events::Duration duration = allDay
                                        ? std::chrono::seconds(86399)
                                        : std::chrono::minutes(m_durationEdit->value());

  // "Tutto il giorno" SENZA ripetizione -> un'attivita' dalle 00:00 di 24h
  if (allDay && !m_repeatCheck->isChecked()) {
    // Attivita' che parte alle 00:00 e dura 24h (fino alle 00:00 del giorno
    // dopo): la striscia in alto la riconosce perche' copre un giorno intero.
    result.push_back(events::makeActivity(events::ActivityConfig{
        .title = title.toStdString(),
        .start = start,
        .duration = std::chrono::seconds(86400)}));
    return result;
  }

  // Niente ripetizione -> una semplice attivita' singola
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

  // Il modello non ha un limite di conteggio nel generatore: "dopo N
  // occorrenze" si traduce qui in un `end` pari alla data dell'N-esima
  // occorrenza, calcolata avanzando il generatore stesso (align + next),
  // cosi' il clamping di calendario (fine mese, anni bisestili, ...) resta
  // corretto.
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
    const int baseDow = m_startEdit->date().dayOfWeek();
    const QDate startDate = m_startEdit->date();
    const QTime time = allDay ? QTime(0, 0) : m_startEdit->time();

    // Giorni selezionati (id del QButtonGroup = giorno Qt), fallback: il
    // giorno dell'inizio.
    QList<int> selected;
    for (QAbstractButton* button : m_dayGroup->buttons()) {
      if (button->isChecked()) {
        selected.append(m_dayGroup->id(button));
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
      std::ranges::sort(offsets);
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
  } else {  // anni
    pushRecurrent(std::make_shared<events::YearlyGenerator>(every), start, end);
  }
  return result;
}

std::unique_ptr<events::Activity> ActivitySidebarWidget::buildActivity() const {
  const QString title = m_titleEdit->text().trimmed();
  if (title.isEmpty()) {
    return nullptr;
  }
  const events::Duration duration = std::chrono::minutes(m_durationEdit->value());

  switch (m_typeCombo->currentIndex()) {
  case kMeeting: {
    auto meeting = events::makeMeeting(events::MeetingConfig(
        events::ActivityConfig{.title = title.toStdString(),
                               .start = toTimePoint(m_startEdit->dateTime()),
                               .duration = duration},
        m_locationEdit->text().trimmed().toStdString()));
    for (int i = 0; i < m_attendeesList->count(); ++i) {
      meeting->addAttendee(m_attendeesList->item(i)->text().toStdString());
    }
    return meeting;
  }

  case kTask: {
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
                               .start = toTimePoint(m_startEdit->dateTime()),
                               .duration = duration},
        priority));
    if (m_doneCheck->isEnabled()) {
      task->setDone(m_doneCheck->isChecked());
    }
    return task;
  }

  default:
    return nullptr;
  }
}

void ActivitySidebarWidget::onAddAttendee() {
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

void ActivitySidebarWidget::onRemoveAttendee() {
  if (QListWidgetItem* item = m_attendeesList->currentItem()) {
    delete item;
  }
}

void ActivitySidebarWidget::onDelete() {
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
    emit closeRequested();
  }
}

void ActivitySidebarWidget::onSave() {
  // Tipo Evento: attivita' singola o una o piu' serie
  if (m_typeCombo->currentIndex() == kEvent) {
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
      emit closeRequested();
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
    emit closeRequested();
  } else {
    m_errorLabel->setText(tr("Operazione non riuscita."));
  }
}

} // namespace app
