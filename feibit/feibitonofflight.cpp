#include "feibitonofflight.h"
#include "extern-plugininfo.h"

FeiBitOnOffLight::FeiBitOnOffLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, device, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoit, m_node->endpoints()) {
        if (endpoit->deviceId() == Zigbee::LightLinkDeviceOnOffLight) {
            m_endpoint = endpoit;
            break;
        }
    }

    qCDebug(dcZigbee()) << m_device << m_endpoint;
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
    // Configure reporting
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            m_endpoint->configureReporting(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
        }

        if (cluster->clusterId() == Zigbee::ClusterIdLevelControl) {
            m_endpoint->configureReporting(cluster, { ZigbeeCluster::LevelClusterAttributeCurrentLevel });
        }
    }
}

void FeiBitOnOffLight::setPower(bool power)
{
    qCDebug(dcZigbee()) << m_device << "set power" << power;
    m_endpoint->sendOnOffClusterCommand(power ? ZigbeeCluster::OnOffClusterCommandOn : ZigbeeCluster::OnOffClusterCommandOff);
    readAttribute();
}

void FeiBitOnOffLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    if (state == ZigbeeNetwork::StateRunning) {
        device()->setStateValue(feibitOnOffLightConnectedStateTypeId, true);
        readAttribute();
    } else {
        device()->setStateValue(feibitOnOffLightConnectedStateTypeId, false);
    }
}

void FeiBitOnOffLight::onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << device() << "cluster attribute changed" << cluster << attribute;

    if (cluster->clusterId() == Zigbee::ClusterIdOnOff && attribute.id() == ZigbeeCluster::OnOffClusterAttributeOnOff) {
        if (attribute.dataType() != Zigbee::DataType::Bool || attribute.data().count() == 0) {
            qCWarning(dcZigbee()) << "Unexpected data type for attribute changed signal" << device() << cluster << attribute;
            return;
        }

        bool power = static_cast<bool>(attribute.data().at(0));
        device()->setStateValue(feibitOnOffLightPowerStateTypeId, power);
    }
}
