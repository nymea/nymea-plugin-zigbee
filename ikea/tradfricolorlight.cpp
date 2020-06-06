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

#include <zigbeeutils.h>

#include "tradfricolorlight.h"
#include "extern-plugininfo.h"

#include <QDataStream>

TradfriColorLight::TradfriColorLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoint, m_node->endpoints()) {
        if ((endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceColourLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceExtendedColourLight)) {
            m_endpoint = endpoint;
            break;
        }
    }

    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    m_onOffCluster = m_endpoint->inputCluster<ZigbeeClusterOnOff>(Zigbee::ClusterIdOnOff);
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_onOffCluster, &ZigbeeClusterOnOff::powerChanged, this, [this](bool power){
            qCDebug(dcZigbee()) << m_thing << "power state changed" << power;
            m_thing->setStateValue(tradfriColorLightPowerStateTypeId, power);
        });
    }

    m_levelControlCluster = m_endpoint->inputCluster<ZigbeeClusterLevelControl>(Zigbee::ClusterIdLevelControl);
    if (!m_levelControlCluster) {
        qCWarning(dcZigbee()) << "Could not find the LevelControl input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_levelControlCluster, &ZigbeeClusterLevelControl::currentLevelChanged, this, [this](quint8 level){
            int percentage = qRound(level * 100.0 / 255.0);
            qCDebug(dcZigbee()) << m_thing << "level state changed" << level << percentage << "%";
            m_thing->setStateValue(tradfriColorLightBrightnessStateTypeId, percentage);
        });
    }


    m_identifyCluster = m_endpoint->inputCluster<ZigbeeClusterIdentify>(Zigbee::ClusterIdIdentify);
    if (!m_identifyCluster) {
        qCWarning(dcZigbee()) << "Could not find the identify input cluster on" << m_thing << m_endpoint;
    }

    m_colorCluster = m_endpoint->inputCluster<ZigbeeClusterColorControl>(Zigbee::ClusterIdColorControl);
    if (!m_identifyCluster) {
        qCWarning(dcZigbee()) << "Could not find the color control input cluster on" << m_thing << m_endpoint;
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriColorLight::onNetworkStateChanged);
}

void TradfriColorLight::removeFromNetwork()
{
    m_network->removeZigbeeNode(m_node->extendedAddress());
}

void TradfriColorLight::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(tradfriColorLightConnectedStateTypeId, true);
        thing()->setStateValue(tradfriColorLightVersionStateTypeId, m_endpoint->softwareBuildId());
//        readOnOffState();
//        readLevelValue();
//        readColorXy();
    } else {
        thing()->setStateValue(tradfriColorLightConnectedStateTypeId, false);
    }
}

