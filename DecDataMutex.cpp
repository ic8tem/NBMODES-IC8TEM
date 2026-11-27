#include "DecDataMutex.hpp"
#include "NbmLazyAlloc.hpp"

#include <atomic>
#include <QMutex>

namespace
{
std::atomic_bool& input_blocked ()
{
  static std::atomic_bool blocked {false};
  return blocked;
}
}

QMutex& dec_data_mutex()
{
  static QMutex mutex;
  return mutex;
}

bool dec_data_input_blocked ()
{
  return input_blocked ().load (std::memory_order_acquire);
}

void set_dec_data_input_blocked (bool blocked)
{
  input_blocked ().store (blocked, std::memory_order_release);
}

// NBMODES-IC8TEM: allocazione a zeri su richiesta (vedi NbmLazyAlloc.hpp). Sta qui perché ogni
// programma che usa le aree audio collega già DecDataMutex.cpp.

#include <new>
#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <sys/mman.h>
#endif

namespace nbm
{
  void * lazy_zero_alloc (std::size_t n)
  {
#ifdef _WIN32
    void * p = VirtualAlloc (nullptr, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!p) throw std::bad_alloc {};
#else
    void * p = mmap (nullptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) throw std::bad_alloc {};
#endif
    return p;
  }

  void lazy_free (void * p, std::size_t n)
  {
    if (!p) return;
#ifdef _WIN32
    (void) n;
    VirtualFree (p, 0, MEM_RELEASE);
#else
    munmap (p, n);
#endif
  }
}
