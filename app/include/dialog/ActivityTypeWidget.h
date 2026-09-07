#pragma once

#include <QWidget>

#include <memory>

#include "events.h"

namespace app {

/** @brief Interfaccia comune alle sezioni di form specifiche di un tipo di
 *  attivita' (Riunione, Compito, Evento): sostituisce con il polimorfismo
 *  lo switch su m_typeCombo->currentIndex() che prima viveva in
 *  ActivitySidebarWidget. */
class ActivityTypeWidget : public QWidget {
    Q_OBJECT
public:
    using QWidget::QWidget;
    ~ActivityTypeWidget() override = default;

    virtual void clear() = 0;

    /** @brief Il chiamante garantisce che activity sia del tipo dinamico
     *  giusto per questa sezione (Task per TaskFormWidget, ...). */
    virtual void populateFrom(const events::Activity& activity) = 0;

    /** @brief Punto di estensione per arricchire config con dati specifici
     *  del tipo prima di createActivity(); chi non serve lascia config invariata. */
    virtual void applyToConfig(events::ActivityConfig& config) const = 0;

    virtual std::unique_ptr<events::Activity> createActivity(events::ActivityConfig config) const = 0;
};

} // namespace app
