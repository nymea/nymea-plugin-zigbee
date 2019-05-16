include(/home/timon/guh/development/nymea/nymea/libnymea/plugin/plugin.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginzigbee)

message(============================================)
message("Qt version: $$[QT_VERSION]")
message("Building $$deviceplugin$${TARGET}.so")

QT += serialport

LIBS += -L/home/timon/guh/development/nymea-zigbee/build-nymea-zigbee-Desktop-Debug/libnymea-zigbee -lnymea-zigbee1
INCLUDEPATH += /home/timon/guh/development/nymea-zigbee/nymea-zigbee/libnymea-zigbee

SOURCES += \
    devicepluginzigbee.cpp \
    xiaomi/xiaomibuttonsensor.cpp \
    xiaomi/xiaomimagnetsensor.cpp \
    xiaomi/xiaomimotionsensor.cpp \
    xiaomi/xiaomitemperaturesensor.cpp

HEADERS += \
    devicepluginzigbee.h \
    xiaomi/xiaomibuttonsensor.h \
    xiaomi/xiaomimagnetsensor.h \
    xiaomi/xiaomimotionsensor.h \
    xiaomi/xiaomitemperaturesensor.h

