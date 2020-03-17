#ifndef tradfriOnOffSwitch_H
#define tradfriOnOffSwitch_H

#include <QObject>

#include "zigbeedevice.h"

class TradfriOnOffSwitch : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit TradfriOnOffSwitch(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent = nullptr);

    void identify();
    void factoryResetNode();

    void removeFromNetwork() override;
    void checkOnlineStatus() override;

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;

    void readAttribute();
    void configureReporting();


private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);

};

#endif // tradfriOnOffSwitch_H
