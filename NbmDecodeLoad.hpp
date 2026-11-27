#ifndef NBM_DECODE_LOAD_HPP_
#define NBM_DECODE_LOAD_HPP_
// NBMODES-IC8TEM: adattamento del decoder alla potenza del PC.
// Si misura il tempo di ogni decodifica finale dal vivo. Se supera il tempo disponibile si toglie lavoro
// (livello +1, o +2 se molto oltre); dopo una serie di decodifiche ben dentro il tempo si restituisce
// un livello. Se un livello restituito torna subito a sforare, per il prossimo tentativo servono
// il doppio delle decodifiche veloci (massimo 48), così il decoder non oscilla.
// Livello 0 = impostazioni scelte dall'utente, che non vengono mai modificate.
#include <QtGlobal>
#include <algorithm>

namespace nbm
{
  struct Ft8Work { int cycles; int sensitivity; };

  // Sequenza dei livelli FT8 multithread: prima la sensibilità 3 -> 2 (niente sottopassaggio),
  // poi i cicli uno alla volta fino a 1, infine la sensibilità 2 -> 1 (soglia normale).
  inline Ft8Work ft8_work_at (int cycles, int sensitivity, int level)
  {
    Ft8Work w {std::max (1, cycles), std::max (1, sensitivity)};
    for (int i = 0; i < level; ++i)
      {
        if (w.sensitivity == 3) w.sensitivity = 2;
        else if (w.cycles > 1) --w.cycles;
        else if (w.sensitivity == 2) w.sensitivity = 1;
        else break;
      }
    return w;
  }

  inline int ft8_max_level (int cycles, int sensitivity)
  {
    int n = 0;
    while (true)
      {
        auto const a = ft8_work_at (cycles, sensitivity, n);
        auto const b = ft8_work_at (cycles, sensitivity, n + 1);
        if (a.cycles == b.cycles && a.sensitivity == b.sensitivity) return n;
        ++n;
      }
  }

  // Profondità (Quick=1, Medium=2, Deep=3) per FT4 e per FT8 a un solo thread
  inline int depth_at (int depth, int level) {return std::max (1, depth - level);}
  inline int depth_max_level (int depth) {return std::max (0, depth - 1);}

  class DecodeLoadGovernor
  {
  public:
    void reset () {level_ = 0; good_ = 0; need_ = 6; sinceUp_ = 99;}
    int level () const {return level_;}

    // Restituisce true se il livello è cambiato.
    bool observe (qint64 elapsedMs, qint64 budgetMs, int maxLevel)
    {
      if (elapsedMs < 0 || budgetMs <= 0) return false;
      int const old = level_;
      ++sinceUp_;
      if (elapsedMs > budgetMs)
        {
          level_ += (elapsedMs > budgetMs * 7 / 4) ? 2 : 1;
          good_ = 0;
          if (sinceUp_ <= 3) need_ = std::min (48, need_ * 2);   // il livello restituito non regge
        }
      else if (elapsedMs * 100 < budgetMs * 45)
        {
          if (level_ > 0 && ++good_ >= need_) {--level_; good_ = 0; sinceUp_ = 0;}
        }
      else good_ = 0;
      level_ = std::max (0, std::min (level_, maxLevel));
      return level_ != old;
    }

  private:
    int level_ {0};
    int good_ {0};
    int need_ {6};
    int sinceUp_ {99};
  };

  // Tempo disponibile per la decodifica finale: oltre, la risposta automatica partirebbe in ritardo
  // e la decodifica invaderebbe il periodo successivo.
  inline qint64 decode_budget_ms (double trPeriodSeconds)
  {
    if (trPeriodSeconds <= 8.) return 2000;    // FT4
    if (trPeriodSeconds <= 16.) return 4000;   // FT8
    return qint64 (trPeriodSeconds * 1000. * 0.3);
  }
}
#endif
