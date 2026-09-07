#pragma once

#include <QColor>

namespace app::theme {

/** @brief Palette scura fissa (main.cpp forza Fusion + questa QPalette, stesso
 *  aspetto ovunque). Colori per il codice che le QSS non raggiungono:
 *  QPainter diretto, MenuShortcutStyle, colori a runtime in
 *  OccurrenceWidget/YearView. Duplicati in resources/style.qss: se cambi un
 *  colore aggiornalo anche li'. */

inline const QColor kWindowBackground(QStringLiteral("#202124")); // sfondo di base (finestra, griglie)
inline const QColor kPanelBackground(QStringLiteral("#2a2b2e"));  // sfondo pannelli/intestazioni/celle
inline const QColor kAccentBlue(QStringLiteral("#8ab4f8"));       // selezione, "oggi" (blu chiaro da dark theme)
inline const QColor kBorderGray(QStringLiteral("#3c4043"));       // separatori/bordi
inline const QColor kSecondaryText(QStringLiteral("#9aa0a6"));    // testo secondario
inline const QColor kPrimaryText(QStringLiteral("#e8eaed"));      // testo principale
inline const QColor kMutedText(QStringLiteral("#80868b"));        // testo attenuato
inline const QColor kDoneGray(QStringLiteral("#5f6368"));         // Compito evaso

} // namespace app::theme
