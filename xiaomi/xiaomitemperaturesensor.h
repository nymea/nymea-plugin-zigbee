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

#ifndef XIAOMITEMPERATURESENSOR_H
#define XIAOMITEMPERATURESENSOR_H

#include <QObject>

#include "zigbeenode.h"

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
