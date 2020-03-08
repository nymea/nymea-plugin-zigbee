#include "tradfriremote.h"
#include "extern-plugininfo.h"

TradfriRemote::TradfriRemote(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, device, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoit, m_node->endpoints()) {
        if (endpoit->deviceId() == Zigbee::LightLinkDeviceNonColourSceneController) {
            m_remoteEndpoint = endpoit;
            break;
        }
    }

    qCDebug(dcZigbee()) << m_device << m_remoteEndpoint;
    qCDebug(dcZigbee()) << "Input clusters";
    foreach (ZigbeeCluster *cluster, m_remoteEndpoint->inputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    qCDebug(dcZigbee()) << "Output clusters";
    foreach (ZigbeeCluster *cluster, m_remoteEndpoint->outputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    // Enable reporting


    // Read stuff



    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriRemote::onNetworkStateChanged);
}

void TradfriRemote::readAttribute()
{
    foreach (ZigbeeCluster *cluster, m_remoteEndpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdBasic) {
            m_remoteEndpoint->readAttribute(cluster, {0x0004, 0x0005});
        }
    }
}

void TradfriRemote::configureReporting()
{
    foreach (ZigbeeCluster *cluster, m_remoteEndpoint->outputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            m_remoteEndpoint->configureReporting(cluster, {0x0000});
        }
    }
}

void TradfriRemote::identify()
{
    //m_remoteEndpoint->identify(5);
    //configureReporting();
    //readAttribute();

//    foreach (ZigbeeCluster *cluster, m_remoteEndpoint->outputClusters()) {
//        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
//            m_remoteEndpoint->readAttribute(cluster, {0x0000});
//        }
//    }
    // Add the coordinator endpoint to the group
    //m_network->coordinatorNode()->getEndpoint(0x01)->addGroup(0x01, 0x0001);

    //m_remoteEndpoint->addGroup(0x01, 0x0001);
    m_remoteEndpoint->bindUnicast(Zigbee::ClusterIdOnOff, m_network->coordinatorNode()->extendedAddress(), 0x01);
    //m_remoteEndpoint->bindGroup(Zigbee::ClusterIdOnOff, 0x0001, 0x01);
//    foreach (ZigbeeCluster *cluster, m_remoteEndpoint->outputClusters()) {
//        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
//        }
//    }

}

void TradfriRemote::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    if (state == ZigbeeNetwork::StateRunning) {
        device()->setStateValue(tradfriRemoteConnectedStateTypeId, true);
    } else {
        device()->setStateValue(tradfriRemoteConnectedStateTypeId, false);
    }
}
