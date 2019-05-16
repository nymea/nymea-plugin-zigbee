#ifndef XIAOMITEMPERATURESENSOR_H
#define XIAOMITEMPERATURESENSOR_H

#include <QObject>

#include "zigbeecluster.h"

class XiaomiTemperatureSensor : public QObject
{
    Q_OBJECT
public:
    explicit XiaomiTemperatureSensor(QObject *parent = nullptr);

    void onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

signals:

public slots:
};

#endif // XIAOMITEMPERATURESENSOR_H
