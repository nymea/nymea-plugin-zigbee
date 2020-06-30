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

#include "tradfrionoffswitch.h"
#include "extern-plugininfo.h"

TradfriOnOffSwitch::TradfriOnOffSwitch(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoint, m_node->endpoints()) {
        if (endpoint->deviceId() == Zigbee::HomeAutomationDeviceNonColourController) {
            m_endpoint = endpoint;
            break;
        }
    }

    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice could not find endpoint.");

    // Update signal strength
    connect(m_node, &ZigbeeNode::lqiChanged, this, [this](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbee()) << m_thing << "signal strength changed" << signalStrength << "%";
        m_thing->setStateValue(tradfriOnOffSwitchSignalStrengthStateTypeId, signalStrength);
    });

    m_thing->setStateValue(tradfriOnOffSwitchSignalStrengthStateTypeId, qRound(m_node->lqi() * 100.0 / 255.0));

    // Get the onOff client cluster in order to receive signals if the cluster executed a command
    m_onOffCluster = m_endpoint->outputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find on/off client cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_onOffCluster, &ZigbeeClusterOnOff::commandSent, this, [this](ZigbeeClusterOnOff::Command command){
            qCDebug(dcZigbee()) << m_thing << "button pressed" << command;
            if (command == ZigbeeClusterOnOff::CommandOn) {
                emit onPressed();
            } else if (command == ZigbeeClusterOnOff::CommandOff) {
                emit offPressed();
            } else {
                // Ignore any other command executed by the on/off client cluster
            }
        });
    }

    // Get the level client cluster in order to receive signals if the cluster executed a command
    m_levelCluster = m_endpoint->outputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
    if (!m_levelCluster) {
        qCWarning(dcZigbee()) << "Could not find level client cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_levelCluster, &ZigbeeClusterLevelControl::commandSent, this, [this](ZigbeeClusterLevelControl::Command command, const QByteArray &payload){
            qCDebug(dcZigbee()) << m_thing << "button pressed" << command << payload.toHex();
            switch (command) {
            case ZigbeeClusterLevelControl::CommandMoveWithOnOff:
                emit onLongPressed();
                break;
            case ZigbeeClusterLevelControl::CommandMove:
                emit offLongPressed();
                break;
            default:
                break;
            }
        });
    }

    m_powerCluster = m_endpoint->inputCluster<ZigbeeClusterPowerConfiguration>(ZigbeeClusterLibrary::ClusterIdPowerConfiguration);
    if (!m_powerCluster) {
        qCWarning(dcZigbee()) << "Could not find power configuration cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_powerCluster, &ZigbeeClusterPowerConfiguration::batteryPercentageChanged, this, [this](double percentage){
            qCDebug(dcZigbee()) << "Battery percentage changed" << percentage << "%" << m_thing;
            m_thing->setStateValue(tradfriOnOffSwitchBatteryLevelStateTypeId, percentage);
            m_thing->setStateValue(tradfriOnOffSwitchBatteryCriticalStateTypeId, (percentage < 10.0));
        });
    }

    //    ZigbeeClusterReply *reply = m_powerCluster->readAttributes({ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining});
    //    connect(reply, &ZigbeeClusterReply::finished, this, [this, reply](){
    //        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
    //            qCWarning(dcZigbee()) << "Failed to read power cluster attributes" << reply->error();
    //            return;
    //        }

    //        // Bind
    //        ZigbeeDeviceObjectReply *zdoReply =
    //                m_node->deviceObject()->requestBindIeeeAddress(m_endpoint->endpointId(),
    //                                                               static_cast<quint16>(m_powerCluster->clusterId()),
    //                                                               m_network->coordinatorNode()->extendedAddress(), 0x01);
    //        connect(zdoReply, &ZigbeeDeviceObjectReply::finished, this, [zdoReply](){
    //            if (zdoReply->error() != ZigbeeDeviceObjectReply::ErrorNoError) {
    //                qCWarning(dcZigbee()) << "Failed to bind power cluster attributes" << zdoReply->error();
    //                return;
    //            }
    //        });
    //    });

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriOnOffSwitch::onNetworkStateChanged);
}

void TradfriOnOffSwitch::removeFromNetwork()
{
    m_network->removeZigbeeNode(m_node->extendedAddress());
}

void TradfriOnOffSwitch::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(tradfriOnOffSwitchConnectedStateTypeId, true);
        thing()->setStateValue(tradfriOnOffSwitchVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        thing()->setStateValue(tradfriOnOffSwitchConnectedStateTypeId, false);
    }
}

void TradfriOnOffSwitch::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == tradfriOnOffSwitchRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }/* else if (info->action().actionTypeId() == tradfriOnOffSwitchTestActionTypeId) {
        testAction();
        info->finish(Thing::ThingErrorNoError);
    }*/
}

void TradfriOnOffSwitch::testAction()
{
    qCDebug(dcZigbee()) << "Test action !!!!!!";

    // Get basic cluster
    if (!m_powerCluster) {
        qCWarning(dcZigbee()) << "Could not get power cluster";
        return;
    }

    ZigbeeClusterLibrary::AttributeReportingConfiguration reportingConfig;
    reportingConfig.attributeId = ZigbeeClusterPowerConfiguration::AttributeBatteryPercentageRemaining;
    reportingConfig.dataType = Zigbee::Uint8;
    reportingConfig.minReportingInterval = 300;
    reportingConfig.maxReportingInterval = 2700;
    reportingConfig.reportableChange = ZigbeeDataType(static_cast<quint8>(1)).data();

    ZigbeeClusterReply *reply = m_powerCluster->configureReporting({reportingConfig});
    connect(reply, &ZigbeeClusterReply::finished, this, [reply](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbee()) << "Failed to read power cluster attributes" << reply->error();
            return;
        }

        qCDebug(dcZigbee()) << "Reporting config finished" << ZigbeeClusterLibrary::parseAttributeReportingStatusRecords(reply->responseFrame().payload);
    });
}

void TradfriOnOffSwitch::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}
