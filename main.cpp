#include <QCoreApplication>

#include "market_watcher.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    MarketWatcher marketWather;
    return a.exec();
}
