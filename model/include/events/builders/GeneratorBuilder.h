#ifndef GENERATOR_BUILDER_H
#define GENERATOR_BUILDER_H

#include <chrono>
#include <cstddef>
#include <memory>

#include "events/core/CommonTypes.h"
#include "events/core/DateGenerator.h"

namespace events {

/**
 * @enum RecurrenceUnit
 * @brief Unita' di misura per la ricorrenza temporale del generatore.
 */
enum class RecurrenceUnit {
    Single,
    Daily,
    Weekly,
    Monthly,
    Yearly
};

/**
 * @class GeneratorBuilder
 * @brief Builder fluente (DSL) per la creazione di DateGenerator e dei relativi Decorator.
 *
 * Permette di costruire in modo dichiarativo e pulito qualsiasi tipologia di generatore
 * (singolo, a intervallo fisso come giornaliero/settimanale, mensile o annuale),
 * applicando opzionalmente limiti di fine o di occorrenze.
 */
class GeneratorBuilder {
private:
    TimePoint m_start;
    RecurrenceUnit m_type = RecurrenceUnit::Single;
    
    // Parametri di configurazione
    Duration m_intervalValue = Duration::zero(); ///< Usato per FixedIntervalGenerator (es. 24h, 7d)
    int m_numericParam = 1;                      ///< Usato per mesi o anni (es. ogni 2 mesi)
    TimePoint m_end = TimePoint::max();          ///< Data di fine serie
    std::size_t m_maxOccurrences = 0;            ///< Limite massimo occorrenze (0 = illimitate)

    explicit GeneratorBuilder(TimePoint start);

public:
    /** 
     * @brief Punto d'ingresso statico per avviare la costruzione fluente.
     * @param start Istante temporale di inizio della serie.
     * @return GeneratorBuilder un'istanza pronta per la configurazione.
     */
    static GeneratorBuilder from(TimePoint start);

    //@{
    /** @name Metodi di Configurazione della Frequenza */

    /** @brief Imposta l'attivita' come evento singolo (nessuna ricorrenza). */
    GeneratorBuilder& asSingle();

    /** @brief Imposta una ricorrenza a intervallo fisso in durata (es. giorni o ore). */
    GeneratorBuilder& repeatEvery(Duration interval);

    /** @brief Imposta una ricorrenza mensile di calendario. */
    GeneratorBuilder& repeatMonthly(int intervalMonths = 1);

    /** @brief Imposta una ricorrenza annuale di calendario. */
    GeneratorBuilder& repeatYearly(int intervalYears = 1);
    //@}


    //@{
    /**
     * @name Limiti e Vincoli Temporali
     */

    /** @brief Imposta la data di fine oltre la quale non generare occorrenze. */
    GeneratorBuilder& until(TimePoint end);

    /** @brief Limita la serie a un numero massimo di occorrenze (applica MaxOccurrencesDecorator). */
    GeneratorBuilder& limitTo(std::size_t maxOccurrences);
    //@}

    /** 
     * @brief Assembla e restituisce il DateGenerator configurato.
     * 
     * Applica automaticamente i decorator necessari (es. @ref MaxOccurrencesDecorator)
     * e trasferisce l'ownership esclusiva tramite `std::unique_ptr`.
     * 
     * @return std::unique_ptr<DateGenerator> Il generatore 
     */
    [[nodiscard]] std::unique_ptr<DateGenerator> build();
};

// Alias opzionale per una sintassi ancora piu' compatta (es. Gen::from(...))
using Gen = GeneratorBuilder;

} // namespace events

#endif // GENERATOR_BUILDER_H