#ifndef LUMIMOTIONSENSOR_H
#define LUMIMOTIONSENSOR_H

#include <QObject>
#include <QTimer>

#include "zigbeedevice.h"

class LumiMotionSensor : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit LumiMotionSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    void identify();

    void checkOnlineStatus() override;

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;
    QTimer *m_delayTimer = nullptr;
    bool m_present = false;

    void setPresent(bool present);

signals:
    void motionDetected();

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);
    void onDelayTimerTimeout();

};

#endif // LUMIMOTIONSENSOR_H
