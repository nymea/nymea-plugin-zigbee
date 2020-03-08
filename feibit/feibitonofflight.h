#ifndef FEIBITONOFFLIGHT_H
#define FEIBITONOFFLIGHT_H

#include <QObject>

#include "zigbeedevice.h"

class FeiBitOnOffLight : public ZigbeeDevice
{
    Q_OBJECT

public:
    FeiBitOnOffLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    void identify();
    void readAttribute();
    void configureReporting();

    void setPower(bool power);

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

signals:

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);


};

#endif // FEIBITONOFFLIGHT_H
