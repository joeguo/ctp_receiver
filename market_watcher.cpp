#include <QSettings>
#include <QDebug>

#include "market.h"
#include "market_watcher.h"
#include "market_watcher_adaptor.h"
#include "tick_receiver.h"

extern QList<Market> markets;

/*!
 * \brief getInstrumentName
 * 从合约代码里提取交易品种名(就是去掉交割月份)
 * 比如 cu1703 --> cu, i1705 --> i, CF705 --> CF
 *
 * \param instrumentID 合约代码
 * \return 交易品种名
 */
static QString getInstrumentName(const QString &instrumentID) {
    const int len = instrumentID.length();
    for (int i = 0; i < len; i++) {
        if (instrumentID[i].isDigit()) {
            return instrumentID.left(i);
        }
    }
    return "";
}

MarketWatcher::MarketWatcher(QObject *parent) :
    QObject(parent)
{
    nRequestID = 0;

    loadCommonMarketData();

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
    QStringList subscribeList = settings.childKeys();
    foreach (const QString &key, subscribeList) {
        if (settings.value(key).toString() == "1") {
            subscribeSet.insert(key);
        }
    }

    settings.endGroup();

    foreach (const QString &instrumentID, subscribeSet) {
        if (!checkTradingTimes(instrumentID)) {
            qDebug() << instrumentID << "has no proper trading time!";
        }
    }

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

/*!
 * \brief MarketWatcher::checkTradingTimes
 * 查找各个交易市场, 找到相应合约的交易时间并事先储存到map里
 *
 * \param instrumentID 合约代码
 * \return 是否找到了该合约的交易时间
 */
bool MarketWatcher::checkTradingTimes(const QString &instrumentID)
{
    const QString instrument = getInstrumentName(instrumentID);
    foreach (const auto &market, markets) {
        foreach (const auto &code, market.codes) {
            if (instrument == code) {
                int i = 0, size = market.regexs.size();
                for (; i < size; i++) {
                    if (QRegExp(market.regexs[i]).exactMatch(instrumentID)) {
                        tradingTimeMap[instrumentID] = market.tradetimeses[i];
                        return true;
                    }
                }
                return false;   // instrumentID未能匹配任何正则表达式
            }
        }
    }
    return false;
}

static inline quint8 charToDigit(const char ten, const char one)
{
    return quint8(10 * (ten - '0') + one - '0');
}

static inline bool isWithinRange(const QTime &t, const QTime &rangeStart, const QTime &rangeEnd)
{
    if (rangeStart < rangeEnd) {
        return rangeStart <= t && t <= rangeEnd;
    } else {
        return rangeStart <= t || t <= rangeEnd;
    }
}

/*!
 * \brief MarketWatcher::processDepthMarketData
 * 处理深度市场数据:
 * 1. 过滤无效的(如在交易时间外的, 或数据有错误的)行情消息
 * 2. 发送新行情数据(newMarketData signal)
 * 3. 如果需要, 将行情数据保存到文件
 *
 * \param depthMarketDataField 深度市场数据
 */
void MarketWatcher::processDepthMarketData(const CThostFtdcDepthMarketDataField& depthMarketDataField)
{
    quint8 hour, minute, second;
    hour   = charToDigit(depthMarketDataField.UpdateTime[0], depthMarketDataField.UpdateTime[1]);
    minute = charToDigit(depthMarketDataField.UpdateTime[3], depthMarketDataField.UpdateTime[4]);
    second = charToDigit(depthMarketDataField.UpdateTime[6], depthMarketDataField.UpdateTime[7]);

    QString instrumentID(depthMarketDataField.InstrumentID);
    QTime time(hour, minute, second);

    foreach (const auto &tradetime, tradingTimeMap[instrumentID]) {
        if (isWithinRange(time, tradetime.first, tradetime.second)) {
            QTime emitTime = (time == tradetime.second) ? time.addSecs(-1) : time;
            emit newMarketData(instrumentID,
                               QTime(0, 0).secsTo(emitTime),
                               depthMarketDataField.LastPrice,
                               depthMarketDataField.Volume,
                               depthMarketDataField.Turnover,
                               depthMarketDataField.OpenInterest);

            // TODO save tick
            break;
        }
    }
}

/*!
 * \brief MarketWatcher::getTradingDay
 * 获取交易日
 *
 * \return 交易日(YYYYMMDD)
 */
QString MarketWatcher::getTradingDay() const
{
    return pUserApi->GetTradingDay();
}

/*!
 * \brief MarketWatcher::getSubscribeList
 * 获取订阅合约列表
 *
 * \return 订阅合约列表
 */
QStringList MarketWatcher::getSubscribeList() const
{
    return subscribeSet.toList();
}

/*!
 * \brief MarketWatcher::quit
 * 退出
 */
void MarketWatcher::quit()
{
    QCoreApplication::quit();
}
