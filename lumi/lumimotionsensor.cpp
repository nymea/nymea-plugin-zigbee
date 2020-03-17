#include "lumimotionsensor.h"
#include "extern-plugininfo.h"

#include <QDateTime>

#include <math.h>

LumiMotionSensor::LumiMotionSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    connect(m_delayTimer, &QTimer::timeout, this, &LumiMotionSensor::onDelayTimerTimeout);

    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint is sending the button notifications
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiMotionSensor::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &LumiMotionSensor::onEndpointClusterAttributeChanged);
}

void LumiMotionSensor::identify()
{
    m_endpoint->identify(2);
}

void LumiMotionSensor::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void LumiMotionSensor::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(lumiMotionSensorConnectedStateTypeId, true);
        thing()->setStateValue(lumiMotionSensorVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        thing()->setStateValue(lumiMotionSensorConnectedStateTypeId, false);
    }
}

void LumiMotionSensor::setPresent(bool present)
{
    m_present = present;
    thing()->setStateValue(lumiMotionSensorIsPresentStateTypeId, m_present);
    if (m_present) {
        m_delayTimer->setInterval(thing()->settings().paramValue(lumiMotionSensorSettingsTimeoutParamTypeId).toInt() * 1000);
        m_delayTimer->start();
    } else {
        m_delayTimer->stop();
    }
}

void LumiMotionSensor::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void LumiMotionSensor::onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << "Lumi motion sensor cluster attribute changed" << cluster << attribute;
    if (cluster->clusterId() == Zigbee::ClusterIdMeasurementIlluminance && attribute.id() == 0x0000) {
        QByteArray data = attribute.data();
        QDataStream stream(&data, QIODevice::ReadOnly);
        quint16 illuminanceRaw = 0;
        stream >> illuminanceRaw;
        // Note: this is again out of spec. The actual lux would be according to spec pow(10, (illuminanceRaw / 10000)) -1
        qCDebug(dcZigbee()) << "Illuminance value" << illuminanceRaw << "lux";
        thing()->setStateValue(lumiMotionSensorLightIntensityStateTypeId, illuminanceRaw);
    }

    if (cluster->clusterId() == Zigbee::ClusterIdOccapancySensing && attribute.id() == 0x0000) {
        qCDebug(dcZigbee()) << "Motion detected" << static_cast<bool>(attribute.data().at(0));
        thing()->setStateValue(lumiMotionSensorLastSeenTimeStateTypeId, QDateTime::currentDateTime().toTime_t());
        emit motionDetected();
        setPresent(true);
    }
}

void LumiMotionSensor::onDelayTimerTimeout()
{
    setPresent(false);
}
