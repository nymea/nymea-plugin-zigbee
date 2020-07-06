#include "xiaomimotionsensor.h"
#include "extern-plugininfo.h"

XiaomiMotionSensor::XiaomiMotionSensor(ZigbeeNode *node, QObject *parent) :
    QObject(parent),
    m_node(node)
{
    m_delayTimer = new QTimer(this);
    m_delayTimer->setInterval(m_delay * 1000);
    m_delayTimer->setSingleShot(true);

    connect(m_delayTimer, &QTimer::timeout, this, &XiaomiMotionSensor::onDelayTimerTimeout);

    setConnected(m_node->connected());

    connect(node, &ZigbeeNode::connectedChanged, this, &XiaomiMotionSensor::onNodeConnectedChanged);
    connect(node, &ZigbeeNode::clusterAttributeChanged, this, &XiaomiMotionSensor::onClusterAttributeChanged);
}

bool XiaomiMotionSensor::connected() const
{
    return m_connected;
}

bool XiaomiMotionSensor::present() const
{
    return m_present;
}

int XiaomiMotionSensor::delay() const
{
    return m_delay;
}

void XiaomiMotionSensor::setDelay(int delay)
{
    if (m_delay == delay)
        return;

    m_delay = delay;
    emit delayChanged(m_delay);
}

void XiaomiMotionSensor::setConnected(bool connected)
{
    if (m_connected == connected)
        return;

    m_connected = connected;
    emit connectedChanged(m_connected);
}

void XiaomiMotionSensor::setPresent(bool present)
{
    if (m_present == present)
        return;

    m_present = present;
    emit presentChanged(m_present);

    if (m_present) {
        m_delayTimer->setInterval(m_delay * 1000);
        m_delayTimer->start();
    } else {
        m_delayTimer->stop();
    }
}

void XiaomiMotionSensor::onNodeConnectedChanged(bool connected)
{
    setConnected(connected);
}

void XiaomiMotionSensor::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    Q_UNUSED(attribute)

    switch (cluster->clusterId()) {
    case Zigbee::ClusterIdOccapancySensing:
        setPresent(true);
        emit motionDetected();
        break;
    default:
        break;
    }
}

void XiaomiMotionSensor::onDelayTimerTimeout()
{
    setPresent(false);
}
