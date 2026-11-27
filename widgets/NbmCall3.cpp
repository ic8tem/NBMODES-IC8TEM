#include "NbmCall3.hpp"

#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>
#include <QFutureWatcher>
#include <QPair>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>

namespace
{
  char const * const kManual  = "// [LIVETROPOVHF-MANUALI]";
  char const * const kCluster = "// [LIVETROPOVHF-CLUSTER]";
  char const * const kBody    = "// [LIVETROPOVHF-FONTI]";

  bool valid_grid (QString const& g)
  {
    static QRegularExpression const re {"^[A-R]{2}[0-9]{2}([A-X]{2})?$"};
    return re.match (g).hasMatch ();
  }

  bool looks_like_call (QString const& c)
  {
    if (c.size () < 3 || c.size () > 16) return false;
    bool digit = false, letter = false;
    for (auto ch : c)
      {
        if (ch.isDigit ()) digit = true;
        else if (ch.isLetter ()) letter = true;
        else if (ch != '/') return false;
      }
    return digit && letter;
  }

  // CALL3.TXT: CALL,LOCATORE,... con righe di commento //
  bool parse_line (QString const& raw, QString& call, QString& grid)
  {
    auto const line = raw.trimmed ();
    if (line.isEmpty () || line.startsWith ("//") || line.startsWith ("ZZZZ")) return false;
    auto const p = line.split (',');
    if (p.size () < 2) return false;
    call = p[0].trimmed ().toUpper ();
    grid = p[1].trimmed ().toUpper ();
    return looks_like_call (call) && valid_grid (grid);
  }

  QList<QPair<QString, QString>> parse_call3 (QByteArray const& bytes)
  {
    QList<QPair<QString, QString>> r;
    QString call, grid;
    for (auto const& raw : QString::fromLatin1 (bytes).split ('\n'))
      if (parse_line (raw, call, grid)) r << qMakePair (call, grid);
    return r;
  }

  // grid_data.bin di JTDX: qCompress di un QDataStream (quint32 righe, poi coppie di QString)
  QList<QPair<QString, QString>> parse_grid_data (QByteArray const& raw)
  {
    QList<QPair<QString, QString>> r;
    if (raw.size () <= 8 || static_cast<unsigned char> (raw[4]) != 0x78) return r;
    auto const d = qUncompress (raw);
    if (d.isEmpty ()) return r;
    QDataStream ds {d};
    ds.setByteOrder (QDataStream::BigEndian);
    quint32 rows = 0;
    ds >> rows;
    if (rows == 0 || rows > 2000000) return r;
    for (quint32 i = 0; i < rows && !ds.atEnd (); ++i)
      {
        QString c, g;
        ds >> c >> g;
        if (ds.status () != QDataStream::Ok) break;
        c = c.trimmed ().toUpper ().remove ('<').remove ('>');
        g = g.trimmed ().toUpper ();
        if (looks_like_call (c) && valid_grid (g)) r << qMakePair (c, g);
      }
    return r;
  }

  QString num (int n) {return QLocale {QLocale::Italian}.toString (n);}
}

namespace nbm
{
  QList<Call3Source> Call3Updater::sources ()
  {
    return {
      {"WSJT-X Improved", "https://downloads.sourceforge.net/project/wsjt-x-improved/Additional%20Files/CALL3.TXT", false},
      {"W7GJ (EME)", "https://www.bigskyspaces.com/w7gj/CALL3.TXT", false},
      {"JTDX grid_data.bin", "https://downloads.sourceforge.net/project/jtdx/grid_data.bin", true},
    };
  }

