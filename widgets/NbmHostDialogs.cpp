// NBMODES-IC8TEM — popup di adattamento al computer e popup di chiusura.
// Il popup di chiusura riprende quello di YAEC e YA-NEC (icona, domanda in grassetto,
// ringraziamento in corsivo, barra grigia con Si' e No).
#include "NbmHostDialogs.hpp"

#include <QStandardPaths>

#include <QApplication>
#include <QDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QStringList>
#include <QSysInfo>
#include <QThread>
#include <QVBoxLayout>
#include <QIcon>
#include <QStyle>

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <cpuid.h>
#include <cstring>
#endif

extern "C" int nbm_sldpc_avx2_c ();

namespace nbm
{
  namespace
  {
    QString cpuBrand ()
    {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
      unsigned max = __get_cpuid_max (0x80000000u, nullptr);
      if (max >= 0x80000004u)
        {
          char brand[49] {};
          unsigned * p = reinterpret_cast<unsigned *> (brand);
          for (unsigned leaf = 0; leaf < 3; ++leaf)
            __get_cpuid (0x80000002u + leaf, p + 4 * leaf, p + 4 * leaf + 1, p + 4 * leaf + 2, p + 4 * leaf + 3);
          return QString::fromLatin1 (brand).simplified ();
        }
#endif
      return QSysInfo::currentCpuArchitecture ();
    }

    QFrame * greyBar (QWidget * parent, QList<QPushButton *> const& buttons)
    {
      auto bar = new QFrame {parent};
      bar->setObjectName ("nbmGreyBar");
      bar->setStyleSheet ("#nbmGreyBar { background: palette(window); border-top: 1px solid palette(mid); }");
      bar->setAutoFillBackground (true);
      auto pal = bar->palette ();
      pal.setColor (QPalette::Window, pal.color (QPalette::Window).darker (108));
      bar->setPalette (pal);
      auto h = new QHBoxLayout {bar};
      h->setContentsMargins (16, 10, 16, 10);
      h->setSpacing (10);
      h->addStretch ();
      for (auto b : buttons) { b->setMinimumWidth (90); h->addWidget (b); }
      return bar;
    }

    // Trattino non separabile (U+2011): il nome non va mai a capo su "NBMODES-".
    QString nbmName () { return QString::fromUtf8 ("NBMODES\u2011IC8TEM"); }

    // Colore attenuato per il ringraziamento, leggibile su temi chiari e scuri.
    void muted (QLabel * l)
    {
      auto c = l->palette ().color (QPalette::WindowText);
      c.setAlphaF (0.70);
      l->setStyleSheet (QString {"color: rgba(%1,%2,%3,%4);"}.arg (c.red ()).arg (c.green ()).arg (c.blue ()).arg (c.alphaF ()));
    }

    // Le etichette a capo vanno misurate dopo aver fissato la larghezza, altrimenti Qt taglia l'ultima riga.
    void fitHeight (QDialog & d, int width)
    {
      d.layout ()->setSizeConstraint (QLayout::SetFixedSize);
      for (auto l : d.findChildren<QLabel *> ())
        if (l->wordWrap ()) l->setMinimumWidth (width - 64 - 16 - 40);
      d.adjustSize ();
    }

    QLabel * appIcon (QWidget * parent)
    {
      auto pic = new QLabel {parent};
      auto icon = QApplication::windowIcon ();
      if (icon.isNull ()) icon = parent->style ()->standardIcon (QStyle::SP_MessageBoxInformation);
      pic->setPixmap (icon.pixmap (64, 64));
      pic->setFixedSize (64, 64);
      pic->setAlignment (Qt::AlignTop);
      return pic;
    }
  }

  QString HostProfile::key () const
  {
    return QStringList {cpu, QString::number (cores), avx2 ? "avx2" : "sse2", os, screens}.join ('|');
  }

  HostProfile detectHost ()
  {
    HostProfile h;
    h.cpu = cpuBrand ();
    h.cores = QThread::idealThreadCount ();
    h.avx2 = nbm_sldpc_avx2_c () != 0;
    h.os = QSysInfo::prettyProductName ();
    h.kernel = QSysInfo::kernelType () + ' ' + QSysInfo::kernelVersion ();
    QStringList s;
    auto const screens = QGuiApplication::screens ();
    for (int i = 0; i < screens.size (); ++i)
      {
        auto const sc = screens[i];
        auto const dpr = sc->devicePixelRatio ();
        auto const px = sc->geometry ().size () * dpr;
        int const scale = qRound (100.0 * dpr * sc->logicalDotsPerInch () / 96.0);
        s << QObject::tr ("Monitor %1: %2×%3 pixel, scala di sistema %4%")
          .arg (i + 1).arg (px.width ()).arg (px.height ()).arg (scale);
      }
    h.screens = s.join ('\n');
    return h;
  }

  bool interactiveSession ()
  {
    if (qEnvironmentVariableIsSet ("NBM_NO_EXIT_PROMPT")) return false;
    if (QStandardPaths::isTestModeEnabled ()) return false;   // --test-mode e prove automatiche: niente finestre modali
    auto const platform = QGuiApplication::platformName ();
    return platform != "offscreen" && platform != "minimal";
  }

