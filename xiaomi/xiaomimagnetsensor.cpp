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

#include "xiaomimagnetsensor.h"

#include <QDataStream>

XiaomiMagnetSensor::XiaomiMagnetSensor(ZigbeeNode *node, QObject *parent) :
    QObject(parent),
    m_node(node)
{
    // Init values
    if (m_node->hasOutputCluster(Zigbee::ClusterIdOnOff)) {
        ZigbeeCluster *cluster = m_node->getOutputCluster(Zigbee::ClusterIdOnOff);
        QByteArray closedData = cluster->attribute(0x0000).data();
        if (!closedData.isEmpty()) {
            QDataStream stream(&closedData, QIODevice::ReadOnly);
            quint8 closedRaw = 0;
            stream >> closedRaw;
            setClosed(!static_cast<bool>(closedRaw));
        }
    }

    setConnected(m_node->connected());

    connect(node, &ZigbeeNode::connectedChanged, this, &XiaomiMagnetSensor::onNodeConnectedChanged);
    connect(node, &ZigbeeNode::clusterAttributeChanged, this, &XiaomiMagnetSensor::onClusterAttributeChanged);
}

bool XiaomiMagnetSensor::connected() const
{
    return m_connected;
}

bool XiaomiMagnetSensor::closed() const
{
    return m_closed;
}

void XiaomiMagnetSensor::setConnected(bool connected)
{
    if (m_connected == connected)
        return;

    m_connected = connected;
    emit connectedChanged(m_connected);
}

void XiaomiMagnetSensor::setClosed(bool closed)
{
    if (m_closed == closed)
        return;

    m_closed = closed;
    emit closedChanged(m_closed);
}

void XiaomiMagnetSensor::onNodeConnectedChanged(bool connected)
{
    setConnected(connected);
}

void XiaomiMagnetSensor::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    Q_UNUSED(attribute)
    switch (cluster->clusterId()) {
    case Zigbee::ClusterIdOnOff:
        if (attribute.id() == 0) {
            QByteArray data = attribute.data();
            QDataStream stream(&data, QIODevice::ReadOnly);
            quint8 closedRaw = 0;
            stream >> closedRaw;
            setClosed(!static_cast<bool>(closedRaw));
        }
        break;
    default:
        break;

    }
}
