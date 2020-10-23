/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef LUMIRELAY_H
#define LUMIRELAY_H

#include <QObject>
#include <QTimer>

#include "zigbeedevice.h"

class LumiRelay : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit LumiRelay(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent = nullptr);

    void removeFromNetwork() override;
    void checkOnlineStatus() override;
    void executeAction(ThingActionInfo *info) override;

private:
    ZigbeeNodeEndpoint *m_endpoint1 = nullptr;
    ZigbeeNodeEndpoint *m_endpoint2 = nullptr;

    ZigbeeClusterOnOff *m_onOffCluster1 = nullptr;
    ZigbeeClusterOnOff *m_onOffCluster2 = nullptr;

    QTimer *m_longPressedTimer = nullptr;

    bool m_pressed = false;
    void setPressed(bool pressed);

signals:
    void buttonPressed();
    void buttonLongPressed();

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);
    void onLongPressedTimeout();

};

#endif // LUMIRELAY_H
