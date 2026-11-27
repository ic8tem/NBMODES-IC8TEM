#include "NbmGpu.hpp"
#include "NbmHostDialogs.hpp"

#include <QFile>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSettings>
#include <QThread>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <dxgi.h>
#endif

namespace nbm
{
  namespace
  {
#ifdef _WIN32
    void dxgiInfo (GpuInfo & g)
    {
      auto dll = LoadLibraryW (L"dxgi.dll");
      if (!dll) return;
      using Create = HRESULT (WINAPI *) (REFIID, void **);
      auto create = reinterpret_cast<Create> (reinterpret_cast<void *> (GetProcAddress (dll, "CreateDXGIFactory1")));
      IDXGIFactory1 * f = nullptr;
      if (create && SUCCEEDED (create (__uuidof (IDXGIFactory1), reinterpret_cast<void **> (&f))) && f)
        {
          IDXGIAdapter1 * a = nullptr;
          qint64 best = -1;
          for (UINT i = 0; f->EnumAdapters1 (i, &a) != DXGI_ERROR_NOT_FOUND; ++i)
            {
              DXGI_ADAPTER_DESC1 d;
              if (SUCCEEDED (a->GetDesc1 (&d)))
                {
                  bool const sw = d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
                  qint64 const score = (sw ? 0 : (qint64 (1) << 40)) + qint64 (d.DedicatedVideoMemory) + qint64 (d.SharedSystemMemory) / 4;
                  if (score > best)
                    {
                      best = score;
                      g.name = QString::fromWCharArray (d.Description).trimmed ();
                      g.dedicatedMB = qint64 (d.DedicatedVideoMemory / (1024 * 1024));
                      g.sharedMB = qint64 (d.SharedSystemMemory / (1024 * 1024));
                      g.software = sw;
                    }
                }
              a->Release ();
            }
          f->Release ();
        }
      // la DLL resta caricata: la usa anche Qt
    }

    // Contatori PDH caricati a runtime: su Windows 7 "GPU Engine" non esiste e si risponde -1.
    struct Pdh
    {
      using Open = LONG (WINAPI *) (LPCWSTR, DWORD_PTR, HANDLE *);
      using Add = LONG (WINAPI *) (HANDLE, LPCWSTR, DWORD_PTR, HANDLE *);
      using Collect = LONG (WINAPI *) (HANDLE);
      using Array = LONG (WINAPI *) (HANDLE, DWORD, LPDWORD, LPDWORD, void *);
      HMODULE dll {LoadLibraryW (L"pdh.dll")};
      Open open {dll ? reinterpret_cast<Open> (reinterpret_cast<void *> (GetProcAddress (dll, "PdhOpenQueryW"))) : nullptr};
      Add add {dll ? reinterpret_cast<Add> (reinterpret_cast<void *> (GetProcAddress (dll, "PdhAddEnglishCounterW"))) : nullptr};
      Collect collect {dll ? reinterpret_cast<Collect> (reinterpret_cast<void *> (GetProcAddress (dll, "PdhCollectQueryData"))) : nullptr};
      Array array {dll ? reinterpret_cast<Array> (reinterpret_cast<void *> (GetProcAddress (dll, "PdhGetFormattedCounterArrayW"))) : nullptr};
      HANDLE query {nullptr};
      HANDLE counter {nullptr};
      bool ok {false};
      Pdh ()
      {
        if (!open || !add || !collect || !array) return;
        if (open (nullptr, 0, &query) != 0) return;
        if (add (query, L"\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &counter) != 0) return;
        collect (query);
        ok = true;
      }
    };
#endif
  }

