// NBMODES-IC8TEM — innesto di superldpc (Decodium, GPLv3) nel decoder FT8 multithread (lib/ft8var).
// Modo (variabile d'ambiente NBM_SLDPC, poi dalla tendina decoder):
//   0 = decoder originale soltanto ("WSJT-X puro")
//   1 = superldpc al posto di BP+OSD (come Decodium)
//   2 = originale, poi superldpc sui candidati falliti (predefinito)
// Backend scelto a runtime: AVX2+FMA se la CPU (e il sistema) li supportano, altrimenti SSE2.
#include <cstdlib>
#include <cstring>

#if defined(NBM_SUPERLDPC_X86)
extern "C" void superldpc_simd_set_ft8_mode_c (int);
extern "C" void superldpc_simd_decode174_91_c (float const*, int, int, int, signed char const*,
                                               signed char*, signed char*, int*, int*, float*);
extern "C" void superldpc_sse2_set_ft8_mode_c (int);
extern "C" void superldpc_sse2_decode174_91_c (float const*, int, int, int, signed char const*,
                                               signed char*, signed char*, int*, int*, float*);
#endif

namespace {
bool cpu_has_avx2 ()
{
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
  __builtin_cpu_init ();
  return __builtin_cpu_supports ("avx2") && __builtin_cpu_supports ("fma");
#else
  return false;
#endif
}
bool const g_avx2 = cpu_has_avx2 () && !std::getenv ("NBM_SLDPC_FORCE_SSE2");   // la variabile serve solo per i test
int g_mode = [] {
  char const* e = std::getenv ("NBM_SLDPC");
  return e ? std::atoi (e) : 2;
} ();
}

// Usato dal backend solo per FT2: non serve in FT8, resta definito per il linker.
extern "C" void ftx_ft2_rvec_c (signed char* out77)
{
  static const int r[77]={0,1,0,0,1,0,1,0,0,1,0,1,1,1,1,0,1,0,0,0,1,0,0,1,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,0,0,1,0,0,0,1,0,1,0,0,1,1,1,1,0,0,1,0,1,0,1,0,1,0,1,1,0,1,1,1,1,1,0,0,0,1,0,1};
  if (out77) for (int i = 0; i < 77; ++i) out77[i] = (signed char) r[i];
}

extern "C" int nbm_sldpc_mode_ () { return g_mode; }
extern "C" void nbm_sldpc_set_mode_c (int m) { g_mode = m; }
extern "C" int nbm_sldpc_avx2_c () { return g_avx2 ? 1 : 0; }

extern "C" void nbm_sldpc_decode_ (float const* llr, signed char const* apmask, signed char* message77,
                                   signed char* cw, int* nharderrors, float* dmin)
{
  signed char m91[91]; int ntype = 0;
#if !defined(NBM_SUPERLDPC_X86)
  (void) llr; (void) apmask; (void) cw; (void) dmin; (void) m91; (void) ntype;
  *nharderrors = -1;        // nessun backend su questa architettura: resta il decoder originale
  return;
#else
  if (g_avx2) {
    superldpc_simd_set_ft8_mode_c (1);
    superldpc_simd_decode174_91_c (llr, 91, 2, 2, apmask, m91, cw, &ntype, nharderrors, dmin);
  } else {
    superldpc_sse2_set_ft8_mode_c (1);
    superldpc_sse2_decode174_91_c (llr, 91, 2, 2, apmask, m91, cw, &ntype, nharderrors, dmin);
  }
  if (*nharderrors >= 0) std::memcpy (message77, m91, 77);
#endif
}
