#include "NbmTheme.hpp"

#include <QFontDatabase>
#include <QHash>
#include <QRegularExpression>

namespace
{
  // Colori presi dai bozzetti (Main.dc / Colori.dc), stessi nomi di variabile
  QHash<QString, QString> const& themes ()
  {
    static QHash<QString, QString> const t {
      {"Chiaro", "back:#F0F0F0;panel:#FFFFFF;card:#FFFFFF;head:#E6E8F0;border:#D5DCEA;fore:#000000;fore2:#1B2430;muted:#6D6D6D;bar:#1E293B;barborder:#2C384F;barfore:#E2E8F0;barmuted:#94A3B8;barbtn:#475569;accent:#1F6FD1;accentfore:#FFFFFF;accentsoft:#EBF5FF;pill:#FFFFFF;pillfore:#2554A0;pillborder:#D5DCEA;green:#139650;orange:#D97B00;warnbg:#FFF5DC"},
      {"Notte", "back:#0B1220;panel:#121B2E;card:#121B2E;head:#1A2640;border:#24314F;fore:#DBE4F5;fore2:#C3CEE3;muted:#7F8DB0;bar:#060B16;barborder:#24314F;barfore:#DBE4F5;barmuted:#7F8DB0;barbtn:#2E4870;accent:#5FA8FF;accentfore:#0B1220;accentsoft:#162846;pill:#1C2842;pillfore:#DBE4F5;pillborder:#24314F;green:#3DDC84;orange:#FFB340;warnbg:#4A3A1A"},
      {"Nero", "back:#000000;panel:#121212;card:#121212;head:#222222;border:#343434;fore:#EBEBEB;fore2:#D0D0D0;muted:#8C8C8C;bar:#0C0C0C;barborder:#343434;barfore:#EBEBEB;barmuted:#8C8C8C;barbtn:#444444;accent:#50B4FF;accentfore:#000000;accentsoft:#0E1E38;pill:#202020;pillfore:#EBEBEB;pillborder:#343434;green:#3CEB82;orange:#FFAA28;warnbg:#46340E"},
      {"Pastello azzurro", "back:#D3E6F9;panel:#F6FAFE;card:#F0F7FD;head:#C0DBF6;border:#C7E0F8;fore:#000000;fore2:#1B2430;muted:#5E6670;bar:#1E293B;barborder:#2C384F;barfore:#E2E8F0;barmuted:#94A3B8;barbtn:#475569;accent:#1F6FD1;accentfore:#FFFFFF;accentsoft:#E9F2FC;pill:#F0F7FD;pillfore:#2554A0;pillborder:#C7E0F8;green:#139650;orange:#D97B00;warnbg:#FFF5DC"},
      {"Scia meteorica", "back:#161230;panel:#1C1740;card:#1C1740;head:#221C47;border:#3A3270;fore:#EEEBFA;fore2:#D9D4F0;muted:#B3ADD6;bar:#0E0B22;barborder:#3A3270;barfore:#EEEBFA;barmuted:#B3ADD6;barbtn:#3A3270;accent:#38BDF8;accentfore:#0E0B22;accentsoft:#0B2233;pill:#2B2160;pillfore:#EEEBFA;pillborder:#3A3270;green:#86EFAC;orange:#FB923C;warnbg:#3A2312"},
    };
    return t;
  }

  QHash<QString, QString> vars (QString const& name)
  {
    QHash<QString, QString> v;
    for (auto const& kv : themes ().value (name).split (';', Qt::SkipEmptyParts))
      {
        auto const i = kv.indexOf (':');
        if (i > 0) v.insert (kv.left (i), kv.mid (i + 1));
      }
    return v;
  }

  char const * const kBegin = "/*NBM-THEME-BEGIN*/";
  char const * const kEnd = "/*NBM-THEME-END*/";

  // Il blocco agisce solo sulle superfici e sui controlli comuni; i pulsanti con colori di stato
  // (Abilita TX, Halt, Tune...) hanno un loro foglio di stile per widget e restano come in WSJT-X.
  char const * const kTemplate = R"QSS(
QMainWindow, QDialog { background-color: @back@; }
QMainWindow > QWidget, QDialog > QWidget { color: @fore@; }
QLabel, QCheckBox, QRadioButton, QGroupBox { color: @fore@; }
QGroupBox { border: 1px solid @border@; border-radius: 6px; margin-top: 8px; padding-top: 6px; }
QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: @fore2@; }
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateTimeEdit, QTextEdit, QPlainTextEdit, QTextBrowser, QListView, QTreeView, QTableView {
  background-color: @panel@; color: @fore@; border: 1px solid @pillborder@; border-radius: 4px;
  selection-background-color: @accent@; selection-color: @accentfore@; }
