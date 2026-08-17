#ifndef APP_ACTIVITY_FORM_PAGE_H
#define APP_ACTIVITY_FORM_PAGE_H

#include <QDateTime>
#include <QWidget>

#include <memory>
#include <optional>

#include "events/events.h"

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTimeEdit;

namespace app {

class CalendarController;

/** @brief Pagina "creazione / modifica" di un'attivita'.
 *
 *  Ogni tipo ha un pannello con i propri campi, organizzati in righe con
 *  l'etichetta a sinistra e la box di input affiancata a destra (es.
 *  "Titolo [________]", "Data e ora [__/__/____ __:__]"). Cambiando tipo,
 *  titolo e data vengono conservati (i campi non sono condivisi tra i
 *  pannelli, evita i bug di visibilita' nei QStackedWidget).
 */
class ActivityFormPage : public QWidget {
    Q_OBJECT
public:
    enum class Mode { Create, EditActivity, EditOccurrence };

    explicit ActivityFormPage(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Avvia la creazione: tipo libero, data/ora suggerita (es. dal
     *  doppio clic su una cella della settimana) se valida. */
    void startCreate(const QDateTime& suggestedStart = QDateTime());

    /** @brief Avvia la modifica di un'attivita' esistente. */
    void startEditActivity(const events::Activity* activity);

    /** @brief Avvia la modifica di una singola occorrenza (sostituzione con evento singolo). */
    void startEditOccurrence(const events::Occurrence& occurrence);

signals:
    void backRequested();

private slots:
    void onSave();
    void onTypeChanged(int index);
    void onRecurrenceEndToggled(bool checked);

private:
    /** @brief Orario di fine dell'evento ripetuto (inizio + durata). */
    QTime recurrenceEndTime() const;

    /** @brief Sincronizza l'orario della scadenza con la fine dell'evento
     *  ripetuto (la data di scadenza resta quella scelta). */
    void syncRecurrenceEndTime();
    // Pannelli dei campi (form specifico per tipo, vincolo PAO)
    QWidget* buildEventPanel();
    QWidget* buildRecurrentPanel();
    QWidget* buildDeadlinePanel();
    QWidget* buildReminderPanel();

    // Lettura dei campi -> oggetto di dominio
    std::unique_ptr<events::Activity> buildActivity() const;

    // Popolamento dei campi in modifica
    void populateEvent(const events::Event& event);
    void populateRecurrent(const events::RecurrentEvent& event);
    void populateDeadline(const events::Deadline& deadline);
    void populateReminder(const events::Reminder& reminder);

    // Campi comuni per pannello (titolo/data/durata) e loro sincronizzazione
    QLineEdit* titleOf(int panel) const;
    QDateTimeEdit* dateOf(int panel) const;
    QTimeEdit* durationOf(int panel) const;
    void syncCommonFields(int fromPanel, int toPanel);

    // Conversioni locale/UTC
    static events::TimePoint toTimePoint(const QDateTime& local);
    static QDateTime toLocal(const events::TimePoint tp);

    CalendarController* m_controller;
    Mode m_mode = Mode::Create;
    const events::Activity* m_editingActivity = nullptr;
    std::optional<events::Occurrence> m_editingOccurrence;

    QComboBox* m_typeCombo = nullptr;
    QStackedWidget* m_forms = nullptr;

    // Evento
    QLineEdit* m_titleE = nullptr;
    QDateTimeEdit* m_startE = nullptr;
    QTimeEdit* m_durationE = nullptr;

    // Ricorrente
    QLineEdit* m_titleR = nullptr;
    QDateTimeEdit* m_startR = nullptr;
    QTimeEdit* m_durationR = nullptr;
    QSpinBox* m_intervalDays = nullptr;
    QCheckBox* m_hasEndCheck = nullptr;
    QDateTimeEdit* m_endEdit = nullptr;

    // Scadenza
    QLineEdit* m_titleD = nullptr;
    QDateTimeEdit* m_dueEdit = nullptr;
    QComboBox* m_priorityCombo = nullptr;
    QCheckBox* m_doneCheck = nullptr;

    // Promemoria
    QLineEdit* m_titleM = nullptr;
    QDateTimeEdit* m_triggerEdit = nullptr;
    QLineEdit* m_messageEdit = nullptr;
    QSpinBox* m_repeatDays = nullptr;

    QPushButton* m_saveButton = nullptr;
    QLabel* m_errorLabel = nullptr;
};

} // namespace app

#endif // APP_ACTIVITY_FORM_PAGE_H
