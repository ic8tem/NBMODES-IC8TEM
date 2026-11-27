#ifndef NBM_LAZY_ALLOC_HPP_
#define NBM_LAZY_ALLOC_HPP_
// NBMODES-IC8TEM: memoria a zeri "su richiesta" per le grandi aree audio (dec_data_t: 48 MB, di cui
// 43 MB per 30 minuti di audio, pensati per FST4W-1800). Il sistema operativo fornisce pagine già a
// zero e le rende residenti solo quando vengono scritte: in FT8 si usano i primi 15 secondi.
// Con new T{} invece tutte le pagine venivano scritte a zero all'avvio e restavano occupate.
#include <cstddef>

namespace nbm
{
  void * lazy_zero_alloc (std::size_t n);         // lancia std::bad_alloc se manca memoria
  void lazy_free (void * p, std::size_t n);

  template<typename T> struct LazyDelete
  {
    void operator () (T * p) const { lazy_free (p, sizeof (T)); }
  };

  // Solo per tipi "plain old data": i byte a zero sono già un oggetto valido.
  template<typename T> T * lazy_zero_new () { return static_cast<T *> (lazy_zero_alloc (sizeof (T))); }
}
#endif
