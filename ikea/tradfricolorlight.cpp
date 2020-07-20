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

#include "tradfricolorlight.h"
#include "extern-plugininfo.h"

#include <QDataStream>
#include <zigbeeutils.h>

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

    // Update signal strength
    connect(m_node, &ZigbeeNode::lqiChanged, this, [this](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbee()) << m_thing << "signal strength changed" << signalStrength << "%";
        m_thing->setStateValue(tradfriColorLightSignalStrengthStateTypeId, signalStrength);
    });

    m_onOffCluster = m_endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_onOffCluster, &ZigbeeClusterOnOff::powerChanged, this, [this](bool power){
            qCDebug(dcZigbee()) << m_thing << "power state changed" << power;
            m_thing->setStateValue(tradfriColorLightPowerStateTypeId, power);
        });
    }

    m_levelControlCluster = m_endpoint->inputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
    if (!m_levelControlCluster) {
        qCWarning(dcZigbee()) << "Could not find the LevelControl input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_levelControlCluster, &ZigbeeClusterLevelControl::currentLevelChanged, this, [this](quint8 level){
            int percentage = qRound(level * 100.0 / 255.0);
            qCDebug(dcZigbee()) << m_thing << "level state changed" << level << percentage << "%";
            m_thing->setStateValue(tradfriColorLightBrightnessStateTypeId, percentage);
        });
    }

    m_identifyCluster = m_endpoint->inputCluster<ZigbeeClusterIdentify>(ZigbeeClusterLibrary::ClusterIdIdentify);
    if (!m_identifyCluster) {
        qCWarning(dcZigbee()) << "Could not find the identify input cluster on" << m_thing << m_endpoint;
    }

    m_colorCluster = m_endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
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
        m_thing->setStateValue(tradfriColorLightConnectedStateTypeId, true);
        m_thing->setStateValue(tradfriColorLightVersionStateTypeId, m_endpoint->softwareBuildId());
        m_thing->setStateValue(tradfriColorLightSignalStrengthStateTypeId, qRound(m_node->lqi() * 100.0 / 255.0));
        readStates();
    } else {
        m_thing->setStateValue(tradfriColorLightConnectedStateTypeId, false);
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
                m_thing->setStateValue(tradfriColorLightPowerStateTypeId, power);
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

        ZigbeeClusterReply *reply = m_levelControlCluster->commandMoveToLevelWithOnOff(level, 2);
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, brightness](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                qCDebug(dcZigbee()) << "Set brightness" << brightness << "% finished successfully";
                m_thing->setStateValue(tradfriColorLightPowerStateTypeId, (brightness > 0 ? true : false));
                m_thing->setStateValue(tradfriColorLightBrightnessStateTypeId, brightness);
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorLightColorTemperatureActionTypeId) {
        int colorTemperature = info->action().param(tradfriColorLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
        // Note: the color temperature command/attribute is not supported. It does support only xy, so we have to interpolate the colors
        int minValue = m_thing->thingClass().getStateType(tradfriColorLightColorTemperatureStateTypeId).minValue().toInt();
        int maxValue = m_thing->thingClass().getStateType(tradfriColorLightColorTemperatureStateTypeId).maxValue().toInt();
        QColor temperatureColor = ZigbeeUtils::interpolateColorFromColorTemperature(colorTemperature, minValue, maxValue);
        QPoint temperatureColorXyInt = ZigbeeUtils::convertColorToXYInt(temperatureColor);
        ZigbeeClusterReply *reply = m_colorCluster->commandMoveToColor(temperatureColorXyInt.x(), temperatureColorXyInt.y(), 2);
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, colorTemperature](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                qCDebug(dcZigbee()) << "Set color temperature" << colorTemperature << "mired finished successfully";
                m_thing->setStateValue(tradfriColorLightColorTemperatureStateTypeId, colorTemperature);
                info->finish(Thing::ThingErrorNoError);
                //readColorXy();
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorLightColorActionTypeId) {
        QColor color = info->action().param(tradfriColorLightColorActionColorParamTypeId).value().value<QColor>();
        QPoint xyColorInt = ZigbeeUtils::convertColorToXYInt(color);
        qCDebug(dcZigbee()) << "Set color" << color.toRgb() << xyColorInt;
        // Note: time unit is 1/10 s
        ZigbeeClusterReply *reply = m_colorCluster->commandMoveToColor(xyColorInt.x(), xyColorInt.y(), 2);
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, color, xyColorInt](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                qCDebug(dcZigbee()) << "Set color" << color.toRgb() << xyColorInt << "finished successfully";
                info->finish(Thing::ThingErrorNoError);
                readColorXy();
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorLightRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }  else if (info->action().actionTypeId() == tradfriColorLightTestActionTypeId) {
        if (!m_colorCluster) {
            qCWarning(dcZigbee()) << "Could not find color cluster on" << m_thing;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        readColorCapabilities();
        info->finish(Thing::ThingErrorNoError);
    }
}

void TradfriColorLight::readStates()
{
    // Start reading sequentially, on/off -> level -> color
    readOnOffState();
}

void TradfriColorLight::readColorCapabilities()
{
    if (!m_colorCluster) {
        qCWarning(dcZigbee()) << "Cannot read color capabilities. Could not find color cluster on" << m_thing;
        return;
    }

    // Read the color capabilities
    if (m_colorCluster->hasAttribute(ZigbeeClusterColorControl::AttributeColorCapabilities)) {
        ZigbeeClusterAttribute colorCapabilitiesAttribute = m_colorCluster->attribute(ZigbeeClusterColorControl::AttributeColorCapabilities);
        bool valueOk = false;
        quint16 colorCapabilitiesValue = colorCapabilitiesAttribute.dataType().toUInt16(&valueOk);
        if (!valueOk) {
            qCWarning(dcZigbee()) << "Failed to read color capabilities attribute value and convert it" << colorCapabilitiesAttribute;
            return;
        }
        ZigbeeClusterColorControl::ColorCapabilities colorCapabilities = static_cast<ZigbeeClusterColorControl::ColorCapabilities>(colorCapabilitiesValue);
        qCDebug(dcZigbee()) << "Cached color capabilities" << colorCapabilities;
    } else {
        // We have to read the capabilities
        ZigbeeClusterReply *reply = m_colorCluster->readAttributes({ZigbeeClusterColorControl::AttributeColorCapabilities});
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                return;
            }

            ZigbeeClusterAttribute colorCapabilitiesAttribute = m_colorCluster->attribute(ZigbeeClusterColorControl::AttributeColorCapabilities);
            bool valueOk = false;
            quint16 colorCapabilitiesValue = colorCapabilitiesAttribute.dataType().toUInt16(&valueOk);
            if (!valueOk) {
                qCWarning(dcZigbee()) << "Failed to read color capabilities attribute value and convert it" << colorCapabilitiesAttribute;
                return;
            }
            ZigbeeClusterColorControl::ColorCapabilities colorCapabilities = static_cast<ZigbeeClusterColorControl::ColorCapabilities>(colorCapabilitiesValue);
            qCDebug(dcZigbee()) << "Reading color capabilities finished successfully" << colorCapabilities;
        });
    }
}

