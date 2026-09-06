#include "views/MonthView.h"

#include <QDate>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QVBoxLayout>

#include <algorithm>

#include "views/OccurrenceWidget.h"
#include "views/ViewShared.h"

namespace app {

namespace {
constexpr int kMaxChipsPerDay = 3;
} // namespace

// ---------------------------------------------------------------------------
// MonthDayCell: una cella della griglia mensile. Widget reale (QFrame) che
// possiede il numero del giorno e fino a kMaxChipsPerDay chip (OccurrenceWidget,
// lo stesso usato da WeekView) per le attivita' del giorno.
// ---------------------------------------------------------------------------
class MonthDayCell : public QFrame {
    Q_OBJECT
public:
    explicit MonthDayCell(QWidget* parent = nullptr) : QFrame(parent) {
        setAttribute(Qt::WA_StyledBackground, true);

        m_dayLabel = new QLabel(this);

        m_chipsBox = new QWidget(this);
        m_chipsLayout = new QVBoxLayout(m_chipsBox);
        m_chipsLayout->setContentsMargins(0, 0, 0, 0);
        m_chipsLayout->setSpacing(2);

        m_moreLabel = new QLabel(this);
        m_moreLabel->setStyleSheet(
            QStringLiteral("color: #9aa0a6; background: transparent; border: none;"));
        QFont moreFont = m_moreLabel->font();
        moreFont.setPointSize(8);
        m_moreLabel->setFont(moreFont);
        m_moreLabel->hide();

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(3, 2, 3, 2);
        layout->setSpacing(2);
        layout->addWidget(m_dayLabel);
        layout->addWidget(m_chipsBox);
        layout->addWidget(m_moreLabel);
        layout->addStretch(1);
    }

    void setDate(const QDate& date, bool inMonth) {
        m_date = date;
        const bool isToday = date == QDate::currentDate();
        m_dayLabel->setText(QString::number(date.day()));
        QFont f = m_dayLabel->font();
        f.setBold(isToday);
        f.setPointSize(10);
        m_dayLabel->setFont(f);
        const QString color = isToday ? QStringLiteral("#1a73e8")
                                      : (inMonth ? QStringLiteral("#202124")
                                                 : QStringLiteral("#9aa0a6"));
        m_dayLabel->setStyleSheet(
            QStringLiteral("color: %1; background: transparent; border: none;").arg(color));
        setStyleSheet(QStringLiteral("app--MonthDayCell { background: %1;"
                                     " border: 1px solid #dadce0; }")
                          .arg(inMonth ? QStringLiteral("white") : QStringLiteral("#f8f9fa")));
        setToolTip(tr("Giorno del %1").arg(date.toString(QStringLiteral("dd/MM/yyyy"))));
    }

    void setOccurrences(const std::vector<events::Occurrence>& dayOccurrences) {
        // deleteLater(), non delete: per coerenza/robustezza con lo stesso
        // pattern usato in WeekView (dove e' necessario perche' un chip puo'
        // essere in mezzo al proprio QDrag::exec() quando questa viene
        // richiamata ricorsivamente da un cambiamento che lui stesso ha
        // causato). Qui i chip non sono trascinabili, ma non c'e' motivo di
        // rischiare la stessa classe di bug se in futuro lo diventassero.
        for (OccurrenceWidget* chip : m_chips) {
            chip->hide();
            chip->deleteLater();
        }
        m_chips.clear();

        std::vector<events::Occurrence> sorted = dayOccurrences;
        std::sort(sorted.begin(), sorted.end(),
                  [](const events::Occurrence& a, const events::Occurrence& b) {
                      return a.start < b.start;
                  });

        const int shown = std::min<int>(kMaxChipsPerDay, static_cast<int>(sorted.size()));
        for (int i = 0; i < shown; ++i) {
            const events::Occurrence& occ = sorted[i];
            auto* chip = new OccurrenceWidget(occ, OccurrenceWidget::Style::Chip,
                                              isRecurrent(occ.source),
                                              /*draggable=*/false, m_chipsBox);
            connect(chip, &OccurrenceWidget::pressed, this,
                    [this, chip](const events::Occurrence& o) { emit chipPressed(chip, o); });
            connect(chip, &OccurrenceWidget::doneToggled, this, &MonthDayCell::doneToggled);
            connect(chip, &OccurrenceWidget::infoRequested, this, &MonthDayCell::infoRequested);
            connect(chip, &OccurrenceWidget::editRequested,
                    this, &MonthDayCell::activityEditRequested);
            connect(chip, &OccurrenceWidget::modifyInstanceRequested,
                    this, &MonthDayCell::modifyEventRequested);
            connect(chip, &OccurrenceWidget::deleteRequested,
                    this, &MonthDayCell::deleteEventRequested);
            connect(chip, &OccurrenceWidget::doubleClicked, this,
                    [this](const events::Occurrence& o) {
                        if (isRecurrent(o.source) && o.start > o.source->getStart()) {
                            emit occurrenceEditChoiceRequested(o);
                        } else {
                            emit activityEditRequested(o);
                        }
                    });
            m_chipsLayout->addWidget(chip);
            m_chips.push_back(chip);
        }
        const int extra = static_cast<int>(sorted.size()) - shown;
        if (extra > 0) {
            m_moreLabel->setText(tr("+%1").arg(extra));
            m_moreLabel->show();
        } else {
            m_moreLabel->hide();
        }
    }

signals:
    void emptySlotClicked(const QDateTime& start);
    void activityEditRequested(const events::Occurrence& occurrence);
    void occurrenceEditChoiceRequested(const events::Occurrence& occurrence);
    void infoRequested(const events::Occurrence& occurrence);
    void modifyEventRequested(const events::Occurrence& occurrence);
    void deleteEventRequested(const events::Occurrence& occurrence);
    void doneToggled(const events::Occurrence& occurrence);
    void chipPressed(OccurrenceWidget* chip, const events::Occurrence& occurrence);

protected:
    void mouseDoubleClickEvent(QMouseEvent*) override {
        emit emptySlotClicked(QDateTime(m_date, QTime(9, 0)));
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::RightButton) {
            QMenu menu(this);
            QAction* createAction = menu.addAction(tr("Nuova attivita'..."));
            if (menu.exec(event->globalPosition().toPoint()) == createAction) {
                emit emptySlotClicked(QDateTime(m_date, QTime(9, 0)));
            }
            return;
        }
        QFrame::mousePressEvent(event);
    }

private:
    QDate m_date;
    QLabel* m_dayLabel;
    QWidget* m_chipsBox;
    QVBoxLayout* m_chipsLayout;
    QLabel* m_moreLabel;
    std::vector<OccurrenceWidget*> m_chips;
};

