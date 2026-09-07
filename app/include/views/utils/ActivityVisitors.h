#pragma once

#include <QString>

#include "core/ActivityVisitor.h"
#include "core/DateGeneratorVisitor.h"

namespace app {

/** @brief Visitor di sola visualizzazione: etichetta del tipo dinamico
 *  dell'attivita' (Evento/Ricorrente/Riunione/Compito). Il tipo e' il
 *  dispatch dinamico (Activity/Task/Meeting); la ricorrenza si deduce dal
 *  generatore, non dal tipo. Usato da ActivityViewHelpers::typeLabel. */
class TypeLabelVisitor : public events::ActivityVisitor {
public:
    QString label;

    void visit(const events::Activity& activity) override;
    void visit(const events::Task& task) override;
    void visit(const events::Meeting& meeting) override;
};

/** @brief Visitor di sola visualizzazione: traduce un DateGenerator in una
 *  frase leggibile ("7 giorno/i", "anno", "una volta", ...). Usato sia da
 *  ActivityViewHelpers::recurrenceRuleLabel sia da ActivitySummaryVisitor. */
class RecurrenceRuleVisitor : public events::DateGeneratorVisitor {
public:
    QString rule;

    void visit(const events::FixedIntervalGenerator& generator) override;
    void visit(const events::YearlyGenerator& generator) override;
    void visit(const events::MonthlyGenerator& generator) override;
    void visit(const events::SingleGenerator& generator) override;
};

/** @brief Visitor di sola visualizzazione: riga descrittiva sintetica
 *  (data/ora + regola di ricorrenza o stato) per tipo, usata nell'elenco
 *  delle attivita' (ActivityListPage). Usato da
 *  ActivityViewHelpers::summaryLabel. */
class ActivitySummaryVisitor : public events::ActivityVisitor {
public:
    QString summary;

    void visit(const events::Activity& activity) override;
    void visit(const events::Task& task) override;
    void visit(const events::Meeting& meeting) override;
};

} // namespace app
