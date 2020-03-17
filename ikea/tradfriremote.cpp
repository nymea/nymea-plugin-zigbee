#include "tradfriremote.h"
#include "extern-plugininfo.h"

TradfriRemote::TradfriRemote(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, device, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoit, m_node->endpoints()) {
        if (endpoit->deviceId() == Zigbee::LightLinkDeviceNonColourSceneController) {
            m_endpoint = endpoit;
            break;
        }
    }

    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice could not find endpoint.");

    qCDebug(dcZigbee()) << m_device << m_endpoint;
    qCDebug(dcZigbee()) << "Input clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    qCDebug(dcZigbee()) << "Output clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    configureReporting();


    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriRemote::onNetworkStateChanged);
}

void TradfriRemote::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void TradfriRemote::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        device()->setStateValue(tradfriRemoteConnectedStateTypeId, true);
        device()->setStateValue(tradfriRemoteVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        device()->setStateValue(tradfriRemoteConnectedStateTypeId, false);
    }
}

void TradfriRemote::identify()
{
    m_endpoint->identify(5);
}


void TradfriRemote::readAttribute()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdBasic) {
            m_endpoint->readAttribute(cluster, {0x0004, 0x0005});
        }
    }
}

void TradfriRemote::configureReporting()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            qCWarning(dcZigbee()) << "===============";
            ZigbeeClusterReportConfigurationRecord reportConfiguration;
            reportConfiguration.direction = 0x00;
            reportConfiguration.dataType = Zigbee::Bool;
            reportConfiguration.attributeId = 0x0000; // OnOff
            reportConfiguration.minInterval = 0x0000;
            reportConfiguration.maxInterval = 0x0001;
            reportConfiguration.timeout = 0x0001;
            reportConfiguration.change = 0x02;

            m_endpoint->configureReporting(cluster, { reportConfiguration });
        }
    }
}

void TradfriRemote::addGroup()
{
}


void TradfriRemote::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}
