#include "NbmG7.hpp"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <cstring>

namespace
{
  char const * const kManual  = "// [LIVETROPOVHF-MANUALI]";
  char const * const kCluster = "// [LIVETROPOVHF-CLUSTER]";
  char const * const kBody    = "// [LIVETROPOVHF-FONTI]";
  char const * const kApi     = "https://api.g7vrd.co.uk/v1/locators/by-callsign/";

  bool valid_grid (QString const& g)
  {
    static QRegularExpression const re {"^[A-R]{2}[0-9]{2}([A-X]{2})?$"};
    return re.match (g).hasMatch ();
  }

  bool split_line (QString const& raw, QString& call, QString& grid)
  {
    auto const line = raw.trimmed ();
    if (line.isEmpty () || line.startsWith ("//") || line.startsWith ("ZZZZ")) return false;
    auto const p = line.split (',');
    if (p.size () < 2) return false;
    call = p[0].trimmed ().toUpper ();
    grid = p[1].trimmed ().toUpper ();
    return !call.isEmpty () && valid_grid (grid);
  }

  QString pick_value (QJsonValue const& v)
  {
    if (v.isString ())
      {
        auto const s = v.toString ().trimmed ().toUpper ();
        return s.size () == 6 && valid_grid (s) && !s.endsWith ("LL") && !s.endsWith ("MM") ? s : QString {};
      }
    if (v.isArray ())
      {
        for (auto const& e : v.toArray ()) {auto r = pick_value (e); if (!r.isEmpty ()) return r;}
        return {};
      }
    if (v.isObject ())
      {
        auto const o = v.toObject ();
        for (auto const name : {"locator", "grid", "maidenhead", "value"})
          if (o.contains (name)) {auto r = pick_value (o.value (name)); if (!r.isEmpty ()) return r;}
        for (auto it = o.begin (); it != o.end (); ++it) {auto r = pick_value (it.value ()); if (!r.isEmpty ()) return r;}
      }
    return {};
  }
}

namespace nbm
{
  quint64 Call3Index::hash (QString const& call)
  {
    // FNV-1a a 64 bit sul nominativo in maiuscolo
    quint64 h = 1469598103934665603ULL;
    for (auto ch : call) {h ^= quint64 (ch.toUpper ().unicode ()); h *= 1099511628211ULL;}
    return h;
  }

  std::shared_ptr<Call3Index> Call3Index::load (QString const& path)
  {
    auto idx = std::make_shared<Call3Index> ();
    QFile f {path};
    if (!f.open (QIODevice::ReadOnly)) return idx;
    idx->entries_.reserve (1100000);
    bool pinned = false;
    QString call, grid;
    while (!f.atEnd ())
      {
        auto const raw = QString::fromLatin1 (f.readLine ());
        auto const t = raw.trimmed ();
        if (t == kManual || t == kCluster) {pinned = true; continue;}
        if (t == kBody) {pinned = false; continue;}
        if (!split_line (raw, call, grid)) continue;
        Entry e {hash (call), {}, quint8 (grid.size ()), quint8 (pinned)};
        std::memcpy (e.grid, grid.toLatin1 ().constData (), std::min<int> (6, grid.size ()));
        idx->entries_.push_back (e);
      }
    std::stable_sort (idx->entries_.begin (), idx->entries_.end (),
                      [] (Entry const& a, Entry const& b) {return a.key < b.key;});
    // a parità di nominativo vale la prima voce (blocchi a mano e cluster sono in cima)
    idx->entries_.erase (std::unique (idx->entries_.begin (), idx->entries_.end (),
                                      [] (Entry const& a, Entry const& b) {return a.key == b.key;}),
                         idx->entries_.end ());
    idx->entries_.shrink_to_fit ();
    return idx;
  }

