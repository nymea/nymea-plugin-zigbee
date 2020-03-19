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

#include "genericcolorlight.h"
#include "extern-plugininfo.h"

#include <zigbeeutils.h>

GenericColorLight::GenericColorLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoint, m_node->endpoints()) {
        if ((endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceColourLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceExtendedColourLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceExtendedColourLight)) {
            m_endpoint = endpoint;
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

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &GenericColorLight::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &GenericColorLight::onClusterAttributeChanged);
}

void GenericColorLight::identify()
{
    m_endpoint->identify(1);
}

void GenericColorLight::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void GenericColorLight::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(genericColorLightConnectedStateTypeId, true);
        thing()->setStateValue(genericColorLightVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        thing()->setStateValue(genericColorLightConnectedStateTypeId, false);
    }
}

void GenericColorLight::setPower(bool power)
{
    qCDebug(dcZigbee()) << m_thing << "set power" << power;
    m_endpoint->sendOnOffClusterCommand(power ? ZigbeeCluster::OnOffClusterCommandOn : ZigbeeCluster::OnOffClusterCommandOff);
    thing()->setStateValue(genericColorLightPowerStateTypeId, power);
    readOnOffState();
}

void GenericColorLight::setBrightness(int brightness)
{
    if (brightness > 100)
        brightness = 100;

    if (brightness < 0)
        brightness = 0;

    quint8 level = static_cast<quint8>(qRound(255.0 * brightness / 100.0));
    // Note: time unit is 1/10 s
    m_endpoint->sendLevelCommand(ZigbeeCluster::LevelClusterCommandMoveToLevel, level, true, 5);
    thing()->setStateValue(genericColorLightBrightnessStateTypeId, brightness);
    // Note: due to triggersOnOff is true
    thing()->setStateValue(genericColorLightPowerStateTypeId, (level > 0));
}

void GenericColorLight::setColorTemperature(int colorTemperature)
{
    // FIXME: check capabilities
    int minValue = thing()->thingClass().getStateType(genericColorLightColorTemperatureStateTypeId).minValue().toInt();
    int maxValue = thing()->thingClass().getStateType(genericColorLightColorTemperatureStateTypeId).maxValue().toInt();
    QColor temperatureColor = ZigbeeUtils::interpolateColorFromColorTemperature(colorTemperature, minValue, maxValue);
    QPointF temperatureColorXy = ZigbeeUtils::convertColorToXY(temperatureColor);
    m_endpoint->sendMoveToColor(temperatureColorXy.x(), temperatureColorXy.y(), 5);
    thing()->setStateValue(genericColorLightColorTemperatureStateTypeId, colorTemperature);
    readColorXy();
}

void GenericColorLight::setColor(const QColor &color)
{
    QPointF xyColor = ZigbeeUtils::convertColorToXY(color);
    // Note: time unit is 1/10 s
    m_endpoint->sendMoveToColor(xyColor.x(), xyColor.y(), 5);
    readColorXy();
}

void GenericColorLight::readColorCapabilities()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
            // Note: set the color once both attribute read
            m_colorAttributesArrived = 0;
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::ColorControlClusterAttributeColorMode,
                                                 ZigbeeCluster::ColorControlClusterAttributeColorCapabilities });
        }
    }
}

void GenericColorLight::readOnOffState()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
        }
    }
}

void GenericColorLight::readLevelValue()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdLevelControl) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::LevelClusterAttributeCurrentLevel });
        }
    }
}

void GenericColorLight::readColorXy()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
            // Note: set the color once both attribute read
            m_colorAttributesArrived = 0;
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::ColorControlClusterAttributeCurrentX,
                                                 ZigbeeCluster::ColorControlClusterAttributeCurrentY });
        }
    }
}

void GenericColorLight::configureReporting()
{

}

void GenericColorLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    checkOnlineStatus();
    if (state == ZigbeeNetwork::StateRunning) {
        readOnOffState();
        readLevelValue();
        readColorXy();
    }
}

void GenericColorLight::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << thing() << "cluster attribute changed" << cluster << attribute;

    if (cluster->clusterId() == Zigbee::ClusterIdOnOff && attribute.id() == ZigbeeCluster::OnOffClusterAttributeOnOff) {
        bool power = static_cast<bool>(attribute.data().at(0));
        thing()->setStateValue(genericColorLightPowerStateTypeId, power);
    } else if (cluster->clusterId() == Zigbee::ClusterIdLevelControl && attribute.id() == ZigbeeCluster::LevelClusterAttributeCurrentLevel) {
        quint8 currentLevel = static_cast<quint8>(attribute.data().at(0));
        thing()->setStateValue(genericColorLightBrightnessStateTypeId, qRound(currentLevel * 100.0 / 255.0));
    } else if (cluster->clusterId() == Zigbee::ClusterIdColorControl && attribute.id() == ZigbeeCluster::ColorControlClusterAttributeCurrentX) {
        QByteArray data = attribute.data();
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream >> m_currentX;
        m_colorAttributesArrived++;
        if (m_colorAttributesArrived >= 2) {
            m_colorAttributesArrived = 0;
            // Color x and y read. Calculate color and update state
            QColor color = ZigbeeUtils::convertXYToColor(m_currentX, m_currentY);
            thing()->setStateValue(genericColorLightColorStateTypeId, color);
        }
    } else if (cluster->clusterId() == Zigbee::ClusterIdColorControl && attribute.id() == ZigbeeCluster::ColorControlClusterAttributeCurrentY) {
        QByteArray data = attribute.data();
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream >> m_currentY;
        m_colorAttributesArrived++;
        if (m_colorAttributesArrived >= 2) {
            m_colorAttributesArrived = 0;
            // Color x and y read. Calculate color and update state
            QColor color = ZigbeeUtils::convertXYToColor(m_currentX, m_currentY);
            thing()->setStateValue(genericColorLightColorStateTypeId, color);
        }
    }
}
