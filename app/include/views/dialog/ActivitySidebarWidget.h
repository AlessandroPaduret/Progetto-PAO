#pragma once

#include <QDateTime>
#include <QWidget>

#include <memory>
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

class ActivityTypeWidget;
class CalendarController;
class FormSaveStrategy;
class RecurrenceFormWidget;

/** @brief Pannello laterale (sidebar) per creare/modificare un'attivita': un
 *  QWidget integrato nella MainWindow tramite QSplitter (non un QDialog).
 *
 *  Non esiste una vista di sola lettura separata: aprire un'attivita'
 *  esistente equivale ad aprirla direttamente in modifica.
 *
 *  Agisce da semplice COORDINATORE: possiede solo i campi comuni a tutti i
 *  tipi (Titolo/Data/Durata) e delega ogni sezione specifica a un widget
 *  figlio autonomo -- RecurrenceFormWidget (Tutto il giorno/Si ripete,
 *  comune a Evento/Riunione/Compito) piu' un `ActivityTypeWidget` per tipo
 *  (Evento/Riunione/Compito, in `m_typeWidgets`), mostrando/nascondendo
 *  solo Riunione/Compito in base al tipo scelto nella combo (l'Evento non
 *  ha campi propri). Il polimorfismo su `ActivityTypeWidget` sostituisce lo
 *  switch che prima costruiva/popolava l'attivita' in base al tipo: la
 *  logica di ciascuna sezione (giorni della settimana, fine ricorrenza,
 *  elenco partecipanti, costruzione della propria Activity/Meeting/Task,
 *  ...) vive nel rispettivo widget figlio, non qui.
 */
class ActivitySidebarWidget : public QWidget {
    Q_OBJECT
public:
    explicit ActivitySidebarWidget(CalendarController* controller, QWidget* parent = nullptr);
    ~ActivitySidebarWidget() override;

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
    /** @brief Mostra la sola sezione specifica del tipo scelto (Riunione o
     *  Compito; l'Evento non ne ha una propria). */
    void showSection(int type);

    /** @brief Emette l'anteprima corrente (campi comuni). */
    void emitPreview();

    /** @brief Costruisce le attivita' del tipo scelto (Evento/Riunione/
     *  Compito): una sola attivita', oppure una o piu' serie ricorrenti (una
     *  per giorno della settimana selezionato, per la ricorrenza settimanale)
     *  — la ricorrenza e' comune a tutti e tre i tipi. Riduce i campi del
     *  form al caso da costruire e lo delega al sotto-metodo adatto (vedi
     *  sotto): la matematica delle date vive in RecurrenceBuilder, non qui. */
    std::vector<std::unique_ptr<events::Activity>> buildActivities() const;

    // --- Sotto-passi di buildActivities(), un caso ciascuno (SRP) -----------
    std::vector<std::unique_ptr<events::Activity>> buildSingleActivity(
        const QString& title, events::TimePoint start, events::Duration duration) const;
    std::vector<std::unique_ptr<events::Activity>> buildRecurrentSeries(
        const QString& title, events::TimePoint start, events::Duration duration,
        events::TimePoint end, int countLimit,
        std::shared_ptr<const events::DateGenerator> generator) const;
    std::vector<std::unique_ptr<events::Activity>> buildWeeklySeries(
        const QString& title, events::Duration duration, bool allDay,
        events::TimePoint end, int countLimit) const;

    // Letture del form comuni a piu' sotto-passi di buildActivities()
    events::TimePoint resolveStart(bool allDay) const;
    events::TimePoint resolveSeriesEnd() const;
    int resolveCountLimit() const;

    /** @brief Costruisce l'attivita' del tipo corrente a partire dai campi
     *  comuni gia' pronti (titolo/data/durata/end/generatore): delega al
     *  `m_typeWidgets[m_typeCombo->currentIndex()]` (polimorfismo, vedi
     *  ActivityTypeWidget) l'aggiunta dei soli campi specifici del tipo
     *  (luogo/partecipanti, priorita'/evaso) e la costruzione vera e propria. */
    std::unique_ptr<events::Activity> makeTypedActivity(events::ActivityConfig config) const;

    // Popolamento dei campi in modifica (uguale per i 3 tipi: la ricorrenza
    // non dipende dal tipo dell'attivita'), spezzato per responsabilita':
    // solo titolo/data/durata, solo ricorrenza, solo lettura del generatore.
    // I campi specifici del tipo li popola invece polimorficamente
    // ActivityTypeWidget::populateFrom (vedi showEditActivity nel .cpp).
    void populateBasicFields(const events::Activity& activity);
    void populateRecurrenceFields(const events::Activity& activity);
    void populateGeneratorFields(const events::Activity& activity);

    // Conversioni locale/UTC
    static events::TimePoint toTimePoint(const QDateTime& local);
    static QDateTime toLocal(const events::TimePoint tp);

    CalendarController* m_controller;

    // Comportamento di Salva/Elimina specifico della modalita' corrente del
    // pannello (creazione, modifica di un'intera attivita', modifica di una
    // singola occorrenza): un oggetto polimorfo al posto di un enum "Mode" +
    // switch/if sparsi in onSave()/onDelete() (vedi il .cpp).
    std::unique_ptr<FormSaveStrategy> m_saveStrategy;

    // --- Campi comuni a tutti i tipi (Titolo/Data/Durata: quelli di Activity) ---
    QComboBox* m_typeCombo = nullptr;        // Evento / Riunione / Compito
    QLineEdit* m_titleEdit = nullptr;
    QDateTimeEdit* m_startEdit = nullptr;    // data+ora; "Tutto il giorno" cambia solo il displayFormat
    QSpinBox* m_durationEdit = nullptr;      // minuti
    QFormLayout* m_commonForm = nullptr;     // per setRowVisible(m_durationEdit, ...)

    // --- Sezioni delegate ai widget figli -----------------------------------
    RecurrenceFormWidget* m_recurrence = nullptr;  // comune a tutti e 3 i tipi
    ActivityTypeWidget* m_eventSection = nullptr;    // nessun campo proprio, mai in layout
    ActivityTypeWidget* m_meetingSection = nullptr;
    ActivityTypeWidget* m_taskSection = nullptr;
    // Stesso ordine di kEvent/kMeeting/kTask: m_typeWidgets[m_typeCombo->
    // currentIndex()] e' la sezione da interrogare polimorficamente.
    std::vector<ActivityTypeWidget*> m_typeWidgets;

    QPushButton* m_deleteButton = nullptr;   // Elimina (visibile solo in modifica)
    QLabel* m_errorLabel = nullptr;
};

} // namespace app
