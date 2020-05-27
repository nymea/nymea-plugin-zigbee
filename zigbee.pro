isEmpty(PLUGIN_PRI) {
  exists($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri) {
    include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)
  } else {
    message("plugin.pri not found. Either install libnymea1-dev or use the PLUGIN_PRI argument to point to it.")
    message("For building this project without nymea installed system-wide, you will want to export those variables in addition:")
    message("PKG_CONFIG_PATH=/path/to/build-nymea/libnymea/pkgconfig/")
    message("CPATH=/path/to/nymea/libnymea/")
    message("LIBRARY_PATH=/path/to/build-nymea/libnymea/")
    message("PATH=/path/to/build-nymea/tools/nymea-plugininfocompiler:$PATH")
    message("LD_LIBRARY_PATH=/path/to/build-nymea/libnymea/")
    error("plugin.pri not found. Cannot continue")
  }
} else {
  message("Using $$PLUGIN_PRI")
  include($$PLUGIN_PRI)
}

QT += serialport sql

CONFIG += link_pkgconfig
PKGCONFIG += nymea-zigbee

SOURCES += \
    #generic/genericcolorlight.cpp \
    #generic/genericcolortemperaturelight.cpp \
    generic/genericonofflight.cpp \
    #generic/genericpowersocket.cpp \
    integrationpluginzigbee.cpp \
    #ikea/tradfricolorlight.cpp \
    #ikea/tradfricolortemperaturelight.cpp \
    ikea/tradfrionoffswitch.cpp \
    #ikea/tradfripowersocket.cpp \
    #ikea/tradfrirangeextender.cpp \
    #ikea/tradfriremote.cpp \
    #lumi/lumibuttonsensor.cpp \
    #lumi/lumimagnetsensor.cpp \
    #lumi/lumimotionsensor.cpp \
    #lumi/lumitemperaturesensor.cpp \
    zigbeedevice.cpp

HEADERS += \
    #generic/genericcolorlight.h \
    #generic/genericcolortemperaturelight.h \
    generic/genericonofflight.h \
    #generic/genericpowersocket.h \
    integrationpluginzigbee.h \
    #ikea/tradfricolorlight.h \
    #ikea/tradfricolortemperaturelight.h \
    ikea/tradfrionoffswitch.h \
    #ikea/tradfripowersocket.h \
    #ikea/tradfrirangeextender.h \
    #ikea/tradfriremote.h \
    #lumi/lumibuttonsensor.h \
    #lumi/lumimagnetsensor.h \
    #lumi/lumimotionsensor.h \
    #lumi/lumitemperaturesensor.h \
    zigbeedevice.h

