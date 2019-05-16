include(/home/timon/guh/development/nymea/nymea/libnymea/plugin/plugin.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginzigbee)

message(============================================)
message("Qt version: $$[QT_VERSION]")
message("Building $$deviceplugin$${TARGET}.so")

QT += serialport

LIBS += -lnymea-zigbee1

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

