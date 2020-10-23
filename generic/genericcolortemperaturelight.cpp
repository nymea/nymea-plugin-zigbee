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

#include "genericcolortemperaturelight.h"
#include "extern-plugininfo.h"

#include <QDataStream>

GenericColorTemperatureLight::GenericColorTemperatureLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoint, m_node->endpoints()) {
        if ((endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceColourTemperatureLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceColourTemperatureLight)) {
            m_endpoint = endpoint;
            break;
        }
    }

    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    // Update signal strength
    connect(m_node, &ZigbeeNode::lqiChanged, this, [this](quint8 lqi){
        uint signalStrength = qRound(lqi * 100.0 / 255.0);
        qCDebug(dcZigbee()) << m_thing << "signal strength changed" << signalStrength << "%";
        m_thing->setStateValue(genericColorTemperatureLightSignalStrengthStateTypeId, signalStrength);
    });


    // Get the ZigbeeCluster servers
    m_onOffCluster = m_endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_onOffCluster, &ZigbeeClusterOnOff::powerChanged, this, [this](bool power){
            qCDebug(dcZigbee()) << m_thing << "power state changed" << power;
            m_thing->setStateValue(genericColorTemperatureLightPowerStateTypeId, power);
        });
    }

    m_levelControlCluster = m_endpoint->inputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
    if (!m_levelControlCluster) {
        qCWarning(dcZigbee()) << "Could not find the LevelControl input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_levelControlCluster, &ZigbeeClusterLevelControl::currentLevelChanged, this, [this](quint8 level){
            int percentage = qRound(level * 100.0 / 255.0);
            qCDebug(dcZigbee()) << m_thing << "level state changed" << level << percentage << "%";
            m_thing->setStateValue(genericColorTemperatureLightBrightnessStateTypeId, percentage);
        });
    }

    m_identifyCluster = m_endpoint->inputCluster<ZigbeeClusterIdentify>(ZigbeeClusterLibrary::ClusterIdIdentify);
    if (!m_identifyCluster) {
        qCWarning(dcZigbee()) << "Could not find the identify input cluster on" << m_thing << m_endpoint;
    }

    m_colorCluster = m_endpoint->inputCluster<ZigbeeClusterColorControl>(ZigbeeClusterLibrary::ClusterIdColorControl);
    if (!m_colorCluster) {
        qCWarning(dcZigbee()) << "Could not find the color control input cluster on" << m_thing << m_endpoint;
    }

    // Read the initial color temperature min/max values either from the lamp or from the cache if they are known
    readColorTemperatureRange();

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &GenericColorTemperatureLight::onNetworkStateChanged);
}

void GenericColorTemperatureLight::removeFromNetwork()
{
    m_network->removeZigbeeNode(m_node->extendedAddress());
}

void GenericColorTemperatureLight::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        m_thing->setStateValue(genericColorTemperatureLightConnectedStateTypeId, true);
        m_thing->setStateValue(genericColorTemperatureLightVersionStateTypeId, m_endpoint->softwareBuildId());
        m_thing->setStateValue(genericColorTemperatureLightSignalStrengthStateTypeId, qRound(m_node->lqi() * 100.0 / 255.0));
        readStates();
    } else {
        m_thing->setStateValue(genericColorTemperatureLightConnectedStateTypeId, false);
    }
}

