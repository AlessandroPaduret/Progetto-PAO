#pragma once

#include <QColor>

namespace app::theme {

/** @brief Colori usati dal codice che Qt Style Sheets non puo' raggiungere:
 *  disegno QPainter diretto (WeekView::paintEvent), QProxyStyle custom
 *  (MenuShortcutStyle) e colori calcolati a runtime (OccurrenceWidget,
 *  YearView). Gli stessi valori esadecimali sono usati anche in
 *  `resources/style.qss` per il resto dell'interfaccia (widget "normali",
 *  che Qt sa colorare da un file di stile) — se si cambia un colore qui,
 *  aggiornarlo anche li' per restare coerenti. */

inline const QColor kAccentBlue(QStringLiteral("#1a73e8"));      // selezione, "oggi"
inline const QColor kBorderGray(QStringLiteral("#dadce0"));      // separatori/bordi
inline const QColor kSecondaryText(QStringLiteral("#5f6368"));   // testo secondario
inline const QColor kPrimaryText(QStringLiteral("#202124"));     // testo principale
inline const QColor kMutedText(QStringLiteral("#9aa0a6"));       // testo attenuato
inline const QColor kPanelBackground(QStringLiteral("#f8f9fa")); // sfondo pannelli chiari
inline const QColor kDoneGray(QStringLiteral("#bdc1c6"));        // Compito evaso
inline const QColor kBlack(QStringLiteral("#000000"));           // testo nero forzato (elenco)

} // namespace app::theme
