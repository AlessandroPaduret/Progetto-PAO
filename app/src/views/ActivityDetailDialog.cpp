#include "views/ActivityDetailDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include "CalendarController.h"
#include "views/ActivityViewHelpers.h"

namespace app {

ActivityDetailDialog::ActivityDetailDialog(CalendarController* controller,
                                           QWidget* parent)
    : QFrame(parent), m_controller(controller) {
    setObjectName(QStringLiteral("detailDialog"));
    setStyleSheet(QStringLiteral(
        "#detailDialog { background: palette(base);"
        " border: 1px solid palette(mid); }"));
    // Pannello ridotto: piu' piccolo della finestra principale; la
    // dimensione viene adattata al contenitore in showCentered().
    resize(520, 420);

    // Nascosto all'avvio: si apre solo su richiesta dell'utente
    // (doppio clic su una riga dell'elenco o menu contestuale delle griglie).
    hide();

    // Intestazione del pannello con la "X" di chiusura in alto a destra.
    // Un margine a sinistra pari alla larghezza della "X" tiene il titolo
    // esattamente centrato rispetto alla finestra.
    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_closeButton = new QToolButton(this);
    m_closeButton->setText(QStringLiteral("\u2715"));
    m_closeButton->setAutoRaise(true);
    m_closeButton->setToolTip(tr("Chiudi"));
    m_closeButton->setIconSize(QSize(20, 20));
    m_closeButton->setFixedSize(28, 28);

    auto* titleBar = new QHBoxLayout;
    titleBar->addSpacing(28);                 // compensa la larghezza della X
    titleBar->addWidget(m_titleLabel, 1);
    titleBar->addWidget(m_closeButton);

    // Campi specifici per tipo (Visitor), centrati e in una scrollarea
    // perche' il testo non esca mai dal pannello
    m_fieldsLabel = new QLabel(this);
    m_fieldsLabel->setWordWrap(true);
    m_fieldsLabel->setAlignment(Qt::AlignCenter);
    m_fieldsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont fieldsFont = m_fieldsLabel->font();
    fieldsFont.setPointSize(13);
    m_fieldsLabel->setFont(fieldsFont);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_fieldsLabel);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* modifyButton = new QPushButton(tr("Modifica"), this);
    auto* deleteButton = new QPushButton(tr("Elimina"), this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 12, 10, 10);
    layout->addSpacing(36);              // spinge il titolo verso il centro
    layout->addLayout(titleBar);
    layout->addWidget(scroll, 1);
    auto* bottom = new QHBoxLayout;
    bottom->addStretch(1);
    bottom->addWidget(deleteButton);
    bottom->addWidget(modifyButton);
    layout->addLayout(bottom);

    connect(m_closeButton, &QToolButton::clicked, this, [this] {
        hide();
        emit closed();
    });
    connect(modifyButton, &QPushButton::clicked, this,
            &ActivityDetailDialog::onEdit);
    connect(deleteButton, &QPushButton::clicked, this,
            &ActivityDetailDialog::onDelete);
}

void ActivityDetailDialog::showActivity(const events::Activity* activity) {
    m_activity = activity;
    m_titleLabel->setText(QString::fromStdString(activity->getTitle()));
    m_fieldsLabel->setText(
        ActivityViewHelpers::fieldLines(*activity).join(QLatin1Char('\n')));
}

void ActivityDetailDialog::showCentered() {
    if (QWidget* host = parentWidget()) {
        // Mai piu' grande del contenitore: il pannello non esce mai
        // dai bordi della finestra principale, qualunque sia la sua size.
        const QSize size = this->size().boundedTo(host->size());
        resize(size);
        const QPoint center = host->rect().center();
        // Clamp esplicito: posizione sempre entro [0, host-size] anche con
        // il centraggio intero che puo' sbilanciare di qualche pixel.
        const int x = qBound(0, center.x() - size.width() / 2,
                             host->width() - size.width());
        const int y = qBound(0, center.y() - size.height() / 2,
                             host->height() - size.height());
        move(x, y);
    }
    show();
    raise();
}

void ActivityDetailDialog::onEdit() {
    const events::Activity* activity = m_activity;
    hide();
    emit closed();
    if (activity) {
        emit editRequested(activity);
    }
}

void ActivityDetailDialog::onDelete() {
    if (!m_activity) {
        return;
    }
    if (QMessageBox::question(this, tr("Elimina attivita'"),
                              tr("Eliminare '%1'?").arg(
                                  QString::fromStdString(m_activity->getTitle()))) !=
        QMessageBox::Yes) {
        return;
    }
    m_controller->removeActivity(m_activity);
    m_activity = nullptr;
    hide();
    emit closed();
}

} // namespace app