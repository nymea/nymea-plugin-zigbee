#ifndef XIAOMIBUTTONSENSOR_H
#define XIAOMIBUTTONSENSOR_H

#include <QObject>
#include <QTimer>

#include "zigbeenode.h"

class XiaomiButtonSensor : public QObject
{
    Q_OBJECT
public:
    explicit XiaomiButtonSensor(ZigbeeNode *node, QObject *parent = nullptr);

    bool connected() const;
    bool pressed() const;

private:
    ZigbeeNode *m_node = nullptr;
    QTimer *m_longPressedTimer = nullptr;

    bool m_connected = false;
    bool m_pressed = false;

    void setConnected(bool connected);
    void setPressed(bool pressed);

signals:
    void connectedChanged(bool connected);
    void pressedChanged(bool pressed);
    void buttonPressed();
    void buttonLongPressed();

private slots:
    void onLongPressedTimeout();
    void onNodeConnectedChanged(bool connected);
    void onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

};

#endif // XIAOMIBUTTONSENSOR_H
