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

#include "tradfricolortemperaturelight.h"
#include "extern-plugininfo.h"

#include <zigbeeutils.h>

TradfriColorTemperatureLight::TradfriColorTemperatureLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoit, m_node->endpoints()) {
        if (endpoit->deviceId() == Zigbee::HomeAutomationDeviceColourTemperatureLight) {
            m_endpoint = endpoit;
            break;
        }
    }

    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    qCDebug(dcZigbee()) << m_thing << m_endpoint;
    qCDebug(dcZigbee()) << "Input clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
        foreach(const ZigbeeClusterAttribute &attribute, cluster->attributes()) {
            qCDebug(dcZigbee()) << "   - " << attribute;
        }
    }

    qCDebug(dcZigbee()) << "Output clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
        foreach(const ZigbeeClusterAttribute &attribute, cluster->attributes()) {
            qCDebug(dcZigbee()) << "   - " << attribute;
        }
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriColorTemperatureLight::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &TradfriColorTemperatureLight::onClusterAttributeChanged);
}

void TradfriColorTemperatureLight::identify()
{
    m_endpoint->identify(1);
}

void TradfriColorTemperatureLight::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void TradfriColorTemperatureLight::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(tradfriColorTemperatureLightConnectedStateTypeId, true);
        thing()->setStateValue(tradfriColorTemperatureLightVersionStateTypeId, m_endpoint->softwareBuildId());
        readOnOffState();
        readLevelValue();
        readColorTemperature();
    } else {
        thing()->setStateValue(tradfriColorTemperatureLightConnectedStateTypeId, false);
    }
}

void TradfriColorTemperatureLight::setPower(bool power)
{
    qCDebug(dcZigbee()) << m_thing << "set power" << power;
    m_endpoint->sendOnOffClusterCommand(power ? ZigbeeCluster::OnOffClusterCommandOn : ZigbeeCluster::OnOffClusterCommandOff);
    thing()->setStateValue(tradfriColorTemperatureLightPowerStateTypeId, power);
    readOnOffState();
}

void TradfriColorTemperatureLight::setBrightness(int brightness)
{
    if (brightness > 100)
        brightness = 100;

    if (brightness < 0)
        brightness = 0;

    quint8 level = static_cast<quint8>(qRound(255.0 * brightness / 100.0));
    // Note: time unit is 1/10 s
    m_endpoint->sendLevelCommand(ZigbeeCluster::LevelClusterCommandMoveToLevel, level, true, 5);
    thing()->setStateValue(tradfriColorTemperatureLightBrightnessStateTypeId, brightness);
    // Note: due to triggersOnOff is true
    thing()->setStateValue(tradfriColorTemperatureLightPowerStateTypeId, (level > 0));
}

void TradfriColorTemperatureLight::setColorTemperature(int colorTemperature)
{
    // Note: time unit is 1/10 s
    m_endpoint->sendMoveToColorTemperature(static_cast<quint16>(colorTemperature), 5);
    thing()->setStateValue(tradfriColorTemperatureLightColorTemperatureStateTypeId, colorTemperature);
}

void TradfriColorTemperatureLight::readOnOffState()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
        }
    }
}

void TradfriColorTemperatureLight::readLevelValue()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdLevelControl) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::LevelClusterAttributeCurrentLevel });
        }
    }
}

void TradfriColorTemperatureLight::readColorTemperature()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::ColorControlClusterAttributeColorTemperatureMireds });
        }
    }
}

void TradfriColorTemperatureLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    checkOnlineStatus();
    if (state == ZigbeeNetwork::StateRunning) {
        readOnOffState();
        readLevelValue();
    }
}

void TradfriColorTemperatureLight::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << thing() << "cluster attribute changed" << cluster << attribute;

    if (cluster->clusterId() == Zigbee::ClusterIdOnOff && attribute.id() == ZigbeeCluster::OnOffClusterAttributeOnOff) {
        bool power = static_cast<bool>(attribute.data().at(0));
        thing()->setStateValue(tradfriColorTemperatureLightPowerStateTypeId, power);
    } else if (cluster->clusterId() == Zigbee::ClusterIdLevelControl && attribute.id() == ZigbeeCluster::LevelClusterAttributeCurrentLevel) {
        quint8 currentLevel = static_cast<quint8>(attribute.data().at(0));
        thing()->setStateValue(tradfriColorTemperatureLightBrightnessStateTypeId, qRound(currentLevel * 100.0 / 255.0));
    } else if (cluster->clusterId() == Zigbee::ClusterIdColorControl && attribute.id() == ZigbeeCluster::ColorControlClusterAttributeColorTemperatureMireds) {
        quint16 colorTemperature = 0;
        QByteArray data = attribute.data();
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream >> colorTemperature;
        qCDebug(dcZigbee()) << thing() << "current color temperature mired" << colorTemperature;
        thing()->setStateValue(tradfriColorTemperatureLightColorTemperatureStateTypeId, colorTemperature);
    }
}