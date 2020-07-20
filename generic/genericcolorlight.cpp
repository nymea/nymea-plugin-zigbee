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

#include <QDataStream>

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

    // Update signal strength
    connect(m_node, &ZigbeeNode::lqiChanged, this, [this](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbee()) << m_thing << "signal strength changed" << signalStrength << "%";
        m_thing->setStateValue(genericColorLightSignalStrengthStateTypeId, signalStrength);
    });

    m_onOffCluster = m_endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_onOffCluster, &ZigbeeClusterOnOff::powerChanged, this, [this](bool power){
            qCDebug(dcZigbee()) << m_thing << "power state changed" << power;
            m_thing->setStateValue(genericColorLightPowerStateTypeId, power);
        });
    }

    m_levelControlCluster = m_endpoint->inputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
    if (!m_levelControlCluster) {
        qCWarning(dcZigbee()) << "Could not find the LevelControl input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_levelControlCluster, &ZigbeeClusterLevelControl::currentLevelChanged, this, [this](quint8 level){
            int percentage = qRound(level * 100.0 / 255.0);
            qCDebug(dcZigbee()) << m_thing << "level state changed" << level << percentage << "%";
            m_thing->setStateValue(genericColorLightBrightnessStateTypeId, percentage);
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

    // Read all required information
    readColorCapabilities();
    connect(m_network, &ZigbeeNetwork::stateChanged, this, &GenericColorLight::onNetworkStateChanged);
}

void GenericColorLight::removeFromNetwork()
{
    m_network->removeZigbeeNode(m_node->extendedAddress());
}

void GenericColorLight::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        m_thing->setStateValue(genericColorLightConnectedStateTypeId, true);
        m_thing->setStateValue(genericColorLightVersionStateTypeId, m_endpoint->softwareBuildId());
        m_thing->setStateValue(genericColorLightSignalStrengthStateTypeId, qRound(m_node->lqi() * 100.0 / 255.0));
        readStates();
    } else {
        m_thing->setStateValue(genericColorLightConnectedStateTypeId, false);
    }
}

