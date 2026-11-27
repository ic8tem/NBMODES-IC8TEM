// NBMODES-IC8TEM — popup di adattamento al computer e popup di chiusura (come YAEC e YA-NEC).
#ifndef NBM_HOST_DIALOGS_HPP
#define NBM_HOST_DIALOGS_HPP

#include <QString>
#include <QtGlobal>
class QWidget;
class QSettings;

namespace nbm
{
  // Profilo del computer che ospita NBMODES: processore, sistema, monitor.
  struct HostProfile
  {
    QString cpu;          // marca e modello
    int cores {0};        // thread logici
    bool avx2 {false};    // AVX2+FMA usabili (superldpc veloce)
    QString os;           // es. "Windows 10 Version 22H2"
    QString kernel;       // versione del kernel/build
    QString screens;      // uno per riga: risoluzione, DPI, scala
    qint64 ramMB {0};     // memoria fisica totale
    QString key () const; // firma per capire se il computer e' cambiato
  };

  HostProfile detectHost ();

  // Memoria fisica totale in MB (0 se non si riesce a leggerla)
  qint64 totalRamMB ();
  // Thread del decoder FT8 multithread quando la scelta e' "Auto": con 4 GB al massimo 2,
  // con 8 GB al massimo 4, oltre 0 (= tutti i core, come WSJT-X).
  int autoFt8Threads ();

  // Mostra il popup se il computer (o il monitor) e' diverso da quello salvato,
  // oppure sempre se force == true (voce di menu). Salva la nuova firma.
  void showHostAdaptation (QWidget * parent, QSettings * settings, bool force = false);

  // Popup di chiusura: "Vuoi chiudere NBMODES-IC8TEM?" + ringraziamento in corsivo.
  // Ritorna true se l'utente conferma. Invio = Si', Esc = No.
  bool confirmExit (QWidget * parent);

  // false in prove automatiche (piattaforma offscreen o NBM_NO_EXIT_PROMPT impostata).
  bool interactiveSession ();
}

#endif
