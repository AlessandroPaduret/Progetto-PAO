#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>

#include <memory>
#include <unordered_map>
#include <vector>

#include "events.h"

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

    /** @brief Aggiunge un'attivita' al calendario (ne acquisisce la proprieta').
     *  @param color "#RRGGBB" scelto dall'utente nel form, "" = automatico
     *  (vedi colorFor). Il colore NON e' un campo del modello: vive solo qui,
     *  associato per identita' di puntatore (vedi colorFor). */
    bool addActivity(std::unique_ptr<events::Activity> activity, const QString& color = QString());

    /** @brief Aggiunge piu' attivita' in un colpo solo (es. piu' serie
     *  ricorrenti, una per giorno della settimana): stesso colore per tutte,
     *  scelto una sola volta nel form. Emette un unico activitiesChanged. */
    bool addActivities(std::vector<std::unique_ptr<events::Activity>> activities,
                       const QString& color = QString());

    /** @brief Rimuove l'attivita' identificata dal puntatore */
    bool removeActivity(const events::Activity* activity);

    /** @brief Sostituisce un'attivita' con una nuova (le eccezioni vengono
     *         conservate, se accettate dal nuovo generatore) */
    bool updateActivity(const events::Activity* oldActivity,
                        std::unique_ptr<events::Activity> replacement,
                        const QString& color = QString());

    /** @brief Colore esplicito assegnato all'attivita' (dal form, tramite gli
     *  overload *color* qui sopra), "" se non impostato: in quel caso la GUI
     *  ne deduce uno stabile dall'indirizzo dell'oggetto (vedi
     *  views/utils/ViewShared.h::activityColor). Non e' un dato del modello:
     *  vive solo nel controller, associato per identita' di puntatore, e
     *  viene ritrasferito automaticamente ad ogni operazione che sostituisce
     *  l'oggetto Activity (updateActivity/modifyOccurrence/splitRecurrence),
     *  ed eliminato quando l'attivita' viene rimossa. */
    QString colorFor(const events::Activity* activity) const;

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
                          std::unique_ptr<events::Activity> replacement,
                          const QString& color = QString());

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

    /** @brief Colori scelti dall'utente, per identita' di puntatore (assente
     *  = automatico). Sopravvive a save/loadFromFile tramite una sezione
     *  JSON parallela ("colors", per indice = ordine del calendario), scritta
     *  e letta qui SENZA coinvolgere model/persistence: quel livello resta
     *  ignaro del concetto di colore. */
    std::unordered_map<const events::Activity*, QString> m_colors;

    /** @brief Se un'attività non genera occorrenze la toglie dal calendario
     *  (e dalla mappa colori: il puntatore non sarebbe piu' valido come
     *  chiave). */
    void cleanupActivity(const events::Activity* activity);
};

} // namespace app