QComboBox QAbstractItemView { background-color: @panel@; color: @fore@; }
QHeaderView::section { background-color: @head@; color: @fore2@; border: 0; border-right: 1px solid @border@; padding: 3px 6px; }
QPushButton { background-color: @pill@; color: @fore@; border: 1px solid @pillborder@; border-radius: 5px; padding: 3px 9px; }
QPushButton:hover { border-color: @accent@; }
QPushButton:disabled { color: @muted@; }
QTabWidget::pane { border: 1px solid @border@; border-radius: 6px; }
QTabBar::tab { background: @head@; color: @fore2@; padding: 5px 12px; border: 1px solid @border@; border-bottom: 0; border-top-left-radius: 5px; border-top-right-radius: 5px; }
QTabBar::tab:selected { background: @panel@; color: @fore@; }
QMenuBar { background-color: @bar@; color: @barfore@; }
QMenuBar::item { background: transparent; padding: 4px 10px; }
QMenuBar::item:selected { background: @barbtn@; }
QMenu { background-color: @panel@; color: @fore@; border: 1px solid @border@; }
QMenu::item:selected { background-color: @accent@; color: @accentfore@; }
QStatusBar { background-color: @bar@; color: @barmuted@; }
QStatusBar QLabel { color: @barfore@; }
QToolTip { background-color: @panel@; color: @fore@; border: 1px solid @border@; }
QSplitter::handle { background-color: @border@; }
QScrollBar:vertical, QScrollBar:horizontal { background: @back@; }
QScrollBar::handle { background: @pillborder@; border-radius: 4px; }
DisplayText { background-color: @panel@; color: @fore@; border: 1px solid @border@; border-radius: 6px; }
QWidget#nbmHeader { background-color: @bar@; border-bottom: 1px solid @barborder@; }
QWidget#nbmHeader QLabel { color: @barfore@; }
QWidget#nbmHeader QLabel#nbmSubtitle { color: @barmuted@; }
QWidget#nbmHeader QLabel#nbmTitle { font-size: 16pt; font-weight: 700; }
QLabel#lh_decodes_headings_label, QLabel#rh_decodes_headings_label { background-color: @head@; color: @fore2@; }
QWidget#nbmHeader QPushButton { background: transparent; color: @barfore@; border: 1px solid @barbtn@; border-radius: 6px; padding: 5px 12px; }
QWidget#nbmHeader QPushButton:hover { background: @barbtn@; }
QWidget#nbmModeStrip, QWidget#nbmBandStrip { border-bottom: 1px solid @border@; }
QWidget#nbmModeStrip QLabel, QWidget#nbmBandStrip QLabel { color: @muted@; }
QPushButton[nbmPill="true"] { background: @pill@; color: @pillfore@; border: 1px solid @pillborder@; border-radius: 12px; padding: 3px 10px; }
QPushButton[nbmPill="true"]:checked { background: @accent@; color: @accentfore@; border: 0; font-weight: 600; }
QPushButton[nbmPill="true"]:hover { border-color: @accent@; }
)QSS";
}

namespace nbm
{
  QStringList themeNames ()
  {
    return {"WSJT-X classico", "Chiaro", "Notte", "Nero", "Pastello azzurro", "Scia meteorica"};
  }

  QString defaultThemeName () {return "Chiaro";}

  QString themeColor (QString const& name, QString const& var)
  {
    return vars (name).value (var);
  }

  QString themeStyleSheet (QString const& name)
  {
    if (!themes ().contains (name)) return {};
    auto const v = vars (name);
    QString qss = QString::fromLatin1 (kTemplate);
    static QRegularExpression const re {"@([a-z0-9]+)@"};
    QString out;
    int last = 0;
    auto it = re.globalMatch (qss);
    while (it.hasNext ())
      {
        auto const m = it.next ();
        out += qss.mid (last, m.capturedStart () - last);
        out += v.value (m.captured (1), "#808080");
        last = m.capturedEnd ();
      }
    out += qss.mid (last);
    return out;
  }

  QString applyThemeTo (QString const& baseStyleSheet, QString const& name)
  {
    QString base = baseStyleSheet;
    auto const b = base.indexOf (kBegin);
    if (b >= 0)
      {
        auto const e = base.indexOf (kEnd, b);
        base.remove (b, (e < 0 ? base.size () : e + int (qstrlen (kEnd))) - b);
      }
    auto const qss = themeStyleSheet (name);
    if (qss.isEmpty ()) return base;
    return base + '\n' + kBegin + qss + kEnd;
  }

  bool loadInterFont ()
  {
    static int const loaded = [] {
      int n = 0;
      for (auto const* f : {":/nbm/fonts/Inter-Regular.ttf", ":/nbm/fonts/Inter-Medium.ttf",
                            ":/nbm/fonts/Inter-SemiBold.ttf", ":/nbm/fonts/Inter-Bold.ttf"})
        if (QFontDatabase::addApplicationFont (f) >= 0) ++n;
      return n;
    } ();
    return loaded > 0;
  }

  QFont defaultUiFont (QFont const& systemFont)
  {
    QFont f = systemFont;
    if (loadInterFont ())
      {
        f.setFamily ("Inter");
        auto const size = systemFont.pointSizeF () > 0 ? systemFont.pointSizeF () : 9.;
        f.setPointSizeF (size * 0.9);
      }
    return f;
  }
}
