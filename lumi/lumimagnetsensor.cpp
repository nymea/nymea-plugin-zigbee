#include "lumimagnetsensor.h"
#include "extern-plugininfo.h"

LumiMagnetSensor::LumiMagnetSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, device, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint is sending the temperature notifications
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiMagnetSensor::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &LumiMagnetSensor::onEndpointClusterAttributeChanged);
}

void LumiMagnetSensor::identify()
{
    m_endpoint->identify(2);
}

void LumiMagnetSensor::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void LumiMagnetSensor::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        device()->setStateValue(lumiMagnetSensorConnectedStateTypeId, true);
        device()->setStateValue(lumiMagnetSensorVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        device()->setStateValue(lumiMagnetSensorConnectedStateTypeId, false);
    }
}

void LumiMagnetSensor::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void LumiMagnetSensor::onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    if (cluster->clusterId() == Zigbee::ClusterIdOnOff && attribute.id() == ZigbeeCluster::OnOffClusterAttributeOnOff) {
        QByteArray data = attribute.data();
        QDataStream stream(&data, QIODevice::ReadOnly);
        quint8 closedRaw = 0;
        stream >> closedRaw;
        device()->setStateValue(lumiMagnetSensorClosedStateTypeId, !static_cast<bool>(closedRaw));
    }
}