  GpuInfo const& detectGpu ()
  {
    static GpuInfo const info = [] {
      GpuInfo g;
#ifdef _WIN32
      dxgiInfo (g);
#endif
      QOffscreenSurface surface;
      surface.create ();
      QOpenGLContext ctx;
      if (surface.isValid () && ctx.create () && ctx.makeCurrent (&surface))
        {
          auto * f = ctx.functions ();
          auto str = [f] (GLenum e) {
            auto const * s = reinterpret_cast<char const *> (f->glGetString (e));
            return s ? QString::fromLatin1 (s) : QString {};
          };
          g.glRenderer = str (GL_RENDERER);
          g.glVersion = str (GL_VERSION);
          auto const fmt = ctx.format ();
          g.glOk = ctx.isValid () && (fmt.majorVersion () >= 2 || ctx.isOpenGLES ());
          ctx.doneCurrent ();
        }
      auto const r = g.glRenderer.toLower ();
      if (r.contains ("llvmpipe") || r.contains ("softpipe") || r.contains ("swrast")
          || r.contains ("gdi generic") || r.contains ("basic render")) g.software = true;
      if (g.name.isEmpty ()) g.name = g.glRenderer;
      return g;
    } ();
    return info;
  }

  int gpuLoadPercent ()
  {
#ifdef _WIN32
    static Pdh pdh;
    if (!pdh.ok) return -1;
    if (pdh.collect (pdh.query) != 0) return -1;
    // PDH_FMT_DOUBLE = 0x200; PDH_MORE_DATA = 0x800007D2
    DWORD bytes = 0, count = 0;
    pdh.array (pdh.counter, 0x200, &bytes, &count, nullptr);
    if (!bytes) return -1;
    QByteArray buf (int (bytes), '\0');
    if (pdh.array (pdh.counter, 0x200, &bytes, &count, buf.data ()) != 0) return -1;
    struct Item { LPWSTR name; struct { DWORD status; DWORD pad; double value; } v; };
    double sum = 0;
    auto const * items = reinterpret_cast<Item const *> (buf.constData ());
    for (DWORD i = 0; i < count; ++i) sum += items[i].v.value;
    return int (std::min (100.0, sum) + 0.5);
#else
    return -1;
#endif
  }

  qint64 availableRamMB ()
  {
#ifdef _WIN32
    MEMORYSTATUSEX m;
    m.dwLength = sizeof m;
    return GlobalMemoryStatusEx (&m) ? qint64 (m.ullAvailPhys / (1024 * 1024)) : -1;
#else
    QFile f {"/proc/meminfo"};
    if (!f.open (QIODevice::ReadOnly)) return -1;
    while (!f.atEnd ())
      {
        auto const l = f.readLine ();
        if (l.startsWith ("MemAvailable:"))
          return l.mid (13).trimmed ().split (' ').value (0).toLongLong () / 1024;
      }
    return -1;
#endif
  }

  GpuWaterfall gpuWaterfallSetting (QSettings * s)
  {
    return s ? GpuWaterfall (s->value ("NBM/GpuWaterfall", 0).toInt ()) : GpuWaterfall::Auto;
  }

  bool gpuWaterfallWanted (QSettings * s, QString * why)
  {
    auto const set = gpuWaterfallSetting (s);
    auto const& g = detectGpu ();
    auto say = [why] (QString const& t) { if (why) *why = t; };
    if (set == GpuWaterfall::Off) {say (QObject::tr ("disegnato dalla CPU (scelta tua)")); return false;}
    if (!g.glOk) {say (QObject::tr ("disegnato dalla CPU: OpenGL non disponibile")); return false;}
    if (set == GpuWaterfall::On) {say (QObject::tr ("disegnato dalla GPU (scelta tua)")); return true;}
    if (g.software) {say (QObject::tr ("disegnato dalla CPU: c'e' solo un driver grafico software")); return false;}
    auto const ram = totalRamMB ();
    if (ram > 0 && ram < 3000) {say (QObject::tr ("disegnato dalla CPU: meno di 3 GB di RAM")); return false;}
    auto const load = gpuLoadPercent ();
    if (load > 70) {say (QObject::tr ("disegnato dalla CPU: la GPU e' gia' occupata al %1%").arg (load)); return false;}
    say (QObject::tr ("disegnato dalla GPU (OpenGL)"));
    return true;
  }
}
