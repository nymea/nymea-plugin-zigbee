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
    ikea/tradfrionoffswitch.cpp \
    ikea/tradfriremote.cpp \
    lumi/lumibuttonsensor.cpp \
    lumi/lumimagnetsensor.cpp \
    lumi/lumimotionsensor.cpp \
    lumi/lumitemperaturesensor.cpp \
    zigbeedevice.cpp

HEADERS += \
    devicepluginzigbee.h \
    feibit/feibitonofflight.h \
    ikea/tradfricolorlight.h \
    ikea/tradfrionoffswitch.h \
    ikea/tradfriremote.h \
    lumi/lumibuttonsensor.h \
    lumi/lumimagnetsensor.h \
    lumi/lumimotionsensor.h \
    lumi/lumitemperaturesensor.h \
    zigbeedevice.h