  Call3MergeResult Call3Updater::merge (QList<Call3Source> const& srcs, QList<QByteArray> const& data,
                                        QByteArray const& previous)
  {
    Call3MergeResult res;
    // file precedente: blocchi a mano e cluster tenuti così come sono, il resto come voci precedenti
    QStringList manual, cluster;
    QHash<QString, QString> prev;
    QSet<QString> pinned;     // nominativi dei blocchi a mano e cluster: compaiono una volta sola
    enum {Head, Manual, Cluster, Body} where = Head;
    bool marked = previous.contains (kManual) || previous.contains (kBody);
    QString call, grid;
    for (auto raw : QString::fromLatin1 (previous).split ('\n'))
      {
        if (raw.endsWith ('\r')) raw.chop (1);
        auto const t = raw.trimmed ();
        if (t == kManual) {where = Manual; continue;}
        if (t == kCluster) {where = Cluster; continue;}
        if (t == kBody) {where = Body; continue;}
        if (!parse_line (raw, call, grid)) continue;
        if (marked && where == Manual) {manual << t; pinned << call;}
        else if (marked && where == Cluster) {cluster << t; pinned << call;}
        else if (!marked || where == Body) prev.insert (call, grid);
      }

    QHash<QString, QString> merged;
    merged.reserve (1100000);
    QStringList parts;
    for (int i = 0; i < srcs.size (); ++i)
      {
        auto const& s = srcs[i];
        auto const bytes = i < data.size () ? data[i] : QByteArray {};
        if (bytes.isEmpty ()) {parts << s.name + ": non riuscito"; continue;}
        auto const list = s.binary ? parse_grid_data (bytes) : parse_call3 (bytes);
        if (list.size () < 300) {parts << s.name + ": file non valido"; continue;}
        int added = 0;
        for (auto const& e : list)
          if (!merged.contains (e.first)) {merged.insert (e.first, e.second); ++added;}
        parts << QString {"%1: %2 (%3 nuovi)"}.arg (s.name, num (list.size ()), num (added));
      }
    if (merged.size () < 50000)
      {
        res.summary = "fonti non raggiungibili o troppo piccole: " + parts.join ("; ");
        return res;
      }
    int kept = 0, six = 0;
    for (auto it = prev.cbegin (); it != prev.cend (); ++it)
      {
        auto m = merged.find (it.key ());
        if (m == merged.end ()) {merged.insert (it.key (), it.value ()); ++kept;}
        else if (m.value ().size () == 4 && it.value ().size () == 6 && it.value ().startsWith (m.value ()))
          {m.value () = it.value (); ++six;}
      }
    parts << QString {"dal CALL3.TXT precedente: %1 (+%2 a 6 caratteri)"}.arg (num (kept), num (six));
    for (auto const& c : pinned) merged.remove (c);

    QStringList keys = merged.keys ();
    std::sort (keys.begin (), keys.end ());
    auto const now = QDateTime::currentDateTimeUtc ();
    int const total = keys.size () + pinned.size ();
    QByteArray out;
    out.reserve (keys.size () * 16 + 4096);
    auto line = [&out] (QString const& s) {out += s.toLatin1 (); out += '\n';};
    line ("// CALL3.TXT di NBMODES-IC8TEM, riscritto il " + now.toString ("yyyy-MM-dd HH:mm") + " UTC (stesso formato e stesse regole di LIVETROPOVHF)");
    line ("// Version: NBMODES " + now.toString ("yyyy-MM-dd"));
    line ("// Fonti, in ordine di precedenza: voci a mano, correzioni dal cluster, WSJT-X Improved (DG2YCB), CALL3.TXT EME di W7GJ, grid_data.bin di JTDX; poi le voci del CALL3.TXT precedente e i locatori imparati");
    line ("// " + parts.join (" - "));
    line (QString {"// Contains %1 unique callsigns."}.arg (total));
    line ("//");
    line (kManual);
    for (auto const& l : manual) line (l);
    line (kCluster);
    for (auto const& l : cluster) line (l);
    line (kBody);
    for (auto const& k : keys) line (k + ',' + merged.value (k) + ",,,");
    res.ok = true;
    res.count = total;
    res.summary = parts.join (" - ");
    res.text = out;
    return res;
  }

  Call3Updater::Call3Updater (QNetworkAccessManager * nam, QString const& call3_path, QObject * parent)
    : QObject {parent}
    , nam_ {nam}
    , path_ {call3_path}
  {
  }

  void Call3Updater::start ()
  {
    if (busy_) return;
    busy_ = true;
    index_ = 0;
    data_.clear ();
    next ();
  }

  void Call3Updater::next ()
  {
    auto const srcs = sources ();
    if (index_ >= srcs.size ()) {done (); return;}
    QNetworkRequest req {QUrl {srcs[index_].url}};
    req.setAttribute (QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader ("User-Agent", "NBMODES-IC8TEM (+https://github.com/ic8tem/NBMODES-IC8TEM)");
    auto * reply = nam_->get (req);
    auto * timer = new QTimer {reply};
    timer->setSingleShot (true);
    connect (timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    timer->start (120000);
    connect (reply, &QNetworkReply::finished, this, [this, reply] {
        data_ << (reply->error () == QNetworkReply::NoError ? reply->readAll () : QByteArray {});
        reply->deleteLater ();
        ++index_;
        next ();
      });
  }

  void Call3Updater::done ()
  {
    // unione e scrittura in un thread a parte: con un milione di nominativi servono alcuni secondi
    auto const path = path_;
    auto const data = data_;
    data_.clear ();
    auto * watcher = new QFutureWatcher<QPair<bool, QString>> {this};
    connect (watcher, &QFutureWatcher<QPair<bool, QString>>::finished, this, [this, watcher] {
        auto const r = watcher->result ();
        watcher->deleteLater ();
        busy_ = false;
        Q_EMIT finished (r.first, r.second);
      });
    watcher->setFuture (QtConcurrent::run ([path, data] () -> QPair<bool, QString> {
        QByteArray previous;
        {
          QFile f {path};
          if (f.open (QIODevice::ReadOnly)) previous = f.readAll ();
        }
        auto const r = merge (sources (), data, previous);
        if (!r.ok) return {false, r.summary};
        if (!previous.isEmpty ())
          {
            QFileInfo fi {path};
            auto const bak = fi.absolutePath () + '/' + fi.completeBaseName () + ".bak";
            QFile::remove (bak);
            QFile::copy (path, bak);
          }
        QFile::setPermissions (path, QFile::permissions (path) | QFileDevice::WriteOwner);
        QSaveFile out {path};
        if (!out.open (QIODevice::WriteOnly) || out.write (r.text) != r.text.size () || !out.commit ())
          return {false, QStringLiteral ("scrittura di CALL3.TXT non riuscita")};
        return {true, QString {"%1 nominativi — %2. Il precedente è in CALL3.bak."}.arg (num (r.count), r.summary)};
      }));
  }
}
