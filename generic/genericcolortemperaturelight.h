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

#ifndef GENERICCOLORTEMPERATURELIGHT_H
#define GENERICCOLORTEMPERATURELIGHT_H

#include <QObject>

#include "zigbeedevice.h"

class GenericColorTemperatureLight : public ZigbeeDevice
{
    Q_OBJECT
public:
    explicit GenericColorTemperatureLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Thing *thing, QObject *parent = nullptr);

    void removeFromNetwork() override;
    void checkOnlineStatus() override;
    void executeAction(ThingActionInfo *info) override;

private:
    ZigbeeNodeEndpoint *m_endpoint = nullptr;
    ZigbeeClusterIdentify *m_identifyCluster= nullptr;
    ZigbeeClusterOnOff *m_onOffCluster = nullptr;
    ZigbeeClusterLevelControl *m_levelControlCluster = nullptr;
    ZigbeeClusterColorControl *m_colorCluster = nullptr;

    void readOnOffState();
    void readLevelValue();
    void readColorTemperature();

    // Use default values until we can load them from the device and map them
    quint16 m_minColorTemperature = 250;
    quint16 m_maxColorTemperature = 450;

    // Used for scaling
    int m_minScaleValue = 0;
    int m_maxScaleValue = 200;

    // Map methods between scale and actual value
    quint16 mapScaledValueToColorTemperature(int scaledColorTemperature);
    int mapColorTemperatureToScaledValue(quint16 colorTemperature);

    void readColorTemperatureRange();
    bool readCachedColorTemperatureRange();

private slots:
    void onNetworkStateChanged(ZigbeeNetwork::State state);

};

#endif // GENERICCOLORTEMPERATURELIGHT_H
