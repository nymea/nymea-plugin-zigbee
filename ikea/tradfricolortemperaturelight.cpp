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
#include <QDataStream>

TradfriColorTemperatureLight::TradfriColorTemperatureLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
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
        m_thing->setStateValue(tradfriColorTemperatureLightSignalStrengthStateTypeId, signalStrength);
    });

    // Get the ZigbeeCluster servers
    m_onOffCluster = m_endpoint->inputCluster<ZigbeeClusterOnOff>(ZigbeeClusterLibrary::ClusterIdOnOff);
    if (!m_onOffCluster) {
        qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_onOffCluster, &ZigbeeClusterOnOff::powerChanged, this, [this](bool power){
            qCDebug(dcZigbee()) << m_thing << "power state changed" << power;
            m_thing->setStateValue(tradfriColorTemperatureLightPowerStateTypeId, power);
        });
    }

    m_levelControlCluster = m_endpoint->inputCluster<ZigbeeClusterLevelControl>(ZigbeeClusterLibrary::ClusterIdLevelControl);
    if (!m_levelControlCluster) {
        qCWarning(dcZigbee()) << "Could not find the LevelControl input cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_levelControlCluster, &ZigbeeClusterLevelControl::currentLevelChanged, this, [this](quint8 level){
            int percentage = qRound(level * 100.0 / 255.0);
            qCDebug(dcZigbee()) << m_thing << "level state changed" << level << percentage << "%";
            m_thing->setStateValue(tradfriColorTemperatureLightBrightnessStateTypeId, percentage);
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

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriColorTemperatureLight::onNetworkStateChanged, Qt::QueuedConnection);
}

void TradfriColorTemperatureLight::removeFromNetwork()
{
    m_network->removeZigbeeNode(m_node->extendedAddress());
}

void TradfriColorTemperatureLight::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        m_thing->setStateValue(tradfriColorTemperatureLightConnectedStateTypeId, true);
        m_thing->setStateValue(tradfriColorTemperatureLightVersionStateTypeId, m_endpoint->softwareBuildId());
        m_thing->setStateValue(tradfriColorTemperatureLightSignalStrengthStateTypeId, qRound(m_node->lqi() * 100.0 / 255.0));
        readStates();
    } else {
        m_thing->setStateValue(tradfriColorTemperatureLightConnectedStateTypeId, false);
    }
}

void TradfriColorTemperatureLight::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == tradfriColorTemperatureLightAlertActionTypeId) {
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
    } else if (info->action().actionTypeId() == tradfriColorTemperatureLightPowerActionTypeId) {
        if (!m_onOffCluster) {
            qCWarning(dcZigbee()) << "Could not find the OnOff input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        bool power = info->action().param(tradfriColorTemperatureLightPowerActionPowerParamTypeId).value().toBool();
        ZigbeeClusterReply *reply = (power ? m_onOffCluster->commandOn() : m_onOffCluster->commandOff());
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, power](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                m_thing->setStateValue(tradfriColorTemperatureLightPowerStateTypeId, power);
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorTemperatureLightBrightnessActionTypeId) {
        int brightness = info->action().param(tradfriColorTemperatureLightBrightnessActionBrightnessParamTypeId).value().toInt();
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
                m_thing->setStateValue(tradfriColorTemperatureLightPowerStateTypeId, (brightness > 0 ? true : false));
                m_thing->setStateValue(tradfriColorTemperatureLightBrightnessStateTypeId, brightness);
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorTemperatureLightColorTemperatureActionTypeId) {
        quint16 colorTemperature = info->action().param(tradfriColorTemperatureLightColorTemperatureActionColorTemperatureParamTypeId).value().toUInt();
        if (!m_colorCluster) {
            qCWarning(dcZigbee()) << "Could not find the ColorControl input cluster on" << m_thing << m_endpoint;
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        // Note: time unit is 1/10 s
        ZigbeeClusterReply *reply = m_colorCluster->commandMoveToColorTemperature(colorTemperature, 5);
        connect(reply, &ZigbeeClusterReply::finished, this, [this, reply, info, colorTemperature](){
            // Note: reply will be deleted automatically
            if (reply->error() != ZigbeeClusterReply::ErrorNoError) {
                info->finish(Thing::ThingErrorHardwareFailure);
            } else {
                info->finish(Thing::ThingErrorNoError);
                m_thing->setStateValue(tradfriColorTemperatureLightColorTemperatureStateTypeId, colorTemperature);
            }
        });
    } else if (info->action().actionTypeId() == tradfriColorTemperatureLightRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void TradfriColorTemperatureLight::readStates()
{
    // Start reading sequentially, on/off -> level ->  color temperature
    readOnOffState();
}

void TradfriColorTemperatureLight::readOnOffState()
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

void TradfriColorTemperatureLight::readLevelValue()
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

void TradfriColorTemperatureLight::readColorTemperature()
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

        m_thing->setStateValue(tradfriColorTemperatureLightColorTemperatureStateTypeId, colorTemperature);
    });
}

void TradfriColorTemperatureLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

