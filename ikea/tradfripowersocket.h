#ifndef TRADFRIPOWERSOCKET_H
#define TRADFRIPOWERSOCKET_H

#include <QObject>

#include "zigbeedevice.h"

class TradfriPowerSocket : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit TradfriPowerSocket(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent = nullptr);

    void identify();
    void removeFromNetwork() override;
    void checkOnlineStatus() override;

    void setPower(bool power);

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

    void readAttribute();
    void configureReporting();


private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

};

#endif // TRADFRIPOWERSOCKET_H
