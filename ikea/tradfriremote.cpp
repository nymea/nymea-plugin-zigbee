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

#include "tradfriremote.h"
#include "extern-plugininfo.h"

TradfriRemote::TradfriRemote(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoint, m_node->endpoints()) {
        if (endpoint->deviceId() == Zigbee::LightLinkDeviceNonColourSceneController) {
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

    configureReporting();


    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriRemote::onNetworkStateChanged);
}

void TradfriRemote::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void TradfriRemote::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(tradfriRemoteConnectedStateTypeId, true);
        thing()->setStateValue(tradfriRemoteVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        thing()->setStateValue(tradfriRemoteConnectedStateTypeId, false);
    }
}

void TradfriRemote::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == tradfriRemoteRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void TradfriRemote::readAttribute()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdBasic) {
            m_endpoint->readAttribute(cluster, {0x0004, 0x0005});
        }
    }
}

void TradfriRemote::configureReporting()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            qCWarning(dcZigbee()) << "===============";
            ZigbeeClusterReportConfigurationRecord reportConfiguration;
            reportConfiguration.direction = 0x00;
            reportConfiguration.dataType = Zigbee::Bool;
            reportConfiguration.attributeId = 0x0000; // OnOff
            reportConfiguration.minInterval = 0x0000;
            reportConfiguration.maxInterval = 0x0001;
            reportConfiguration.timeout = 0x0001;
            reportConfiguration.change = 0x02;

            m_endpoint->configureReporting(cluster, { reportConfiguration });
        }
    }
}

void TradfriRemote::addGroup()
{
}


void TradfriRemote::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}
