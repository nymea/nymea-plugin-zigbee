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

#include "lumiwatersensor.h"
#include "extern-plugininfo.h"

#include <QDataStream>

LumiWaterSensor::LumiWaterSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint contains temperature and humidity clusters
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    m_iasZoneCluster = m_endpoint->inputCluster<ZigbeeClusterIasZone>(Zigbee::ClusterIdIasZone);
    if (!m_iasZoneCluster) {
        qCWarning(dcZigbee()) << "Could not find the IAS zone server cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_iasZoneCluster, &ZigbeeClusterIasZone::zoneStatusChanged, this, [this](ZigbeeClusterIasZone::ZoneStatusFlags zoneStatus, quint8 extendedStatus, quint8 zoneId, quint16 delay){
            qCDebug(dcZigbee()) << m_thing << "zone status changed" << zoneStatus << extendedStatus << zoneId << delay;

            // Water detected gets indicated in the Alarm1 flag
            if (zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusFlagAlarm1)) {
                m_thing->setStateValue(lumiWaterSensorWaterDetectedStateTypeId, true);
            } else {
                m_thing->setStateValue(lumiWaterSensorWaterDetectedStateTypeId, false);
            }

            // Battery alarm
            if (zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusFlagBattery)) {
                m_thing->setStateValue(lumiWaterSensorBatteryCriticalStateTypeId, true);
            } else {
                m_thing->setStateValue(lumiWaterSensorBatteryCriticalStateTypeId, true);
            }
        });
    }
}

void LumiWaterSensor::removeFromNetwork()
{
    // FIXME
    //m_node->deviceObject()->requestMgmtLeaveNetwork();
}

void LumiWaterSensor::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(lumiWaterSensorConnectedStateTypeId, true);
        thing()->setStateValue(lumiWaterSensorVersionStateTypeId, m_endpoint->softwareBuildId());
        // TODO: read the initial status once connected
    } else {
        thing()->setStateValue(lumiWaterSensorConnectedStateTypeId, false);
    }
}

void LumiWaterSensor::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == lumiWaterSensorRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}
