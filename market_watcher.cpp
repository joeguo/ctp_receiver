#include <QSettings>
#include <QDebug>

#include "market.h"
#include "market_watcher.h"
#include "market_watcher_adaptor.h"
#include "tick_receiver.h"

MarketWatcher::MarketWatcher(QObject *parent) :
    QObject(parent)
{
    nRequestID = 0;

    loadCommonMarketData(tradeTimeMap, instrumentMap);

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ctp", "market_watcher");
    QByteArray flowPath = settings.value("FlowPath").toByteArray();

    settings.beginGroup("AccountInfo");
    brokerID = settings.value("BrokerID").toByteArray();
    userID = settings.value("UserID").toByteArray();
    password = settings.value("Password").toByteArray();
    settings.endGroup();

    // Pre-convert QString to char*
    c_brokerID = brokerID.data();
    c_userID = userID.data();
    c_password = password.data();

    settings.beginGroup("SubscribeList");
    subscribeSet = settings.childKeys().toSet();
    settings.endGroup();

    pUserApi = CThostFtdcMdApi::CreateFtdcMdApi(flowPath.data());
    pReceiver = new CTickReceiver(this);
    pUserApi->RegisterSpi(pReceiver);

    settings.beginGroup("FrontSites");
    QStringList keys = settings.childKeys();
    const QString protocol = "tcp://";
    foreach (const QString &str, keys) {
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
        auto *fevent = static_cast<FrontDisconnectedEvent*>(event);
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
    {
        auto *hevent = static_cast<HeartBeatWarningEvent*>(event);
        emit heartBeatWarning(hevent->getLapseTime());
    }
        break;
    case RSP_USER_LOGIN:
        subscribe();
        break;
    case RSP_USER_LOGOUT:
        break;
    case RSP_ERROR:
    case RSP_SUB_MARKETDATA:
    case RSP_UNSUB_MARKETDATA:
        break;
    case DEPTH_MARKET_DATA:
    {
        auto *devent = static_cast<DepthMarketDataEvent*>(event);
        processDepthMarketData(devent->DepthMarketDataField);
    }
        break;
    default:
        QObject::customEvent(event);
        break;
    }
}

/*!
 * \brief MarketWatcher::login
 * 用配置文件中的账号信息登陆行情端
 */
void MarketWatcher::login()
{
    CThostFtdcReqUserLoginField reqUserLogin;
    memset(&reqUserLogin, 0, sizeof (CThostFtdcReqUserLoginField));
    strcpy(reqUserLogin.BrokerID, c_brokerID);
    strcpy(reqUserLogin.UserID, c_userID);
    strcpy(reqUserLogin.Password, c_password);

    pUserApi->ReqUserLogin(&reqUserLogin, nRequestID.fetchAndAddRelaxed(1));
}

/*!
 * \brief MarketWatcher::subscribe
 * 订阅subscribeSet里的合约
 */
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
    delete[] ppInstrumentID;
    delete[] subscribe_array;
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

    uint time = (hour * 3600) + (minute * 60) + second;

    emit newMarketData(depthMarketDataField.InstrumentID,
                       time,
                       depthMarketDataField.LastPrice,
                       depthMarketDataField.Volume,
                       depthMarketDataField.Turnover,
                       depthMarketDataField.OpenInterest);

    // TODO save tick
}

/*!
 * \brief CtpExecuter::getTradingDay
 * 获取交易日
 *
 * \return 交易日(YYYYMMDD)
 */
QString MarketWatcher::getTradingDay() const
{
    return pUserApi->GetTradingDay();
}

QStringList MarketWatcher::getSubscribeList() const
{
    return subscribeSet.toList();
}

void MarketWatcher::quit()
{
    QCoreApplication::quit();
}
