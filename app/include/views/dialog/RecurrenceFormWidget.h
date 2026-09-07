#pragma once

#include <QDate>
#include <QWidget>

#include <vector>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QFormLayout;
class QPushButton;
class QRadioButton;
class QSpinBox;

namespace app {

/** @brief Sezione "Tutto il giorno" / "Si ripete", comune a Evento, Riunione
 *  e Compito: estratta da ActivitySidebarWidget in un widget figlio
 *  autonomo, che non conosce ne' il tipo di attivita' ne' il modello
 *  (`events::`) — espone solo lo stato scelto dall'utente tramite getter/
 *  setter e un unico segnale `changed()`, lasciando all'ActivitySidebarWidget
 *  la traduzione da/verso `events::DateGenerator`. */
class RecurrenceFormWidget : public QWidget {
    Q_OBJECT
public:
    enum Unit { Days = 0, Weeks = 1, Months = 2, Years = 3 };
    enum class EndMode { Never, OnDate, AfterCount };

    explicit RecurrenceFormWidget(QWidget* parent = nullptr);

    bool isAllDay() const;
    bool isRepeating() const;
    Unit unit() const;
    int every() const;
    /** @brief Giorni della settimana selezionati (id Qt, 1=Lun..7=Dom,
     *  QDate::dayOfWeek()); vuoto se nessuno (unita' diversa da settimane, o
     *  nessun pulsante ancora premuto). */
    std::vector<int> selectedWeekdays() const;
    EndMode endMode() const;
    QDate endDate() const;
    int endCount() const;

    void setAllDay(bool on);
    void setRepeating(bool on);
    void setUnit(Unit unit);
    void setEvery(int value);
    void setSelectedWeekdays(const std::vector<int>& days);
    void setEndNever();
    void setEndOnDate(const QDate& date);
    void setEndAfterCount(int count);

    /** @brief Riporta il pannello ai valori di default (nuova attivita'). */
    void resetToDefaults();

    /** @brief Giorno di inizio dell'attivita' (campo comune, fuori da questo
     *  widget): usato per pre-selezionare il pulsante del giorno quando si
     *  attiva "Si ripete" con unita' settimane e nessun giorno e' ancora
     *  selezionato. Va richiamato dal genitore ad ogni cambio della data. */
    void setReferenceDate(const QDate& date);

signals:
    /** @brief "Tutto il giorno" e' stato attivato/disattivato: il genitore
     *  reagisce nascondendo/mostrando i propri campi Data/Durata. */
    void allDayToggled(bool on);

    /** @brief Qualunque altro campo e' cambiato (utile per l'anteprima). */
    void changed();

private slots:
    void onRepeatToggled(bool checked);
    void onUnitChanged(int index);

private:
    void preselectWeekdayIfNone();

    QCheckBox* m_allDayCheck = nullptr;
    QCheckBox* m_repeatCheck = nullptr;
    QWidget* m_repeatBox = nullptr;
    QFormLayout* m_repeatForm = nullptr;
    QComboBox* m_unitCombo = nullptr;
    QSpinBox* m_everySpin = nullptr;
    QWidget* m_dayRow = nullptr;
    QButtonGroup* m_dayGroup = nullptr;
    QRadioButton* m_endNever = nullptr;
    QRadioButton* m_endDateRadio = nullptr;
    QDateEdit* m_endDate = nullptr;
    QRadioButton* m_endCountRadio = nullptr;
    QSpinBox* m_countSpin = nullptr;

    QDate m_referenceDate;
};

} // namespace app