void TradfriColorLight::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == tradfriColorLightAlertActionTypeId) {
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
    } else if (info->action().actionTypeId() == tradfriColorLightPowerActionTypeId) {
        if (!m_onOffCluster) {
            qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        bool power = info->action().param(tradfriColorLightPowerActionPowerParamTypeId).value().toBool();
        ZigbeeClusterReply *reply = (power ? m_onOffCluster->commandOn() : m_onOffCluster->commandOff());
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, power](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                thing()->setStateValue(tradfriColorLightPowerStateTypeId, power);
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorLightBrightnessActionTypeId) {
        int brightness = info->action().param(tradfriColorLightBrightnessActionBrightnessParamTypeId).value().toInt();
        quint8 level = static_cast<quint8>(qRound(255.0 * brightness / 100.0));
        if (!m_levelControlCluster) {
            qCWarning(dcZigbee()) << "Could not find the LevelControl input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        ZigbeeClusterReply *reply = m_levelControlCluster->commandMoveToLevelWithOnOff(level);
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, brightness](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                thing()->setStateValue(tradfriColorLightPowerStateTypeId, (brightness > 0 ? true : false));
                thing()->setStateValue(tradfriColorLightBrightnessStateTypeId, brightness);
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorLightColorTemperatureActionTypeId) {
        int colorTemperature = info->action().param(tradfriColorLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
        // Note: the color temperature command/attribute is not supported. It does support only xy, so we have to interpolate the colors
        int minValue = thing()->thingClass().getStateType(tradfriColorLightColorTemperatureStateTypeId).minValue().toInt();
        int maxValue = thing()->thingClass().getStateType(tradfriColorLightColorTemperatureStateTypeId).maxValue().toInt();
        QColor temperatureColor = ZigbeeUtils::interpolateColorFromColorTemperature(colorTemperature, minValue, maxValue);
        QPointF temperatureColorXy = ZigbeeUtils::convertColorToXY(temperatureColor);
        ZigbeeClusterReply *reply = m_colorCluster->commandMoveToColor(temperatureColorXy.x(), temperatureColorXy.y(), 5);
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, colorTemperature](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                thing()->setStateValue(tradfriColorLightColorTemperatureStateTypeId, colorTemperature);
                info->finish(Thing::ThingErrorNoError);
                readColorXy();
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorLightColorActionTypeId) {
        QPointF xyColor = ZigbeeUtils::convertColorToXY(info->action().param(tradfriColorLightColorActionColorParamTypeId).value().value<QColor>());
        // Note: time unit is 1/10 s
        ZigbeeClusterReply *reply = m_colorCluster->commandMoveToColor(xyColor.x(), xyColor.y(), 5);
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                readColorXy();
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorLightRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void TradfriColorLight::readColorCapabilities()
{
//    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
//        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
//            // Note: set the color once both attribute read
//            m_colorAttributesArrived = 0;
//            m_endpoint->readAttribute(cluster, { ZigbeeCluster::ColorControlClusterAttributeColorMode,
//                                                 ZigbeeCluster::ColorControlClusterAttributeColorCapabilities });
//        }
//    }
}

void TradfriColorLight::readOnOffState()
{
//    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
//        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
//            m_endpoint->readAttribute(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
//        }
//    }
}

void TradfriColorLight::readLevelValue()
{
//    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
//        if (cluster->clusterId() == Zigbee::ClusterIdLevelControl) {
//            m_endpoint->readAttribute(cluster, { ZigbeeCluster::LevelClusterAttributeCurrentLevel });
//        }
//    }
}

void TradfriColorLight::readColorXy()
{
//    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
//        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
//            // Note: set the color once both attribute read
//            m_colorAttributesArrived = 0;
//            m_endpoint->readAttribute(cluster, { ZigbeeCluster::ColorControlClusterAttributeCurrentX,
//                                                 ZigbeeCluster::ColorControlClusterAttributeCurrentY });
//        }
//    }
}

void TradfriColorLight::configureReporting()
{
    //    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
    //        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
    //            m_endpoint->configureReporting(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
    //        }

    //        if (cluster->clusterId() == Zigbee::ClusterIdLevelControl) {
    //            m_endpoint->configureReporting(cluster, { ZigbeeCluster::LevelClusterAttributeCurrentLevel });
    //        }

    //        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
    //            m_endpoint->configureReporting(cluster, { ZigbeeCluster::ColorControlClusterAttributeColorMode,
    //                                                      ZigbeeCluster::ColorControlClusterAttributeColorTemperatureMireds });
    //        }
    //    }
}

void TradfriColorLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

//void TradfriColorLight::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
//{
//    qCDebug(dcZigbee()) << thing() << "cluster attribute changed" << cluster << attribute;

//    if (cluster->clusterId() == Zigbee::ClusterIdOnOff && attribute.id() == ZigbeeCluster::OnOffClusterAttributeOnOff) {
//        bool power = static_cast<bool>(attribute.data().at(0));
//        thing()->setStateValue(tradfriColorLightPowerStateTypeId, power);
//    } else if (cluster->clusterId() == Zigbee::ClusterIdLevelControl && attribute.id() == ZigbeeCluster::LevelClusterAttributeCurrentLevel) {
//        quint8 currentLevel = static_cast<quint8>(attribute.data().at(0));
//        thing()->setStateValue(tradfriColorLightBrightnessStateTypeId, qRound(currentLevel * 100.0 / 255.0));
//    } else if (cluster->clusterId() == Zigbee::ClusterIdColorControl && attribute.id() == ZigbeeCluster::ColorControlClusterAttributeCurrentX) {
//        QByteArray data = attribute.data();
//        QDataStream stream(&data, QIODevice::ReadOnly);
//        stream >> m_currentX;
//        m_colorAttributesArrived++;
//        if (m_colorAttributesArrived >= 2) {
//            m_colorAttributesArrived = 0;
//            // Color x and y read. Calculate color and update state
//            QColor color = ZigbeeUtils::convertXYToColor(m_currentX, m_currentY);
//            thing()->setStateValue(tradfriColorLightColorStateTypeId, color);
//        }
//    } else if (cluster->clusterId() == Zigbee::ClusterIdColorControl && attribute.id() == ZigbeeCluster::ColorControlClusterAttributeCurrentY) {
//        QByteArray data = attribute.data();
//        QDataStream stream(&data, QIODevice::ReadOnly);
//        stream >> m_currentY;
//        m_colorAttributesArrived++;
//        if (m_colorAttributesArrived >= 2) {
//            m_colorAttributesArrived = 0;
//            // Color x and y read. Calculate color and update state
//            QColor color = ZigbeeUtils::convertXYToColor(m_currentX, m_currentY);
//            thing()->setStateValue(tradfriColorLightColorStateTypeId, color);
//        }
//    }
//}
