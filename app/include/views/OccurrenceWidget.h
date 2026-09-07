#pragma once

#include <QPoint>
#include <QWidget>

#include "core/Occurrence.h"

class QCheckBox;
class QLabel;

namespace app {

/** @brief Chip di un'occorrenza: widget Qt reale (non un rettangolo
 *  disegnato a mano) usato da WeekView/DayView (blocco nella griglia oraria
 *  o nella striscia "tutto il giorno") e da MonthView (chip compatto).
 *
 *  Delega a Qt tutto cio' che puo': il testo/la spunta sono QLabel/QCheckBox
 *  reali, il tooltip e' quello nativo (QWidget::setToolTip), il menu
 *  contestuale usa QWidget::contextMenuEvent, il trascinamento usa
 *  QDrag/QMimeData. Il genitore si limita a posizionarlo (setGeometry) in
 *  base al proprio layout a griglia/colonne.
 */
class OccurrenceWidget : public QWidget {
    Q_OBJECT
public:
    /** @brief Stile compatto (chip di MonthView) o esteso (blocco di
     *  WeekView/DayView, con testo piu' leggibile e spunta piu' grande). */
    enum class Style { Block, Chip };

    OccurrenceWidget(const events::Occurrence& occurrence, Style style,
                     bool recurrent, bool draggable, QWidget* parent = nullptr);

    const events::Occurrence& occurrence() const { return m_occurrence; }

    /** @brief Evidenzia il chip (bordo blu) e mostra l'ora nel testo. */
    void setSelected(bool selected);

signals:
    /** @brief Clic sinistro (a prescindere da un eventuale drag successivo):
     *  il genitore lo usa per la selezione visiva. */
    void pressed(const events::Occurrence& occurrence);
    void doneToggled(const events::Occurrence& occurrence);
    /** @brief Doppio clic: e' il genitore a decidere se e' ambiguo (serie
     *  ricorrente, occorrenza successiva alla prima -> chiede) o diretto. */
    void doubleClicked(const events::Occurrence& occurrence);
    void infoRequested(const events::Occurrence& occurrence);
    /** @brief Menu "Modifica": sempre diretto sulla serie/attivita' sorgente. */
    void editRequested(const events::Occurrence& occurrence);
    /** @brief Menu "Modifica istanza": sempre diretto sulla singola occorrenza. */
    void modifyInstanceRequested(const events::Occurrence& occurrence);
    void deleteRequested(const events::Occurrence& occurrence);

protected:
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
};

} // namespace app
