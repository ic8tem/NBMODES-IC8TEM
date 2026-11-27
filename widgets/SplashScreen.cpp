#include "SplashScreen.hpp"

#include <QPixmap>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>

#include "revision_utils.hpp"
#include "pimpl_impl.hpp"

#include "moc_SplashScreen.cpp"

class SplashScreen::impl
{
public:
  impl ()
    : checkbox_ {"Do not show this again"}
  {
    main_layout_.addStretch ();
    main_layout_.addWidget (&checkbox_, 0, Qt::AlignRight);
  }

  QVBoxLayout main_layout_;
  QCheckBox checkbox_;
};

SplashScreen::SplashScreen ()
  : QSplashScreen {QPixmap {":/splash.png"}, Qt::WindowStaysOnTopHint}
{
  setLayout (&m_->main_layout_);

  setObjectName ("SplashScreen");
  setWindowTitle (QCoreApplication::translate ("SplashScreen", "NBMODES-IC8TEM — Benvenuto"));
  setAccessibleName (windowTitle ());
  setAccessibleDescription (
    QCoreApplication::translate ("SplashScreen",
      "Informazioni di avvio di NBMODES-IC8TEM. Esc per chiudere."));
  m_->checkbox_.setObjectName ("doNotShowSplashAgain");
  m_->checkbox_.setAccessibleName (m_->checkbox_.text ());

  // A QSplashScreen is a non-activating window and never becomes the key
  // window, so a key press event override would never fire. Filter the
  // application instead so Escape dismisses the splash regardless of which
  // window holds focus, giving keyboard and assistive-tech users a dismiss
  // path that the mouse-only click-to-dismiss does not.
  QCoreApplication::instance ()->installEventFilter (this);

  showMessage ("<h2>" + QString {"NBMODES-IC8TEM v" +
        QCoreApplication::applicationVersion() + " " +
        revision ()}.simplified () + "</h2>"
    "di Costantino Cerrotta, IC8TEM &mdash; basato su WSJT-X 3.3.0-beta8 (K1JT e WSJT Development Group)<br />"
    "Segnalazioni: https://github.com/ic8tem/NBMODES-IC8TEM/issues<br />"
    "(salva i file .wav quando serve)<br /><br />"
    "<b>Crediti e licenze nel menu Aiuto &rsaquo; Informazioni.</b><br />"
    "<img src=\":/icon_128x128.png\" />"
    "<img src=\":/gpl-v3-logo.svg\" height=\"80\" />", Qt::AlignCenter);
  connect (&m_->checkbox_, &QCheckBox::stateChanged, [this] (int s) {
      if (Qt::Checked == s) Q_EMIT disabled ();
    });
}

SplashScreen::~SplashScreen ()
{
}

bool SplashScreen::eventFilter (QObject * object, QEvent * event)
{
  if (isVisible () && QEvent::KeyPress == event->type ()
      && Qt::Key_Escape == static_cast<QKeyEvent *> (event)->key ())
    {
      close ();
      return true;
    }
  return QSplashScreen::eventFilter (object, event);
}
