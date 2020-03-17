#ifndef ZIGBEEDEVICE_H
#define ZIGBEEDEVICE_H

#include <QObject>

#include <integrations/thing.h>
#include <zigbeenetwork.h>
#include <zigbeeaddress.h>
#include <zigbeenodeendpoint.h>

class ZigbeeDevice : public QObject
{
    Q_OBJECT

public:
    explicit ZigbeeDevice(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent = nullptr);

    ZigbeeNetwork *network() const;
    ZigbeeAddress ieeeAddress() const;
    Thing *thing() const;

    virtual void checkOnlineStatus() = 0;
    virtual void removeFromNetwork() = 0;

protected:
    ZigbeeNetwork *m_network = nullptr;
    ZigbeeAddress m_ieeeAddress;
    Thing *m_thing = nullptr;
    ZigbeeNode *m_node = nullptr;
};

#endif // ZIGBEEDEVICE_H
