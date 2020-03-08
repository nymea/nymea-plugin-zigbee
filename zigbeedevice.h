#ifndef ZIGBEEDEVICE_H
#define ZIGBEEDEVICE_H

#include <QObject>

#include "devices/device.h"
#include "nymea-zigbee/zigbeenetwork.h"
#include "nymea-zigbee/zigbeeaddress.h"
#include "nymea-zigbee/zigbeenodeendpoint.h"

class ZigbeeDevice : public QObject
{
    Q_OBJECT

public:
    explicit ZigbeeDevice(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    ZigbeeNetwork *network() const;
    ZigbeeAddress ieeeAddress() const;
    Device *device() const;

protected:
    ZigbeeNetwork *m_network = nullptr;
    ZigbeeAddress m_ieeeAddress;
    Device *m_device = nullptr;
    ZigbeeNode *m_node = nullptr;
};

#endif // ZIGBEEDEVICE_H
