#ifndef XIAOMITEMPERATURESENSOR_H
#define XIAOMITEMPERATURESENSOR_H

#include <QObject>

#include "nymea-zigbee/zigbeenode.h"

class XiaomiTemperatureSensor : public QObject
{
    Q_OBJECT
public:
    explicit XiaomiTemperatureSensor(ZigbeeNode *node, QObject *parent = nullptr);

    bool connected() const;
    double temperature() const;
    double humidity() const;

private:
    ZigbeeNode *m_node = nullptr;

    bool m_connected = false;
    double m_temperature = 0;
    double m_humidity = 0;

    void setConnected(bool connected);
    void setTemperature(double temperature);
    void setHumidity(double humidity);

signals:
    void connectedChanged(bool connected);
    void temperatureChanged(double temperature);
    void humidityChanged(double humidity);

private slots:
    void onNodeConnectedChanged(bool connected);
    void onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

};

#endif // XIAOMITEMPERATURESENSOR_H