void GenericColorLight::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == genericColorLightIdentifyActionTypeId) {
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
    } else if (info->action().actionTypeId() == genericColorLightPowerActionTypeId) {
        if (!m_onOffCluster) {
            qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        bool power = info->action().param(genericColorLightPowerActionPowerParamTypeId).value().toBool();
        ZigbeeClusterReply *reply = (power ? m_onOffCluster->commandOn() : m_onOffCluster->commandOff());
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, power](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                m_thing->setStateValue(genericColorLightPowerStateTypeId, power);
            }
        });
    } else if (info->action().actionTypeId() == genericColorLightBrightnessActionTypeId) {
        int brightness = info->action().param(genericColorLightBrightnessActionBrightnessParamTypeId).value().toInt();
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
                m_thing->setStateValue(genericColorLightPowerStateTypeId, (brightness > 0 ? true : false));
                m_thing->setStateValue(genericColorLightBrightnessStateTypeId, brightness);
            }
        });
    } else if (info->action().actionTypeId() == genericColorLightColorTemperatureActionTypeId) {
        if (!m_colorCluster) {
            qCWarning(dcZigbee()) << "Could not find the ColorControl input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // This is the caled value we can use to interpolate
        int colorTemperatureScaled = info->action().param(genericColorLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt();

        if (m_colorCapabilities.testFlag(ZigbeeClusterColorControl::ColorCapabilityColorTemperature)) {
            // We can send the actual color temperature to the device since it supports that
            // Note: since the min/max colortemperature is device dependent, we need to map them in the range [0, 200]
            int colorTemperatureScaled = info->action().param(genericColorLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            quint16 colorTemperature = mapScaledValueToColorTemperature(colorTemperatureScaled);
            qCDebug(dcZigbee()) << "Mapping action value" << colorTemperatureScaled << "to actual color temperature in the range of [" << m_minColorTemperature << "," << m_maxColorTemperature << "] -->" << colorTemperature << "mired";
            // Note: time unit is 1/10 s
            ZigbeeClusterReply *reply = m_colorCluster->commandMoveToColorTemperature(colorTemperature, 5);
            connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, colorTemperatureScaled](){
                // Note: reply will be deleted automatically
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    info->finish(Thing::ThingErrorNoError);
                    m_thing->setStateValue(genericColorLightColorTemperatureStateTypeId, colorTemperatureScaled);
                }
            });
        } else {
            // Note: there is no color temperature capability, we have to emulate the color using default min/max values
            // Convert the scaled value into the min/max color temperature interval
            quint16 colorTemperature = mapScaledValueToColorTemperature(colorTemperatureScaled);
            qCDebug(dcZigbee()) << "Mapping action value" << colorTemperatureScaled << "to the color temperature in the range of [" << m_minColorTemperature << "," << m_maxColorTemperature << "] -->" << colorTemperature << "mired";
            // Note: the color temperature command/attribute is not supported. Using xy colors to interpolate the temperature colors
            QColor temperatureColor = ZigbeeUtils::interpolateColorFromColorTemperature(colorTemperature, m_minColorTemperature, m_maxColorTemperature);
            QPoint temperatureColorXyInt = ZigbeeUtils::convertColorToXYInt(temperatureColor);
            qCDebug(dcZigbee()) << "Mapping interpolated value" << temperatureColor << "mired to the xy color" << temperatureColorXyInt;
            ZigbeeClusterReply *reply = m_colorCluster->commandMoveToColor(temperatureColorXyInt.x(), temperatureColorXyInt.y(), 2);
            connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, colorTemperature, colorTemperatureScaled](){
                if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                    info->finish(Thing::ThingErrorHardwareFailure);
                } else {
                    qCDebug(dcZigbee()) << "Set color temperature" << colorTemperature << "mired finished successfully" << "(scalled" << colorTemperatureScaled << ")";
                    m_thing->setStateValue(genericColorLightColorTemperatureStateTypeId, colorTemperatureScaled);
                    info->finish(Thing::ThingErrorNoError);
                    //readColorXy();
                }
            });
            return;
        }
    } else if (info->action().actionTypeId() == genericColorLightColorActionTypeId) {

        // FIXME: check color capabilities (hsv or xy) and fetch the gamut triangle for getting the closest color

        QColor color = info->action().param(genericColorLightColorActionColorParamTypeId).value().value<QColor>();
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
    } else if (info->action().actionTypeId() == genericColorLightRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void GenericColorLight::readColorCapabilities()
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
        m_colorCapabilities = static_cast<ZigbeeClusterColorControl::ColorCapabilities>(colorCapabilitiesValue);
        qCDebug(dcZigbee()) << "Loaded cached color capabilities" << m_colorCapabilities;
        processColorCapabilities();
    } else {
        // We have to read the capabilities
        qCDebug(dcZigbee()) << "Reading color capabilities from node" << m_thing;
        ZigbeeClusterReply *reply = m_colorCluster->readAttributes({ZigbeeClusterColorControl::AttributeColorCapabilities});
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply](){
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                return;
            }

            // Read the attribute status record
            QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
            if (attributeStatusRecords.count() != 1) {
                qCWarning(dcZigbee()) << "Did not receive attribute status record from color capabilities attribute read request";
                return;
            }

            bool valueOk = false;
            quint16 colorCapabilitiesValue = attributeStatusRecords.first().dataType.toUInt16(&valueOk);
            if (!valueOk) {
                qCWarning(dcZigbee()) << "Failed to read color capabilities attribute value and convert it" << attributeStatusRecords.first();
                return;
            }
            m_colorCapabilities = static_cast<ZigbeeClusterColorControl::ColorCapabilities>(colorCapabilitiesValue);
            qCDebug(dcZigbee()) << "Reading color capabilities finished successfully" << m_colorCapabilities;
            processColorCapabilities();
        });
    }
}

void GenericColorLight::processColorCapabilities()
{
    // Fetch all information required depending on the capabilities
    qCDebug(dcZigbee()) << "Loading information depending on the lamp capabilities" << m_colorCapabilities;
    if (m_colorCapabilities.testFlag(ZigbeeClusterColorControl::ColorCapabilityColorTemperature)) {
        qCDebug(dcZigbee()) << "The lamp is capable of native controlling the color temperature";
        // Read min/max value, otherwise emulate the color temperature using the color map
        readColorTemperatureRange();
    } else {
        qCDebug(dcZigbee()) << "The lamp has no color temperature capabilities, emulating them using color map.";

        // TODO: continue with color fetching (xy, hsv, gamut values)

        qCDebug(dcZigbee()) << "Lamp capabilities information complete";
        m_lampInformationCompleted = true;
    }
}

