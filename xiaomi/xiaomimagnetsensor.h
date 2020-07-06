#ifndef XIAOMIMAGNETSENSOR_H
#define XIAOMIMAGNETSENSOR_H

#include <QObject>

#include "nymea-zigbee/zigbeenode.h"

class XiaomiMagnetSensor : public QObject
{
    Q_OBJECT
public:
    explicit XiaomiMagnetSensor(ZigbeeNode *node, QObject *parent = nullptr);

    bool connected() const;
    bool closed() const;

private:
    ZigbeeNode *m_node = nullptr;

    bool m_connected = false;
    bool m_closed = false;

    void setConnected(bool connected);
    void setClosed(bool closed);

signals:
    void connectedChanged(bool connected);
    void closedChanged(bool closed);


private slots:
    void onNodeConnectedChanged(bool connected);
    void onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

};

#endif // XIAOMIMAGNETSENSOR_H
