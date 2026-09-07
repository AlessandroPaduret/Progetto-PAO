#include "dialog/RecurrenceFormWidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

namespace app {

namespace {

// Nomi brevi dei giorni della settimana: indice 0 = Lunedi', coerente con
// l'id 1..7 assegnato ai pulsanti di m_dayGroup (QDate::dayOfWeek()).
constexpr std::array<const char*, 7> kDayLabels = {
    "Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};

QDateEdit* makeDay(QWidget* parent) {
    auto* edit = new QDateEdit(QDate::currentDate(), parent);
    edit->setCalendarPopup(true);
    edit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    return edit;
}

} // namespace

RecurrenceFormWidget::RecurrenceFormWidget(QWidget* parent) : QWidget(parent) {
    m_allDayCheck = new QCheckBox(tr("Tutto il giorno"), this);
    m_repeatCheck = new QCheckBox(tr("Si ripete"), this);

    auto* checksRow = new QHBoxLayout;
    checksRow->addWidget(m_allDayCheck);
    checksRow->addWidget(m_repeatCheck);
    checksRow->addStretch(1);

    // --- Sotto-pannello di ricorrenza (visibile se "Si ripete") ------------
    m_repeatBox = new QWidget(this);
    m_repeatBox->setVisible(false);

    m_unitCombo = new QComboBox(m_repeatBox);
    m_unitCombo->addItem(tr("giorni"));
    m_unitCombo->addItem(tr("settimane"));
    m_unitCombo->addItem(tr("mesi"));
    m_unitCombo->addItem(tr("anni"));

    m_everySpin = new QSpinBox(m_repeatBox);
    m_everySpin->setRange(1, 3650);
    m_everySpin->setValue(1);
    m_everySpin->setSuffix(tr(" giorni"));

    // Giorni della settimana: QButtonGroup NON esclusivo (piu' giorni
    // selezionabili insieme), id del pulsante = giorno Qt (1=Lun..7=Dom,
    // QDate::dayOfWeek()): elimina la necessita' di scandire a mano una
    // lista di pulsanti per sapere "quale" giorno rappresentano.
    m_dayRow = new QWidget(m_repeatBox);
    auto* dayLayout = new QHBoxLayout(m_dayRow);
    dayLayout->setContentsMargins(0, 0, 0, 0);
    m_dayGroup = new QButtonGroup(m_dayRow);
    m_dayGroup->setExclusive(false);
    for (int i = 0; i < 7; ++i) {
        auto* button = new QPushButton(QString::fromLatin1(kDayLabels[i]), m_dayRow);
        button->setCheckable(true);
        button->setMinimumWidth(36);
        dayLayout->addWidget(button);
        m_dayGroup->addButton(button, i + 1);
    }

    // Fine della ricorrenza: mai / fino a / dopo N occorrenze. Ogni radio
    // fa da "etichetta" della propria riga (QFormLayout::addRow accetta
    // qualunque QWidget come label, non solo QLabel).
    auto* endGroup = new QGroupBox(tr("Fine"), m_repeatBox);
    m_endNever = new QRadioButton(tr("Mai"), endGroup);
    m_endDateRadio = new QRadioButton(tr("Fino al"), endGroup);
    m_endDate = makeDay(endGroup);
    m_endDate->setEnabled(false);
    m_endCountRadio = new QRadioButton(tr("Dopo"), endGroup);
    m_countSpin = new QSpinBox(endGroup);
    m_countSpin->setRange(1, 10000);
    m_countSpin->setValue(5);
    m_countSpin->setSuffix(tr(" occorrenze"));
    m_countSpin->setEnabled(false);
    auto* endForm = new QFormLayout(endGroup);
    endForm->addRow(m_endNever);
    endForm->addRow(m_endDateRadio, m_endDate);
    endForm->addRow(m_endCountRadio, m_countSpin);

    m_repeatForm = new QFormLayout(m_repeatBox);
    m_repeatForm->setContentsMargins(0, 0, 0, 0);
    m_repeatForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_repeatForm->addRow(tr("Unita'"), m_unitCombo);
    m_repeatForm->addRow(tr("Ogni"), m_everySpin);
    m_repeatForm->addRow(tr("Giorni"), m_dayRow);
    m_repeatForm->addRow(endGroup);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(checksRow);
    layout->addWidget(m_repeatBox);

    connect(m_allDayCheck, &QCheckBox::toggled, this, &RecurrenceFormWidget::allDayToggled);
    connect(m_allDayCheck, &QCheckBox::toggled, this, &RecurrenceFormWidget::changed);
    connect(m_repeatCheck, &QCheckBox::toggled, this, &RecurrenceFormWidget::onRepeatToggled);
    connect(m_unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RecurrenceFormWidget::onUnitChanged);
    connect(m_everySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RecurrenceFormWidget::changed);
    for (QAbstractButton* button : m_dayGroup->buttons()) {
        connect(button, &QAbstractButton::toggled, this, &RecurrenceFormWidget::changed);
    }
    connect(m_endDateRadio, &QRadioButton::toggled, this, [this](bool on) {
        m_endDate->setEnabled(on);
        emit changed();
    });
    connect(m_endCountRadio, &QRadioButton::toggled, this, [this](bool on) {
        m_countSpin->setEnabled(on);
        emit changed();
    });
    connect(m_endDate, &QDateEdit::dateChanged, this, &RecurrenceFormWidget::changed);
    connect(m_countSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RecurrenceFormWidget::changed);
}

bool RecurrenceFormWidget::isAllDay() const { return m_allDayCheck->isChecked(); }
bool RecurrenceFormWidget::isRepeating() const { return m_repeatCheck->isChecked(); }

RecurrenceFormWidget::Unit RecurrenceFormWidget::unit() const {
    return static_cast<Unit>(m_unitCombo->currentIndex());
}

int RecurrenceFormWidget::every() const { return m_everySpin->value(); }

std::vector<int> RecurrenceFormWidget::selectedWeekdays() const {
    std::vector<int> days;
    for (QAbstractButton* button : m_dayGroup->buttons()) {
        if (button->isChecked()) {
            days.push_back(m_dayGroup->id(button));
        }
    }
    return days;
}

RecurrenceFormWidget::EndMode RecurrenceFormWidget::endMode() const {
    if (m_endDateRadio->isChecked()) return EndMode::OnDate;
    if (m_endCountRadio->isChecked()) return EndMode::AfterCount;
    return EndMode::Never;
}

QDate RecurrenceFormWidget::endDate() const { return m_endDate->date(); }
int RecurrenceFormWidget::endCount() const { return m_countSpin->value(); }

void RecurrenceFormWidget::setAllDay(bool on) { m_allDayCheck->setChecked(on); }

void RecurrenceFormWidget::setRepeating(bool on) {
    m_repeatCheck->setChecked(on);
    m_repeatBox->setVisible(on);
}

void RecurrenceFormWidget::setUnit(Unit unit) {
    m_unitCombo->setCurrentIndex(static_cast<int>(unit));
}

void RecurrenceFormWidget::setEvery(int value) { m_everySpin->setValue(value); }

void RecurrenceFormWidget::setSelectedWeekdays(const std::vector<int>& days) {
    for (QAbstractButton* button : m_dayGroup->buttons()) {
        button->setChecked(std::ranges::find(days, m_dayGroup->id(button)) != days.end());
    }
}

void RecurrenceFormWidget::setEndNever() { m_endNever->setChecked(true); }

void RecurrenceFormWidget::setEndOnDate(const QDate& date) {
    m_endDateRadio->setChecked(true);
    m_endDate->setDate(date);
}

void RecurrenceFormWidget::setEndAfterCount(int count) {
    m_endCountRadio->setChecked(true);
    m_countSpin->setValue(count);
}

void RecurrenceFormWidget::resetToDefaults() {
    m_allDayCheck->setChecked(false);
    m_allDayCheck->setEnabled(true);
    m_repeatCheck->setChecked(false);
    m_repeatCheck->setEnabled(true);
    m_repeatBox->setVisible(false);
    m_unitCombo->setCurrentIndex(Days);
    m_everySpin->setValue(1);
    for (QAbstractButton* button : m_dayGroup->buttons()) {
        button->setChecked(false);
    }
    m_endNever->setChecked(true);
    m_countSpin->setValue(5);
    m_endDate->setDate(QDate::currentDate());
}

void RecurrenceFormWidget::setReferenceDate(const QDate& date) {
    m_referenceDate = date;
    m_endDate->setDate(date);
}

void RecurrenceFormWidget::onRepeatToggled(bool checked) {
    m_repeatBox->setVisible(checked);
    if (checked && m_unitCombo->currentIndex() == Weeks) {
        preselectWeekdayIfNone();
    }
    emit changed();
}

void RecurrenceFormWidget::onUnitChanged(int index) {
    const bool weeks = index == Weeks;
    m_repeatForm->setRowVisible(m_dayRow, weeks);
    switch (index) {
    case Days:
        m_everySpin->setSuffix(tr(" giorni"));
        break;
    case Weeks:
        m_everySpin->setSuffix(tr(" settimane"));
        break;
    case Months:
        m_everySpin->setSuffix(tr(" mesi"));
        break;
    default:
        m_everySpin->setSuffix(tr(" anni"));
        break;
    }
    if (weeks) {
        preselectWeekdayIfNone();
    }
    emit changed();
}

void RecurrenceFormWidget::preselectWeekdayIfNone() {
    if (!m_referenceDate.isValid()) {
        return;
    }
    if (!std::ranges::any_of(m_dayGroup->buttons(), &QAbstractButton::isChecked)) {
        m_dayGroup->button(m_referenceDate.dayOfWeek())->setChecked(true);
    }
}

} // namespace app
