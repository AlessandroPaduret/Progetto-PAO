#pragma once

#include <QWidget>

namespace app {

/** @brief Colonna delle ore (00:00..23:00) a sinistra della griglia: 24
 *  QLabel alte esattamente kWeekHourHeight, per restare allineate riga per
 *  riga con le linee orarie disegnate da ciascuna DayColumnWidget. */
class TimeGutterWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimeGutterWidget(QWidget* parent = nullptr);
};

} // namespace app