  int Call3Index::lookup (QString const& call, QString * grid, bool * pinned) const
  {
    auto const k = hash (call);
    auto it = std::lower_bound (entries_.begin (), entries_.end (), k,
                                [] (Entry const& e, quint64 key) {return e.key < key;});
    if (it == entries_.end () || it->key != k) return 0;
    if (grid) *grid = QString::fromLatin1 (it->grid, it->len);
    if (pinned) *pinned = it->pinned;
    return it->len;
  }

  QString G7Locator::pick (QString const& text)
  {
    auto const doc = QJsonDocument::fromJson (("[" + text + "]").toUtf8 ());
    return doc.isArray () ? pick_value (doc.array ().at (0)) : QString {};
  }

  G7Locator::G7Locator (QNetworkAccessManager * nam, QString const& call3_path, QObject * parent)
    : QObject {parent}
    , nam_ {nam}
    , path_ {call3_path}
    , tick_ {new QTimer {this}}
    , flushTimer_ {new QTimer {this}}
  {
    connect (&loader_, &QFutureWatcher<std::shared_ptr<Call3Index>>::finished, this, [this] {
        index_ = loader_.result ();
      });
    connect (tick_, &QTimer::timeout, this, &G7Locator::tick);
    tick_->start (60 * 1000);
    connect (flushTimer_, &QTimer::timeout, this, &G7Locator::flush);
    flushTimer_->start (10 * 60 * 1000);
    reloadIndex ();
  }

  void G7Locator::reloadIndex ()
  {
    if (loader_.isRunning ()) return;
    auto const path = path_;
    loader_.setFuture (QtConcurrent::run ([path] {return Call3Index::load (path);}));
  }

  void G7Locator::request (QString const& raw)
  {
    if (!enabled_ || !index_) return;
    auto const call = raw.trimmed ().toUpper ();
    if (call.size () < 3 || call.contains ('/') || call.contains ('-') || call.contains ('<')) return;
    if (overlay_.contains (call)) return;
    if (index_->lookup (call) == 6) return;
    auto const now = QDateTime::currentDateTimeUtc ();
    auto const a = asked_.constFind (call);
    if (a != asked_.cend () && a.value ().secsTo (now) < 24 * 3600) return;
    asked_.insert (call, now);
    queue_ << call;
  }

