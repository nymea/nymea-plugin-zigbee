#include "tradfrionoffswitch.h"
#include "extern-plugininfo.h"

TradfriOnOffSwitch::TradfriOnOffSwitch(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, device, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoit, m_node->endpoints()) {
        if (endpoit->deviceId() == Zigbee::HomeAutomationDeviceNonColourController) {
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

    // Enable reporting
    configureReporting();

    // Read stuff



    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriOnOffSwitch::onNetworkStateChanged);
}

void TradfriOnOffSwitch::identify()
{
    //m_endpoint->addGroup(0x01, 0x0001);
    m_endpoint->bindUnicast(Zigbee::ClusterIdOnOff, m_network->coordinatorNode()->extendedAddress(), 0x01);
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            qCWarning(dcZigbee()) << "===============";
            ZigbeeClusterReportConfigurationRecord reportConfiguration;
            reportConfiguration.direction = 0x00;
            reportConfiguration.dataType = Zigbee::Bool;
            reportConfiguration.attributeId = 0x0000; // OnOff
            reportConfiguration.minInterval = 0x0001;
            reportConfiguration.maxInterval = 0x0001;
            reportConfiguration.timeout = 0x0001;
            reportConfiguration.change = 0x01;

            m_endpoint->configureReporting(cluster, { reportConfiguration });
        }
    }
}

void TradfriOnOffSwitch::factoryResetNode()
{
    m_endpoint->factoryReset();
}

void TradfriOnOffSwitch::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void TradfriOnOffSwitch::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        device()->setStateValue(tradfriOnOffSwitchConnectedStateTypeId, true);
        device()->setStateValue(tradfriOnOffSwitchVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        device()->setStateValue(tradfriOnOffSwitchConnectedStateTypeId, false);
    }
}

void TradfriOnOffSwitch::readAttribute()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdBasic) {
            m_endpoint->readAttribute(cluster, {0x0004, 0x0005});
        }
    }
}

void TradfriOnOffSwitch::configureReporting()
{
//    //m_endpoint->addGroup(0x01, 0x0001);
//    m_endpoint->bindUnicast(Zigbee::ClusterIdOnOff, m_network->coordinatorNode()->extendedAddress(), 0x01);
//    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
//        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
//            m_endpoint->configureReporting(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
//        }
//    }
}

void TradfriOnOffSwitch::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}
