#ifndef TRADFRICOLORTEMPERATURELIGHT_H
#define TRADFRICOLORTEMPERATURELIGHT_H

#include <QObject>
#include <QColor>

#include "zigbeedevice.h"

class TradfriColorTemperatureLight : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit TradfriColorTemperatureLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent = nullptr);

    void identify();
    void removeFromNetwork() override;
    void checkOnlineStatus() override;

    void setPower(bool power);
    void setBrightness(int brightness);
    void setColorTemperature(int colorTemperature);

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

    void readOnOffState();
    void readLevelValue();
    void readColorTemperature();

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

};

#endif // TRADFRICOLORTEMPERATURELIGHT_H
