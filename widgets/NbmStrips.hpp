#ifndef NBM_STRIPS_HPP_
#define NBM_STRIPS_HPP_
// NBMODES-IC8TEM: barra superiore, fascia dei modi e fascia delle bande dei bozzetti (Main.dc).
// Le pillole dei modi usano le azioni del menu Modo; quelle delle bande la tendina delle bande
// (cioe' le bande con frequenze di lavoro scelte in Configurazione per il modo attivo).
#include <QWidget>
#include <QList>
#include <QPointer>

class QAction;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QPushButton;

namespace nbm
{
  class TopBars : public QWidget
  {
    Q_OBJECT
  public:
    explicit TopBars (QWidget * parent = nullptr);
    void addModeGroup (QList<QAction *> const& actions);   // un gruppo, separato dal precedente
    void setBandSource (QComboBox * bands);
    void setCompact (bool compact);
    Q_SIGNAL void openSpectrum ();
    Q_SIGNAL void openMsDisplay ();
  private:
    void rebuildBands ();
    QWidget * header_;
    QLabel * subtitle_;
    QHBoxLayout * modes_;
    QHBoxLayout * bands_;
    QPointer<QComboBox> bandSource_;
    QList<QPushButton *> bandPills_;
    int modeGroups_ {0};
  };
}
#endif
