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

    qCDebug(dcZigbee()) << m_device << m_endpoint;
    qCDebug(dcZigbee()) << "Input clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    qCDebug(dcZigbee()) << "Output clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    // Enable reporting


    // Read stuff



    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriRemote::onNetworkStateChanged);
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
//    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
//        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
//            ZigbeeClusterReportConfigurationRecord reportConfiguration;
//            reportConfiguration.attributeId = 0x0000;

//            m_endpoint->configureReporting(cluster, {0x0000});
//        }
    //    }
}

void TradfriRemote::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        device()->setStateValue(tradfriRemoteConnectedStateTypeId, true);
    } else {
        device()->setStateValue(tradfriRemoteConnectedStateTypeId, false);
    }
}

void TradfriRemote::identify()
{
    //m_endpoint->identify(5);

    //m_endpoint->addGroup(0x01, 0x0000);
    //m_endpoint->bindUnicast(Zigbee::ClusterIdOnOff, m_network->coordinatorNode()->extendedAddress(), 0x01);
    m_endpoint->bindGroup(Zigbee::ClusterIdOnOff, 0x0000, 0x01);
//    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
//        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
//        }
//    }

}

void TradfriRemote::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}
