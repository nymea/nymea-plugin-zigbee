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

#include "lumimotionsensor.h"
#include "extern-plugininfo.h"

#include <QDateTime>
#include <QDataStream>

#include <math.h>

LumiMotionSensor::LumiMotionSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    connect(m_delayTimer, &QTimer::timeout, this, [this](){
        setPresent(false);
    });

    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint is sending the button notifications
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    // Update signal strength
    connect(m_node, &ZigbeeNode::lqiChanged, this, [this](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbee()) << m_thing << "signal strength changed" << signalStrength << "%";
        m_thing->setStateValue(lumiMotionSensorSignalStrengthStateTypeId, signalStrength);
    });

    m_occupancyCluster = m_endpoint->inputCluster<ZigbeeClusterOccupancySensing>(ZigbeeClusterLibrary::ClusterIdOccupancySensing);
    if (!m_occupancyCluster) {
        qCWarning(dcZigbee()) << "Could not find the occupancy sensing server cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_occupancyCluster, &ZigbeeClusterOccupancySensing::occupancyChanged, this, [this](bool occupied){
            qCDebug(dcZigbee()) << m_thing << "occupied changed" << occupied;
            emit motionDetected();
            setPresent(true);
            m_thing->setStateValue(lumiMotionSensorLastSeenTimeStateTypeId, QDateTime::currentMSecsSinceEpoch() / 1000);
        });
    }

    m_illuminanceCluster = m_endpoint->inputCluster<ZigbeeClusterIlluminanceMeasurment>(ZigbeeClusterLibrary::ClusterIdIlluminanceMeasurement);
    if (!m_illuminanceCluster) {
        qCWarning(dcZigbee()) << "Could not find the illuminance measurement server cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_illuminanceCluster, &ZigbeeClusterIlluminanceMeasurment::illuminanceChanged, this, [this](quint16 illuminance){
            // Note: this is again out of spec. The actual lux would be according to spec pow(10, (illuminanceRaw / 10000)) -1
            qCDebug(dcZigbee()) << m_thing << "illuminance changed" << illuminance << "lux";
            m_thing->setStateValue(lumiMotionSensorLightIntensityStateTypeId, illuminance);
        });
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiMotionSensor::onNetworkStateChanged);
}

void LumiMotionSensor::removeFromNetwork()
{
    m_network->removeZigbeeNode(m_node->extendedAddress());
}

void LumiMotionSensor::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        m_thing->setStateValue(lumiMotionSensorConnectedStateTypeId, true);
        m_thing->setStateValue(lumiMotionSensorVersionStateTypeId, m_endpoint->softwareBuildId());
        m_thing->setStateValue(lumiMotionSensorSignalStrengthStateTypeId, qRound(m_node->lqi() * 100.0 / 255.0));
    } else {
        m_thing->setStateValue(lumiMotionSensorConnectedStateTypeId, false);
    }
}

void LumiMotionSensor::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == lumiMotionSensorRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void LumiMotionSensor::setPresent(bool present)
{
    qCDebug(dcZigbee()) << m_thing << "present changed to" << present;
    m_present = present;
    m_thing->setStateValue(lumiMotionSensorIsPresentStateTypeId, m_present);
    if (m_present) {
        m_delayTimer->setInterval(m_thing->settings().paramValue(lumiMotionSensorSettingsTimeoutParamTypeId).toInt() * 1000);
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
