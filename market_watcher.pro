QT += core dbus
QT -= gui

TARGET = market_watcher
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    tick_receiver.cpp \
    market_watcher.cpp

HEADERS += \
    ThostFtdcMdApi.h \
    ThostFtdcUserApiDataType.h \
    ThostFtdcUserApiStruct.h \
    market_watcher.h \
    tick_receiver.h

DBUS_ADAPTORS += market_watcher.xml

DISTFILES +=

unix:LIBS += "$$_PRO_FILE_PWD_/thostmduserapi.so"
win32:LIBS += "$$_PRO_FILE_PWD_/thostmduserapi.lib"
