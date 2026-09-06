#pragma once

#include <QColor>
#include <QPainter>
#include <QPalette>
#include <QProxyStyle>
#include <QStyleOptionMenuItem>

namespace app {

/** @brief Stile dei menu: la scorciatoia (es. Ctrl+O) viene disegnata a
 *  DESTRA della voce (posizione standard) ma piu' attenuata del testo
 *  normale, per restare leggibile senza competere con l'etichetta. Il
 *  colore non e' un grigio fisso: e' il ruolo "testo disabilitato" della
 *  QPalette corrente (item->palette), che segue automaticamente il tema
 *  di sistema (incluso il tema scuro) invece di un valore scritto a mano.
 *  Qt6 passa la scorciatoia dentro `QStyleOptionMenuItem::text` dopo un tab
 *  ("testo\tCtrl+O"). */
class MenuShortcutStyle : public QProxyStyle {
public:
    explicit MenuShortcutStyle(QStyle* baseStyle) : QProxyStyle(baseStyle) {}

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
                    // Scorciatoia a destra (posizione standard), grigio marcato
                    const int shortcutW =
                        item->fontMetrics.horizontalAdvance(shortcut) + 14;
                    const QRect shortcutRect(
                        item->rect.right() - shortcutW, item->rect.top(),
                        shortcutW - 8, item->rect.height());
                    QColor shortcutColor =
                        selected ? item->palette.color(QPalette::Active,
                                                       QPalette::HighlightedText)
                                : item->palette.color(QPalette::Disabled,
                                                      QPalette::Text);
                    shortcutColor.setAlpha(selected ? 200 : 255);
                    painter->setPen(shortcutColor);
                    painter->drawText(shortcutRect,
                                      Qt::AlignRight | Qt::AlignVCenter, shortcut);
                    // Etichetta a sinistra, elisa se non ci sta prima della scorciatoia
                    painter->setPen(
                        selected ? item->palette.highlightedText().color()
                                 : item->palette.text().color());
                    const QRect labelRect(
                        item->rect.left() + 8, item->rect.top(),
                        item->rect.width() - shortcutW - 8, item->rect.height());
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
