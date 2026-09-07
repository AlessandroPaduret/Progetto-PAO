#include "views/dialog/ActivitySidebarWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTimeZone>
#include <QVBoxLayout>

#include <algorithm>
#include <memory>

#include "controller/CalendarController.h"
#include "builders/ActivityConfig.h"
#include "domain/Meeting.h"
#include "domain/Task.h"
#include "generators/FixedIntervalGenerator.h"
#include "generators/MonthlyGenerator.h"
#include "generators/YearlyGenerator.h"
#include "views/dialog/MeetingFormWidget.h"
#include "views/dialog/RecurrenceFormWidget.h"
#include "views/dialog/TaskFormWidget.h"
#include "views/utils/ViewShared.h"

namespace app {

namespace {

// Indici del tipo (combo "Tipo" e sezione rivelata sotto alla ricorrenza)
constexpr int kEvent = 0;
constexpr int kMeeting = 1;
constexpr int kTask = 2;
constexpr int kTypeCount = 3;

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

// -----------------------------------------------------------------------
// Comportamento di Salva/Elimina per modalita' del pannello (Polimorfismo
// al posto di un enum "Mode" + switch/if sparsi in onSave()/onDelete()):
// una sottoclasse per Create/EditActivity/EditOccurrence, ciascuna sa da
// sola quale metodo del controller chiamare per salvare e (se supportata)
// per eliminare l'elemento in modifica.
// -----------------------------------------------------------------------
class FormSaveStrategy {
public:
    virtual ~FormSaveStrategy() = default;

    virtual bool execute(CalendarController* controller,
                         std::vector<std::unique_ptr<events::Activity>> activities) = 0;

    /** @brief Elimina l'elemento in modifica; il default "non supportato"
     *  copre la creazione, dove non c'e' ancora nulla da eliminare. */
    virtual bool remove(CalendarController* /*controller*/) { return false; }

    /** @brief Oggetto del messaggio di conferma ("Eliminare l'attivita'?"/
     *  "Eliminare questa occorrenza?"); vuoto se l'eliminazione non e'
     *  supportata (il chiamante nasconde/disabilita il pulsante Elimina). */
    virtual QString deleteSubject() const { return {}; }
};

namespace {

class CreateStrategy : public FormSaveStrategy {
public:
    bool execute(CalendarController* controller,
                 std::vector<std::unique_ptr<events::Activity>> activities) override {
        return activities.size() > 1 ? controller->addActivities(std::move(activities))
                                      : controller->addActivity(std::move(activities[0]));
    }
};

class EditActivityStrategy : public FormSaveStrategy {
public:
    explicit EditActivityStrategy(const events::Activity* activity) : m_activity(activity) {}

    bool execute(CalendarController* controller,
                 std::vector<std::unique_ptr<events::Activity>> activities) override {
        return controller->updateActivity(m_activity, std::move(activities[0]));
    }
    bool remove(CalendarController* controller) override {
        return controller->removeActivity(m_activity);
    }
    QString deleteSubject() const override {
        return ActivitySidebarWidget::tr("l'attivita'");
    }

private:
    const events::Activity* m_activity;
};

class EditOccurrenceStrategy : public FormSaveStrategy {
public:
    explicit EditOccurrenceStrategy(events::Occurrence occurrence) : m_occurrence(occurrence) {}

    bool execute(CalendarController* controller,
                 std::vector<std::unique_ptr<events::Activity>> activities) override {
        return controller->modifyOccurrence(m_occurrence, std::move(activities[0]));
    }
    bool remove(CalendarController* controller) override {
        return controller->deleteOccurrence(m_occurrence);
    }
    QString deleteSubject() const override {
        return ActivitySidebarWidget::tr("questa occorrenza");
    }

private:
    events::Occurrence m_occurrence;
};

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

    // Sezioni delegate ai widget figli: la ricorrenza e' sempre visibile
    // (comune ai 3 tipi), Riunione/Compito si alternano in showSection().
    m_recurrence = new RecurrenceFormWidget(this);
    m_meetingSection = new MeetingFormWidget(this);
    m_taskSection = new TaskFormWidget(this);

    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->addLayout(m_commonForm);
    contentLayout->addWidget(m_recurrence);
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

