#pragma once

#include <QDate>
#include <QWidget>

#include <vector>

class QLabel;

namespace app {

/** @brief Intestazione dei giorni sopra la griglia: un QLabel per giorno
 *  ("Lun 12", oggi in grassetto via proprieta' dinamica + QSS), allineata
 *  alle colonne sotto da uno spaziatore iniziale largo quanto
 *  TimeGutterWidget e uno finale largo quanto la scrollbar verticale
 *  (sempre visibile, vedi WeekView). */
class HeaderWidget : public QWidget {
    Q_OBJECT
public:
    explicit HeaderWidget(QWidget* parent = nullptr);

    /** @brief Ricostruisce le etichette per i giorni [viewStart, viewStart+dayCount). */
    void setDays(const QDate& viewStart, int dayCount);

private:
    std::vector<QLabel*> m_dayLabels;
};

} // namespace app
