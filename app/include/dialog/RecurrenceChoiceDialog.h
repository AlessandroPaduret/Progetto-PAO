#pragma once

#include <QDialog>

class QLabel;

namespace app {

/** @brief Chiede se operare sull'INTERA serie ricorrente, sulla SINGOLA
 *  occorrenza o "da questo momento in poi". Niente segnali propri: il
 *  risultato e' il valore di ritorno di exec()/done(), esposto dall'helper
 *  statico ask(). */
class RecurrenceChoiceDialog : public QDialog {
    Q_OBJECT
public:
    enum class Choice { Cancel, EntireSeries, FromHereOn, SingleInstance };

    explicit RecurrenceChoiceDialog(QWidget* parent = nullptr);

    /** @return Choice::Cancel se l'utente annulla o preme Esc. */
    static Choice ask(QWidget* parent, const QString& message);

private:
    QLabel* m_messageLabel;
};

} // namespace app