    // Ricorrenza (comune a Evento/Riunione/Compito): il widget figlio gestisce
    // da solo la propria UI interna, qui si reagisce solo a "Tutto il giorno"
    // (che tocca campi comuni esterni al widget: Data/Durata) e all'anteprima.
    connect(m_recurrence, &RecurrenceFormWidget::allDayToggled,
            this, &ActivitySidebarWidget::onAllDayToggled);
    connect(m_recurrence, &RecurrenceFormWidget::changed,
            this, &ActivitySidebarWidget::emitPreview);

    connect(saveButton, &QPushButton::clicked, this, &ActivitySidebarWidget::onSave);
    connect(m_deleteButton, &QPushButton::clicked, this, &ActivitySidebarWidget::onDelete);
    connect(cancelButton, &QPushButton::clicked, this, &ActivitySidebarWidget::closeRequested);

    // Anteprima live: ogni modifica dei campi comuni aggiorna la griglia
    // (un solo set di segnali, non uno per pannello: i campi sono condivisi)
    connect(m_titleEdit, &QLineEdit::textChanged, this, &ActivitySidebarWidget::emitPreview);
    connect(m_startEdit, &QDateTimeEdit::dateTimeChanged, this, &ActivitySidebarWidget::emitPreview);
    connect(m_startEdit, &QDateTimeEdit::dateTimeChanged, this, [this](const QDateTime& dt) {
        m_recurrence->setReferenceDate(dt.date());
    });
    connect(m_durationEdit, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ActivitySidebarWidget::emitPreview);

