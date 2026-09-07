#pragma once

#include <QDateTime>
#include <QList>
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
class QStackedWidget;

namespace app {

class CalendarController;

/** @brief Pannello laterale (sidebar) per visualizzare/creare/modificare
 *  un'attivita': un QWidget integrato nella MainWindow tramite QSplitter
 *  (non un QDialog: niente exec()/accept()/reject()/QDialogButtonBox, niente
 *  finestra separata da centrare).
 *
 *  Internamente ha uno QStackedWidget con due pagine:
 *   - Index 0, Dettaglio: campi in sola lettura (Visitor) + Modifica/Elimina
 *     + Chiudi (X).
 *   - Index 1, Form: gli stessi 4 pannelli "a domande" di prima (Evento/
 *     Riunione/Compito/Anniversario) + Salva/Annulla/Elimina.
 *
 *  "Modifica" nel Dettaglio passa alla pagina Form SENZA nascondere il
 *  pannello (stessa sidebar, stesso QWidget); Annulla/Chiudi/un salvataggio o
 *  un'eliminazione riusciti emettono `closeRequested()`, che la MainWindow
 *  usa per nascondere la sidebar (il pannello stesso non decide la propria
 *  visibilita' nello splitter).
 */
class ActivitySidebarWidget : public QWidget {
    Q_OBJECT
public:
    explicit ActivitySidebarWidget(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Mostra il dettaglio in sola lettura di un'attivita' esistente. */
    void showDetail(const events::Activity* activity);

    /** @brief Apre il form di creazione: tipo libero, data/ora suggerita
     *  (es. dal doppio clic su una cella della settimana) se valida. */
    void showCreate(const QDateTime& suggestedStart = QDateTime());

    /** @brief Apre il form di creazione preselezionando il tipo (0=Evento,
     *  1=Riunione, 2=Compito, 3=Anniversario), dal menu "Nuova attivita'". */
    void showCreateType(int typeIndex, const QDateTime& suggestedStart = QDateTime());

    /** @brief Apre il form di modifica di un'attivita' esistente. */
    void showEditActivity(const events::Activity* activity);

    /** @brief Apre il form di modifica di una singola occorrenza (sostituzione
     *  con evento singolo). */
    void showEditOccurrence(const events::Occurrence& occurrence);

signals:
    /** @brief L'utente ha chiuso il pannello (Chiudi/Annulla) o un'azione
     *  (Salva/Elimina) e' andata a buon fine: la MainWindow nasconde la
     *  sidebar nello splitter. */
    void closeRequested();

    /** @brief Anteprima dell'evento in fase di creazione/modifica: emesso a
     *  ogni cambiamento dei campi del form (titolo, data/ora, durata). */
    void previewChanged(const QString& title, const QDateTime& start,
                        qint64 durationSeconds, bool valid);

private slots:
    void onDetailEdit();
    void onDetailDelete();
    void onSave();
    void onDelete();
    void onTypeChanged(int index);
    void onRepeatToggled(bool checked);
    void onUnitChanged(int index);
    void onAddAttendee();
    void onRemoveAttendee();

private:
    enum class Mode { Create, EditActivity, EditOccurrence };

    QWidget* buildDetailPage();
    QWidget* buildFormPage();

    /** @brief Emette l'anteprima corrente (dati del pannello di tipo attivo). */
    void emitPreview();

    /** @brief Costruisce le attivita' del pannello Evento "a domande":
     *  una attivita' singola oppure una o piu' serie ricorrenti (una per
     *  giorno della settimana selezionato, per la ricorrenza settimanale). */
    std::vector<std::unique_ptr<events::Activity>> buildEventActivities() const;

    // Pannelli dei campi del form (per tipo di attivita', vincolo PAO)
    QWidget* buildEventPanel();
    QWidget* buildMeetingPanel();
    QWidget* buildTaskPanel();
    QWidget* buildAnniversaryPanel();

    // Lettura dei campi del form -> oggetto di dominio
    std::unique_ptr<events::Activity> buildActivity() const;

    // Popolamento dei campi del form in modifica
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

    QStackedWidget* m_stack = nullptr;   // Index 0 = Dettaglio, Index 1 = Form

    // --- Pagina Dettaglio (sola lettura) ------------------------------------
    const events::Activity* m_detailActivity = nullptr;
    QLabel* m_detailTitleLabel = nullptr;
    QLabel* m_detailFieldsLabel = nullptr;

    // --- Pagina Form ---------------------------------------------------------
    Mode m_mode = Mode::Create;
    const events::Activity* m_editingActivity = nullptr;
    std::optional<events::Occurrence> m_editingOccurrence;

    QComboBox* m_typeCombo = nullptr;
    QStackedWidget* m_forms = nullptr;   // Evento / Riunione / Compito / Anniversario

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

    QPushButton* m_deleteButton = nullptr;   // Elimina nel form (visibile solo in modifica)
    QLabel* m_errorLabel = nullptr;
};

} // namespace app
