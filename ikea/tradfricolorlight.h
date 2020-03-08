#ifndef TRADFRICOLORLIGHT_H
#define TRADFRICOLORLIGHT_H

#include <QObject>
#include <QColor>

#include "zigbeedevice.h"

class TradfriColorLight : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit TradfriColorLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    void identify();

    void setPower(bool power);
    void setBrightness(int brightness);
    void setColorTemperature(int colorTemperature);
    void setColor(const QColor &color);

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

    void readAttribute();
    void configureReporting();

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);
};

#endif // TRADFRICOLORLIGHT_H
