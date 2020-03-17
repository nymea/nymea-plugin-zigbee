#ifndef TRADFRIREMOTE_H
#define TRADFRIREMOTE_H

#include <QObject>

#include "zigbeedevice.h"

class TradfriRemote : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit TradfriRemote(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent = nullptr);

    void identify();

    void removeFromNetwork() override;
    void checkOnlineStatus() override;

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

    void readAttribute();
    void configureReporting();

    void addGroup();
    void bindCluster();

signals:

public slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);

};

#endif // TRADFRIREMOTE_H
