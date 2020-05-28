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

#include "lumitemperaturesensor.h"
#include "extern-plugininfo.h"

#include <QDataStream>

LumiTemperatureSensor::LumiTemperatureSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint contains temperature and humidity clusters
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    m_temperatureCluster = m_endpoint->inputCluster<ZigbeeClusterTemperatureMeasurement>(Zigbee::ClusterIdTemperatureMeasurement);
    if (!m_temperatureCluster) {
        qCWarning(dcZigbee()) << "Could not find the temperature measurement server cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_temperatureCluster, &ZigbeeClusterTemperatureMeasurement::temperatureChanged, this, [this](double temperature){
            qCDebug(dcZigbee()) << m_thing << "temperature changed" << temperature << "°C";
            m_thing->setStateValue(lumiTemperatureHumidityTemperatureStateTypeId, temperature);
        });
    }

    m_humidityCluster = m_endpoint->inputCluster<ZigbeeClusterRelativeHumidityMeasurement>(Zigbee::ClusterIdRelativeHumidityMeasurement);
    if (!m_humidityCluster) {
        qCWarning(dcZigbee()) << "Could not find the relative humidity measurement server cluster on" << m_thing << m_endpoint;
    } else {
        connect(m_humidityCluster, &ZigbeeClusterRelativeHumidityMeasurement::humidityChanged, this, [this](double humidity){
            qCDebug(dcZigbee()) << m_thing << "humidity changed" << humidity << "%";
            m_thing->setStateValue(lumiTemperatureHumidityHumidityStateTypeId, humidity);
        });
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiTemperatureSensor::onNetworkStateChanged);
}

void LumiTemperatureSensor::removeFromNetwork()
{
    //m_node->leaveNetworkRequest();
}

void LumiTemperatureSensor::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(lumiTemperatureHumidityConnectedStateTypeId, true);
        thing()->setStateValue(lumiTemperatureHumidityVersionStateTypeId, m_endpoint->softwareBuildId());    
    } else {
        thing()->setStateValue(lumiTemperatureHumidityConnectedStateTypeId, false);
    }
}

void LumiTemperatureSensor::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == lumiTemperatureHumidityRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void LumiTemperatureSensor::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

