#pragma once

#include <QWidget>

#include <memory>

#include "events.h"

namespace app {

/** @brief Interfaccia comune alle sezioni di form specifiche di un tipo di
 *  attivita' (Riunione, Compito, Evento): sostituisce lo switch su
 *  `m_typeCombo->currentIndex()` in
 *  ActivitySidebarWidget::makeTypedActivity/showEditActivity con il
 *  polimorfismo. Ogni sezione sa da sola svuotarsi (creazione), popolarsi
 *  da un'attivita' esistente del proprio tipo dinamico (modifica) e
 *  costruire la propria attivita' concreta a partire dai campi comuni gia'
 *  pronti in una `events::ActivityConfig` (titolo/data/durata/end/
 *  generatore, gestiti dal coordinatore ActivitySidebarWidget). */
class ActivityTypeWidget : public QWidget {
    Q_OBJECT
public:
    using QWidget::QWidget;
    ~ActivityTypeWidget() override = default;

    /** @brief Svuota i campi specifici di questa sezione (nuova attivita'). */
    virtual void clear() = 0;

    /** @brief Popola i campi specifici da un'attivita' esistente: il
     *  chiamante garantisce che `activity` sia del tipo dinamico giusto
     *  per questa sezione (Task per TaskFormWidget, Meeting per
     *  MeetingFormWidget, ...). */
    virtual void populateFrom(const events::Activity& activity) = 0;

    /** @brief Punto di estensione per arricchire `config` con dati comuni
     *  specifici del tipo prima di createActivity(); le sezioni che non ne
     *  hanno bisogno lasciano `config` invariata. */
    virtual void applyToConfig(events::ActivityConfig& config) const = 0;

    /** @brief Costruisce l'attivita' concreta (Activity/Meeting/Task) a
     *  partire dai campi comuni gia' pronti in `config` e da quelli
     *  specifici raccolti da questo widget. */
    virtual std::unique_ptr<events::Activity> createActivity(events::ActivityConfig config) const = 0;
};

} // namespace app
