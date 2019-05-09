include(/usr/include/nymea/plugin.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginzigbee)

message(============================================)
message("Qt version: $$[QT_VERSION]")
message("Building $$deviceplugin$${TARGET}.so")

QT += serialport

LIBS += -L/home/timon/guh/development/nymea-zigbee/build-nymea-zigbee-Desktop-Debug/libnymea-zigbee -lnymea-zigbee1
INCLUDEPATH += /home/timon/guh/development/nymea-zigbee/nymea-zigbee/libnymea-zigbee

SOURCES += \
    devicepluginzigbee.cpp

HEADERS += \
    devicepluginzigbee.h

