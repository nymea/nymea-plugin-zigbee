#ifndef XIAOMIMOTIONSENSOR_H
#define XIAOMIMOTIONSENSOR_H

#include <QObject>
#include <QTimer>

#include "nymea-zigbee/zigbeenode.h"

class XiaomiMotionSensor : public QObject
{
    Q_OBJECT
public:
    explicit XiaomiMotionSensor(ZigbeeNode *node, QObject *parent = nullptr);

    bool connected() const;
    bool present() const;

    int delay() const;
    void setDelay(int delay);

private:
    ZigbeeNode *m_node = nullptr;
    QTimer *m_delayTimer = nullptr;

    bool m_connected = false;
    bool m_present = false;
    int m_delay = 60;

    void setConnected(bool connected);
    void setPresent(bool present);

signals:
    void connectedChanged(bool connected);
    void delayChanged(int delay);
    void presentChanged(bool present);
    void motionDetected();

private slots:
    void onNodeConnectedChanged(bool connected);
    void onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

    void onDelayTimerTimeout();
};

#endif // XIAOMIMOTIONSENSOR_H
