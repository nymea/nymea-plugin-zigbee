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

#include "lumirelay.h"
#include "extern-plugininfo.h"

#include <QDataStream>

LumiRelay::LumiRelay(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    m_longPressedTimer = new QTimer(this);
    m_longPressedTimer->setInterval(300);
    m_longPressedTimer->setSingleShot(true);
    connect(m_longPressedTimer, &QTimer::timeout, this, &LumiRelay::onLongPressedTimeout);

    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint is sending the button notifications
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    // Update signal strength
    connect(m_node, &ZigbeeNode::lqiChanged, this, [this](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbee()) << m_thing << "signal strength changed" << signalStrength << "%";
        m_thing->setStateValue(lumiRelaySignalStrengthStateTypeId, signalStrength);
    });

    // Get the ZigbeeClusterOnOff server
    m_onOffCluster = m_endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_onOffCluster, &ZigbeeClusterOnOff::powerChanged, this, [this](bool power){
            bool pressed = !power;
            qCDebug(dcZigbee()) << m_thing << "state changed" << (pressed ? "pressed" : "released");
            setPressed(!power);
        });
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiRelay::onNetworkStateChanged);
}

void LumiRelay::removeFromNetwork()
{
    m_network->removeZigbeeNode(m_node->extendedAddress());
}

void LumiRelay::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        m_thing->setStateValue(lumiRelayConnectedStateTypeId, true);
        m_thing->setStateValue(lumiRelayVersionStateTypeId, m_endpoint->softwareBuildId());
        m_thing->setStateValue(lumiRelaySignalStrengthStateTypeId, qRound(m_node->lqi() * 100.0 / 255.0));
    } else {
        m_thing->setStateValue(lumiRelayConnectedStateTypeId, false);
    }
}

void LumiRelay::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == lumiRelayRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void LumiRelay::setPressed(bool pressed)
{
    if (m_pressed == pressed)
        return;

    m_pressed = pressed;
    if (m_pressed) {
        m_longPressedTimer->start();
    } else {
        if (m_longPressedTimer->isActive()) {
            m_longPressedTimer->stop();
            emit buttonPressed();
        }
    }
}

void LumiRelay::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void LumiRelay::onLongPressedTimeout()
{
    m_longPressedTimer->stop();
    emit buttonLongPressed();
}
