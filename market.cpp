#include <QFile>
#include <QSettings>
#include <QDomDocument>

#include "market.h"

void loadCommonMarketData(QMultiMap<QString, QPair<QString, QString>> &tradeTimeMap, QMultiMap<QString, QString> &instrumentMap)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ctp", "common");
    QString path_to_TradeMgr_xml = settings.value("TradeMgr").toString();
    QDomDocument doc("TradeMgr");
    QFile file(path_to_TradeMgr_xml);
    if (!file.open(QIODevice::ReadOnly))
        return;
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();

    tradeTimeMap = loadTradeTime(doc);
    instrumentMap = loadInstrument(doc);
}

QMultiMap<QString, QPair<QString, QString>> loadTradeTime(const QDomDocument &doc)
{
    QMultiMap<QString, QPair<QString, QString>> map;

    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.namedItem("TradeTime").firstChild();

    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if (!e.isNull()) {
            QString key = e.attribute("name");

            QDomNode ot = e.firstChild();
            while (!ot.isNull()) {
                QDomElement OpenTime = ot.toElement();
                QString start = OpenTime.attribute("start");
                QString end = OpenTime.attribute("end");
                map.insert(key, qMakePair(start, end));
                ot = ot.nextSibling();
            }
        }
        n = n.nextSibling();
    }

    return map;
}

QMultiMap<QString, QString> loadInstrument(const QDomDocument &doc)
{
    QMultiMap<QString, QString> map;

    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.namedItem("Instrument").firstChild();

    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if (!e.isNull()) {
            QString value = e.attribute("ID");
            QString key = e.attribute("MarketID");

            map.insert(key, value);
        }
        n = n.nextSibling();
    }

    return map;
}
