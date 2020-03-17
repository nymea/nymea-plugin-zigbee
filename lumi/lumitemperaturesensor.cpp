#include "lumitemperaturesensor.h"
#include "extern-plugininfo.h"

LumiTemperatureSensor::LumiTemperatureSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, thing, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint 0x01 since that endpoint is sending the temperature notifications
    m_endpoint = m_node->getEndpoint(0x01);
    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &LumiTemperatureSensor::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &LumiTemperatureSensor::onEndpointClusterAttributeChanged);
}

void LumiTemperatureSensor::identify()
{
    m_endpoint->identify(2);
}

void LumiTemperatureSensor::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
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

void LumiTemperatureSensor::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    Q_UNUSED(state)
    checkOnlineStatus();
}

void LumiTemperatureSensor::onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
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
            thing()->setStateValue(lumiTemperatureHumidityTemperatureStateTypeId, temperature);
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
            thing()->setStateValue(lumiTemperatureHumidityHumidityStateTypeId, humidity);
        }
        break;
    default:
        break;
    }
}
