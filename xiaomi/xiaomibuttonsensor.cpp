#include "xiaomibuttonsensor.h"
#include "extern-plugininfo.h"

#include <QDataStream>

XiaomiButtonSensor::XiaomiButtonSensor(ZigbeeNode *node, QObject *parent) :
    QObject(parent),
    m_node(node)

{
    m_longPressedTimer = new QTimer(this);
    m_longPressedTimer->setInterval(300);
    m_longPressedTimer->setSingleShot(true);
    connect(m_longPressedTimer, &QTimer::timeout, this, &XiaomiButtonSensor::onLongPressedTimeout);

    // Init values

    setConnected(m_node->connected());

    connect(node, &ZigbeeNode::connectedChanged, this, &XiaomiButtonSensor::onNodeConnectedChanged);
    connect(node, &ZigbeeNode::clusterAttributeChanged, this, &XiaomiButtonSensor::onClusterAttributeChanged);
}

bool XiaomiButtonSensor::connected() const
{
    return m_connected;
}

bool XiaomiButtonSensor::pressed() const
{
    return m_pressed;
}

void XiaomiButtonSensor::setConnected(bool connected)
{
    if (m_connected == connected)
        return;

    m_connected = connected;
    emit connectedChanged(m_connected);
}

void XiaomiButtonSensor::setPressed(bool pressed)
{
    if (m_pressed == pressed)
        return;

    m_pressed = pressed;
    emit pressedChanged(m_pressed);
    if (m_pressed) {
        qCDebug(dcZigbee()) << "Button pressed";
        emit buttonPressed();
        m_longPressedTimer->start();
    } else {
        qCDebug(dcZigbee()) << "Button released";
        if (m_longPressedTimer->isActive()) {
            m_longPressedTimer->stop();
        }
    }
}

void XiaomiButtonSensor::onLongPressedTimeout()
{
    qCDebug(dcZigbee()) << "Button long pressed";
    m_longPressedTimer->stop();
    emit buttonLongPressed();
}

void XiaomiButtonSensor::onNodeConnectedChanged(bool connected)
{
    setConnected(connected);
}

void XiaomiButtonSensor::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    Q_UNUSED(attribute)
    switch (cluster->clusterId()) {
    case Zigbee::ClusterIdOnOff:
        if (attribute.id() == 0) {
            QByteArray data = attribute.data();
            QDataStream stream(&data, QIODevice::ReadOnly);
            quint8 pressedRaw = 0;
            stream >> pressedRaw;
            setPressed(!static_cast<bool>(pressedRaw));
        }
        break;
    default:
        break;

    }
}
