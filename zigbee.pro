include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)

QT += serialport

CONFIG += link_pkgconfig
PKGCONFIG += nymea-zigbee

SOURCES += \
    integrationpluginzigbee.cpp \
    xiaomi/xiaomibuttonsensor.cpp \
    xiaomi/xiaomimagnetsensor.cpp \
    xiaomi/xiaomimotionsensor.cpp \
    xiaomi/xiaomitemperaturesensor.cpp

HEADERS += \
    integrationpluginzigbee.h \
    xiaomi/xiaomibuttonsensor.h \
    xiaomi/xiaomimagnetsensor.h \
    xiaomi/xiaomimotionsensor.h \
    xiaomi/xiaomitemperaturesensor.h

