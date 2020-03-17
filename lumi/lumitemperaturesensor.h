#ifndef LUMITEMPERATURESENSOR_H
#define LUMITEMPERATURESENSOR_H

#include <QObject>

#include "zigbeedevice.h"

class LumiTemperatureSensor : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit LumiTemperatureSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent = nullptr);

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

#endif // LUMITEMPERATURESENSOR_H
