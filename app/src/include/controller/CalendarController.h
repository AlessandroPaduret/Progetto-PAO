#ifndef APP_CALENDAR_CONTROLLER_H
#define APP_CALENDAR_CONTROLLER_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>

#include <memory>
#include <vector>

#include "events/events.h"

namespace app {

/**
 * @brief Controller MVC: possiede il calendario logico (events::Calendar)
 *        e applica le operazioni richieste dalla GUI, emettendo
 *        `activitiesChanged` dopo ogni modifica (le viste si aggiornano).
 *
 * La GUI non tocca mai direttamente il modello: passa sempre da qui.
 */
class CalendarController : public QObject {
    Q_OBJECT
public:
    explicit CalendarController(QObject* parent = nullptr);

    /** @return Il calendario logico (solo lettura per le viste) */
    const events::Calendar& calendar() const;

    // --- CRUD sulle attivita' -----------------------------------------------

    /** @brief Aggiunge un'attivita' al calendario (ne acquisisce la proprieta') */
    bool addActivity(std::unique_ptr<events::Activity> activity);

    /** @brief Aggiunge piu' attivita' in un colpo solo (es. piu' serie
     *  ricorrenti, una per giorno della settimana). Emette un unico
     *  activitiesChanged. */
    bool addActivities(std::vector<std::unique_ptr<events::Activity>> activities);

    /** @brief Rimuove l'attivita' identificata dal puntatore */
    bool removeActivity(const events::Activity* activity);

    /** @brief Sostituisce un'attivita' con una nuova (le eccezioni vengono
     *         conservate, se accettate dal nuovo generatore) */
    bool updateActivity(const events::Activity* oldActivity,
                        std::unique_ptr<events::Activity> replacement);

    /** @brief Sposta un'attivita' al nuovo istante (drag&drop nella settimana).
     *  @param activity L'attivita' da spostare
     *  @param newStart Nuovo istante di inizio/riferimento (data/ora scelta)
     *  @return true se lo spostamento e' riuscito
     */
    bool moveActivity(const events::Activity* activity, const QDateTime& newStart);

    /** @brief "Da questo momento in poi": ferma la serie attuale prima
     *  dell'occorrenza indicata e ne crea una nuova con le stesse regole di
     *  ricorrenza (stesso intervallo, durata dell'occorrenza) ma inizio
     *  diverso; la data di scadenza rimane invariata.
     *  @param occurrence L'occorrenza da cui la serie attuale termina
     *  @param newStart Nuovo inizio della serie (es. destinazione del drag)
     *  @return true se la divisione e' riuscita
     */
    bool splitRecurrence(const events::Occurrence& occurrence,
                         const QDateTime& newStart);

    // --- Query --------------------------------------------------------------

    /** @brief Ricerca per titolo (case-insensitive); vuoto = tutte */
    QVector<const events::Activity*> search(const QString& needle) const;

    /** @brief Occorrenze nell'intervallo [from, to] (QDateTime UTC) */
    std::vector<events::Occurrence> occurrencesIn(const QDateTime& fromUtc,
                                                  const QDateTime& toUtc) const;

    // --- Azioni sulle occorrenze --------------------------------------------

    /** @brief Elimina un'occorrenza.
     *  Se proviene da un'attivita' ricorrente aggiunge un'eccezione (EXDATE),
     *  se andFollowing e' true tronca la ricorrenza ("questa e le successive");
     *  altrimenti rimuove l'attivita' singola.
     */
    bool deleteOccurrence(const events::Occurrence& occurrence, bool andFollowing = false);

    /** @brief Modifica una singola istanza: l'originale viene escluso
     *  (eccezione interna se ricorrente) e sostituito da un evento singolo. */
    bool modifyOccurrence(const events::Occurrence& occurrence,
                          std::unique_ptr<events::Activity> replacement);

    /** @brief Inverte lo stato di completamento di un COMPITO (l'unico tipo
     *  con stato "evaso/da fare"). Non ha effetto sugli altri tipi. */
    bool toggleDone(const events::Occurrence& occurrence);

    // --- Persistenza --------------------------------------------------------

    /** @brief Salva il calendario su file JSON (percorso da QFileDialog) */
    bool saveToFile(const QString& filePath, QString* error = nullptr);

    /** @brief Carica il calendario da file JSON (sostituisce il contenuto) */
    bool loadFromFile(const QString& filePath, QString* error = nullptr);

signals:
    /** @brief Emesso dopo ogni operazione che modifica il calendario */
    void activitiesChanged();

private:
    events::Calendar m_calendar;

    /** @brief Se un'attività non genera occorrenze la toglie dall calendario */
    void cleanupActivity(const events::Activity* activity);
};

} // namespace app

#endif // APP_CALENDAR_CONTROLLER_H