  void G7Locator::tick ()
  {
    if (!enabled_ || busy_ || queue_.isEmpty ()) return;
    QStringList batch;
    while (batch.size () < 40 && !queue_.isEmpty ()) batch << queue_.takeFirst ();
    QStringList enc;
    for (auto const& c : batch) enc << QString::fromLatin1 (QUrl::toPercentEncoding (c));
    QNetworkRequest req {QUrl {QString {kApi} + enc.join (',')}};
    req.setRawHeader ("User-Agent", "NBMODES-IC8TEM (+https://github.com/ic8tem/NBMODES-IC8TEM)");
    busy_ = true;
    auto * reply = nam_->get (req);
    auto * timer = new QTimer {reply};
    timer->setSingleShot (true);
    connect (timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    timer->start (30000);
    int const asked = batch.size ();
    connect (reply, &QNetworkReply::finished, this, [this, reply, asked] {
        busy_ = false;
        reply->deleteLater ();
        if (reply->error () != QNetworkReply::NoError)
          {
            status_ = "G7VRD: " + reply->errorString ();
            Q_EMIT statusChanged (status_);
            return;
          }
        auto const doc = QJsonDocument::fromJson (reply->readAll ());
        int n = 0;
        if (doc.isObject ())
          {
            auto const o = doc.object ();
            for (auto it = o.begin (); it != o.end (); ++it)
              {
                auto const loc = pick_value (it.value ());
                if (loc.isEmpty ()) continue;
                auto const call = it.key ().toUpper ();
                QString have; bool pinned = false;
                int const len = index_ ? index_->lookup (call, &have, &pinned) : 0;
                if (pinned || len == 6) continue;
                if (len == 4 && !loc.startsWith (have)) continue;
                learned_.insert (call, loc);
                overlay_.insert (call, loc);
                ++n;
              }
          }
        status_ = QString {"G7VRD: %1 chiesti, %2 locatori a 6 caratteri"}.arg (asked).arg (n);
        Q_EMIT statusChanged (status_);
      });
  }

  void G7Locator::flush ()
  {
    if (learned_.isEmpty () || writing_ || (canWrite && !canWrite ())) return;
    writing_ = true;
    auto const path = path_;
    auto const batch = learned_;
    learned_.clear ();
    auto * watcher = new QFutureWatcher<int> {this};
    connect (watcher, &QFutureWatcher<int>::finished, this, [this, watcher, batch] {
        auto const n = watcher->result ();
        watcher->deleteLater ();
        writing_ = false;
        if (n < 0) {for (auto it = batch.cbegin (); it != batch.cend (); ++it) learned_.insert (it.key (), it.value ());}
        else reloadIndex ();
      });
    watcher->setFuture (QtConcurrent::run ([path, batch] {return applyLearned (path, batch);}));
  }

  void G7Locator::flushNow ()
  {
    if (learned_.isEmpty ()) return;
    if (applyLearned (path_, learned_) >= 0) learned_.clear ();
  }

  int G7Locator::applyLearned (QString const& path, QHash<QString, QString> const& learned)
  {
    QFile in {path};
    if (!in.open (QIODevice::ReadOnly)) return -1;
    auto const text = QString::fromLatin1 (in.readAll ());
    in.close ();
    QStringList head, manual, cluster;
    QMap<QString, QString> body;          // ordinato come in LIVETROPOVHF
    QSet<QString> pinned;
    bool const marked = text.contains (kManual) || text.contains (kBody);
    enum {Head, Manual, Cluster, Body} where = marked ? Head : Body;
    QString call, grid;
    for (auto raw : text.split ('\n'))
      {
        if (raw.endsWith ('\r')) raw.chop (1);
        auto const t = raw.trimmed ();
        if (t == kManual) {where = Manual; continue;}
        if (t == kCluster) {where = Cluster; continue;}
        if (t == kBody) {where = Body; continue;}
        if (where == Head) {if (!t.isEmpty ()) head << raw; continue;}
        if (!split_line (raw, call, grid)) continue;
        if (where == Manual) {manual << t; pinned << call;}
        else if (where == Cluster) {cluster << t; pinned << call;}
        else if (!body.contains (call) && !pinned.contains (call)) body.insert (call, grid);
      }
    if (!marked)
      head = QStringList {"// CALL3.TXT di NBMODES-IC8TEM (stesso formato e stesse regole di LIVETROPOVHF)", "//"};
    int n = 0;
    for (auto it = learned.cbegin (); it != learned.cend (); ++it)
      {
        if (pinned.contains (it.key ())) continue;
        auto b = body.find (it.key ());
        if (b == body.end ()) {body.insert (it.key (), it.value ()); ++n;}
        else if (b.value ().size () == 4 && it.value ().startsWith (b.value ())) {b.value () = it.value (); ++n;}
      }
    if (!n) return 0;
    QByteArray out;
    out.reserve (body.size () * 16 + 4096);
    auto line = [&out] (QString const& s) {out += s.toLatin1 (); out += '\n';};
    for (auto const& h : head) line (h);
    line (kManual);
    for (auto const& l : manual) line (l);
    line (kCluster);
    for (auto const& l : cluster) line (l);
    line (kBody);
    for (auto it = body.cbegin (); it != body.cend (); ++it) line (it.key () + ',' + it.value () + ",,,");
    QFile::setPermissions (path, QFile::permissions (path) | QFileDevice::WriteOwner);
    QSaveFile sf {path};
    if (!sf.open (QIODevice::WriteOnly) || sf.write (out) != out.size () || !sf.commit ()) return -1;
    return n;
  }
}
