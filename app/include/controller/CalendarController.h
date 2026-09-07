#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>

#include <memory>
#include <vector>

#include "events.h"

namespace app {

/** @brief Controller MVC: la GUI non tocca mai il modello direttamente, passa
 *  sempre da qui. `activitiesChanged` viene emesso dopo ogni modifica. */
class CalendarController : public QObject {
    Q_OBJECT
public:
    explicit CalendarController(QObject* parent = nullptr);

    const events::Calendar& calendar() const;

    // --- CRUD sulle attivita' -----------------------------------------------

    bool addActivity(std::unique_ptr<events::Activity> activity);

    /** @brief Piu' serie in un colpo solo (es. una per giorno della settimana), un solo activitiesChanged. */
    bool addActivities(std::vector<std::unique_ptr<events::Activity>> activities);

    bool removeActivity(const events::Activity* activity);

    /** @brief Le eccezioni vengono conservate se accettate dal nuovo generatore. */
    bool updateActivity(const events::Activity* oldActivity,
                        std::unique_ptr<events::Activity> replacement);

    /** @brief Drag&drop nella settimana. */
    bool moveActivity(const events::Activity* activity, const QDateTime& newStart);

    /** @brief "Da questo momento in poi": ferma la serie attuale prima
     *  dell'occorrenza e ne crea una nuova con le stesse regole ma inizio
     *  diverso; la scadenza resta invariata. */
    bool splitRecurrence(const events::Occurrence& occurrence,
                         const QDateTime& newStart);

    // --- Query --------------------------------------------------------------

    QVector<const events::Activity*> search(const QString& needle) const;

    std::vector<events::Occurrence> occurrencesIn(const QDateTime& fromUtc,
                                                  const QDateTime& toUtc) const;

    // --- Azioni sulle occorrenze --------------------------------------------

    /** @brief Se ricorrente aggiunge un'eccezione (EXDATE), o tronca la serie
     *  se andFollowing e' true ("questa e le successive"); altrimenti rimuove
     *  l'attivita' singola. */
    bool deleteOccurrence(const events::Occurrence& occurrence, bool andFollowing = false);

    /** @brief L'originale viene escluso (eccezione se ricorrente) e sostituito da un evento singolo. */
    bool modifyOccurrence(const events::Occurrence& occurrence,
                          std::unique_ptr<events::Activity> replacement);

    /** @brief Solo i Compiti hanno stato "evaso/da fare"; sugli altri tipi non ha effetto. */
    bool toggleDone(const events::Occurrence& occurrence);

    // --- Persistenza --------------------------------------------------------

    bool saveToFile(const QString& filePath, QString* error = nullptr);

    bool loadFromFile(const QString& filePath, QString* error = nullptr);

signals:
    void activitiesChanged();

private:
    events::Calendar m_calendar;

    /** @brief Rimuove dal calendario un'attivita' che non genera piu' occorrenze. */
    void cleanupActivity(const events::Activity* activity);
};

} // namespace app