    showSection(kEvent);
}

// Out-of-line: FormSaveStrategy e' un tipo incompleto nell'header (solo
// dichiarato), std::unique_ptr<FormSaveStrategy> ne richiede la definizione
// completa per generare il distruttore di m_saveStrategy, disponibile qui.
ActivitySidebarWidget::~ActivitySidebarWidget() = default;

void ActivitySidebarWidget::showSection(int type) {
    // L'Evento non ha una propria sezione specifica: la ricorrenza (comune
    // a tutti e tre i tipi) e' gia' sempre visibile fuori da questo switch.
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

void ActivitySidebarWidget::onAllDayToggled(bool on) {
  // "Tutto il giorno": l'ora sparisce cambiando il displayFormat dello
  // stesso QDateTimeEdit (niente widget separato da nascondere), e la riga
  // "Durata" si nasconde con QFormLayout::setRowVisible (l'attivita' dura
  // sempre 24h esatte). Si puo' combinare con "Si ripete": una serie che
  // ricorre a giornate intere.
  m_startEdit->setDisplayFormat(on ? QStringLiteral("dd/MM/yyyy")
                                    : QStringLiteral("dd/MM/yyyy HH:mm"));
  m_commonForm->setRowVisible(m_durationEdit, !on);
  emitPreview();
}

void ActivitySidebarWidget::showCreate(const QDateTime& suggestedStart) {
  showCreateType(kEvent, suggestedStart);
}

void ActivitySidebarWidget::showCreateType(int typeIndex,
                                           const QDateTime& suggestedStart) {
  m_saveStrategy = std::make_unique<CreateStrategy>();
  m_errorLabel->clear();

  m_titleEdit->clear();
  m_meetingSection->clear();
  m_durationEdit->setValue(60);
  m_taskSection->resetToDefaults();

  // Reset della ricorrenza. Esplicito (non solo affidato al segnale
  // "toggled" di RecurrenceFormWidget, che non scatta se il valore precedente
  // era gia' false): la riga Durata torna visibile e il formato torna a
  // includere l'ora, qualunque fosse lo stato precedente.
  m_recurrence->resetToDefaults();
  m_startEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
  m_commonForm->setRowVisible(m_durationEdit, true);
  m_deleteButton->setVisible(false);

  // Data/ora suggerita (doppio clic su una cella)
  const QDateTime value = suggestedStart.isValid()
                              ? suggestedStart
                              : QDateTime::currentDateTime();
  m_startEdit->setDateTime(value);
  m_recurrence->setReferenceDate(value.date());

  m_typeCombo->setEnabled(true);
  const int clamped = std::clamp(typeIndex, 0, kTypeCount - 1);
  m_typeCombo->setCurrentIndex(clamped);
  // Esplicito: se il tipo non e' cambiato rispetto a prima, currentIndexChanged
  // non scatta e la sezione mostrata resterebbe quella dell'uso precedente.
  showSection(clamped);
  emitPreview();
}

void ActivitySidebarWidget::showEditActivity(const events::Activity* activity) {
  m_saveStrategy = std::make_unique<EditActivityStrategy>(activity);
  m_errorLabel->clear();

  // Il tipo dinamico e' Activity/Task/Meeting; la ricorrenza si deduce dal
  // generatore, allo stesso modo per tutti e tre i tipi.
  if (auto* task = dynamic_cast<const events::Task*>(activity)) {
    populateTask(*task);
  } else if (auto* meeting = dynamic_cast<const events::Meeting*>(activity)) {
    populateMeeting(*meeting);
  } else {
    populateEventLike(*activity);
  }
  m_typeCombo->setEnabled(false);
  m_taskSection->setDoneEnabled(true);
  m_deleteButton->setVisible(true);
  emitPreview();
}

void ActivitySidebarWidget::showEditOccurrence(const events::Occurrence& occurrence) {
  m_saveStrategy = std::make_unique<EditOccurrenceStrategy>(occurrence);
  m_errorLabel->clear();

  // L'istanza singola diventa un Evento: niente ricorrenza, niente all-day
  m_recurrence->setAllDay(false);
  m_startEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
  m_commonForm->setRowVisible(m_durationEdit, true);
  m_recurrence->setRepeating(false);
  m_typeCombo->setEnabled(false);
  m_typeCombo->setCurrentIndex(kEvent);
  showSection(kEvent);
  m_titleEdit->setText(QString::fromStdString(occurrence.source->getTitle()));
  m_startEdit->setDateTime(toLocal(occurrence.start));
  m_recurrence->setReferenceDate(m_startEdit->date());
  m_durationEdit->setValue(std::max(1, static_cast<int>(occurrence.duration.count() / 60)));
  m_deleteButton->setVisible(true);
  emitPreview();
}

void ActivitySidebarWidget::populateCommonAndRecurrence(const events::Activity& activity) {
  m_titleEdit->setText(QString::fromStdString(activity.getTitle()));
  m_startEdit->setDateTime(toLocal(activity.getStart()));
  m_recurrence->setReferenceDate(m_startEdit->date());
  m_durationEdit->setValue(std::max(1, static_cast<int>(activity.getDuration().count() / 60)));
  // "Tutto il giorno" se copre un giorno intero. La durata e' l'unico
  // criterio affidabile: l'evento all-day e' salvato a mezzanotte UTC, quindi
  // il suo inizio in ORA LOCALE non e' necessariamente alle 00:00.
  const bool allDay = activity.getDuration().count() >= 86399;
  m_recurrence->setAllDay(allDay);
  m_startEdit->setDisplayFormat(allDay ? QStringLiteral("dd/MM/yyyy")
                                       : QStringLiteral("dd/MM/yyyy HH:mm"));
  m_commonForm->setRowVisible(m_durationEdit, !allDay);
  const bool recurrent = isRecurrent(&activity);
  m_recurrence->setRepeating(recurrent);

  m_recurrence->setSelectedWeekdays({});
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
      m_recurrence->setUnit(RecurrenceFormWidget::Weeks);
      m_recurrence->setEvery(static_cast<int>(intervalDays / 7));
      m_recurrence->setSelectedWeekdays({startDow});
    } else {
      m_recurrence->setUnit(RecurrenceFormWidget::Days);
      m_recurrence->setEvery(static_cast<int>(intervalDays));
    }
  } else if (const auto* monthly =
                 dynamic_cast<const events::MonthlyGenerator*>(gen)) {
    m_recurrence->setUnit(RecurrenceFormWidget::Months);
    m_recurrence->setEvery(monthly->getMonths());
  } else if (const auto* yearly =
                 dynamic_cast<const events::YearlyGenerator*>(gen)) {
    m_recurrence->setUnit(RecurrenceFormWidget::Years);
    m_recurrence->setEvery(yearly->getYears());
  }

