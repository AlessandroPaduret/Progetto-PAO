#pragma once

#include <QDialog>

class QLabel;

namespace app {

/** @brief Dialog nativo che chiede se operare sull'INTERA serie di eventi
 *  ricorrenti, sulla SINGOLA occorrenza o "da questo momento in poi".
 *
 *  Non emette segnali propri: il risultato della scelta e' il valore di
 *  ritorno di `exec()`/`done()`, esposto tramite l'helper statico `ask()`
 *  (uso idiomatico di QDialog, niente slot da collegare per reagire).
 */
class RecurrenceChoiceDialog : public QDialog {
    Q_OBJECT
public:
    enum class Choice { Cancel, EntireSeries, FromHereOn, SingleInstance };

    explicit RecurrenceChoiceDialog(QWidget* parent = nullptr);

    /** @brief Istanzia il dialog, mostra `message` e lo esegue modale.
     *  Restituisce la scelta dell'utente, o Choice::Cancel se ha annullato
     *  o premuto Esc. */
    static Choice ask(QWidget* parent, const QString& message);

private:
    QLabel* m_messageLabel;
};

} // namespace app
