#include <cstddef>
#include <string.h>

#include <QCoreApplication>
#include <QDebug>

#include "market_watcher.h"
#include "tick_receiver.h"

CTickReceiver::CTickReceiver(QObject *obj) :
    receiver(obj)
{
}

CTickReceiver::~CTickReceiver()
{

}

void CTickReceiver::postToReceiver(QEvent *event)
{
    QCoreApplication::postEvent(receiver, event);
}

void CTickReceiver::OnFrontConnected()
{
    postToReceiver(new FrontConnectedEvent());
}

void CTickReceiver::OnFrontDisconnected(int Reason)
{
    postToReceiver(new FrontDisconnectedEvent(Reason));
}

void CTickReceiver::OnHeartBeatWarning(int nTimeLapse)
{
    postToReceiver(new HeartBeatWarningEvent(nTimeLapse));
}

void CTickReceiver::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (pRspInfo != NULL) {
        if (pRspInfo->ErrorID == 0) {
            postToReceiver(new UserLoginEvent());
        } else {
            qDebug() << pRspInfo->ErrorMsg;
        }
    }
}

void CTickReceiver::OnRspUserLogout(CThostFtdcUserLogoutField *, CThostFtdcRspInfoField *, int, bool)
{

}

void CTickReceiver::OnRspError(CThostFtdcRspInfoField *, int, bool)
{

}

void CTickReceiver::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *, CThostFtdcRspInfoField *, int, bool)
{

}

void CTickReceiver::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *, CThostFtdcRspInfoField *, int, bool)
{

}

void CTickReceiver::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    postToReceiver(new DepthMarketDataEvent(pDepthMarketData));
}
