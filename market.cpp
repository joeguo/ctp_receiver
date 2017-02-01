#include <QFile>
#include <QSettings>
#include <QDomDocument>
#include <QDebug>
#include <QTime>

#include "market.h"

QMultiMap<QString, QPair<QString, QString>> tradeTimeMap;
QMultiMap<QString, QString> instrumentMap;
QList<Market> markets;

QDebug operator<<(QDebug dbg, const Market &market)
{
    dbg.nospace() << market.label << "\n"
                  << market.codes << "\n"
                  << market.descs << "\n"
                  << market.regexs << "\n"
                  << market.tradetimeses;
    return dbg.space();
}

void loadCommonMarketData()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ctp", "common");
    QString path_to_TradeMgr_xml = settings.value("TradeMgr").toString();
    QDomDocument doc;
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

    settings.beginGroup("Markets");
    QStringList marketsKey = settings.childKeys();
    foreach (const auto &key, marketsKey) {
        QString market_xml_file = settings.value(key).toString();
        auto market = loadMkt(market_xml_file);
        markets << market;
    }
}

Market loadMkt(const QString &file_name)
{
    Market market;
    QDomDocument doc;
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly))
        return market;
    if (!doc.setContent(&file)) {
        file.close();
        return market;
    }
    file.close();

    QDomElement docElem = doc.documentElement();

    QString label = docElem.namedItem("general").toElement().attribute("label");
    market.label = label;

    QDomNode n = docElem.namedItem("openclose").firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if (!e.isNull()) {
            QString mask = e.attribute("mask");
            if (mask.endsWith("*") && !mask.endsWith(".*")) {
                // 修正正则表达式
                mask.chop(1);
                market.regexs << (mask + ".*");
            } else {
                market.regexs << mask;
            }

            QString tradetime = e.attribute("tradetime");
            QList<QPair<QTime, QTime>> tradetimes;
            QStringList list1 = tradetime.trimmed().split(';');
            foreach (const auto &item, list1) {
                QStringList list2 = item.trimmed().split('-');

                QStringList list3 = list2[0].trimmed().split(':');
                int hour = list3[0].toInt();
                int min = list3[1].toInt();
                QTime start(hour, min);

                list3 = list2[1].trimmed().split(':');
                hour = list3[0].toInt();
                min = list3[1].toInt();
                QTime end(hour, min);

                tradetimes << qMakePair(start, end);
             }

            market.tradetimeses << tradetimes;
        }
        n = n.nextSibling();
    }

    n = docElem.namedItem("indcode").firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if (!e.isNull()) {
            QString code = e.attribute("code");
            QString desc = e.attribute("desc");
            market.codes << code;
            market.descs << desc;
        }
        n = n.nextSibling();
    }

    return market;
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
