#pragma once

#include <QWidget>

class QLabel;

namespace app {

/** @brief Barra condivisa dalle viste temporali: Oggi/<-/-> + etichetta
 *  periodo (testo impostato dall'esterno). Non sa nulla di date o vista
 *  attiva: emette solo le richieste, la MainWindow decide il passo. */
class NavigationBar : public QWidget {
    Q_OBJECT
public:
    explicit NavigationBar(QWidget* parent = nullptr);

    void setLabel(const QString& text);

signals:
    void todayRequested();
    void previousRequested();
    void nextRequested();

private:
    QLabel* m_label;
};

} // namespace app
