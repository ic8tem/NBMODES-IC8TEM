#ifndef NBM_CALL_LISTS_HPP_
#define NBM_CALL_LISTS_HPP_
// NBMODES-IC8TEM: confronto esatto con ignore.list (una voce "CALL," per riga).
// Prima si usava list.contains(call + ","), che trova anche WK1ABC cercando K1ABC.
#include <QString>
#include "Radio.hpp"

inline bool nbm_ignore_has_exact (QString const& list, QString const& call)
{
  if (call.isEmpty () || list.isEmpty ()) return false;
  QString const key = call + ',';
  int from = 0;
  while ((from = list.indexOf (key, from)) >= 0)
    {
      if (from == 0 || list.at (from - 1) == '\n' || list.at (from - 1) == '\r'
          || list.at (from - 1) == ',' || list.at (from - 1) == ' ')
        return true;
      from += key.size ();
    }
  return false;
}
// Nessun limite di lunghezza (è un file), i nominativi barrati sono ammessi.
// IC8TEM nell'elenco esclude anche IC8TEM/P, F/IC8TEM...; IC8TEM/P esclude solo IC8TEM/P.
inline bool nbm_ignore_has (QString const& list, QString const& call)
{
  if (nbm_ignore_has_exact (list, call)) return true;
  if (call.contains ('/'))
    {
      auto const base = Radio::base_callsign (call);
      if (base != call && nbm_ignore_has_exact (list, base)) return true;
    }
  return false;
}
#endif
