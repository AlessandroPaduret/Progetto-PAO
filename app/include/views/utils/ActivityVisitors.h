#pragma once

#include <QString>

#include "core/ActivityVisitor.h"
#include "core/DateGeneratorVisitor.h"

namespace app {

/** @brief Etichetta del tipo (Evento/Ricorrente/Riunione/Compito): il tipo e'
 *  il dispatch dinamico (Activity/Task/Meeting), la ricorrenza si deduce dal
 *  generatore, non dal tipo. Usato da ActivityViewHelpers::typeLabel. */
class TypeLabelVisitor : public events::ActivityVisitor {
public:
    QString label;

    void visit(const events::Activity& activity) override;
    void visit(const events::Task& task) override;
    void visit(const events::Meeting& meeting) override;
};

/** @brief Traduce un DateGenerator in una frase leggibile ("7 giorno/i",
 *  "anno", "una volta", ...). Usato da ActivityViewHelpers::recurrenceRuleLabel
 *  e ActivitySummaryVisitor. */
class RecurrenceRuleVisitor : public events::DateGeneratorVisitor {
public:
    QString rule;

    void visit(const events::FixedIntervalGenerator& generator) override;
    void visit(const events::YearlyGenerator& generator) override;
    void visit(const events::MonthlyGenerator& generator) override;
    void visit(const events::SingleGenerator& generator) override;
};

/** @brief Riga sintetica (data/ora + regola o stato) per tipo, usata in ActivityListPage. */
class ActivitySummaryVisitor : public events::ActivityVisitor {
public:
    QString summary;

    void visit(const events::Activity& activity) override;
    void visit(const events::Task& task) override;
    void visit(const events::Meeting& meeting) override;
};

} // namespace app
