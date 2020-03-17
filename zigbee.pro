include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)

QT += serialport

CONFIG += link_pkgconfig
PKGCONFIG += nymea-zigbee

SOURCES += \
    devicepluginzigbee.cpp \
    feibit/feibitonofflight.cpp \
    ikea/tradfricolorlight.cpp \
    ikea/tradfricolortemperaturelight.cpp \
    ikea/tradfrionoffswitch.cpp \
    ikea/tradfripowersocket.cpp \
    ikea/tradfrirangeextender.cpp \
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
    ikea/tradfricolortemperaturelight.h \
    ikea/tradfrionoffswitch.h \
    ikea/tradfripowersocket.h \
    ikea/tradfrirangeextender.h \
    ikea/tradfriremote.h \
    lumi/lumibuttonsensor.h \
    lumi/lumimagnetsensor.h \
    lumi/lumimotionsensor.h \
    lumi/lumitemperaturesensor.h \
    zigbeedevice.h

