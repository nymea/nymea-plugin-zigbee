#include "zigbeedevice.h"
#include "extern-plugininfo.h"

ZigbeeDevice::ZigbeeDevice(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    QObject(parent),
    m_network(network),
    m_ieeeAddress(ieeeAddress),
    m_thing(thing)
{
    m_node = m_network->getZigbeeNode(m_ieeeAddress);
}

ZigbeeNetwork *ZigbeeDevice::network() const
{
    return m_network;
}

ZigbeeAddress ZigbeeDevice::ieeeAddress() const
{
    return m_ieeeAddress;
}

Thing *ZigbeeDevice::thing() const
{
    return m_thing;
}
