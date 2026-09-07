#pragma once

#include <QDateTime>
#include <QFrame>

#include "events.h"

class QLabel;
class QToolButton;

namespace app {

class ActivityFormPage;
class CalendarController;

/** @brief Schermata di creazione/modifica di un'attivita': un pannello
 *  ridotto mostrato DENTRO la finestra principale (widget figlio della
 *  MainWindow), con barra del titolo e pulsante di chiusura "X".
 *
 *  Essendo un widget figlio, non puo' uscire dalla finestra principale
 *  (e' clipato ai suoi bordi); viene ri-centrato automaticamente anche
 *  quando la finestra viene ridimensionata.
 */
class ActivityFormDialog : public QFrame {
    Q_OBJECT
public:
    explicit ActivityFormDialog(CalendarController* controller, QWidget* parent = nullptr);

    /** @brief Avvia la creazione (data/ora suggerita, es. dal doppio clic). */
    void startCreate(const QDateTime& suggestedStart = QDateTime());

    /** @brief Avvia la creazione preselezionando il tipo (0=Evento,
     *  1=Riunione, 2=Compito, 3=Anniversario), dal menu "Nuova attivita'". */
    void startCreateType(int typeIndex, const QDateTime& suggestedStart = QDateTime());

    /** @brief Avvia la modifica di un'attivita' esistente. */
    void startEditActivity(const events::Activity* activity);

    /** @brief Avvia la modifica di una singola occorrenza. */
    void startEditOccurrence(const events::Occurrence& occurrence);

    /** @brief Centra il pannello nella finestra principale e lo mostra. */
    void showCentered();

signals:
    /** @brief Inoltra l'anteprima del form (aggiornata a ogni modifica). */
    void previewChanged(const QString& title, const QDateTime& start,
                        qint64 durationSeconds, bool valid);
    /** @brief Il pannello si e' chiuso (Salva/Annulla/Elimina). */
    void closed();

private:
    ActivityFormPage* m_page;
    QLabel* m_titleLabel;
};

} // namespace app
