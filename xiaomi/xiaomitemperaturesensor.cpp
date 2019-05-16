#include "xiaomitemperaturesensor.h"

XiaomiTemperatureSensor::XiaomiTemperatureSensor(QObject *parent) : QObject(parent)
{

}

void XiaomiTemperatureSensor::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    Q_UNUSED(attribute)

    switch (cluster->clusterId()) {
    case Zigbee::ClusterIdBasic:

        break;
    default:
        break;

    }
}
