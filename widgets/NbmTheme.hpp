#ifndef NBM_THEME_HPP_
#define NBM_THEME_HPP_
// NBMODES-IC8TEM: temi dei bozzetti (Chiaro, Notte, Nero, Pastello azzurro, Scia meteorica) e carattere Inter.
// "WSJT-X classico" lascia l'aspetto originale (con lo stile scuro opzionale di WSJT-X).
#include <QString>
#include <QStringList>
#include <QFont>

namespace nbm
{
  QStringList themeNames ();                 // il primo e' "WSJT-X classico"
  QString defaultThemeName ();               // "Chiaro"
  QString themeStyleSheet (QString const& name);   // vuoto per "WSJT-X classico"
  QString themeColor (QString const& name, QString const& var);   // es. "accent"
  bool loadInterFont ();                     // registra Inter dalle risorse; true se disponibile
  QFont defaultUiFont (QFont const& systemFont);   // Inter al 90% del carattere di sistema
  QString applyThemeTo (QString const& baseStyleSheet, QString const& name);   // base + blocco tema
}
#endif