void GenericColorLight::readColorTemperatureRange()
{
    qCDebug(dcZigbee()) << "Reading color temperature range from" << m_thing;
    if (!m_colorCluster) {
        qCWarning(dcZigbee()) << "Could not read color temperature value range" << m_thing << m_endpoint << "There is no color cluster available.";
        return;
    }

    // Check if we can use the cached values from the database
    if (readCachedColorTemperatureRange()) {
        qCDebug(dcZigbee()) << "Using cached color temperature mireds interval for mapping" << m_thing << "[" <<  m_minColorTemperature << "," << m_maxColorTemperature << "] mired";
        qCDebug(dcZigbee()) << "Lamp capabilities information complete";
        m_lampInformationCompleted = true;
        return;
    }

    // We need to read them from the lamp
    ZigbeeClusterReply *reply = m_colorCluster->readAttributes({ZigbeeClusterColorControl::AttributeColorTempPhysicalMinMireds, ZigbeeClusterColorControl::AttributeColorTempPhysicalMaxMireds});
    connect(reply, &ZigbeeClusterReply::finished, this, [this, reply](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbee()) << "Reading color temperature range attributes finished with error" << reply->error();
            qCWarning(dcZigbee()) << "Failed to read color temperature min/max interval values. Using default values for" << m_thing << "[" <<  m_minColorTemperature << "," << m_maxColorTemperature << "] mired";
            return;
        }

        QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
        if (attributeStatusRecords.count() != 2) {
            qCWarning(dcZigbee()) << "Did not receive temperature min/max interval values from" << m_thing;
            qCWarning(dcZigbee()) << "Using default values for" << m_thing << "[" <<  m_minColorTemperature << "," << m_maxColorTemperature << "] mired" ;
            return;
        }

        // Parse the attribute status records
        foreach (const ZigbeeClusterLibrary::ReadAttributeStatusRecord &attributeStatusRecord, attributeStatusRecords) {
            if (attributeStatusRecord.attributeId == ZigbeeClusterColorControl::AttributeColorTempPhysicalMinMireds) {
                bool valueOk = false;
                quint16 minMiredsValue = attributeStatusRecord.dataType.toUInt16(&valueOk);
                if (!valueOk) {
                    qCWarning(dcZigbee()) << "Failed to read color temperature min mireds attribute value and convert it" << attributeStatusRecord;
                    break;
                }

                m_minColorTemperature = minMiredsValue;
            }

            if (attributeStatusRecord.attributeId == ZigbeeClusterColorControl::AttributeColorTempPhysicalMaxMireds) {
                bool valueOk = false;
                quint16 maxMiredsValue = attributeStatusRecord.dataType.toUInt16(&valueOk);
                if (!valueOk) {
                    qCWarning(dcZigbee()) << "Failed to read color temperature max mireds attribute value and convert it" << attributeStatusRecord;
                    break;
                }

                m_maxColorTemperature = maxMiredsValue;
            }
        }

        qCDebug(dcZigbee()) << "Using lamp specific color temperature mireds interval for mapping" << m_thing << "[" <<  m_minColorTemperature << "," << m_maxColorTemperature << "] mired";

        qCDebug(dcZigbee()) << "Lamp capabilities information complete";
        m_lampInformationCompleted = true;
    });
}

bool GenericColorLight::readCachedColorTemperatureRange()
{
    if (m_colorCluster->hasAttribute(ZigbeeClusterColorControl::AttributeColorTempPhysicalMinMireds) && m_colorCluster->hasAttribute(ZigbeeClusterColorControl::AttributeColorTempPhysicalMaxMireds)) {
        ZigbeeClusterAttribute minMiredsAttribute = m_colorCluster->attribute(ZigbeeClusterColorControl::AttributeColorTempPhysicalMinMireds);
        bool valueOk = false;
        quint16 minMiredsValue = minMiredsAttribute.dataType().toUInt16(&valueOk);
        if (!valueOk) {
            qCWarning(dcZigbee()) << "Failed to read color temperature min mireds attribute value and convert it" << minMiredsAttribute;
            return false;
        }

        ZigbeeClusterAttribute maxMiredsAttribute = m_colorCluster->attribute(ZigbeeClusterColorControl::AttributeColorTempPhysicalMaxMireds);
        quint16 maxMiredsValue = maxMiredsAttribute.dataType().toUInt16(&valueOk);
        if (!valueOk) {
            qCWarning(dcZigbee()) << "Failed to read color temperature max mireds attribute value and convert it" << maxMiredsAttribute;
            return false;
        }

        m_minColorTemperature = minMiredsValue;
        m_maxColorTemperature = maxMiredsValue;
        return true;
    }

    return false;
}

