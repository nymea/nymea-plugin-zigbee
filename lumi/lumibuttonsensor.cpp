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

#include "lumibuttonsensor.h"
#include "extern-plugininfo.h"

#include <QDataStream>

LumiButtonSensor::LumiButtonSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    m_longPressedTimer = new QTimer(this);
    m_longPressedTimer->setInterval(300);
    m_longPressedTimer->setSingleShot(true);
    connect(m_longPressedTimer, &QTimer::timeout, this, &LumiButtonSensor::onLongPressedTimeout);

    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint is sending the button notifications
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    // Get the ZigbeeClusterOnOff server
    m_onOffCluster = m_endpoint->inputCluster<ZigbeeClusterOnOff>(Zigbee::ClusterIdOnOff);
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_onOffCluster, &ZigbeeClusterOnOff::powerChanged, this, [this](bool power){
            qCDebug(dcZigbee()) << m_thing << "power state changed" << power;
            setPressed(power);
        });
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiButtonSensor::onNetworkStateChanged);
}

void LumiButtonSensor::removeFromNetwork()
{
    //m_node->leaveNetworkRequest();
}

void LumiButtonSensor::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(lumiButtonSensorConnectedStateTypeId, true);
        thing()->setStateValue(lumiButtonSensorVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        thing()->setStateValue(lumiButtonSensorConnectedStateTypeId, false);
    }
}

void LumiButtonSensor::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == lumiButtonSensorRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void LumiButtonSensor::setPressed(bool pressed)
{
    m_pressed = pressed;
    if (m_pressed) {
        qCDebug(dcZigbee()) << "Button pressed" << thing();
        m_longPressedTimer->start();
    } else {
        qCDebug(dcZigbee()) << "Button released" << thing();
        m_longPressedTimer->stop();
        emit buttonPressed();
    }
}

void LumiButtonSensor::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void LumiButtonSensor::onLongPressedTimeout()
{
    qCDebug(dcZigbee()) << "Button long pressed" << thing();
    m_longPressedTimer->stop();
    emit buttonLongPressed();
}
