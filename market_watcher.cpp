#include <QSettings>

#include "market_watcher.h"
#include "market_watcher_adaptor.h"
#include "tick_receiver.h"

MarketWatcher::MarketWatcher(QObject *parent) :
    QObject(parent)
{
    nRequestID = 0;

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ctp", "market_watcher");
    settings.beginGroup("SubscribeList");
    this->subscribeSet = settings.childKeys().toSet();
    settings.endGroup();

    pUserApi = CThostFtdcMdApi::CreateFtdcMdApi();
    pReceiver = new CTickReceiver(this);
    pUserApi->RegisterSpi(pReceiver);

    settings.beginGroup("FrontSites");
    QStringList keys = settings.childKeys();
    const QString protocol = "tcp://";
    foreach (const QString str, keys) {
        QString address = settings.value(str).toString();
        pUserApi->RegisterFront((protocol + address).toLatin1().data());
    }
    settings.endGroup();

    new Market_watcherAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/market_watcher", this);
    dbus.registerService("org.ctp.market_watcher");

    pUserApi->Init();
}

MarketWatcher::~MarketWatcher()
{
    pUserApi->Release();
    delete pReceiver;
}

void MarketWatcher::customEvent(QEvent *event)
{
    qDebug() << "customEvent: " << int(event->type());
    switch (int(event->type())) {
    case FRONT_CONNECTED:
        login();
        break;
    case FRONT_DISCONNECTED:
    {
        FrontDisconnectedEvent *fevent = static_cast<FrontDisconnectedEvent*>(event);
        // TODO
        switch (fevent->getReason()) {
        case 0x1001: // 网络读失败
            break;
        case 0x1002: // 网络写失败
            break;
        case 0x2001: // 接收心跳超时
            break;
        case 0x2002: // 发送心跳失败
            break;
        case 0x2003: // 收到错误报文
            break;
        default:
            break;
        }
    }
        break;
    case HEARTBEAT_WARNING:
        break;
    case RSP_USER_LOGIN:
        subscribe();
        break;
    case RSP_USER_LOGOUT:
    case RSP_ERROR:
    case RSP_SUB_MARKETDATA:
    case RSP_UNSUB_MARKETDATA:
        break;
    case DEPTH_MARKET_DATA:
    {
        DepthMarketDataEvent *devent = static_cast<DepthMarketDataEvent*>(event);
        processDepthMarketData(devent->DepthMarketDataField);
    }
        break;
    default:
        QObject::customEvent(event);
        break;
    }
}

void MarketWatcher::login()
{
    QSettings settings("ctp", "market_watcher");
    settings.beginGroup("AccountInfo");
    QString brokerID = settings.value("BrokerID").toString();
    QString userID = settings.value("UserID").toString();
    QString password = settings.value("Password").toString();
    QString userProductInfo = settings.value("UserProductInfo").toString();
    settings.endGroup();

    CThostFtdcReqUserLoginField reqUserLogin;
    strcpy(reqUserLogin.BrokerID, brokerID.toLatin1().data());
    strcpy(reqUserLogin.UserID, userID.toLatin1().data());
    strcpy(reqUserLogin.Password, password.toLatin1().data());
    strcpy(reqUserLogin.UserProductInfo, userProductInfo.toLatin1().data());

    pUserApi->ReqUserLogin(&reqUserLogin, nRequestID.fetchAndAddRelaxed(1));
}

void MarketWatcher::subscribe()
{
    const int num = subscribeSet.size();
    char* subscribe_array = new char[num * 8];
    char** ppInstrumentID = new char*[num];
    QSetIterator<QString> iterator(subscribeSet);
    for (int i = 0; i < num; i++) {
        ppInstrumentID[i] = strcpy(subscribe_array + 8 * i, iterator.next().toLatin1().data());
    }

    pUserApi->SubscribeMarketData(ppInstrumentID, num);
    delete subscribe_array;
    delete ppInstrumentID;
}

static inline quint8 charToDigit(const char ten, const char one)
{
    return quint8(10 * (ten - '0') + one - '0');
}

void MarketWatcher::processDepthMarketData(const CThostFtdcDepthMarketDataField& depthMarketDataField)
{
    quint8 hour, minute, second;
    hour   = charToDigit(depthMarketDataField.UpdateTime[0], depthMarketDataField.UpdateTime[1]);
    minute = charToDigit(depthMarketDataField.UpdateTime[3], depthMarketDataField.UpdateTime[4]);
    second = charToDigit(depthMarketDataField.UpdateTime[6], depthMarketDataField.UpdateTime[7]);

    quint32 time = (hour << 16) + (minute << 8) + second;
    // int actionDay = strtol(depthMarketDataField.ActionDay, NULL, 10);
    // int tradingDay = strtol(depthMarketDataField.TradingDay, NULL, 10);

    emit newTick(depthMarketDataField.Volume,
                 depthMarketDataField.Turnover,
                 depthMarketDataField.OpenInterest,
                 time,
                 depthMarketDataField.LastPrice,
                 depthMarketDataField.InstrumentID);

    // TODO save tick

    /*
    quint8 year, month, day;
    year  = charToDigit(depthMarketDataField.ActionDay[2], depthMarketDataField.ActionDay[3]);
    month = charToDigit(depthMarketDataField.ActionDay[4], depthMarketDataField.ActionDay[5]);
    day   = charToDigit(depthMarketDataField.ActionDay[6], depthMarketDataField.ActionDay[7]);
    */
}

QStringList MarketWatcher::getSubscribeList()
{
    return this->subscribeSet.values();
}

void MarketWatcher::quit()
{
    QCoreApplication::quit();
}
