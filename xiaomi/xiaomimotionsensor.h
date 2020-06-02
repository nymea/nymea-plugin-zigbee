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

#ifndef XIAOMIMOTIONSENSOR_H
#define XIAOMIMOTIONSENSOR_H

#include <QObject>
#include <QTimer>

#include "zigbeenode.h"

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
