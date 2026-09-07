#pragma once

#include <QDateTime>
#include <QWidget>

#include <memory>
#include <vector>

#include "events.h"
#include "dialog/utils/ActivitySeriesBuilder.h"

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

/** @brief Sidebar (QWidget in QSplitter, non un QDialog) per creare/
 *  modificare un'attivita': niente vista di sola lettura, aprirne una
 *  esistente apre direttamente il form di modifica.
 *  Fa da coordinatore: tiene i campi comuni (Titolo/Data/Durata) e delega
 *  ogni sezione specifica a un widget figlio (RecurrenceFormWidget + un
 *  ActivityTypeWidget per tipo in m_typeWidgets), sostituendo con il
 *  polimorfismo lo switch che prima costruiva/popolava l'attivita' per tipo. */
class ActivitySidebarWidget : public QWidget {
    Q_OBJECT
public:
    explicit ActivitySidebarWidget(CalendarController* controller, QWidget* parent = nullptr);
    ~ActivitySidebarWidget() override;

    /** @brief suggestedStart es. dal doppio clic su una cella della settimana. */
    void showCreate(const QDateTime& suggestedStart = QDateTime());

    /** @brief typeIndex: 0=Evento, 1=Riunione, 2=Compito (dal menu "Nuova attivita'"). */
    void showCreateType(int typeIndex, const QDateTime& suggestedStart = QDateTime());

    void showEditActivity(const events::Activity* activity);

    /** @brief Modifica di una singola occorrenza: si traduce in un evento singolo. */
    void showEditOccurrence(const events::Occurrence& occurrence);

signals:
    /** @brief Chiusura (Annulla) o azione riuscita (Salva/Elimina): la
     *  MainWindow nasconde la sidebar. */
    void closeRequested();

    /** @brief Emesso ad ogni cambiamento dei campi comuni, per l'anteprima live. */
    void previewChanged(const QString& title, const QDateTime& start,
                        qint64 durationSeconds, bool valid);

private slots:
    void onSave();
    void onDelete();
    void onTypeChanged(int index);
    void onAllDayToggled(bool on);

private:
    /** @brief Mostra la sola sezione del tipo scelto (Riunione o Compito;
     *  l'Evento non ne ha una propria). */
    void showSection(int type);

    void emitPreview();

    /** @brief Legge lo stato di RecurrenceFormWidget in una RecurrenceRule:
     *  la sidebar raccoglie i dati, non li calcola (lo fa ActivitySeriesBuilder). */
    RecurrenceRule readRecurrenceRule() const;

    // Popolamento in modifica, spezzato per responsabilita' (i campi
    // specifici del tipo li popola invece ActivityTypeWidget::populateFrom).
    void populateBasicFields(const events::Activity& activity);
    void populateRecurrenceFields(const events::Activity& activity);
    void populateGeneratorFields(const events::Activity& activity);

    // Sola lettura per popolare il form (la costruzione vive in ActivitySeriesBuilder)
    static QDateTime toLocal(const events::TimePoint tp);

    CalendarController* m_controller;

    // Comportamento di Salva/Elimina per la modalita' corrente (creazione,
    // modifica attivita', modifica occorrenza): polimorfismo al posto di un
    // enum "Mode" + switch/if in onSave()/onDelete().
    std::unique_ptr<FormSaveStrategy> m_saveStrategy;

    // --- Campi comuni a tutti i tipi (Titolo/Data/Durata) ---
    QComboBox* m_typeCombo = nullptr;        // Evento / Riunione / Compito
    QLineEdit* m_titleEdit = nullptr;
    QDateTimeEdit* m_startEdit = nullptr;    // "Tutto il giorno" cambia solo il displayFormat
    QSpinBox* m_durationEdit = nullptr;      // minuti
    QFormLayout* m_commonForm = nullptr;     // per setRowVisible(m_durationEdit, ...)

    // --- Sezioni delegate ai widget figli -----------------------------------
    RecurrenceFormWidget* m_recurrence = nullptr;  // comune a tutti e 3 i tipi
    ActivityTypeWidget* m_eventSection = nullptr;    // nessun campo proprio, mai in layout
    ActivityTypeWidget* m_meetingSection = nullptr;
    ActivityTypeWidget* m_taskSection = nullptr;
    // Stesso ordine di kEvent/kMeeting/kTask.
    std::vector<ActivityTypeWidget*> m_typeWidgets;

    QPushButton* m_deleteButton = nullptr;   // Elimina (visibile solo in modifica)
    QLabel* m_errorLabel = nullptr;
};

} // namespace app
