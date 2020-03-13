#ifndef TRADFRICOLORTEMPERATURELIGHT_H
#define TRADFRICOLORTEMPERATURELIGHT_H

#include <QObject>
#include <QColor>

#include "zigbeedevice.h"

class TradfriColorTemperatureLight : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit TradfriColorTemperatureLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    void identify();

    void checkOnlineStatus() override;

    void setPower(bool power);
    void setBrightness(int brightness);
    void setColorTemperature(int colorTemperature);

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;
    void readColorCapabilities();
    void readOnOffState();
    void readLevelValue();

    void configureReporting();

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

};

#endif // TRADFRICOLORTEMPERATURELIGHT_H
