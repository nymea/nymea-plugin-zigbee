include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)

QT += serialport

#CONFIG += link_pkgconfig
#PKGCONFIG += nymea-zigbee

INCLUDEPATH=/home/timon/guh/development/nymea-zigbee/nymea-zigbee/
LIBS += -L/home/timon/guh/development/nymea-zigbee/build-nymea-zigbee-Desktop-Debug/libnymea-zigbee -lnymea-zigbee1

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