void GenericColorTemperatureLight::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == genericColorTemperatureLightAlertActionTypeId) {
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

    } else if (info->action().actionTypeId() == genericColorTemperatureLightPowerActionTypeId) {
        if (!m_onOffCluster) {
            qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        bool power = info->action().param(genericColorTemperatureLightPowerActionPowerParamTypeId).value().toBool();
        ZigbeeClusterReply *reply = (power ? m_onOffCluster->commandOn() : m_onOffCluster->commandOff());
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, power](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                m_thing->setStateValue(genericColorTemperatureLightPowerStateTypeId, power);
            }
        });
    } else if (info->action().actionTypeId() == genericColorTemperatureLightBrightnessActionTypeId) {
        int brightness = info->action().param(genericColorTemperatureLightBrightnessActionBrightnessParamTypeId).value().toInt();
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
                m_thing->setStateValue(genericColorTemperatureLightPowerStateTypeId, (brightness > 0 ? true : false));
                m_thing->setStateValue(genericColorTemperatureLightBrightnessStateTypeId, brightness);
            }
        });
    } else if (info->action().actionTypeId() == genericColorTemperatureLightColorTemperatureActionTypeId) {
        if (!m_colorCluster) {
            qCWarning(dcZigbee()) << "Could not find the ColorControl input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Note: since the min/max colortemperature is device dependent, we need to map them in the range [-100, 100]
        int colorTemperatureScaled = info->action().param(genericColorTemperatureLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
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
                m_thing->setStateValue(genericColorTemperatureLightColorTemperatureStateTypeId, colorTemperatureScaled);
            }
        });
    } else if (info->action().actionTypeId() == genericColorTemperatureLightRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void GenericColorTemperatureLight::readStates()
{
    // Start reading sequentially, on/off -> level ->  color temperature
    readOnOffState();
}

void GenericColorTemperatureLight::readOnOffState()
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

void GenericColorTemperatureLight::readLevelValue()
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
        readColorTemperature();
    });
}

void GenericColorTemperatureLight::readColorTemperature()
{
    if (!m_levelControlCluster) {
        qCWarning(dcZigbee()) << "Could not find the LevelControl input cluster on" << m_thing << m_endpoint;
        return;
    }

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

        qCDebug(dcZigbee()) << "Actual color temperature is" << colorTemperature << "mireds";
        int mappedValue = mapColorTemperatureToScaledValue(colorTemperature);
        qCDebug(dcZigbee()) << "Mapped color temperature is" << mappedValue;
        m_thing->setStateValue(genericColorTemperatureLightColorTemperatureStateTypeId, mappedValue);
    });
}

quint16 GenericColorTemperatureLight::mapScaledValueToColorTemperature(int scaledColorTemperature)
{
    double percentage = static_cast<double>((scaledColorTemperature - m_minScaleValue)) / (m_maxScaleValue - m_minScaleValue);
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << scaledColorTemperature << "between" << m_minScaleValue << m_maxScaleValue << "is" << percentage * 100 << "%";
    double mappedValue = (m_maxColorTemperature - m_minColorTemperature) * percentage + m_minColorTemperature;
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << scaledColorTemperature << "is" << mappedValue << "mireds";
    return static_cast<quint16>(qRound(mappedValue));
}

int GenericColorTemperatureLight::mapColorTemperatureToScaledValue(quint16 colorTemperature)
{
    double percentage = static_cast<double>((colorTemperature - m_minColorTemperature)) / (m_maxColorTemperature - m_minColorTemperature);
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << colorTemperature << "mirred" << m_minColorTemperature << m_maxColorTemperature << "is" << percentage * 100 << "%";
    double mappedValue = (m_maxScaleValue - m_minScaleValue) * percentage + m_minScaleValue;
    //qCDebug(dcZigbee()) << "Mapping color temperature value" << colorTemperature << "results into the scaled value of" << mappedValue;
    return static_cast<int>(qRound(mappedValue));
}

void GenericColorTemperatureLight::readColorTemperatureRange()
{
    if (!m_colorCluster) {
        qCWarning(dcZigbee()) << "Could not read color temperature value range" << m_thing << m_endpoint << "There is no color cluster available.";
        return;
    }

    // Check if we can use the cached values from the database
    if (readCachedColorTemperatureRange()) {
        qCDebug(dcZigbee()) << "Using cached color temperature mireds interval for mapping" << m_thing << "[" <<  m_minColorTemperature << "," << m_maxColorTemperature << "] mired";
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
    });
}

bool GenericColorTemperatureLight::readCachedColorTemperatureRange()
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

void GenericColorTemperatureLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}
