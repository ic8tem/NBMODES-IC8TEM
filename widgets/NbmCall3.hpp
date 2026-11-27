#ifndef NBM_CALL3_HPP_
#define NBM_CALL3_HPP_
// NBMODES-IC8TEM: aggiornamento di CALL3.TXT con le stesse regole di LIVETROPOVHF.
// Fonti, nell'ordine (la prima che ha un nominativo vince): CALL3.TXT di WSJT-X Improved,
// CALL3.TXT EME di W7GJ, grid_data.bin di JTDX. Le voci a mano // [LIVETROPOVHF-MANUALI] e le
// correzioni dal cluster // [LIVETROPOVHF-CLUSTER] restano in cima invariate; restano anche le voci
// del file precedente che le fonti non hanno, e un locatore a 6 caratteri dove una fonte ne ha 4
// dello stesso quadrato. Sotto i 50.000 nominativi il file non si tocca. Il vecchio va in CALL3.bak.
// Un solo file, nello stesso formato (CALL,LOCATORE,,,): vale anche per LIVETROPOVHF, JTDX e MSHV.
#include <QObject>
#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

namespace nbm
{
  struct Call3Source { QString name; QString url; bool binary; };

  struct Call3MergeResult
  {
    bool ok {false};
    int count {0};
    QString summary;
    QByteArray text;
  };

  class Call3Updater : public QObject
  {
    Q_OBJECT
  public:
    Call3Updater (QNetworkAccessManager *, QString const& call3_path, QObject * parent = nullptr);
    static QList<Call3Source> sources ();
    static Call3MergeResult merge (QList<Call3Source> const& srcs, QList<QByteArray> const& data,
                                   QByteArray const& previous);
    bool busy () const {return busy_;}
    void start ();
    Q_SIGNAL void finished (bool ok, QString const& summary);
  private:
    void next ();
    void done ();
    QNetworkAccessManager * nam_;
    QString path_;
    bool busy_ {false};
    int index_ {0};
    QList<QByteArray> data_;
  };
}
#endif
