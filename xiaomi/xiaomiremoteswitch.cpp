#include "xiaomiremoteswitch.h"
#include "extern-plugininfo.h"

#include <QDataStream>

XiaomiRemoteSwitch::XiaomiRemoteSwitch(ZigbeeNode *node, QObject *parent) :
    QObject(parent),
    m_node(node)

{
    m_longPressedTimer = new QTimer(this);
    m_longPressedTimer->setInterval(300);
    m_longPressedTimer->setSingleShot(true);
    connect(m_longPressedTimer, &QTimer::timeout, this, &XiaomiRemoteSwitch::onLongPressedTimeout);

    // Init values

    setConnected(m_node->connected());

    connect(node, &ZigbeeNode::connectedChanged, this, &XiaomiRemoteSwitch::onNodeConnectedChanged);
    connect(node, &ZigbeeNode::clusterAttributeChanged, this, &XiaomiRemoteSwitch::onClusterAttributeChanged);
}

bool XiaomiRemoteSwitch::connected() const
{
    return m_connected;
}

bool XiaomiRemoteSwitch::pressed() const
{
    return m_pressed;
}

void XiaomiRemoteSwitch::setConnected(bool connected)
{
    if (m_connected == connected)
        return;

    m_connected = connected;
    emit connectedChanged(m_connected);
}

void XiaomiRemoteSwitch::setPressed(bool pressed)
{
    if (m_pressed == pressed)
        return;

    m_pressed = pressed;
    emit pressedChanged(m_pressed);
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

void XiaomiRemoteSwitch::onLongPressedTimeout()
{
    qCDebug(dcZigbee()) << "Button long pressed";
    m_longPressedTimer->stop();
    emit buttonLongPressed();
}

void XiaomiRemoteSwitch::onNodeConnectedChanged(bool connected)
{
    setConnected(connected);
}

void XiaomiRemoteSwitch::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    switch (cluster->clusterId()) {
    case Zigbee::ClusterIdOnOff:
        if (attribute.id() == 0) {
            QByteArray data = attribute.data();
            QDataStream stream(&data, QIODevice::ReadOnly);
            quint8 pressedRaw = 0;
            stream >> pressedRaw;
            //TODO read which button has been pressed, change event to incl button number
            setPressed(!static_cast<bool>(pressedRaw));
        }
        qCDebug(dcZigbee()) << "Unhandled attribute id: " << attribute.id();
        break;
    default:
        qCDebug(dcZigbee()) << "Remote switch unhandled cluser event: " << cluster->clusterId();
        break;

    }
}