// ---------------------------------------------------------------------------
// MonthView
// ---------------------------------------------------------------------------
MonthView::MonthView(QWidget* parent) : QWidget(parent) {
    m_grid = new QGridLayout(this);
    m_grid->setSpacing(0);
    m_grid->setContentsMargins(0, 0, 0, 0);

    for (int d = 0; d < kCols; ++d) {
        auto* header = new QLabel(QString::fromLatin1(shortDayName(d + 1)), this);
        header->setAlignment(Qt::AlignCenter);
        header->setStyleSheet(QStringLiteral(
            "font-weight: bold; color: #5f6368; background: #f8f9fa;"
            " border-bottom: 1px solid #dadce0;"));
        m_grid->addWidget(header, 0, d);
        m_grid->setColumnStretch(d, 1);
    }
    for (int r = 0; r < kRows; ++r) {
        m_grid->setRowStretch(r + 1, 1);
        for (int c = 0; c < kCols; ++c) {
            auto* cell = new MonthDayCell(this);
            connect(cell, &MonthDayCell::emptySlotClicked, this, &MonthView::emptySlotClicked);
            connect(cell, &MonthDayCell::activityEditRequested,
                    this, &MonthView::activityEditRequested);
            connect(cell, &MonthDayCell::occurrenceEditChoiceRequested,
                    this, &MonthView::occurrenceEditChoiceRequested);
            connect(cell, &MonthDayCell::infoRequested, this, &MonthView::infoRequested);
            connect(cell, &MonthDayCell::modifyEventRequested,
                    this, &MonthView::modifyEventRequested);
            connect(cell, &MonthDayCell::deleteEventRequested,
                    this, &MonthView::deleteEventRequested);
            connect(cell, &MonthDayCell::doneToggled, this, &MonthView::doneToggled);
            connect(cell, &MonthDayCell::chipPressed, this, &MonthView::setSelectedChip);
            m_grid->addWidget(cell, r + 1, c);
            m_cells[r * kCols + c] = cell;
        }
    }
    setMinimumSize(baseWidth(), baseHeight());
}

int MonthView::baseWidth() const {
    return kCols * 100;
}

int MonthView::baseHeight() const {
    return 20 + kRows * 90;
}

QDate MonthView::gridStart() const {
    const QDate first(m_month.year(), m_month.month(), 1);
    return first.addDays(1 - first.dayOfWeek());
}

void MonthView::setMonth(const QDate& firstOfMonth) {
    m_month = firstOfMonth;
    const QDate start = gridStart();
    for (int i = 0; i < kRows * kCols; ++i) {
        const QDate date = start.addDays(i);
        m_cells[i]->setDate(date, date.month() == m_month.month() && date.year() == m_month.year());
    }
}

void MonthView::setOccurrences(const std::vector<events::Occurrence>& occurrences) {
    m_selectedChip = nullptr;
    m_selectedOccurrence.reset();

    const QDate start = gridStart();
    for (int i = 0; i < kRows * kCols; ++i) {
        const QDate date = start.addDays(i);
        std::vector<events::Occurrence> dayOccurrences;
        for (const events::Occurrence& occ : occurrences) {
            if (localTime(occ.start).date() == date) {
                dayOccurrences.push_back(occ);
            }
        }
        m_cells[i]->setOccurrences(dayOccurrences);
    }
}

void MonthView::setSelectedChip(OccurrenceWidget* chip, const events::Occurrence& occurrence) {
    if (m_selectedChip) {
        m_selectedChip->setSelected(false);
    }
    m_selectedChip = chip;
    m_selectedOccurrence = occurrence;
    chip->setSelected(true);
}

const events::Occurrence* MonthView::selectedOccurrence() const {
    return m_selectedOccurrence ? &(*m_selectedOccurrence) : nullptr;
}

} // namespace app

#include "MonthView.moc"
