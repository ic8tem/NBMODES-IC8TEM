#include "NbmStrips.hpp"

#include <QAction>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace
{
  QPushButton * pill (QString const& text, QWidget * parent)
  {
    auto * b = new QPushButton {text, parent};
    b->setProperty ("nbmPill", true);
    b->setCheckable (true);
    b->setFocusPolicy (Qt::NoFocus);
    b->setCursor (Qt::PointingHandCursor);
    return b;
  }

  QWidget * separator (QWidget * parent)
  {
    auto * f = new QFrame {parent};
    f->setFrameShape (QFrame::VLine);
    f->setFixedHeight (18);
    return f;
  }

  QString bandLabel (QString t)
  {
    // "20m" -> "20 m", "70cm" -> "70 cm", "1.25cm" -> "1,25 cm"
    t = t.trimmed ();
    static QRegularExpression const re {"^([0-9.]+)\\s*(mm|cm|m)$"};
    auto const m = re.match (t);
    if (!m.hasMatch ()) return t;
    auto num = m.captured (1);
    num.replace ('.', ',');
    return num + ' ' + m.captured (2);
  }
}

namespace nbm
{
  TopBars::TopBars (QWidget * parent)
    : QWidget {parent}
  {
    auto * v = new QVBoxLayout {this};
    v->setContentsMargins (0, 0, 0, 0);
    v->setSpacing (0);

    header_ = new QWidget {this};
    header_->setObjectName ("nbmHeader");
    header_->setAttribute (Qt::WA_StyledBackground);
    auto * h = new QHBoxLayout {header_};
    h->setContentsMargins (14, 6, 14, 6);
    h->setSpacing (14);
    auto * title = new QLabel {"NBMODES-IC8TEM", header_};
    title->setObjectName ("nbmTitle");
    auto tf = title->font ();
    tf.setBold (true);
    tf.setPointSizeF (tf.pointSizeF () * 1.45);
    title->setFont (tf);
    subtitle_ = new QLabel {tr ("modi digitali a banda stretta\na cura di Costantino IC8TEM"), header_};
    subtitle_->setObjectName ("nbmSubtitle");
    auto * spectrum = new QPushButton {tr ("Apri spettro"), header_};
    auto * ms = new QPushButton {tr ("Apri display MS"), header_};
    for (auto * b : {spectrum, ms}) b->setFocusPolicy (Qt::NoFocus);
    connect (spectrum, &QPushButton::clicked, this, &TopBars::openSpectrum);
    connect (ms, &QPushButton::clicked, this, &TopBars::openMsDisplay);
    h->addWidget (title);
    h->addWidget (subtitle_);
    h->addStretch (1);
    h->addWidget (spectrum);
    h->addWidget (ms);
    v->addWidget (header_);

    auto strip = [this, v] (QString const& name, QString const& label) {
      auto * w = new QWidget {this};
      w->setObjectName (name);
      w->setAttribute (Qt::WA_StyledBackground);
      auto * l = new QHBoxLayout {w};
      l->setContentsMargins (14, 4, 14, 4);
      l->setSpacing (5);
      auto * lab = new QLabel {label, w};
      lab->setMinimumWidth (44);
      l->addWidget (lab);
      v->addWidget (w);
      return l;
    };
    modes_ = strip ("nbmModeStrip", tr ("Modo"));
    modes_->addStretch (1);
    bands_ = strip ("nbmBandStrip", tr ("Banda"));
    bands_->addStretch (1);
    bands_->parentWidget ()->hide ();   // visibile solo con setBandSource ()
  }

  void TopBars::addModeGroup (QList<QAction *> const& actions)
  {
    auto * parent = modes_->parentWidget ();
    int insertAt = modes_->count () - 1;        // prima dello stretch finale
    if (modeGroups_++ > 0) modes_->insertWidget (insertAt++, separator (parent));
    for (auto * a : actions)
      {
        if (!a) continue;
        auto * b = pill (a->text ().remove ('&'), parent);
        b->setToolTip (a->toolTip ());
        b->setChecked (a->isChecked ());
        b->setVisible (a->isVisible ());
        connect (b, &QPushButton::clicked, a, [a, b] {
            if (!a->isChecked ()) a->trigger ();
            b->setChecked (a->isChecked ());
          });
        connect (a, &QAction::toggled, b, &QPushButton::setChecked);
        connect (a, &QAction::changed, b, [a, b] { b->setVisible (a->isVisible ()); b->setEnabled (a->isEnabled ()); });
        modes_->insertWidget (insertAt++, b);
      }
  }

  void TopBars::setBandSource (QComboBox * bands)
  {
    bandSource_ = bands;
    if (!bands) return;
    bands_->parentWidget ()->show ();
    auto * m = bands->model ();
    connect (m, &QAbstractItemModel::modelReset, this, &TopBars::rebuildBands);
    connect (m, &QAbstractItemModel::rowsInserted, this, &TopBars::rebuildBands);
    connect (m, &QAbstractItemModel::rowsRemoved, this, &TopBars::rebuildBands);
    connect (m, &QAbstractItemModel::layoutChanged, this, &TopBars::rebuildBands);
    connect (bands, QOverload<int>::of (&QComboBox::currentIndexChanged), this, [this] (int i) {
        for (int k = 0; k < bandPills_.size (); ++k) bandPills_[k]->setChecked (k == i);
      });
    rebuildBands ();
  }

  void TopBars::rebuildBands ()
  {
    for (auto * b : bandPills_) b->deleteLater ();
    bandPills_.clear ();
    if (!bandSource_) return;
    auto * parent = bands_->parentWidget ();
    int const n = bandSource_->count ();
    for (int i = 0; i < n; ++i)
      {
        auto const text = bandSource_->itemText (i);
        if (text.trimmed ().isEmpty ()) continue;
        auto * b = pill (bandLabel (text), parent);
        b->setChecked (i == bandSource_->currentIndex ());
        connect (b, &QPushButton::clicked, this, [this, i] {
            if (!bandSource_) return;
            bandSource_->setCurrentIndex (i);
            Q_EMIT bandSource_->activated (i);
          });
        bands_->insertWidget (bands_->count () - 1, b);
        bandPills_ << b;
      }
  }

  void TopBars::setCompact (bool compact)
  {
    subtitle_->setVisible (!compact);
    auto * hl = header_->layout ();
    hl->setContentsMargins (14, compact ? 2 : 6, 14, compact ? 2 : 6);
  }
}
