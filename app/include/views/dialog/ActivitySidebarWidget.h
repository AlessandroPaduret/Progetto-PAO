#pragma once

#include <QDateTime>
#include <QWidget>

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
class QFormLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;

namespace app {

class CalendarController;

/** @brief Pannello laterale (sidebar) per creare/modificare un'attivita': un
 *  QWidget integrato nella MainWindow tramite QSplitter (non un QDialog).
 *
 *  Non esiste una vista di sola lettura separata: aprire un'attivita'
 *  esistente equivale ad aprirla direttamente in modifica.
 *
 *  I campi comuni a tutti i tipi (Titolo/Data/Durata: gli stessi che
 *  Activity possiede sempre) stanno in un'unica sezione in cima, costruita
 *  una sola volta; sotto, in base al tipo scelto nella combo, si rivela la
 *  sola sezione specifica di quel tipo (ricorrenza per l'Evento, luogo/
 *  partecipanti per la Riunione, priorita'/stato per il Compito) invece di
 *  ripetere titolo/data/durata in un pannello separato per tipo.
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
    void onRepeatToggled(bool checked);
    void onUnitChanged(int index);
    void onAddAttendee();
    void onRemoveAttendee();

private:
    enum class Mode { Create, EditActivity, EditOccurrence };

    QWidget* buildEventSection();
    QWidget* buildMeetingSection();
    QWidget* buildTaskSection();

    /** @brief Mostra la sola sezione specifica del tipo scelto. */
    void showSection(int type);

    /** @brief Emette l'anteprima corrente (campi comuni). */
    void emitPreview();

    /** @brief Costruisce le attivita' del tipo Evento: una attivita' singola
     *  oppure una o piu' serie ricorrenti (una per giorno della settimana
     *  selezionato, per la ricorrenza settimanale). */
    std::vector<std::unique_ptr<events::Activity>> buildEventActivities() const;

    /** @brief Costruisce Riunione/Compito (sempre una sola attivita'). */
    std::unique_ptr<events::Activity> buildActivity() const;

    // Popolamento dei campi in modifica
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

    // --- Sezione Evento: "tutto il giorno" + ricorrenza -------------------------
    QWidget* m_eventSection = nullptr;
    QCheckBox* m_allDayCheck = nullptr;
    QCheckBox* m_repeatCheck = nullptr;
    QWidget* m_repeatBox = nullptr;          // contenitore delle impostazioni di ricorrenza
    QFormLayout* m_repeatForm = nullptr;     // per setRowVisible(m_dayRow, ...)
    QComboBox* m_unitCombo = nullptr;        // giorni / settimane / mesi / anni
    QSpinBox* m_everySpin = nullptr;         // "ogni N"
    QWidget* m_dayRow = nullptr;             // riga dei giorni della settimana
    QButtonGroup* m_dayGroup = nullptr;      // NON esclusivo; id pulsante = giorno Qt (1=Lun..7=Dom)
    QRadioButton* m_endNever = nullptr;
    QRadioButton* m_endDateRadio = nullptr;
    QDateEdit* m_endDate = nullptr;
    QRadioButton* m_endCountRadio = nullptr;
    QSpinBox* m_countSpin = nullptr;         // "dopo N occorrenze"

    // --- Sezione Riunione: luogo + partecipanti ----------------------------------
    QWidget* m_meetingSection = nullptr;
    QLineEdit* m_locationEdit = nullptr;
    QLineEdit* m_attendeeEdit = nullptr;
    QCompleter* m_attendeeCompleter = nullptr;
    QListWidget* m_attendeesList = nullptr;

    // --- Sezione Compito: priorita' + evaso --------------------------------------
    QWidget* m_taskSection = nullptr;
    QComboBox* m_priorityCombo = nullptr;
    QCheckBox* m_doneCheck = nullptr;

    QPushButton* m_deleteButton = nullptr;   // Elimina (visibile solo in modifica)
    QLabel* m_errorLabel = nullptr;
};

} // namespace app
