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

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &GenericOnOffLight::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &GenericOnOffLight::onEndpointClusterAttributeChanged);
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
    m_node->leaveNetworkRequest();
}

void GenericOnOffLight::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == genericOnOffLightIdentifyActionTypeId) {
        ZigbeeNetworkReply *reply = m_endpoint->identify(2);
        connect(reply, &ZigbeeNetworkReply::finished, this, [reply, info](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeNetworkReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
            }
        });
    } else if (info->action().actionTypeId() == genericOnOffLightPowerActionTypeId) {
        bool power = info->action().param(genericOnOffLightPowerActionPowerParamTypeId).value().toBool();
        m_endpoint->sendOnOffClusterCommand(power ? ZigbeeCluster::OnOffClusterCommandOn : ZigbeeCluster::OnOffClusterCommandOff);
        ZigbeeNetworkReply *reply = m_endpoint->factoryReset();
        connect(reply, &ZigbeeNetworkReply::finished, this, [this, reply, info](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeNetworkReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                readOnOffState();
            }
        });
    } else if (info->action().actionTypeId() == genericOnOffLightRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void GenericOnOffLight::readOnOffState()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
        }
    }
}

void GenericOnOffLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void GenericOnOffLight::onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << thing() << "cluster attribute changed" << cluster << attribute;

    if (cluster->clusterId() == Zigbee::ClusterIdOnOff && attribute.id() == ZigbeeCluster::OnOffClusterAttributeOnOff) {
        if (attribute.dataType() != Zigbee::DataType::Bool || attribute.data().count() == 0) {
            qCWarning(dcZigbee()) << "Unexpected data type for attribute changed signal" << thing() << cluster << attribute;
            return;
        }

        bool power = static_cast<bool>(attribute.data().at(0));
        thing()->setStateValue(genericOnOffLightPowerStateTypeId, power);
    }
}
