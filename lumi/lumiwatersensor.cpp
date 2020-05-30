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

    // Initialize the endpoint 0x01 since that endpoint contains ISA zone cluster
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    // Note: the device does not list the IAS zone cluster in the simple descriptor endpoint discovery. This is out of spec.
    // In order to make it work never the less, we can read the attrubte when the device sends the first notification.
    // From that moment on we can also read the current ZoneStatus attribute.

    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &LumiWaterSensor::onClusterAttributeChanged);
    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiWaterSensor::onNetworkStateChanged);
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
        // Note: this device does not respond on reading request
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

void LumiWaterSensor::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void LumiWaterSensor::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << m_thing << "cluster attribute changed" << cluster << attribute;
    if (cluster->clusterId() == Zigbee::ClusterIdIasZone) {
        if (attribute.id() == ZigbeeClusterIasZone::AttributeZoneState) {
            bool valueOk = false;
            ZigbeeClusterIasZone::ZoneStatusFlags zoneStatus = static_cast<ZigbeeClusterIasZone::ZoneStatusFlags>(attribute.dataType().toUInt16(&valueOk));
            if (!valueOk) {
                qCWarning(dcZigbee()) << m_thing << "failed to convert attribute data to uint16 flag. Not updating the states from" << attribute;
            } else {
                qCDebug(dcZigbee()) << m_thing << "zone status changed" << zoneStatus;

                // Water detected gets indicated in the Alarm1 flag
                if (zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusAlarm1)) {
                    m_thing->setStateValue(lumiWaterSensorWaterDetectedStateTypeId, true);
                } else {
                    m_thing->setStateValue(lumiWaterSensorWaterDetectedStateTypeId, false);
                }

                // Battery alarm
                if (zoneStatus.testFlag(ZigbeeClusterIasZone::ZoneStatusBattery)) {
                    m_thing->setStateValue(lumiWaterSensorBatteryCriticalStateTypeId, true);
                } else {
                    m_thing->setStateValue(lumiWaterSensorBatteryCriticalStateTypeId, false);
                }
            }
        }
    }
}
