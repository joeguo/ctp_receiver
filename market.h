#ifndef MARKET_H
#define MARKET_H

#include <QMultiMap>
#include <QPair>
#include <QStringList>

class QTime;
class QDomDocument;

struct Market {
    QString label;
    QStringList codes;
    QStringList descs;
    QStringList masks;
    QList<QList<QPair<QTime, QTime>>> tradetimeses;
};

QDebug operator<<(QDebug dbg, const Market &market);
void loadCommonMarketData();
Market loadMkt(const QString &file_name);
QMultiMap<QString, QPair<QString, QString>> loadTradeTime(const QDomDocument &doc);
QMultiMap<QString, QString> loadInstrument(const QDomDocument &doc);

#endif // MARKET_H
