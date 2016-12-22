#ifndef TICK_RECEIVER_H
#define TICK_RECEIVER_H

#include <QEvent>

#include "ThostFtdcMdApi.h"

#define FRONT_CONNECTED         (QEvent::User + 0)
#define FRONT_DISCONNECTED      (QEvent::User + 1)
#define HEARTBEAT_WARNING       (QEvent::User + 2)
#define RSP_USER_LOGIN          (QEvent::User + 3)
#define RSP_USER_LOGOUT         (QEvent::User + 4)
#define RSP_ERROR               (QEvent::User + 5)
#define RSP_SUB_MARKETDATA      (QEvent::User + 6)
#define RSP_UNSUB_MARKETDATA    (QEvent::User + 7)
#define DEPTH_MARKET_DATA       (QEvent::User + 8)

class FrontConnectedEvent : public QEvent {
public:
    FrontConnectedEvent() :
        QEvent(QEvent::Type(FRONT_CONNECTED)) {}
};

class FrontDisconnectedEvent : public QEvent {
protected:
    const int reason;

public:
    explicit FrontDisconnectedEvent(int Reason) :
        QEvent(QEvent::Type(FRONT_DISCONNECTED)),
        reason(Reason) {}

    int getReason() const { return reason; }
};

class HeartBeatWarningEvent : public QEvent {
protected:
    const int nTimeLapse;

public:
    explicit HeartBeatWarningEvent(int nTimeLapse) :
        QEvent(QEvent::Type(HEARTBEAT_WARNING)),
        nTimeLapse(nTimeLapse) {}

    int getLapseTime() const { return nTimeLapse; }
};

class UserLoginEvent : public QEvent {
public:
    UserLoginEvent() :
        QEvent(QEvent::Type(RSP_USER_LOGIN)) {}
};

class UserLogoutEvent : public QEvent {
public:
    UserLogoutEvent() :
        QEvent(QEvent::Type(RSP_USER_LOGOUT)) {}
};

class DepthMarketDataEvent : public QEvent {
public:
    const CThostFtdcDepthMarketDataField DepthMarketDataField;

    explicit DepthMarketDataEvent(CThostFtdcDepthMarketDataField *pDepthMarketDataField) :
        QEvent(QEvent::Type(DEPTH_MARKET_DATA)),
        DepthMarketDataField(*pDepthMarketDataField) {}
};

class CTickReceiver : public CThostFtdcMdSpi {
    QObject * const receiver;

public:
    explicit CTickReceiver(QObject *obj);
    ~CTickReceiver();

    void postToReceiver(QEvent *event) const;

    void OnFrontConnected();

    void OnFrontDisconnected(int nReason);

    void OnHeartBeatWarning(int nTimeLapse);

    ///登录请求响应
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///登出请求响应
    void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///错误应答
    void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///订阅行情应答
    void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///取消订阅行情应答
    void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///深度行情通知
    void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);
};

#endif // TICK_RECEIVER_H

