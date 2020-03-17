#ifndef LUMIMAGNETSENSOR_H
#define LUMIMAGNETSENSOR_H

#include <QObject>

#include "zigbeedevice.h"

class LumiMagnetSensor : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit LumiMagnetSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    void identify();
    void removeFromNetwork() override;
    void checkOnlineStatus() override;

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

signals:

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

};

#endif // LUMIMAGNETSENSOR_H
