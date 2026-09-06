#include "views/NavigationBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace app {

NavigationBar::NavigationBar(QWidget* parent) : QWidget(parent) {
    auto* todayButton = new QPushButton(tr("Oggi"), this);
    auto* prevButton = new QPushButton(tr("\u2190"), this);
    auto* nextButton = new QPushButton(tr("\u2192"), this);
    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->addWidget(todayButton);
    layout->addWidget(prevButton);
    layout->addWidget(nextButton);
    layout->addWidget(m_label, 1);

    connect(todayButton, &QPushButton::clicked, this, &NavigationBar::todayRequested);
    connect(prevButton, &QPushButton::clicked, this, &NavigationBar::previousRequested);
    connect(nextButton, &QPushButton::clicked, this, &NavigationBar::nextRequested);
}

void NavigationBar::setLabel(const QString& text) {
    m_label->setText(text);
}

} // namespace app
