#ifndef APP_LEFT_SHORTCUT_STYLE_H
#define APP_LEFT_SHORTCUT_STYLE_H

#include <QColor>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleOptionMenuItem>

namespace app {

/** @brief Stile dei menu: la scorciatoia (es. Ctrl+O) viene disegnata a
 *  SINISTRA della voce, in grigio/trasparente, invece che a destra come il
 *  default. Qt6 passa la scorciatoia dentro `QStyleOptionMenuItem::text`
 *  dopo un tab ("testo\tCtrl+O"). */
class LeftShortcutStyle : public QProxyStyle {
public:
    explicit LeftShortcutStyle(QStyle* baseStyle) : QProxyStyle(baseStyle) {}

    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget) const override {
        if (element == CE_MenuItem) {
            const auto* item = static_cast<const QStyleOptionMenuItem*>(option);
            if (item->menuItemType == QStyleOptionMenuItem::Normal) {
                const QString shortcut =
                    item->text.section(QLatin1Char('\t'), 1, 1);
                if (!shortcut.isEmpty()) {
                    const QString label =
                        item->text.section(QLatin1Char('\t'), 0, 0);
                    const bool selected = item->state & State_Selected;
                    // Sfondo evidenziato (hover) a tutta larghezza
                    if (selected) {
                        painter->fillRect(item->rect,
                                          item->palette.highlight());
                    }
                    painter->save();
                    painter->setFont(item->font);
                    // Scorciatoia a sinistra, grigio/trasparente
                    const int shortcutW =
                        item->fontMetrics.horizontalAdvance(shortcut) + 14;
                    painter->setPen(selected ? QColor(255, 255, 255, 180)
                                             : QColor(0, 0, 0, 100));
                    painter->drawText(
                        QRect(item->rect.left() + 8, item->rect.top(),
                              shortcutW - 8, item->rect.height()),
                        Qt::AlignLeft | Qt::AlignVCenter, shortcut);
                    // Etichetta dopo la scorciatoia, elisa se non ci sta
                    painter->setPen(
                        selected ? item->palette.highlightedText().color()
                                 : item->palette.text().color());
                    const QRect labelRect(
                        item->rect.left() + shortcutW, item->rect.top(),
                        item->rect.width() - shortcutW, item->rect.height());
                    painter->drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter,
                                      item->fontMetrics.elidedText(
                                          label, Qt::ElideRight,
                                          labelRect.width()));
                    painter->restore();
                    return;
                }
            }
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }
};

} // namespace app

#endif // APP_LEFT_SHORTCUT_STYLE_H