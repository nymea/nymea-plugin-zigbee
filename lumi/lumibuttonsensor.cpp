#include "lumibuttonsensor.h"
#include "extern-plugininfo.h"

LumiButtonSensor::LumiButtonSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, device, parent)
{
    m_longPressedTimer = new QTimer(this);
    m_longPressedTimer->setInterval(300);
    m_longPressedTimer->setSingleShot(true);
    connect(m_longPressedTimer, &QTimer::timeout, this, &LumiButtonSensor::onLongPressedTimeout);

    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint is sending the button notifications
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiButtonSensor::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &LumiButtonSensor::onEndpointClusterAttributeChanged);
}

void LumiButtonSensor::identify()
{
    m_endpoint->identify(2);
}

void LumiButtonSensor::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        device()->setStateValue(lumiButtonSensorConnectedStateTypeId, true);
    } else {
        device()->setStateValue(lumiButtonSensorConnectedStateTypeId, false);
    }
}

void LumiButtonSensor::setPressed(bool pressed)
{
    if (m_pressed == pressed)
        return;

    m_pressed = pressed;
    if (m_pressed) {
        qCDebug(dcZigbee()) << "Button pressed";
        m_longPressedTimer->start();
    } else {
        qCDebug(dcZigbee()) << "Button released";
        if (m_longPressedTimer->isActive()) {
            m_longPressedTimer->stop();
            emit buttonPressed();
        }
    }
}


void LumiButtonSensor::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void LumiButtonSensor::onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << "Lumi button cluster attribute changed" << cluster << attribute;
    if (cluster->clusterId() == Zigbee::ClusterIdOnOff && attribute.id() == ZigbeeCluster::OnOffClusterAttributeOnOff) {
        QByteArray data = attribute.data();
        QDataStream stream(&data, QIODevice::ReadOnly);
        quint8 pressedRaw = 0;
        stream >> pressedRaw;
        setPressed(!static_cast<bool>(pressedRaw));
    }
}

void LumiButtonSensor::onLongPressedTimeout()
{
    qCDebug(dcZigbee()) << "Button long pressed";
    m_longPressedTimer->stop();
    emit buttonLongPressed();
}
