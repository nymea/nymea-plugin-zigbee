include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)

QT += serialport

#CONFIG += link_pkgconfig
#PKGCONFIG += nymea-zigbee

INCLUDEPATH=/home/timon/guh/development/nymea-zigbee/nymea-zigbee/
LIBS += -L/home/timon/guh/development/nymea-zigbee/build-nymea-zigbee-Desktop-Debug/libnymea-zigbee -lnymea-zigbee1

SOURCES += \
    devicepluginzigbee.cpp \
    feibit/feibitonofflight.cpp \
    ikea/tradfricolorlight.cpp \
    ikea/tradfriremote.cpp \
    xiaomi/xiaomibuttonsensor.cpp \
    xiaomi/xiaomimagnetsensor.cpp \
    xiaomi/xiaomimotionsensor.cpp \
    xiaomi/xiaomitemperaturesensor.cpp \
    zigbeedevice.cpp

HEADERS += \
    devicepluginzigbee.h \
    feibit/feibitonofflight.h \
    genericnoncolourscenecontroller.h \
    ikea/tradfricolorlight.h \
    ikea/tradfriremote.h \
    xiaomi/xiaomibuttonsensor.h \
    xiaomi/xiaomimagnetsensor.h \
    xiaomi/xiaomimotionsensor.h \
    xiaomi/xiaomitemperaturesensor.h \
    zigbeedevice.h

