#include "tradfrirangeextender.h"
#include "extern-plugininfo.h"

TradfriRangeExtender::TradfriRangeExtender(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoit, m_node->endpoints()) {
        if (endpoit->deviceId() == Zigbee::HomeAutomationDeviceRangeExtender) {
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

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriRangeExtender::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &TradfriRangeExtender::onEndpointClusterAttributeChanged);
}

void TradfriRangeExtender::identify()
{
    m_endpoint->identify(2);
}

void TradfriRangeExtender::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void TradfriRangeExtender::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(tradfriRangeExtenderConnectedStateTypeId, true);
        thing()->setStateValue(tradfriRangeExtenderVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        thing()->setStateValue(tradfriRangeExtenderConnectedStateTypeId, false);
    }
}

void TradfriRangeExtender::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void TradfriRangeExtender::onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << thing() << "cluster attribute changed" << cluster << attribute;
}
