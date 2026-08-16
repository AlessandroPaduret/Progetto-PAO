#include "views/ActivityFormDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

#include "CalendarController.h"
#include "views/ActivityFormPage.h"

namespace app {

ActivityFormDialog::ActivityFormDialog(CalendarController* controller,
                                       QWidget* parent)
    : QFrame(parent) {
    setObjectName(QStringLiteral("formDialog"));
    setStyleSheet(QStringLiteral(
        "#formDialog { background: palette(base);"
        " border: 1px solid palette(mid); }"));
    // Pannello ridotto: piu' piccolo della finestra principale; la
    // dimensione viene adattata al contenitore in showCentered().
    resize(460, 620);

    // Nascosto all'avvio: si apre solo su richiesta dell'utente
    // (pulsante "Nuova attivita'", doppio clic, Modifica).
    hide();

    // Intestazione del pannello (senza pulsante di chiusura: si chiude
    // con Salva/Annulla)
    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    auto* titleBar = new QHBoxLayout;
    titleBar->addWidget(m_titleLabel);
    titleBar->addStretch(1);

    // Il form sta in una scroll area: anche con finestre molto piccole
    // il contenuto resta raggiungibile e il pannello non esce mai dai bordi.
    m_page = new ActivityFormPage(controller, this);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_page);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 8);
    layout->addLayout(titleBar);
    layout->addWidget(scroll, 1);

    // Salva/Annulla (dal form) chiudono il pannello
    connect(m_page, &ActivityFormPage::backRequested, this, &QWidget::hide);
}

void ActivityFormDialog::startCreate(const QDateTime& suggestedStart) {
    m_titleLabel->setText(tr("Nuova attivita'"));
    m_page->startCreate(suggestedStart);
}

void ActivityFormDialog::startEditActivity(const events::Activity* activity) {
    m_titleLabel->setText(tr("Modifica attivita'"));
    m_page->startEditActivity(activity);
}

void ActivityFormDialog::startEditOccurrence(const events::Occurrence& occurrence) {
    m_titleLabel->setText(tr("Modifica attivita'"));
    m_page->startEditOccurrence(occurrence);
}

void ActivityFormDialog::showCentered() {
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

} // namespace app
