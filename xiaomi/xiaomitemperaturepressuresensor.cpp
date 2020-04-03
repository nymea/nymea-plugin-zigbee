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

#include "lumitemperaturepressuresensor.h"
#include "extern-plugininfo.h"

#include <QDataStream>

LumiTemperaturePressureSensor::LumiTemperaturePressureSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint is sending the temperature notifications
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiTemperaturePressureSensor::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &LumiTemperaturePressureSensor::onEndpointClusterAttributeChanged);
}

void LumiTemperaturePressureSensor::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void LumiTemperaturePressureSensor::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        thing()->setStateValue(lumiTemperatureHumidityPressureConnectedStateTypeId, true);
        thing()->setStateValue(lumiTemperatureHumidityPressureVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        thing()->setStateValue(lumiTemperatureHumidityPressureConnectedStateTypeId, false);
    }
}

void LumiTemperaturePressureSensor::executeAction(ThingActionInfo *info)
{
    if (info->action().actionTypeId() == lumiTemperatureHumidityPressureRemoveFromNetworkActionTypeId) {
        removeFromNetwork();
        info->finish(Thing::ThingErrorNoError);
    }
}

void LumiTemperaturePressureSensor::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void LumiTemperaturePressureSensor::onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    switch (cluster->clusterId()) {
    case Zigbee::ClusterIdTemperatureMeasurement:
        if (attribute.id() == 0) {
            QByteArray data = attribute.data();
            QDataStream stream(&data, QIODevice::ReadOnly);
            qint16 temperatureRaw = 0;
            stream >> temperatureRaw;
            double temperature = temperatureRaw / 100.0;
            qCDebug(dcZigbee()) << thing() << "temperature changed" << temperature << "°C";
            thing()->setStateValue(lumiTemperatureHumidityPressureTemperatureStateTypeId, temperature);
        }
        break;
    case Zigbee::ClusterIdRelativeHumidityMeasurement:
        if (attribute.id() == 0) {
            QByteArray data = attribute.data();
            QDataStream stream(&data, QIODevice::ReadOnly);
            quint16 humidityRaw = 0;
            stream >> humidityRaw;
            double humidity = humidityRaw / 100.0;
            qCDebug(dcZigbee()) << thing() << "humidity changed" << humidity << "%";
            thing()->setStateValue(lumiTemperatureHumidityPressureHumidityStateTypeId, humidity);
        }
        break;
    case Zigbee::ClusterIdPressureMeasurement:
        if (attribute.id() == 0) {
            QByteArray data = attribute.data();
            QDataStream stream(&data, QIODevice::ReadOnly);
            quint16 pressureRaw = 0;
            stream >> pressureRaw;
            double pressure = pressureRaw / 100.0;
            qCDebug(dcZigbee()) << thing() << "pressure changed" << pressure << "hPa";
            thing()->setStateValue(lumiTemperatureHumidityPressureHumidityStateTypeId, pressure);
        }
        break;
    default:
        break;
    }
}
