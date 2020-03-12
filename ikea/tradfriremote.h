#ifndef TRADFRIREMOTE_H
#define TRADFRIREMOTE_H

#include <QObject>

#include "zigbeedevice.h"

class TradfriRemote : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit TradfriRemote(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    void identify();
    void readAttribute();
    void configureReporting();

    void checkOnlineStatus() override;

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

signals:

public slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);

};

#endif // TRADFRIREMOTE_H
