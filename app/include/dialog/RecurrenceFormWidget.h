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
 *  e Compito: non conosce ne' il tipo di attivita' ne' il modello (events::),
 *  espone solo lo stato scelto dall'utente (getter/setter + changed()) e
 *  lascia la traduzione da/verso events::DateGenerator alla sidebar. */
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
    /** @brief Id Qt (1=Lun..7=Dom, QDate::dayOfWeek()); vuoto se nessuno selezionato. */
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

    void resetToDefaults();

    /** @brief Data di inizio (campo esterno): usata per pre-selezionare il
     *  giorno quando si attiva "Si ripete" a settimane senza selezione. Il
     *  genitore la richiama ad ogni cambio data. */
    void setReferenceDate(const QDate& date);

signals:
    /** @brief Il genitore reagisce nascondendo/mostrando i campi Data/Durata. */
    void allDayToggled(bool on);

    /** @brief Qualunque altro campo e' cambiato (per l'anteprima live). */
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
