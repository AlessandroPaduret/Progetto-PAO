#ifndef APP_VIEW_SHARED_H
#define APP_VIEW_SHARED_H

#include <QColor>
#include <QDateTime>

#include "events/events.h"

namespace app {

/** @brief Colore stabile per un'attivita': deriva dall'indirizzo dell'oggetto. */
inline QColor activityColor(const events::Activity* activity) {
    static const QColor kPalette[] = {
        QColor("#4285F4"), QColor("#EA4335"), QColor("#34A853"), QColor("#FBBC04"),
        QColor("#A142F4"), QColor("#24C1E0"), QColor("#F28B82"), QColor("#81C995"),
    };
    constexpr int count = sizeof(kPalette) / sizeof(kPalette[0]);
    const auto address = reinterpret_cast<quintptr>(activity);
    return kPalette[(address >> 4) % count];
}

/** @brief Converte un istante assoluto (UTC) in data/ora locale. */
inline QDateTime localTime(const events::TimePoint tp) {
    return QDateTime::fromSecsSinceEpoch(tp.time_since_epoch().count()).toLocalTime();
}

/** @brief Nome corto del giorno della settimana (1 = lunedi', 7 = domenica). */
inline const char* shortDayName(int dayOfWeek) {
    static const char* kNames[] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};
    return kNames[qBound(1, dayOfWeek, 7) - 1];
}

} // namespace app

#endif // APP_VIEW_SHARED_H
