#include "xiaomitemperaturesensor.h"
#include "extern-plugininfo.h"

#include <QDataStream>

XiaomiTemperatureSensor::XiaomiTemperatureSensor(ZigbeeNode *node, QObject *parent) :
    QObject(parent),
    m_node(node)
{
    // Init values
    if (m_node->hasOutputCluster(Zigbee::ClusterIdTemperatureMeasurement)) {
        ZigbeeCluster *cluster = m_node->getOutputCluster(Zigbee::ClusterIdTemperatureMeasurement);
        QByteArray temperatureData = cluster->attribute(0x0000).data();
        if (!temperatureData.isEmpty()) {
            QDataStream stream(&temperatureData, QIODevice::ReadOnly);
            qint16 temperatureRaw = 0;
            stream >> temperatureRaw;
            setTemperature(temperatureRaw / 100.0);
        }
    }

    if (m_node->hasOutputCluster(Zigbee::ClusterIdRelativeHumidityMeasurement)) {
        ZigbeeCluster *cluster = m_node->getOutputCluster(Zigbee::ClusterIdRelativeHumidityMeasurement);
        QByteArray humidityData = cluster->attribute(0x0000).data();
        if (!humidityData.isEmpty()) {
            QDataStream stream(&humidityData, QIODevice::ReadOnly);
            quint16 humidityRaw = 0;
            stream >> humidityRaw;
            setHumidity(humidityRaw / 100.0);
        }
    }

    setConnected(m_node->connected());

    connect(node, &ZigbeeNode::connectedChanged, this, &XiaomiTemperatureSensor::onNodeConnectedChanged);
    connect(node, &ZigbeeNode::clusterAttributeChanged, this, &XiaomiTemperatureSensor::onClusterAttributeChanged);
}

bool XiaomiTemperatureSensor::connected() const
{
    return m_connected;
}

double XiaomiTemperatureSensor::temperature() const
{
    return m_temperature;
}

double XiaomiTemperatureSensor::humidity() const
{
    return m_humidity;
}

void XiaomiTemperatureSensor::setConnected(bool connected)
{
    if (m_connected == connected)
        return;

    m_connected = connected;
    emit connectedChanged(m_connected);
}

void XiaomiTemperatureSensor::setTemperature(double temperature)
{
    if (qFuzzyCompare(m_temperature, temperature))
        return;

    m_temperature = temperature;
    emit temperatureChanged(m_temperature);
}

void XiaomiTemperatureSensor::setHumidity(double humidity)
{
    if (qFuzzyCompare(m_humidity, humidity))
        return;

    m_humidity = humidity;
    emit humidityChanged(m_humidity);
}

void XiaomiTemperatureSensor::onNodeConnectedChanged(bool connected)
{
    setConnected(connected);
}

void XiaomiTemperatureSensor::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    Q_UNUSED(attribute)

    switch (cluster->clusterId()) {
    case Zigbee::ClusterIdTemperatureMeasurement:
        if (attribute.id() == 0) {
            QByteArray data = attribute.data();
            QDataStream stream(&data, QIODevice::ReadOnly);
            qint16 temperatureRaw = 0;
            stream >> temperatureRaw;
            setTemperature(temperatureRaw / 100.0);
        }
        break;
    case Zigbee::ClusterIdRelativeHumidityMeasurement:
        if (attribute.id() == 0) {
            QByteArray data = attribute.data();
            QDataStream stream(&data, QIODevice::ReadOnly);
            quint16 humidityRaw = 0;
            stream >> humidityRaw;
            setHumidity(humidityRaw / 100.0);
        }
        break;
    default:
        break;
    }
}
