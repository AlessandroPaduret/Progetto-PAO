#pragma once

#include <QColor>
#include <QPoint>
#include <QWidget>

#include "core/Occurrence.h"

class QCheckBox;
class QLabel;
class QPaintEvent;

namespace app {

/** @brief Chip di un'occorrenza: widget Qt reale (non un rettangolo
 *  disegnato a mano) usato da WeekView/DayView (blocco o chip "tutto il
 *  giorno") e da MonthView (chip compatto). Delega a Qt tutto cio' che puo':
 *  testo/spunta sono QLabel/QCheckBox reali, tooltip nativo, menu
 *  contestuale via contextMenuEvent, trascinamento via QDrag/QMimeData; il
 *  genitore si limita a posizionarlo (setGeometry) in base al proprio layout. */
class OccurrenceWidget : public QWidget {
    Q_OBJECT
public:
    /** @brief Block = griglia oraria, Chip = compatto. */
    enum class Style { Block, Chip };

    OccurrenceWidget(const events::Occurrence& occurrence, Style style,
                     bool recurrent, bool draggable, QWidget* parent = nullptr);

    const events::Occurrence& occurrence() const { return m_occurrence; }

    void setSelected(bool selected);

signals:
    /** @brief Clic sinistro: usato dal genitore per la selezione visiva. */
    void pressed(const events::Occurrence& occurrence);
    void doneToggled(const events::Occurrence& occurrence);
    /** @brief Il genitore decide se e' ambiguo (occorrenza successiva alla
     *  prima di una serie -> chiede) o diretto. */
    void doubleClicked(const events::Occurrence& occurrence);
    void infoRequested(const events::Occurrence& occurrence);
    /** @brief Sempre sulla serie/attivita' sorgente. */
    void editRequested(const events::Occurrence& occurrence);
    /** @brief Sempre sulla singola occorrenza. */
    void modifyInstanceRequested(const events::Occurrence& occurrence);
    void deleteRequested(const events::Occurrence& occurrence);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void applyPalette();
    void startDrag();

    events::Occurrence m_occurrence;
    Style m_style;
    bool m_recurrent;
    bool m_draggable;
    bool m_selected = false;
    QPoint m_dragStartPos;
    QCheckBox* m_checkBox = nullptr;  // solo per i Compiti
    QLabel* m_label = nullptr;        // per tutti gli altri tipi

    // Sfondo/bordo dipinti direttamente (vedi applyPalette()): via QSS
    // "palette(...)" un colore per-istanza si perderebbe al primo repolish
    // con lo stile applicativo attivo (vedi commento in applyPalette()).
    QColor m_fillColor;
    QColor m_borderColor;
};

} // namespace app
