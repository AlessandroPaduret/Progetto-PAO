#pragma once

#include <QDateTime>
#include <QDialog>
#include <QList>

#include <memory>
#include <optional>
#include <vector>

#include "events.h"

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QCompleter;
class QDateEdit;
class QDateTimeEdit;
class QDialogButtonBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QStackedWidget;

namespace app {

class CalendarController;

/** @brief Dialog nativo di creazione/modifica di un'attivita'.
 *
 *  La creazione NON chiede il tipo: pone delle DOMANDE e in base alle
 *  risposte istanzia l'oggetto giusto (meccanica "a domande").
 *
 *  Per un evento: titolo, data/ora, durata e la checkbox "Si ripete?". Se
 *  si ripete: unita' (giorni/settimane/mesi/anno), "ogni N", per le
 *  settimane i giorni della settimana (QButtonGroup non esclusivo) e la
 *  fine (mai / fino a data / dopo N occorrenze). In base alle risposte
 *  viene istanziata un'attivita' singola oppure una o piu' serie ricorrenti
 *  (una per giorno della settimana selezionato).
 *
 *  Gli altri tipi (Riunione, Compito, Anniversario) hanno un pannello
 *  dedicato, scelto dalla combo.
 *
 *  E' un QDialog nativo: apertura/centraggio/modalita' sono gestiti da Qt
 *  tramite exec() (nessun posizionamento manuale). Ogni pannello usa un
 *  QFormLayout (niente griglia gestita a mano); i pulsanti di azione stanno
 *  in una QDialogButtonBox (Salva/Annulla in ordine di piattaforma, Elimina
 *  come pulsante distruttivo separato). Data e ora sono un unico
 *  QDateTimeEdit (per l'Evento cambia solo `displayFormat` quando si
 *  seleziona "Tutto il giorno", invece di due widget separati); la durata
 *  e' un QSpinBox in minuti (QTimeEdit era limitato/ambiguo oltre le 24h); i
 *  giorni della settimana sono un QButtonGroup non esclusivo con l'id di
 *  ogni pulsante pari al giorno Qt (1=Lunedi'..7=Domenica, la stessa
 *  convenzione di QDate::dayOfWeek()).
 */
class ActivityFormDialog : public QDialog {
    Q_OBJECT
public:
    enum class Mode { Create, EditActivity, EditOccurrence };

    explicit ActivityFormDialog(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Avvia la creazione: tipo libero, data/ora suggerita (es. dal
     *  doppio clic su una cella della settimana) se valida. Non mostra il
     *  dialog: il chiamante lo apre con exec(). */
    void startCreate(const QDateTime& suggestedStart = QDateTime());

    /** @brief Avvia la creazione preselezionando il tipo nel menu "Nuova
     *  attivita'" (0=Evento, 1=Riunione, 2=Compito, 3=Anniversario). */
    void startCreateType(int typeIndex, const QDateTime& suggestedStart = QDateTime());

    /** @brief Avvia la modifica di un'attivita' esistente. */
    void startEditActivity(const events::Activity* activity);

    /** @brief Avvia la modifica di una singola occorrenza (sostituzione con evento singolo). */
    void startEditOccurrence(const events::Occurrence& occurrence);

signals:
    /** @brief Anteprima dell'evento in fase di creazione/modifica: emesso a
     *  ogni cambiamento dei campi (titolo, data/ora, durata). Attivo anche
     *  durante l'exec() del dialog (loop di eventi annidato). */
    void previewChanged(const QString& title, const QDateTime& start,
                        qint64 durationSeconds, bool valid);

private slots:
    void onSave();
    void onDelete();
    void onTypeChanged(int index);
    void onRepeatToggled(bool checked);
    void onUnitChanged(int index);
    void onAddAttendee();
    void onRemoveAttendee();

private:
    /** @brief Emette l'anteprima corrente (dati del pannello attivo). */
    void emitPreview();

    /** @brief Costruisce le attivita' del pannello Evento "a domande":
     *  una attivita' singola oppure una o piu' serie ricorrenti (una per
     *  giorno della settimana selezionato, per la ricorrenza settimanale). */
    std::vector<std::unique_ptr<events::Activity>> buildEventActivities() const;

    // Pannelli dei campi (form specifico per tipo, vincolo PAO)
    QWidget* buildEventPanel();
    QWidget* buildMeetingPanel();
    QWidget* buildTaskPanel();
    QWidget* buildAnniversaryPanel();

    // Lettura dei campi -> oggetto di dominio
    std::unique_ptr<events::Activity> buildActivity() const;

    // Popolamento dei campi in modifica
    void populateEventLike(const events::Activity& activity);
    void populateMeeting(const events::Meeting& meeting);
    void populateTask(const events::Task& task);
    void populateAnniversary(const events::Activity& activity);

    // Campi comuni per pannello (titolo/data/durata) e loro sincronizzazione
    QLineEdit* titleOf(int panel) const;
    QDateTimeEdit* dateOf(int panel) const;
    QSpinBox* durationOf(int panel) const;
    void syncCommonFields(int fromPanel, int toPanel);

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

    QComboBox* m_typeCombo = nullptr;
    QStackedWidget* m_forms = nullptr;

    // Evento "a domande"
    QLineEdit* m_titleE = nullptr;
    QFormLayout* m_eventForm = nullptr;      // per setRowVisible(m_durationE, ...)
    QDateTimeEdit* m_startE = nullptr;       // data+ora; "Tutto il giorno" cambia solo il displayFormat
    QSpinBox* m_durationE = nullptr;         // minuti
    QCheckBox* m_allDayCheck = nullptr;      // "Tutto il giorno"
    QCheckBox* m_repeatCheck = nullptr;
    QWidget* m_repeatBox = nullptr;          // contenitore delle impostazioni di ricorrenza
    QFormLayout* m_repeatForm = nullptr;     // per setRowVisible(m_dayRow, ...)
    QComboBox* m_unitCombo = nullptr;        // giorni / settimane / mesi / anno
    QSpinBox* m_everySpin = nullptr;         // "ogni N"
    QWidget* m_dayRow = nullptr;             // riga dei giorni della settimana
    QButtonGroup* m_dayGroup = nullptr;      // NON esclusivo; id pulsante = giorno Qt (1=Lun..7=Dom)
    QRadioButton* m_endNever = nullptr;
    QRadioButton* m_endDateRadio = nullptr;
    QDateEdit* m_endDate = nullptr;
    QRadioButton* m_endCountRadio = nullptr;
    QSpinBox* m_countSpin = nullptr;         // "dopo N occorrenze"

    // Riunione
    QLineEdit* m_titleMt = nullptr;
    QDateTimeEdit* m_startMt = nullptr;
    QSpinBox* m_durationMt = nullptr;        // minuti
    QLineEdit* m_locationMt = nullptr;
    QLineEdit* m_attendeeEdit = nullptr;
    QCompleter* m_attendeeCompleter = nullptr;
    QListWidget* m_attendeesList = nullptr;

    // Compito
    QLineEdit* m_titleT = nullptr;
    QDateTimeEdit* m_dueT = nullptr;
    QComboBox* m_priorityCombo = nullptr;
    QCheckBox* m_doneCheck = nullptr;

    // Anniversario (solo data: nessun componente ora, resta un QDateEdit)
    QLineEdit* m_titleAn = nullptr;
    QDateEdit* m_dateAn = nullptr;

    QDialogButtonBox* m_buttonBox = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QLabel* m_errorLabel = nullptr;
};

} // namespace app
