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
