#ifndef NBM_GPU_HPP_
#define NBM_GPU_HPP_
// NBMODES-IC8TEM: rilevamento della scheda grafica e del suo carico, memoria libera del sistema.
// Windows: DXGI (nome, memoria dedicata/condivisa; funziona anche su Windows 7) e contatori PDH
// "GPU Engine" per il carico (solo Windows 10 1709 e successivi). Ovunque: prova reale di un
// contesto OpenGL, che e' quello che serve al waterfall disegnato dalla GPU.
#include <QString>
#include <QtGlobal>

class QSettings;

namespace nbm
{
  struct GpuInfo
  {
    QString name;               // es. "Intel(R) UHD Graphics 630"
    qint64 dedicatedMB {0};
    qint64 sharedMB {0};
    bool software {false};      // renderer software (Microsoft Basic Render, llvmpipe...)
    bool glOk {false};          // contesto OpenGL 2.0 o superiore creato davvero
    QString glRenderer;
    QString glVersion;
  };

  GpuInfo const& detectGpu ();          // la prima chiamata va fatta dal thread grafico
  int gpuLoadPercent ();                // carico 3D in %; -1 se il sistema non lo fornisce
  qint64 availableRamMB ();             // memoria fisica libera; -1 se non disponibile

  enum class GpuWaterfall {Auto = 0, On = 1, Off = 2};
  GpuWaterfall gpuWaterfallSetting (QSettings *);
  // Decisione all'avvio: OpenGL funzionante, non software, RAM >= 3 GB, GPU non gia' oltre il 70%.
  bool gpuWaterfallWanted (QSettings *, QString * why = nullptr);
}
#endif
