#pragma once

#include <QDateTime>
#include <QWidget>

#include <memory>
#include <optional>
#include <vector>

#include "events.h"

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace app {

class CalendarController;
class MeetingFormWidget;
class RecurrenceFormWidget;
class TaskFormWidget;

/** @brief Pannello laterale (sidebar) per creare/modificare un'attivita': un
 *  QWidget integrato nella MainWindow tramite QSplitter (non un QDialog).
 *
 *  Non esiste una vista di sola lettura separata: aprire un'attivita'
 *  esistente equivale ad aprirla direttamente in modifica.
 *
 *  Agisce da semplice COORDINATORE: possiede solo i campi comuni a tutti i
 *  tipi (Titolo/Data/Durata) e delega ogni sezione specifica a un widget
 *  figlio autonomo -- RecurrenceFormWidget (Tutto il giorno/Si ripete,
 *  comune a Evento/Riunione/Compito), MeetingFormWidget (luogo/partecipanti)
 *  e TaskFormWidget (priorita'/evaso) -- mostrando/nascondendo solo quest
 *  ultime due in base al tipo scelto nella combo. La logica di ciascuna
 *  sezione (giorni della settimana, fine ricorrenza, elenco partecipanti,
 *  ...) vive nel rispettivo widget figlio, non qui.
 */
class ActivitySidebarWidget : public QWidget {
    Q_OBJECT
public:
    explicit ActivitySidebarWidget(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Apre il form di creazione: tipo libero, data/ora suggerita
     *  (es. dal doppio clic su una cella della settimana) se valida. */
    void showCreate(const QDateTime& suggestedStart = QDateTime());

    /** @brief Apre il form di creazione preselezionando il tipo (0=Evento,
     *  1=Riunione, 2=Compito), dal menu "Nuova attivita'". */
    void showCreateType(int typeIndex, const QDateTime& suggestedStart = QDateTime());

    /** @brief Apre il form di modifica di un'attivita' esistente. */
    void showEditActivity(const events::Activity* activity);

    /** @brief Apre il form di modifica di una singola occorrenza (sostituzione
     *  con evento singolo). */
    void showEditOccurrence(const events::Occurrence& occurrence);

signals:
    /** @brief L'utente ha chiuso il pannello (Annulla) o un'azione (Salva/
     *  Elimina) e' andata a buon fine: la MainWindow nasconde la sidebar. */
    void closeRequested();

    /** @brief Anteprima dell'evento in fase di creazione/modifica: emesso a
     *  ogni cambiamento dei campi comuni (titolo, data/ora, durata). */
    void previewChanged(const QString& title, const QDateTime& start,
                        qint64 durationSeconds, bool valid);

private slots:
    void onSave();
    void onDelete();
    void onTypeChanged(int index);
    void onAllDayToggled(bool on);

private:
    enum class Mode { Create, EditActivity, EditOccurrence };

    /** @brief Mostra la sola sezione specifica del tipo scelto (Riunione o
     *  Compito; l'Evento non ne ha una propria). */
    void showSection(int type);

    /** @brief Emette l'anteprima corrente (campi comuni). */
    void emitPreview();

    /** @brief Costruisce le attivita' del tipo scelto (Evento/Riunione/
     *  Compito): una sola attivita', oppure una o piu' serie ricorrenti (una
     *  per giorno della settimana selezionato, per la ricorrenza settimanale)
     *  — la ricorrenza e' comune a tutti e tre i tipi. */
    std::vector<std::unique_ptr<events::Activity>> buildActivities() const;

    /** @brief Costruisce l'attivita' del tipo corrente a partire dai campi
     *  comuni gia' pronti (titolo/data/durata/end/generatore): aggiunge i
     *  soli campi specifici del tipo (luogo/partecipanti, priorita'/evaso). */
    std::unique_ptr<events::Activity> makeTypedActivity(events::ActivityConfig config) const;

    // Popolamento dei campi comuni e della ricorrenza in modifica (uguale
    // per i 3 tipi: la ricorrenza non dipende dal tipo dell'attivita').
    void populateCommonAndRecurrence(const events::Activity& activity);
    void populateEventLike(const events::Activity& activity);
    void populateMeeting(const events::Meeting& meeting);
    void populateTask(const events::Task& task);

    // Suggerimenti (QCompleter) per il campo partecipanti: nomi gia' usati
    // in altre Riunioni del calendario corrente.
    void refreshAttendeeCompleter();

    // Conversioni locale/UTC
    static events::TimePoint toTimePoint(const QDateTime& local);
    static QDateTime toLocal(const events::TimePoint tp);

    CalendarController* m_controller;

    Mode m_mode = Mode::Create;
    const events::Activity* m_editingActivity = nullptr;
    std::optional<events::Occurrence> m_editingOccurrence;

    // --- Campi comuni a tutti i tipi (Titolo/Data/Durata: quelli di Activity) ---
    QComboBox* m_typeCombo = nullptr;        // Evento / Riunione / Compito
    QLineEdit* m_titleEdit = nullptr;
    QDateTimeEdit* m_startEdit = nullptr;    // data+ora; "Tutto il giorno" cambia solo il displayFormat
    QSpinBox* m_durationEdit = nullptr;      // minuti
    QFormLayout* m_commonForm = nullptr;     // per setRowVisible(m_durationEdit, ...)

    // --- Sezioni delegate ai widget figli -----------------------------------
    RecurrenceFormWidget* m_recurrence = nullptr;  // comune a tutti e 3 i tipi
    MeetingFormWidget* m_meetingSection = nullptr;
    TaskFormWidget* m_taskSection = nullptr;

    QPushButton* m_deleteButton = nullptr;   // Elimina (visibile solo in modifica)
    QLabel* m_errorLabel = nullptr;
};

} // namespace app