void TradfriColorLight::readOnOffState()
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

        // Note: the attribute gets updated internally and the state gets updated with the powerChanged signal
        qCDebug(dcZigbee()) << "Reading on/off cluster attribute finished successfully";
        readLevelValue();
    });
}

void TradfriColorLight::readLevelValue()
{
    if (!m_levelControlCluster) {
        qCWarning(dcZigbee()) << "Could not find the LevelControl input cluster on" << m_thing << m_endpoint;
        return;
    }

    ZigbeeClusterReply *reply = m_levelControlCluster->readAttributes({ZigbeeClusterLevelControl::AttributeCurrentLevel});
    connect(reply, &ZigbeeClusterReply::finished, this, [this, reply](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbee()) << "Failed to read LevelControl cluster attribute" << reply->error();
            return;
        }

        // Note: the attribute gets updated internally and the state gets updated with the currentLevelChanged signal
        qCDebug(dcZigbee()) << "Reading LevelControl cluster attribute finished successfully";
        readColorXy();
    });
}

void TradfriColorLight::readColorXy()
{
    qCDebug(dcZigbee()) << "Read current color values" << m_thing;
    if (!m_colorCluster) {
        qCWarning(dcZigbee()) << "Could not find the ColorControl input cluster on" << m_thing << m_endpoint;
        return;
    }

    ZigbeeClusterReply *reply = m_colorCluster->readAttributes({ZigbeeClusterColorControl::AttributeCurrentX, ZigbeeClusterColorControl::AttributeCurrentY});
    connect(reply, &ZigbeeClusterReply::finished, this, [this, reply](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbee()) << "Failed to read ColorControl cluster attribute" << reply->error();
            return;
        }

        // Note: the attribute gets updated internally and the state gets updated with the currentLevelChanged signal
        qCDebug(dcZigbee()) << "Reading ColorControl cluster attribute color x and y finished successfully";

        QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
        if (attributeStatusRecords.count() != 2) {
            qCWarning(dcZigbee()) << "Did not receive color x and y attribute values from" << m_thing;
            return;
        }

        // Parse the attribute status records and calculate the color
        quint16 currentX = 0; quint16 currentY = 0;
        foreach (const ZigbeeClusterLibrary::ReadAttributeStatusRecord &attributeStatusRecord, attributeStatusRecords) {
            qCDebug(dcZigbee()) << "Received read attribute status record" << m_thing << attributeStatusRecord;
            if (attributeStatusRecord.attributeId == ZigbeeClusterColorControl::AttributeCurrentX) {
                bool valueOk = false;
                currentX = attributeStatusRecord.dataType.toUInt16(&valueOk);
                if (!valueOk) {
                    qCWarning(dcZigbee()) << "Failed to convert color x attribute values from" << m_thing << attributeStatusRecord;
                    return;
                }
                continue;
            }

            if (attributeStatusRecord.attributeId == ZigbeeClusterColorControl::AttributeCurrentY) {
                bool valueOk = false;
                currentY = attributeStatusRecord.dataType.toUInt16(&valueOk);
                if (!valueOk) {
                    qCWarning(dcZigbee()) << "Failed to convert color y attribute values from" << m_thing << attributeStatusRecord;
                    return;
                }
                continue;
            }
        }

        // Set the current color
        QColor color = ZigbeeUtils::convertXYToColor(currentX, currentY);
        qCDebug(dcZigbee()) << "Current color" << color.toRgb() << QPoint(currentX, currentY);
        m_thing->setStateValue(tradfriColorLightColorStateTypeId, color);
    });
}

void TradfriColorLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}
