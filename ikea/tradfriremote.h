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

private:
    ZigbeeNodeEndpoint *m_remoteEndpoint = nullptr;

signals:

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);

};

#endif // TRADFRIREMOTE_H