  void showHostAdaptation (QWidget * parent, QSettings * settings, bool force)
  {
    if (!interactiveSession () && !force) return;
    auto const h = detectHost ();
    auto const saved = settings ? settings->value ("NBM/HostProfile").toString () : QString {};
    if (!force && saved == h.key ()) return;
    if (settings) settings->setValue ("NBM/HostProfile", h.key ());

    QDialog d {parent};
    d.setWindowTitle (QObject::tr ("%1 si adatta a questo computer").arg (QStringLiteral ("NBMODES-IC8TEM")));
    d.setModal (true);
    auto v = new QVBoxLayout {&d};
    v->setContentsMargins (0, 0, 0, 0);
    v->setSpacing (0);
    auto top = new QWidget {&d};
    auto th = new QHBoxLayout {top};
    th->setContentsMargins (20, 20, 20, 16);
    th->setSpacing (16);
    th->addWidget (appIcon (&d), 0, Qt::AlignTop);
    auto text = new QVBoxLayout;
    auto title = new QLabel {QObject::tr ("%1 si è regolato su questo computer").arg (nbmName ()), top};
    auto f = title->font (); f.setBold (true); f.setPointSizeF (f.pointSizeF () * 1.25); title->setFont (f);
    title->setWordWrap (true);
    text->addWidget (title);

    QStringList found;
    found << QObject::tr ("Processore: %1, %2 thread").arg (h.cpu).arg (h.cores)
          << QObject::tr ("Sistema: %1 (%2)").arg (h.os, h.kernel);
    found << h.screens.split ('\n');
    QStringList set;
    set << (h.avx2 ? QObject::tr ("Decoder FT8: superldpc nella versione veloce AVX2")
                   : QObject::tr ("Decoder FT8: superldpc nella versione SSE2 per processori senza AVX2 (stesse decodifiche, un po' più lento)"))
        << QObject::tr ("Decoder FT8 multithread: numero di thread automatico (%1 disponibili)").arg (h.cores)
        << QObject::tr ("Interfaccia: scala del sistema operativo per ogni monitor");
    if (h.os.contains ("Windows 7") || h.os.contains ("Windows 8"))
      set << QObject::tr ("Su %1 alcune funzioni di rete recenti potrebbero non essere disponibili").arg (h.os);

    auto html = [] (QStringList const& lines) {
      QStringList out;
      for (auto const& l : lines) out << l.toHtmlEscaped ();
      return out.join ("<br>");
    };
    auto body = new QLabel {top};
    body->setTextFormat (Qt::RichText);
    body->setWordWrap (true);
    body->setTextInteractionFlags (Qt::TextSelectableByMouse);
    body->setText (QString {"<p><b>%1</b><br>%2</p><p><b>%3</b><br>%4</p><p><i>%5</i></p>"}
                   .arg (QObject::tr ("Che cosa ho trovato").toHtmlEscaped (), html (found),
                         QObject::tr ("Come mi sono regolato").toHtmlEscaped (), html (set),
                         QObject::tr ("Puoi cambiare queste scelte in Configurazione: le tue scelte hanno sempre la precedenza. "
                                      "Questo avviso ricompare solo se cambiano il computer o i monitor.").toHtmlEscaped ()));
    text->addWidget (body);
    th->addLayout (text, 1);
    v->addWidget (top);
    auto ok = new QPushButton {QObject::tr ("OK"), &d};
    ok->setDefault (true);
    QObject::connect (ok, &QPushButton::clicked, &d, &QDialog::accept);
    v->addWidget (greyBar (&d, {ok}));
    fitHeight (d, 580);
    d.exec ();
  }

  bool confirmExit (QWidget * parent)
  {
    QDialog d {parent};
    d.setWindowTitle (QObject::tr ("Chiusura del programma"));
    d.setModal (true);
    auto v = new QVBoxLayout {&d};
    v->setContentsMargins (0, 0, 0, 0);
    v->setSpacing (0);
    auto top = new QWidget {&d};
    auto th = new QHBoxLayout {top};
    th->setContentsMargins (20, 20, 20, 16);
    th->setSpacing (16);
    th->addWidget (appIcon (&d), 0, Qt::AlignTop);
    auto text = new QVBoxLayout;
    auto q = new QLabel {QObject::tr ("Vuoi chiudere %1?").arg (nbmName ()), top};
    auto fq = q->font (); fq.setBold (true); fq.setPointSizeF (fq.pointSizeF () * 1.3); q->setFont (fq);
    q->setWordWrap (true);
    auto grazie = new QLabel {QObject::tr ("Grazie per avermi usato: spero di esserti stato utile."), top};
    auto fg = grazie->font (); fg.setItalic (true); fg.setPointSizeF (fg.pointSizeF () * 1.1); grazie->setFont (fg);
    grazie->setWordWrap (true);
    muted (grazie);
    text->addWidget (q);
    text->addSpacing (8);
    text->addWidget (grazie);
    text->addStretch ();
    th->addLayout (text, 1);
    v->addWidget (top);
    auto si = new QPushButton {QObject::tr ("Sì"), &d};
    auto no = new QPushButton {QObject::tr ("No"), &d};
    si->setDefault (true);                    // Invio = Si'
    QObject::connect (si, &QPushButton::clicked, &d, &QDialog::accept);
    QObject::connect (no, &QPushButton::clicked, &d, &QDialog::reject);   // Esc = No (QDialog)
    v->addWidget (greyBar (&d, {si, no}));
    fitHeight (d, 480);
    return d.exec () == QDialog::Accepted;
  }
}
