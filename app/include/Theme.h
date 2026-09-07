#pragma once

#include <QColor>

namespace app::theme {

/** @brief Palette scura unica dell'applicazione (nessuna dipendenza dal tema
 *  di sistema: vedi main.cpp, che forza lo stile Fusion + questa QPalette in
 *  modo che l'aspetto sia identico su qualunque piattaforma/tema ospite).
 *
 *  Qui stanno i colori usati dal codice che Qt Style Sheets non puo'
 *  raggiungere: disegno QPainter diretto (DayColumnWidget::paintEvent),
 *  QProxyStyle custom (MenuShortcutStyle) e colori calcolati a runtime
 *  (OccurrenceWidget, YearView). Gli stessi valori esadecimali sono usati
 *  anche in `resources/style.qss` per il resto dell'interfaccia (widget
 *  "normali", che Qt sa colorare da un file di stile) — se si cambia un
 *  colore qui, aggiornarlo anche li' per restare coerenti. */

inline const QColor kWindowBackground(QStringLiteral("#202124")); // sfondo di base (finestra, griglie)
inline const QColor kPanelBackground(QStringLiteral("#2a2b2e"));  // sfondo pannelli/intestazioni/celle
inline const QColor kAccentBlue(QStringLiteral("#8ab4f8"));       // selezione, "oggi" (blu chiaro da dark theme)
inline const QColor kBorderGray(QStringLiteral("#3c4043"));       // separatori/bordi
inline const QColor kSecondaryText(QStringLiteral("#9aa0a6"));    // testo secondario
inline const QColor kPrimaryText(QStringLiteral("#e8eaed"));      // testo principale
inline const QColor kMutedText(QStringLiteral("#80868b"));        // testo attenuato
inline const QColor kDoneGray(QStringLiteral("#5f6368"));         // Compito evaso

} // namespace app::theme
