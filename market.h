#ifndef MARKET_H
#define MARKET_H

#include <QMultiMap>
#include <QPair>

class QDomDocument;

void loadCommonMarketData(QMultiMap<QString, QPair<QString, QString>> &tradeTimeMap, QMultiMap<QString, QString> &instrumentMap);
QMultiMap<QString, QPair<QString, QString>> loadTradeTime(const QDomDocument &doc);
QMultiMap<QString, QString> loadInstrument(const QDomDocument &doc);

#endif // MARKET_H