  if (end != events::TimePoint::max()) {
    m_recurrence->setEndOnDate(toLocal(end).date());
  } else {
    m_recurrence->setEndNever();
  }
}

void ActivitySidebarWidget::populateEventLike(const events::Activity& activity) {
  populateCommonAndRecurrence(activity);
  m_typeCombo->setCurrentIndex(kEvent);
  showSection(kEvent);
}

void ActivitySidebarWidget::populateMeeting(const events::Meeting& meeting) {
  populateCommonAndRecurrence(meeting);
  m_meetingSection->setLocation(QString::fromStdString(meeting.getLocation()));
  QStringList attendees;
  for (const auto& name : meeting.getAttendees()) {
    attendees.append(QString::fromStdString(name));
  }
  m_meetingSection->setAttendees(attendees);
  m_typeCombo->setCurrentIndex(kMeeting);
  showSection(kMeeting);
}

void ActivitySidebarWidget::populateTask(const events::Task& task) {
  populateCommonAndRecurrence(task);
  m_taskSection->setPriority(task.getPriority());
  m_taskSection->setDone(task.isDone());
  m_typeCombo->setCurrentIndex(kTask);
  showSection(kTask);
}

void ActivitySidebarWidget::emitPreview() {
  const QString title = m_titleEdit->text();
  // Niente anteprima per "tutto il giorno" (va nella striscia in alto, non
  // nella griglia oraria), qualunque sia il tipo: la ricorrenza/all-day e'
  // comune, quindi non serve piu' un dispatch per tipo.
  if (m_recurrence->isAllDay()) {
    emit previewChanged(title, QDateTime(), 0, false);
    return;
  }
  const qint64 durationSeconds = static_cast<qint64>(m_durationEdit->value()) * 60;
  emit previewChanged(title, m_startEdit->dateTime(), durationSeconds, true);
}

std::unique_ptr<events::Activity>
ActivitySidebarWidget::makeTypedActivity(events::ActivityConfig config) const {
  switch (m_typeCombo->currentIndex()) {
  case kMeeting: {
    auto meeting = events::makeMeeting(events::MeetingConfig(
        config, m_meetingSection->location().toStdString()));
    for (const QString& name : m_meetingSection->attendees()) {
      meeting->addAttendee(name.toStdString());
    }
    return meeting;
  }
  case kTask: {
    auto task = events::makeTask(events::TaskConfig(config, m_taskSection->priority()));
    if (m_taskSection->isDoneEnabled()) {
      task->setDone(m_taskSection->isDone());
    }
    return task;
  }
  default:
    return events::makeActivity(config);
  }
}

std::vector<std::unique_ptr<events::Activity>>
ActivitySidebarWidget::buildActivities() const {
  std::vector<std::unique_ptr<events::Activity>> result;
  const QString title = m_titleEdit->text().trimmed();
  if (title.isEmpty()) {
    return result;
  }
  const bool allDay = m_recurrence->isAllDay();
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
  if (allDay && !m_recurrence->isRepeating()) {
    // Attivita' che parte alle 00:00 e dura 24h (fino alle 00:00 del giorno
    // dopo): la striscia in alto la riconosce perche' copre un giorno intero.
    result.push_back(makeTypedActivity(events::ActivityConfig{
        .title = title.toStdString(),
        .start = start,
        .duration = std::chrono::seconds(86400)}));
    return result;
  }

  // Niente ripetizione -> una semplice attivita' singola
  if (!m_recurrence->isRepeating()) {
    result.push_back(makeTypedActivity(events::ActivityConfig{
        .title = title.toStdString(), .start = start, .duration = duration}));
    return result;
  }

  // Fine della ricorrenza scelta dall'utente ("Mai" -> resta max())
  events::TimePoint end = events::TimePoint::max();
  if (m_recurrence->endMode() == RecurrenceFormWidget::EndMode::OnDate) {
    end = toTimePoint(QDateTime(m_recurrence->endDate().addDays(1), QTime(0, 0)).addSecs(-1));
  }
  const int countLimit = m_recurrence->endMode() == RecurrenceFormWidget::EndMode::AfterCount
                              ? m_recurrence->endCount()
                              : 0;

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
    result.push_back(makeTypedActivity(events::ActivityConfig{
        .title = title.toStdString(),
        .start = seriesStart,
        .duration = duration,
        .end = seriesEnd,
        .generator = std::move(generator)}));
  };

