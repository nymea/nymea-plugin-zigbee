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


#ifndef DEVICEPLUGINZIGBEE_H
#define DEVICEPLUGINZIGBEE_H

#include <integrations/integrationplugin.h>
#include "zigbeenetworkmanager.h"
#include "xiaomi/xiaomibuttonsensor.h"
#include "xiaomi/xiaomimotionsensor.h"
#include "xiaomi/xiaomimagnetsensor.h"
#include "xiaomi/xiaomitemperaturesensor.h"
#include "xiaomi/xiaomitemperaturepressuresensor.h"

class IntegrationPluginZigbee: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginzigbee.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginZigbee();

    void init() override;
    void startMonitoringAutoThings() override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;

    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void executeAction(ThingActionInfo *info) override;

private:
    QHash<Thing *, ZigbeeNetworkManager *> m_zigbeeControllers;
    QHash<Thing *, XiaomiTemperatureSensor *> m_xiaomiTemperatureSensors;
    QHash<Thing *, XiaomiMagnetSensor *> m_xiaomiMagnetSensors;
    QHash<Thing *, XiaomiButtonSensor *> m_xiaomiButtonSensors;
    QHash<Thing *, XiaomiMotionSensor *> m_xiaomiMotionSensors;

    ZigbeeNetworkManager *findParentController(Thing *thing) const;
    ZigbeeNetworkManager *findNodeController(ZigbeeNode *node) const;

    Thing *findNodeThing(ZigbeeNode *node);

    void createThingForNode(Thing *parentThing, ZigbeeNode *node);
    void createGenericNodeThingForNode(Thing *parentThing, ZigbeeNode *node);

private slots:
    void onZigbeeControllerStateChanged(ZigbeeNetwork::State state);
    void onZigbeeControllerChannelChanged(uint channel);
    void onZigbeeControllerPanIdChanged(quint64 extendedPanId);
    void onZigbeeControllerPermitJoiningChanged(bool permitJoining);
    void onZigbeeControllerNodeAdded(ZigbeeNode *node);
    void onZigbeeControllerNodeRemoved(ZigbeeNode *node);

    // Xiaomi temperature humidity sensor
    void onXiaomiTemperatureSensorConnectedChanged(bool connected);
    void onXiaomiTemperatureSensorTemperatureChanged(double temperature);
    void onXiaomiTemperatureSensorHumidityChanged(double humidity);

    // Xiaomi magnet sensor
    void onXiaomiMagnetSensorConnectedChanged(bool connected);
    void onXiaomiMagnetSensorClosedChanged(bool closed);

    // Xiaomi button sensor
    void onXiaomiButtonSensorConnectedChanged(bool connected);
    void onXiaomiButtonSensorPressedChanged(bool pressed);
    void onXiaomiButtonSensorPressed();
    void onXiaomiButtonSensorLongPressed();

    // Xiaomi motion sensor
    void onXiaomiMotionSensorConnectedChanged(bool connected);
    void onXiaomiMotionSensorPresentChanged(bool present);
    void onXiaomiMotionSensorMotionDetected();
};

#endif // DEVICEPLUGINZIGBEE_H
