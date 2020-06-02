/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
        m_longPressedTimer->start();
    } else {
        qCDebug(dcZigbee()) << "Button released";
        if (m_longPressedTimer->isActive()) {
            m_longPressedTimer->stop();
            emit buttonPressed();
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
