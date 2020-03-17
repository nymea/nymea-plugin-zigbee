#ifndef LUMIBUTTONSENSOR_H
#define LUMIBUTTONSENSOR_H

#include <QObject>
#include <QTimer>

#include "zigbeedevice.h"

class LumiButtonSensor : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit LumiButtonSensor(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    void identify();

    void removeFromNetwork() override;
    void checkOnlineStatus() override;

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;
    QTimer *m_longPressedTimer = nullptr;

    bool m_pressed = false;
    void setPressed(bool pressed);

signals:
    void buttonPressed();
    void buttonLongPressed();

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onEndpointClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);
    void onLongPressedTimeout();

};

#endif // LUMIBUTTONSENSOR_H
