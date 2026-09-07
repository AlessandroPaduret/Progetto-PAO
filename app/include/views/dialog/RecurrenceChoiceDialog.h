#pragma once

#include <QDialog>

class QLabel;

namespace app {

/** @brief Dialog nativo che chiede se operare sull'INTERA serie di eventi
 *  ricorrenti, sulla SINGOLA occorrenza o "da questo momento in poi".
 *
 *  Non fa nulla da sola: emette `seriesChosen`, `instanceChosen` o
 *  `splitChosen`. Apertura/centraggio/modalita' sono gestiti nativamente
 *  da Qt tramite exec().
 */
class RecurrenceChoiceDialog : public QDialog {
    Q_OBJECT
public:
    explicit RecurrenceChoiceDialog(QWidget* parent = nullptr);

    /** @brief Imposta il messaggio da mostrare. */
    void ask(const QString& text);

signals:
    /** @brief L'utente vuole modificare l'intera serie. */
    void seriesChosen();
    /** @brief L'utente vuole modificare solo questa occorrenza. */
    void instanceChosen();
    /** @brief "Da questo momento in poi": la serie attuale termina qui e ne
     *  nasce una nuova con le stesse regole ma inizio diverso. */
    void splitChosen();

private:
    QLabel* m_messageLabel;
};

} // namespace app