  const int unit = m_recurrence->unit();
  const int every = m_recurrence->every();
  if (unit == RecurrenceFormWidget::Days) {
    pushRecurrent(std::make_shared<events::FixedIntervalGenerator>(
                      events::Duration(events::Days(every))),
                  start, end);
  } else if (unit == RecurrenceFormWidget::Weeks) {
    // Una o piu' serie ricorrenti, una per giorno della settimana scelto.
    // Il limite "dopo N occorrenze" vale sul CALENDARIO COMBINATO, non per
    // singola serie: es. lun+mar+mer con N=5 -> sett1 lun/mar/mer + sett2
    // lun/mar = 5 eventi totali. La fine e' quindi la data della N-esima
    // occorrenza complessiva (non e' rappresentabile avanzando un solo
    // generatore, quindi si calcola con l'aritmetica sui giorni scelti).
    const int baseDow = m_startEdit->date().dayOfWeek();
    const QDate startDate = m_startEdit->date();
    const QTime time = allDay ? QTime(0, 0) : m_startEdit->time();

    // Giorni selezionati (id Qt: 1=Lun..7=Dom), fallback: il giorno
    // dell'inizio.
    std::vector<int> selected = m_recurrence->selectedWeekdays();
    if (selected.empty()) {
      selected.push_back(baseDow);
    }

    // Fine: fino-a (data) / dopo-N (data della N-esima occorrenza combinata)
    events::TimePoint effectiveEnd = end;
    if (countLimit > 0) {
      const int n = countLimit;
      std::vector<int> offsets;
      offsets.reserve(selected.size());
      for (int dow : selected) {
        offsets.push_back((dow - baseDow + 7) % 7);
      }
      std::ranges::sort(offsets);
      const int period = (n - 1) / static_cast<int>(offsets.size());
      const int idx = (n - 1) % static_cast<int>(offsets.size());
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
      result.push_back(makeTypedActivity(events::ActivityConfig{
          .title = title.toStdString(),
          .start = anchor,
          .duration = duration,
          .end = effectiveEnd,
          .generator = std::make_shared<events::FixedIntervalGenerator>(
              events::Duration(events::Days(7 * every)))}));
    }
  } else if (unit == RecurrenceFormWidget::Months) {
    pushRecurrent(std::make_shared<events::MonthlyGenerator>(every), start, end);
  } else {  // anni
    pushRecurrent(std::make_shared<events::YearlyGenerator>(every), start, end);
  }
  return result;
}

void ActivitySidebarWidget::onDelete() {
  const QString subject = m_saveStrategy->deleteSubject();
  if (subject.isEmpty()) {
    return;  // Create: niente da eliminare
  }
  if (QMessageBox::question(this, tr("Elimina"),
                            tr("Eliminare %1?").arg(subject)) != QMessageBox::Yes) {
    return;
  }
  if (m_saveStrategy->remove(m_controller)) {
    emit closeRequested();
  }
}

void ActivitySidebarWidget::onSave() {
  // Un'unica costruzione per tutti e tre i tipi: attivita' singola oppure
  // una o piu' serie ricorrenti (la ricorrenza e' comune a Evento/Riunione/
  // Compito, non serve piu' distinguere il pannello Evento dagli altri).
  auto activities = buildActivities();
  if (activities.empty()) {
    m_errorLabel->setText(tr("Inserire un titolo non vuoto."));
    return;
  }
  // Create/EditActivity/EditOccurrence chiamano ciascuno un metodo diverso
  // del controller: la scelta e' delegata a m_saveStrategy (polimorfismo)
  // invece di uno switch qui.
  if (m_saveStrategy->execute(m_controller, std::move(activities))) {
    m_errorLabel->clear();
    emit closeRequested();
  } else {
    m_errorLabel->setText(tr("Operazione non riuscita."));
  }
}

} // namespace app
