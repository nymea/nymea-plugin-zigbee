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

    qCDebug(dcZigbee()) << m_thing << m_endpoint;
    qCDebug(dcZigbee()) << "Input clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    qCDebug(dcZigbee()) << "Output clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    // Get the ZigbeeClusterOnOff
    m_clusterOnOff = m_endpoint->inputCluster<ZigbeeClusterOnOff>(Zigbee::ClusterIdOnOff);
    if (!m_clusterOnOff) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_clusterOnOff, &ZigbeeClusterOnOff::attributeChanged, this, &GenericOnOffLight::onOnOffClusterAttributeChanged);
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
    m_node->deviceObject()->requestMgmtLeaveNetwork();
}

void GenericOnOffLight::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == genericOnOffLightIdentifyActionTypeId) {
        //        ZigbeeNetworkReply *reply = m_endpoint->identify(2);
        //        connect(reply, &ZigbeeNetworkReply::finished, this, [reply, info](){
        //            // Note: reply will be deleted automatically
        //            if (reply->error() != ZigbeeNetworkReply::ErrorNoError) {
        //                info->finish(Thing::ThingErrorHardwareFailure);
        //            } else {
        //                info->finish(Thing::ThingErrorNoError);
        //            }
        //        });
    } else if (info->action().actionTypeId() == genericOnOffLightPowerActionTypeId) {
        if (!m_clusterOnOff) {
            qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        bool power = info->action().param(genericOnOffLightPowerActionPowerParamTypeId).value().toBool();
        ZigbeeClusterReply *reply = (power ? m_clusterOnOff->commandOn() : m_clusterOnOff->commandOff());
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
        ZigbeeClusterBasic *basicCluster = m_endpoint->inputCluster<ZigbeeClusterBasic>(Zigbee::ClusterIdBasic);
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
    if (!m_clusterOnOff) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
        return;
    }

    ZigbeeClusterReply *reply = m_clusterOnOff->readAttributes({ZigbeeClusterOnOff::AttributeOnOff});
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

void GenericOnOffLight::onOnOffClusterAttributeChanged(const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << m_thing << "on/off cluster attribute changed" << attribute;
    if (attribute.id() == ZigbeeClusterOnOff::AttributeOnOff) {
        bool valueOk = false;
        bool powerValue = attribute.dataType().toBool(&valueOk);
        if (!valueOk) {
            qCWarning(dcZigbee()) << "Failed to convert data type to bool" << m_thing << attribute.dataType();
            return;
        }
        qCDebug(dcZigbee()) << "Power changed for" << m_thing << powerValue;
        m_thing->setStateValue(genericOnOffLightPowerStateTypeId, powerValue);
    }
}
