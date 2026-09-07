#pragma once

#include <QColor>
#include <QDateTime>
#include <QTimeZone>

#include "events.h"
#include "generators/FixedIntervalGenerator.h"
#include "generators/MonthlyGenerator.h"
#include "generators/YearlyGenerator.h"

namespace app {

/** @brief La ricorrenza si deduce dal generatore (Single = singolo; Fixed/Monthly/Yearly = serie). */
inline bool isRecurrent(const events::Activity* activity) {
    const events::DateGenerator* gen = &activity->getGenerator();
    return dynamic_cast<const events::FixedIntervalGenerator*>(gen) != nullptr ||
           dynamic_cast<const events::MonthlyGenerator*>(gen) != nullptr ||
           dynamic_cast<const events::YearlyGenerator*>(gen) != nullptr;
}

/** @brief Copre un giorno di calendario intero (durata >= 24h - 1s). */
inline bool isAllDayActivity(const events::Activity* activity) {
    return activity && activity->getDuration() >=
                           std::chrono::hours(24) - std::chrono::seconds(1);
}

/** @brief Colore stabile per un'attivita': deriva dall'indirizzo dell'oggetto. */
inline QColor activityColor(const events::Activity* activity) {
    static const QColor kPalette[] = {
        QColor("#3B689C"),
        QColor("#A34843"),
        QColor("#3D7A5A"),
        QColor("#8C6239"),
        QColor("#6B4C7D"),
        QColor("#366B73"),
        QColor("#8C4356"),
        QColor("#486B4D")
    };
    constexpr int count = sizeof(kPalette) / sizeof(kPalette[0]);
    const auto address = reinterpret_cast<quintptr>(activity);
    return kPalette[(address >> 4) % count];
}

/** @brief L'unico tipo con stato "evaso"; gli altri tipi restituiscono sempre false. */
inline bool isTask(const events::Activity* activity) {
    return dynamic_cast<const events::Task*>(activity) != nullptr;
}

/** @brief Compito evaso all'occorrenza iniziale (getStart()); un Compito
 *  ricorrente evade per-occorrenza (vedi il sovraccarico sotto), quindi usare
 *  questo solo dove non c'e' una singola occorrenza da controllare (es.
 *  ActivityListPage, un rigo per SERIE). */
inline bool isTaskDone(const events::Activity* activity) {
    if (const auto* task = dynamic_cast<const events::Task*>(activity)) {
        return task->isDone();
    }
    return false;
}

/** @brief Occorrenza a `tp` evasa o no; da usare ovunque si visualizzi/spunti
 *  una singola occorrenza (griglie, chip), dato che un Compito ricorrente ha
 *  uno stato evaso indipendente per occorrenza (Task::m_doneOccurrences). */
inline bool isTaskDone(const events::Activity* activity, const events::TimePoint tp) {
    if (const auto* task = dynamic_cast<const events::Task*>(activity)) {
        return task->isDone(tp);
    }
    return false;
}

inline QDateTime localTime(const events::TimePoint tp) {
    return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count()).toLocalTime();
}

/** @brief Come lo storage del modello: epoch seconds / ISO-8601 UTC. */
inline QDateTime utcTime(const events::TimePoint tp) {
    return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count(), QTimeZone(0));
}

/** @brief Per gli eventi "tutto il giorno" (salvati a mezzanotte UTC) mostra
 *  00:00 invece dell'ora locale spostata dall'offset (es. 02:00 con UTC+2);
 *  per gli altri tipi converte normalmente in ora locale. */
inline QDateTime activityDisplayTime(const events::Activity* activity,
                                     const events::TimePoint tp) {
    if (isAllDayActivity(activity)) {
        return utcTime(tp);
    }
    return localTime(tp);
}

/** @brief True se l'occorrenza copre un giorno di calendario intero (2
 *  mezzanotti UTC consecutive nel suo intervallo) -> va mostrata nella
 *  striscia "tutto il giorno". Le mezzanotti sono valutate in UTC, non in ora
 *  locale. */
inline bool coversFullDay(const events::Occurrence& occ) {
    const QDateTime start = utcTime(occ.start);
    // prima mezzanotte UTC >= inizio
    QDateTime dayStart(start.date(), QTime(0, 0), QTimeZone(0));
    if (start > dayStart) {
        dayStart = dayStart.addDays(1);
    }
    // -1s perche' le attivita' "tutto il giorno" durano 24h - 1s (vedi
    // isAllDayActivity), non 24h esatte: senza il margine le serie ricorrenti
    // all-day non risulterebbero mai coversFullDay
    const QDateTime dayEnd = dayStart.addDays(1).addSecs(-1);
    return dayEnd <= utcTime(occ.end());
}

/** @brief 1 = lunedi', 7 = domenica. */
inline const char* shortDayName(int dayOfWeek) {
    static const char* kNames[] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};
    return kNames[qBound(1, dayOfWeek, 7) - 1];
}

} // namespace app
