#pragma once

#include <QWidget>

namespace app {

/** @brief Colonna delle ore (00:00 .. 23:00) a sinistra della griglia
 *  oraria: 24 QLabel reali impilate in un QVBoxLayout, ognuna alta esattamente
 *  `kWeekHourHeight` (nessun disegno manuale) cosi' da restare allineata riga
 *  per riga con le linee orarie disegnate da ciascuna DayColumnWidget. */
class TimeGutterWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimeGutterWidget(QWidget* parent = nullptr);
};

} // namespace app
