#include "views/dialog/ActivityDetailDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "controller/CalendarController.h"
#include "views/dialog/ActivityViewHelpers.h"

namespace app {

ActivityDetailDialog::ActivityDetailDialog(CalendarController* controller,
                                           QWidget* parent)
    : QDialog(parent), m_controller(controller) {
    resize(480, 420);

    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // Campi specifici per tipo (Visitor), in una scrollarea perche' il
    // testo non esca mai dal dialog
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

    // "Chiudi" segue l'ordine nativo della piattaforma; Modifica/Elimina
    // sono azioni proprie del dialog.
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto* modifyButton = buttonBox->addButton(tr("Modifica"), QDialogButtonBox::ActionRole);
    auto* deleteButton = buttonBox->addButton(tr("Elimina"), QDialogButtonBox::DestructiveRole);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_titleLabel);
    layout->addWidget(scroll, 1);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(modifyButton, &QPushButton::clicked, this, &ActivityDetailDialog::onEdit);
    connect(deleteButton, &QPushButton::clicked, this, &ActivityDetailDialog::onDelete);
}

void ActivityDetailDialog::showActivity(const events::Activity* activity) {
    m_activity = activity;
    m_titleLabel->setText(QString::fromStdString(activity->getTitle()));
    // Righe "campo: valore" come paragrafi distanziati: il dialog si
    // riempie in modo arioso e le informazioni si leggono con respiro.
    const QStringList lines = ActivityViewHelpers::fieldLines(*activity);
    QString html;
    for (const QString& line : lines) {
        html += QStringLiteral("<p style=\"margin: 12px 0;\">%1</p>")
                    .arg(line.toHtmlEscaped());
    }
    m_fieldsLabel->setText(html);
    m_fieldsLabel->setTextFormat(Qt::RichText);
}

void ActivityDetailDialog::onEdit() {
    const events::Activity* activity = m_activity;
    accept();
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
    accept();
}

} // namespace app
