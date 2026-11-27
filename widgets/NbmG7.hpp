#ifndef NBM_G7_HPP_
#define NBM_G7_HPP_
// NBMODES-IC8TEM: ultima fonte di locatori, l'API pubblica di G7VRD "locator by callsign", come in LIVETROPOVHF.
// Si chiede solo per i nominativi decodificati che in CALL3.TXT mancano o hanno un locatore a 4 caratteri.
// Si accetta solo un locatore a 6 caratteri valido che non sia il centro del quadrato (LL/MM).
// Al massimo 40 nominativi al minuto, ogni nominativo una volta ogni 24 ore (servizio hobbistico, senza garanzie).
// Il locatore trovato entra in CALL3.TXT (blocco // [LIVETROPOVHF-FONTI]) se il nominativo non c'è o ha
// solo 4 caratteri dello stesso quadrato; non cambia mai una voce a 6 caratteri, le voci a mano né quelle
// del cluster. Nessun file a parte: CALL3.TXT si riscrive al massimo ogni 10 minuti e alla chiusura.
#include <QObject>
#include <QDateTime>
#include <QFutureWatcher>
#include <QHash>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>
#include <vector>

class QNetworkAccessManager;
class QTimer;

namespace nbm
{
  // Indice compatto di CALL3.TXT: hash a 64 bit del nominativo e locatore (circa 16 byte a voce).
  class Call3Index
  {
  public:
    struct Entry { quint64 key; char grid[6]; quint8 len; quint8 pinned; };
    static quint64 hash (QString const& call);
    static std::shared_ptr<Call3Index> load (QString const& path);
    // 0 = assente, altrimenti 4 o 6; pinned = voce a mano o dal cluster
    int lookup (QString const& call, QString * grid = nullptr, bool * pinned = nullptr) const;
    int size () const {return int (entries_.size ());}
  private:
    std::vector<Entry> entries_;
  };

  class G7Locator : public QObject
  {
    Q_OBJECT
  public:
    G7Locator (QNetworkAccessManager *, QString const& call3_path, QObject * parent = nullptr);
    void setEnabled (bool on) {enabled_ = on;}
    bool enabled () const {return enabled_;}
    void reloadIndex ();          // dopo un aggiornamento di CALL3.TXT
    void request (QString const& call);
    void flush ();                // scrive in CALL3.TXT i locatori trovati (in un thread a parte)
    void flushNow ();             // alla chiusura: scrive subito
    bool writing () const {return writing_;}
    QString lastStatus () const {return status_;}
    std::function<bool ()> canWrite;   // falso mentre l'aggiornamento quotidiano riscrive CALL3.TXT
    Q_SIGNAL void statusChanged (QString const&);
    // scrive in CALL3.TXT i locatori, con le regole di LIVETROPOVHF; restituisce quanti sono entrati
    static int applyLearned (QString const& path, QHash<QString, QString> const& learned);
    static QString pick (QString const& json_value_or_text);
  private:
    void tick ();
    QNetworkAccessManager * nam_;
    QString path_;
    bool enabled_ {true};
    bool busy_ {false};
    bool writing_ {false};
    std::shared_ptr<Call3Index> index_;
    QFutureWatcher<std::shared_ptr<Call3Index>> loader_;
    QHash<QString, QDateTime> asked_;
    QStringList queue_;
    QHash<QString, QString> learned_;      // in attesa di scrittura
    QHash<QString, QString> overlay_;      // già scritti o in attesa: valgono sopra l'indice
    QTimer * tick_;
    QTimer * flushTimer_;
    QString status_;
  };
}
#endif
