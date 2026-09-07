#pragma once

#include <QWidget>

class QLabel;

namespace app {

/** @brief Barra di navigazione condivisa dalle viste temporali (Giorno,
 *  Settimana, Mese, Anno): pulsanti "Oggi" / "<-" / "->" piu' un'etichetta
 *  col periodo corrente (testo impostato dall'esterno, la barra non sa nulla
 *  di date o di quale vista sia attiva).
 *
 *  Nessuna logica propria: emette solo le richieste dell'utente
 *  (`todayRequested`/`previousRequested`/`nextRequested`), la MainWindow
 *  decide come tradurle in spostamenti del riferimento temporale (il passo
 *  dipende dalla vista attiva: giorno/settimana/mese/anno).
 */
class NavigationBar : public QWidget {
    Q_OBJECT
public:
    explicit NavigationBar(QWidget* parent = nullptr);

    /** @brief Imposta il testo del periodo corrente (es. "Settimana del ..."). */
    void setLabel(const QString& text);

signals:
    void todayRequested();
    void previousRequested();
    void nextRequested();

private:
    QLabel* m_label;
};

} // namespace app
