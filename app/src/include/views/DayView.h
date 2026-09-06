#ifndef APP_DAYVIEW_H
#define APP_DAYVIEW_H

#include "views/WeekView.h"

namespace app {

/** @brief Vista "giorno": la stessa griglia della settimana ma con una sola
 *  colonna (WeekView con dayCount = 1), quindi con le ore molto piu' ampie.
 *  Eredita interazione, drag&drop e anteprima live dalla WeekView.
 */
class DayView : public WeekView {
public:
    explicit DayView(QWidget* parent = nullptr);
};

} // namespace app

#endif // APP_DAYVIEW_H
