#include "about.h"

#include <QCoreApplication>
#include <QString>

#include "revision_utils.hpp"

#include "ui_about.h"

CAboutDlg::CAboutDlg(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::CAboutDlg)
{
  ui->setupUi(this);

  ui->labelTxt->setText ("<h2>" + QString {"NBMODES-IC8TEM v"
                                             + QCoreApplication::applicationVersion ()
                                             + " " + display_revision ()}.simplified () + "</h2>"

    "Programma per i modi digitali a banda stretta del radioamatore.<br />"
    "&copy; 2026 Costantino Cerrotta, IC8TEM &mdash; GPLv3<br /><br />"
    "<b>Basato su WSJT-X 3.3.0-beta8</b><br />"
    "WSJT-X&trade; &copy; 2001-2026 by Joe Taylor, K1JT, Bill Somerville, G4WJS, <br />"
    "Steve Franke, K9AN, Nico Palermo, IV3NWV, <br />"
    "Uwe Risse, DG2YCB, Brian Moran, N9ADG, <br />"
    "Roger Rehr, W3SZ, John Nelson, G4KLA, <br />"
    "Charlie Suckling, DL3WDG, Terrell Deppe, KJ5HST,<br />"
    "and David Christle, KD0BTO; contributions from AC6SL, AE4JY,<br />"
    "DF2ET, DJ0OT, DL3WDG, EA4AC, G4KLA, IW3RAB, JA7UDE,<br />"
    "K3WYC, KA1GT, KA6MAL, KA9Q, KB1ZMX, KD6EKQ, KG4IYS, KI7MT,<br />"
    "KK1D, ND0B, PY1ZRJ, PY2SDR, VE1SKY, VK3ACF, VK4BDJ,<br />"
    "VK7MO, VR2UPU, W3DJS, W4TI, W4TV, and W9MDB.<br /><br />"
    "<b>Altri codici e idee, con i loro autori</b><br />"
    "JTDX (decoder FT8 multithread ft8var) &mdash; UA3DJY, ES1JA e team JTDX, GPLv3<br />"
    "Decodium (superldpc) &mdash; IU8LMC, GPLv3<br />"
    "MSHV &mdash; LZ2HV, GPLv3; MadModem (correzione del drift) &mdash; IZ6NNH, per la condivisione di idee<br />"
    "Locatori: G7VRD; CALL3: WSJT-X Improved (DG2YCB), W7GJ, JTDX<br /><br />"
    "NBMODES-IC8TEM e WSJT-X sono distribuiti con la licenza<br />"
    "GNU General Public License versione 3 (GPL) <br /><br />"
    "<a href=" TO_STRING__ (PROJECT_HOMEPAGE) ">"
    "<img src=\":/icon_128x128.png\" /></a>"
    "<a href=\"https://www.gnu.org/licenses/gpl-3.0.txt\">"
    "<img src=\":/gpl-v3-logo.svg\" height=\"80\" /><br />"
    "https://www.gnu.org/licenses/gpl-3.0.txt</a>");
}

CAboutDlg::~CAboutDlg()
{
}
