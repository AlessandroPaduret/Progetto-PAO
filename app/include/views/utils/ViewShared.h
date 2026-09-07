#pragma once

#include <QColor>
#include <QDateTime>
#include <QTimeZone>

#include "events.h"
#include "generators/FixedIntervalGenerator.h"
#include "generators/MonthlyGenerator.h"
#include "generators/YearlyGenerator.h"

namespace app {

/** @brief true se l'attivita' e' una serie ricorrente: la ricorrenza si deduce
 *  dal generatore (Single = evento singolo; Fixed/Monthly/Yearly = serie). */
inline bool isRecurrent(const events::Activity* activity) {
    const events::DateGenerator* gen = &activity->getGenerator();
    return dynamic_cast<const events::FixedIntervalGenerator*>(gen) != nullptr ||
           dynamic_cast<const events::MonthlyGenerator*>(gen) != nullptr ||
           dynamic_cast<const events::YearlyGenerator*>(gen) != nullptr;
}

/** @brief true se l'attivita' e' un anniversario: generatore annuale e durata
 *  "tutto il giorno" (attivita' con YearlyGenerator, come dal builder). */
inline bool isAnniversary(const events::Activity* activity) {
    return dynamic_cast<const events::YearlyGenerator*>(
               &activity->getGenerator()) != nullptr &&
           activity->getDuration() >=
               std::chrono::hours(24) - std::chrono::seconds(1);
}

/** @brief true se l'attivita' e' "tutto il giorno": copre un giorno di
 *  calendario intero (durata >= 24h - 1s). */
inline bool isAllDayActivity(const events::Activity* activity) {
    return activity && activity->getDuration() >=
                           std::chrono::hours(24) - std::chrono::seconds(1);
}

/** @brief Palette di colori preimpostati proposti per le attivita': stessi 8
 *  valori usati sia per il colore "automatico" (activityColor, sotto) sia
 *  per gli swatch selezionabili nel form (ActivityFormPage). I valori
 *  esadecimali sono duplicati come objectName in resources/style.qss
 *  (#colorSwatch0..7, vedi il commento li'): tenerli allineati a mano se si
 *  cambia un colore. */
inline constexpr const char* kActivityColorPalette[] = {
    "#4285F4", "#EA4335", "#34A853", "#FBBC04",
    "#A142F4", "#24C1E0", "#F28B82", "#81C995",
};
inline constexpr int kActivityColorPaletteSize =
    sizeof(kActivityColorPalette) / sizeof(kActivityColorPalette[0]);

/** @brief Colore di un'attivita': `explicitColor` (tipicamente
 *  CalendarController::colorFor(activity), "#RRGGBB" o vuota) se valida,
 *  altrimenti un colore stabile dedotto dall'indirizzo dell'oggetto. Il
 *  colore NON e' un dato del modello (events::Activity non lo conosce
 *  affatto): vive solo nel CalendarController, i chiamanti lo passano qui
 *  esplicitamente invece di lasciare che questa funzione lo vada a cercare
 *  da sola, per non introdurre una dipendenza da CalendarController in
 *  questo header (Qt Gui puro, incluso anche da app_controller_tests). */
inline QColor activityColor(const events::Activity* activity,
                            const QString& explicitColor = QString()) {
    if (!explicitColor.isEmpty()) {
        const QColor custom(explicitColor);
        if (custom.isValid()) {
            return custom;
        }
    }
    const auto address = reinterpret_cast<quintptr>(activity);
    return QColor(kActivityColorPalette[(address >> 4) % kActivityColorPaletteSize]);
}

/** @brief true se l'attivita' e' un Compito (l'unico tipo con stato "evaso").
 *  Non-Comptiti restituiscono sempre false. */
inline bool isTask(const events::Activity* activity) {
    return dynamic_cast<const events::Task*>(activity) != nullptr;
}

/** @brief true se l'attivita' e' un Compito EVASO; false per gli altri tipi. */
inline bool isTaskDone(const events::Activity* activity) {
    if (const auto* task = dynamic_cast<const events::Task*>(activity)) {
        return task->isDone();
    }
    return false;
}

/** @brief Converte un istante assoluto (UTC) in data/ora locale. */
inline QDateTime localTime(const events::TimePoint tp) {
    return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count()).toLocalTime();
}

/** @brief Converte un istante assoluto in data/ora UTC (come lo storage del
 *  modello: epoch seconds / ISO-8601 UTC). */
inline QDateTime utcTime(const events::TimePoint tp) {
    return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count(), QTimeZone(0));
}

/** @brief Data/ora di un istante per un'attivita', adatta al display.
 *  Per gli eventi "tutto il giorno" (salvati a mezzanotte UTC) mostra la
 *  data con ora 00:00 invece dell'ora locale spostata dall'offset (es. 02:00
 *  con UTC+2); per gli altri tipi converte in ora locale. */
inline QDateTime activityDisplayTime(const events::Activity* activity,
                                     const events::TimePoint tp) {
    if (isAllDayActivity(activity)) {
        return utcTime(tp);
    }
    return localTime(tp);
}

/** @brief true se l'occorrenza e' "tutto il giorno": nel suo intervallo
 *  [start, end] cadono 2 mezzanotti consecutive (valutate in UTC, coerente
 *  con lo storage del modello). In tal caso va mostrata nella striscia in
 *  alto degli "eventi tutto il giorno".
 *
 *  Esempi: 12/12 13:50 -> 13/12 23:00 NON copre un giorno intero (normale);
 *  12/12 00:00 UTC -> 13/12 00:00 UTC copre un giorno intero (all-day).
 *  Nota: le mezzanotti sono in UTC, non in ora locale: un evento salvato a
 *  mezzanotte UTC in un fuso con offset e' comunque all-day. */
inline bool coversFullDay(const events::Occurrence& occ) {
    const QDateTime start = utcTime(occ.start);
    // Prima mezzanotte UTC >= all'inizio (se l'inizio e' a mezzanotte, quella stessa)
    QDateTime dayStart(start.date(), QTime(0, 0), QTimeZone(0));
    if (start > dayStart) {
        dayStart = dayStart.addDays(1);
    }
    // -1s: le attivita' "tutto il giorno" sono salvate con durata 24h - 1s
    // (vedi isAllDayActivity), non 24h esatte, quindi l'ultimo istante
    // coperto e' l'ultimo secondo prima della mezzanotte successiva, non
    // la mezzanotte stessa. Senza questo margine le serie ricorrenti
    // "tutto il giorno" (che usano sempre 24h - 1s, a differenza
    // dell'evento singolo che usa 24h esatte) non risultano mai all-day.
    const QDateTime dayEnd = dayStart.addDays(1).addSecs(-1);
    return dayEnd <= utcTime(occ.end());
}

/** @brief Nome corto del giorno della settimana (1 = lunedi', 7 = domenica). */
inline const char* shortDayName(int dayOfWeek) {
    static const char* kNames[] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};
    return kNames[qBound(1, dayOfWeek, 7) - 1];
}

} // namespace app
