#include "xiaomimagnetsensor.h"

#include <QDataStream>

XiaomiMagnetSensor::XiaomiMagnetSensor(ZigbeeNode *node, QObject *parent) :
    QObject(parent),
    m_node(node)
{
//    // Init values
//    if (m_node->hasOutputCluster(Zigbee::ClusterIdOnOff)) {
//        ZigbeeCluster *cluster = m_node->getOutputCluster(Zigbee::ClusterIdOnOff);
//        QByteArray closedData = cluster->attribute(0x0000).data();
//        if (!closedData.isEmpty()) {
//            QDataStream stream(&closedData, QIODevice::ReadOnly);
//            quint8 closedRaw = 0;
//            stream >> closedRaw;
//            setClosed(!static_cast<bool>(closedRaw));
//        }
//    }

//    setConnected(m_node->connected());

//    connect(node, &ZigbeeNode::connectedChanged, this, &XiaomiMagnetSensor::onNodeConnectedChanged);
//    connect(node, &ZigbeeNode::clusterAttributeChanged, this, &XiaomiMagnetSensor::onClusterAttributeChanged);
}

bool XiaomiMagnetSensor::connected() const
{
    return m_connected;
}

bool XiaomiMagnetSensor::closed() const
{
    return m_closed;
}

void XiaomiMagnetSensor::setConnected(bool connected)
{
    if (m_connected == connected)
        return;

    m_connected = connected;
    emit connectedChanged(m_connected);
}

void XiaomiMagnetSensor::setClosed(bool closed)
{
    if (m_closed == closed)
        return;

    m_closed = closed;
    emit closedChanged(m_closed);
}

void XiaomiMagnetSensor::onNodeConnectedChanged(bool connected)
{
    setConnected(connected);
}

void XiaomiMagnetSensor::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    Q_UNUSED(attribute)
    switch (cluster->clusterId()) {
    case Zigbee::ClusterIdOnOff:
        if (attribute.id() == 0) {
            QByteArray data = attribute.data();
            QDataStream stream(&data, QIODevice::ReadOnly);
            quint8 closedRaw = 0;
            stream >> closedRaw;
            setClosed(!static_cast<bool>(closedRaw));
        }
        break;
    default:
        break;

    }
}
