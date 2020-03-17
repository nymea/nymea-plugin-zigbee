#include "feibitonofflight.h"
#include "extern-plugininfo.h"

FeiBitOnOffLight::FeiBitOnOffLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoit, m_node->endpoints()) {
        if (endpoit->deviceId() == Zigbee::LightLinkDeviceOnOffLight) {
            m_endpoint = endpoit;
            break;
        }
    }

    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice could not find endpoint.");

    qCDebug(dcZigbee()) << m_thing << m_endpoint;
    qCDebug(dcZigbee()) << "Input clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    qCDebug(dcZigbee()) << "Output clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &FeiBitOnOffLight::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &FeiBitOnOffLight::onEndpointClusterAttributeChanged);

    configureReporting();
    readAttribute();
}

void FeiBitOnOffLight::identify()
{
    m_endpoint->identify(2);
}

void FeiBitOnOffLight::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(feibitOnOffLightConnectedStateTypeId, true);
        thing()->setStateValue(feibitOnOffLightVersionStateTypeId, m_endpoint->softwareBuildId());
        readAttribute();
    } else {
        thing()->setStateValue(feibitOnOffLightConnectedStateTypeId, false);
    }
}

void FeiBitOnOffLight::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void FeiBitOnOffLight::setPower(bool power)
{
    qCDebug(dcZigbee()) << m_thing << "set power" << power;
    m_endpoint->sendOnOffClusterCommand(power ? ZigbeeCluster::OnOffClusterCommandOn : ZigbeeCluster::OnOffClusterCommandOff);
    readAttribute();
}

void FeiBitOnOffLight::readAttribute()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
        }
    }
}

void FeiBitOnOffLight::configureReporting()
{

}


void FeiBitOnOffLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void FeiBitOnOffLight::onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << thing() << "cluster attribute changed" << cluster << attribute;

    if (cluster->clusterId() == Zigbee::ClusterIdOnOff && attribute.id() == ZigbeeCluster::OnOffClusterAttributeOnOff) {
        if (attribute.dataType() != Zigbee::DataType::Bool || attribute.data().count() == 0) {
            qCWarning(dcZigbee()) << "Unexpected data type for attribute changed signal" << thing() << cluster << attribute;
            return;
        }

        bool power = static_cast<bool>(attribute.data().at(0));
        thing()->setStateValue(feibitOnOffLightPowerStateTypeId, power);
    }
}
