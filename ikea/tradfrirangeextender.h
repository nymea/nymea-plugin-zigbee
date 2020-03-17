#ifndef TRADFRIRANGEEXTENDER_H
#define TRADFRIRANGEEXTENDER_H

#include <QObject>

#include "zigbeedevice.h"

class TradfriRangeExtender : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit TradfriRangeExtender(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent = nullptr);

    void identify();
    void removeFromNetwork() override;
    void checkOnlineStatus() override;

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

};

#endif // TRADFRIRANGEEXTENDER_H