void GenericColorLight::readStates()
{
    // Start reading sequentially, on/off -> level ->  color temperature -> color
    readOnOffState();
}

void GenericColorLight::readOnOffState()
{
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
        return;
    }

    qCDebug(dcZigbee()) << "Reading on/off cluster from node" << m_thing;
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

void GenericColorLight::readLevelValue()
{
    if (!m_levelControlCluster) {
        qCWarning(dcZigbee()) << "Could not find the LevelControl input cluster on" << m_thing << m_endpoint;
        return;
    }

    qCDebug(dcZigbee()) << "Reading level cluster from node" << m_thing;
    ZigbeeClusterReply *reply = m_levelControlCluster->readAttributes({ZigbeeClusterLevelControl::AttributeCurrentLevel});
    connect(reply, &ZigbeeClusterReply::finished, this, [this, reply](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbee()) << "Failed to read LevelControl cluster attribute" << reply->error();
            return;
        }

        // Note: the attribute gets updated internally and the state gets updated with the currentLevelChanged signal
        qCDebug(dcZigbee()) << "Reading LevelControl cluster attribute finished successfully";
        readColorTemperature();
    });
}

void GenericColorLight::readColorTemperature()
{
    if (!m_colorCapabilities.testFlag(ZigbeeClusterColorControl::ColorCapabilityColorTemperature)) {
        // Note: there is no color temperature attribute available, we can skip reading and use the cached value from nymea which was an interpolated xy color
        readColorXy();
        return;
    }

    qCDebug(dcZigbee()) << "Reading color temperature cluster from node" << m_thing;
    ZigbeeClusterReply *reply = m_colorCluster->readAttributes({ZigbeeClusterColorControl::AttributeColorTemperatureMireds});
    connect(reply, &ZigbeeClusterReply::finished, this, [this, reply](){
        if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
            qCWarning(dcZigbee()) << "Failed to read ColorControl cluster attribute" << reply->error();
            return;
        }

        qCDebug(dcZigbee()) << "Reading ColorControl cluster attribute color temperature finished successfully";
        QList<ZigbeeClusterLibrary::ReadAttributeStatusRecord> attributeStatusRecords = ZigbeeClusterLibrary::parseAttributeStatusRecords(reply->responseFrame().payload);
        if (attributeStatusRecords.count() != 1) {
            qCWarning(dcZigbee()) << "Failed to read color temperature from" << m_thing;
            return;
        }

        bool valueOk = false;
        quint16 colorTemperature = attributeStatusRecords.first().dataType.toUInt16(&valueOk);
        if (!valueOk) {
            qCWarning(dcZigbee()) << "Failed to convert color temperature attribute values from" << m_thing << attributeStatusRecords.first();
            return;
        }

        int mappedValue = mapColorTemperatureToScaledValue(colorTemperature);
        qCDebug(dcZigbee()) << "Actual color temperature is" << colorTemperature << "mireds, mapped to generic interval" << mappedValue;
        m_thing->setStateValue(genericColorLightColorTemperatureStateTypeId, mappedValue);
        readColorXy();
    });
}

void GenericColorLight::readColorXy()
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
        m_thing->setStateValue(genericColorLightColorStateTypeId, color);
    });
}

quint16 GenericColorLight::mapScaledValueToColorTemperature(int scaledColorTemperature)
{
    double percentage = static_cast<double>((scaledColorTemperature - m_minScaleValue)) / (m_maxScaleValue - m_minScaleValue);
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << scaledColorTemperature << "between" << m_minScaleValue << m_maxScaleValue << "is" << percentage * 100 << "%";
    double mappedValue = (m_maxColorTemperature - m_minColorTemperature) * percentage + m_minColorTemperature;
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << scaledColorTemperature << "is" << mappedValue << "mireds";
    return static_cast<quint16>(qRound(mappedValue));
}

int GenericColorLight::mapColorTemperatureToScaledValue(quint16 colorTemperature)
{
    double percentage = static_cast<double>((colorTemperature - m_minColorTemperature)) / (m_maxColorTemperature - m_minColorTemperature);
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << colorTemperature << "mirred" << m_minColorTemperature << m_maxColorTemperature << "is" << percentage * 100 << "%";
    double mappedValue = (m_maxScaleValue - m_minScaleValue) * percentage + m_minScaleValue;
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << colorTemperature << "results into the scaled value of" << mappedValue;
    return static_cast<int>(qRound(mappedValue));
}

void GenericColorLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state);
    checkOnlineStatus();
}

