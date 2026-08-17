#ifndef APP_RECURRENCE_CHOICE_DIALOG_H
#define APP_RECURRENCE_CHOICE_DIALOG_H

#include <QFrame>

class QLabel;
class QPushButton;

namespace app {

/** @brief Finestra interna alla MainWindow che chiede se operare sull'INTERA
 *  serie di eventi ricorrenti o sulla SINGOLA occorrenza.
 *
 *  E' un widget figlio della MainWindow: essendo un child non puo' uscire dai
 *  suoi bordi (dimensioni limitate con boundedTo e posizione centrata).
 *  Non fa nulla da sola: emette `seriesChosen` o `instanceChosen`.
 */
class RecurrenceChoiceDialog : public QFrame {
    Q_OBJECT
public:
    explicit RecurrenceChoiceDialog(QWidget* parent = nullptr);

    /** @brief Imposta il messaggio e mostrare il pannello centrato. */
    void ask(const QString& text);

    /** @brief Centra il pannello nella finestra principale e lo mostra. */
    void showCentered();

signals:
    /** @brief L'utente vuole modificare l'intera serie. */
    void seriesChosen();
    /** @brief L'utente vuole modificare solo questa occorrenza. */
    void instanceChosen();
    /** @brief "Da questo momento in poi": la serie attuale termina qui e ne
     *  nasce una nuova con le stesse regole ma inizio diverso. */
    void splitChosen();

private:
    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
};

} // namespace app

#endif // APP_RECURRENCE_CHOICE_DIALOG_H
