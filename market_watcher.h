#ifndef MARKET_WATCHER_H
#define MARKET_WATCHER_H

#include <QAtomicInt>
#include <QStringList>
#include <QSet>

class CThostFtdcMdApi;
class CTickReceiver;
struct CThostFtdcDepthMarketDataField;

class MarketWatcher : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.ctp.market_watcher")
public:
    explicit MarketWatcher(QObject *parent = 0);
    ~MarketWatcher();

protected:
    QAtomicInt nRequestID;
    CThostFtdcMdApi *pUserApi;
    CTickReceiver *pReceiver;
    QSet<QString> subscribeSet;

    void customEvent(QEvent *);

    void login();
    void subscribe();
    void processDepthMarketData(const CThostFtdcDepthMarketDataField&);

signals:
    void newTick(int volume, double turnover, double openInterest, uint time, double lastPrice, const QString& instrumentID);

public slots:
    QStringList getSubscribeList() const;
    void quit() const;
};

#endif // MARKET_WATCHER_H

