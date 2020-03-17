#ifndef FEIBITONOFFLIGHT_H
#define FEIBITONOFFLIGHT_H

#include <QObject>

#include "zigbeedevice.h"

class FeiBitOnOffLight : public ZigbeeDevice
{
    Q_OBJECT

public:
    FeiBitOnOffLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    void checkOnlineStatus() override;
    void removeFromNetwork() override;

    void identify();
    void setPower(bool power);

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

    void readAttribute();
    void configureReporting();

signals:

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);


};

#endif // FEIBITONOFFLIGHT_H
