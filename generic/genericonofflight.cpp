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

#include "genericonofflight.h"
#include "extern-plugininfo.h"

GenericOnOffLight::GenericOnOffLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoint, m_node->endpoints()) {
        if (endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceOnOffLight) {
            m_endpoint = endpoint;
            break;
        }

        if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOffLight) {
            m_endpoint = endpoint;
            break;
        }
    }

    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice could not find endpoint.");

    // Update signal strength
    connect(m_node, &ZigbeeNode::lqiChanged, this, [this](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbee()) << m_thing << "signal strength changed" << signalStrength << "%";
        m_thing->setStateValue(genericOnOffLightSignalStrengthStateTypeId, signalStrength);
    });

    m_thing->setStateValue(genericOnOffLightSignalStrengthStateTypeId, qRound(m_node->lqi() * 100.0 / 255.0));

    // Get the ZigbeeClusterOnOff server
    m_onOffCluster = m_endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_onOffCluster, &ZigbeeClusterOnOff::powerChanged, this, [this](bool power){
            qCDebug(dcZigbee()) << m_thing << "power state changed" << power;
            m_thing->setStateValue(genericOnOffLightPowerStateTypeId, power);
        });
    }

    m_identifyCluster = m_endpoint->inputCluster<ZigbeeClusterIdentify>(ZigbeeClusterLibrary::ClusterIdIdentify);
    if (!m_identifyCluster) {
        qCWarning(dcZigbee()) << "Could not find the identify input cluster on" << m_thing << m_endpoint;
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &GenericOnOffLight::onNetworkStateChanged);
}

void GenericOnOffLight::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(genericOnOffLightConnectedStateTypeId, true);
        thing()->setStateValue(genericOnOffLightVersionStateTypeId, m_endpoint->softwareBuildId());
        readOnOffState();
    } else {
        thing()->setStateValue(genericOnOffLightConnectedStateTypeId, false);
    }
}

void GenericOnOffLight::removeFromNetwork()
{
    m_network->removeZigbeeNode(m_node->extendedAddress());
}

void GenericOnOffLight::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == genericOnOffLightAlertActionTypeId) {
        if (!m_identifyCluster) {
            qCWarning(dcZigbee()) << "Could not find the identify input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        ZigbeeClusterReply *reply =  m_identifyCluster->identify(2);
        connect(reply, &ZigbeeClusterReply::finished, this, [reply, info](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
            }
        });
    } else if (info->action().actionTypeId() == genericOnOffLightPowerActionTypeId) {
        if (!m_onOffCluster) {
            qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        bool power = info->action().param(genericOnOffLightPowerActionPowerParamTypeId).value().toBool();
        ZigbeeClusterReply *reply = (power ? m_onOffCluster->commandOn() : m_onOffCluster->commandOff());
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, power](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                thing()->setStateValue(genericOnOffLightPowerStateTypeId, power);
            }
        });
    } else if (info->action().actionTypeId() == genericOnOffLightRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    } else if (info->action().actionTypeId() == genericOnOffLightTestActionTypeId) {
        qCDebug(dcZigbee()) << "Test action !!!!!!";
        // Get basic cluster
        ZigbeeClusterBasic *basicCluster = m_endpoint->inputCluster<ZigbeeClusterBasic>(ZigbeeClusterLibrary::ClusterIdBasic);
        if (!basicCluster) {
            qCWarning(dcZigbee()) << "Could not get basic cluster";
            return;
        }

        ZigbeeClusterReply *reply = basicCluster->readAttributes({ZigbeeClusterBasic::AttributeZclVersion, ZigbeeClusterBasic::AttributeManufacturerName, ZigbeeClusterBasic::AttributeModelIdentifier, ZigbeeClusterBasic::AttributeSwBuildId});
        connect(reply, &ZigbeeClusterReply::finished, this, [reply](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                qCWarning(dcZigbee()) << "Failed to read basic cluster attributes" << reply->error();
                return;
            }

            qCDebug(dcZigbee()) << "Reading basic cluster attributes finished successfully" << reply->responseFrame().header.command;
            QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
            foreach (const ZigbeeClusterLibrary::ReadAttributeStatusRecord &attributeStatusRecord, attributeStatusRecords) {
                qCDebug(dcZigbee()) << "-->" << attributeStatusRecord;
                if (attributeStatusRecord.dataType == Zigbee::CharString)
                    qCDebug(dcZigbee()) << attributeStatusRecord.dataType.toString();
            }
        });

        info->finish(Thing::ThingErrorNoError);
    }
}

void GenericOnOffLight::readOnOffState()
{
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
        return;
    }

    ZigbeeClusterReply *reply = m_onOffCluster->readAttributes({ZigbeeClusterOnOff::AttributeOnOff});
    connect(reply, &ZigbeeClusterReply::finished, this, [this, reply](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbee()) << "Failed to read on/off cluster attribute" << reply->error();
            return;
        }

        qCDebug(dcZigbee()) << "Reading on/off cluster attribute finished successfully";
        QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
        if (attributeStatusRecords.count() != 1) {
            qCWarning(dcZigbee()) << "Could not read on/off attribute from cluster";
            return;
        }

        bool dataOk = false;
        bool powerValue = attributeStatusRecords.first().dataType.toBool(&dataOk);
        if (!dataOk) {
            qCWarning(dcZigbee()) << thing() << "Could not convert attribute data to bool" << attributeStatusRecords.first().dataType;
            return;
        }
        qCDebug(dcZigbee()) << thing() << "power state" << powerValue;
        thing()->setStateValue(genericOnOffLightPowerStateTypeId, powerValue);
    });
}

void GenericOnOffLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

