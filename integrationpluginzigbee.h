/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Simon Stürz <simon.stuerz@nymea.io>                 *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINZIGBEE_H
#define INTEGRATIONPLUGINZIGBEE_H

#include "integrations/integrationplugin.h"
#include "nymea-zigbee/zigbeenetworkmanager.h"

#include "xiaomi/xiaomibuttonsensor.h"
#include "xiaomi/xiaomimotionsensor.h"
#include "xiaomi/xiaomimagnetsensor.h"
#include "xiaomi/xiaomitemperaturesensor.h"

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

    void createThingForNode(Thing *parent, ZigbeeNode *node);
    void createGenericNodeThingForNode(Thing *parent, ZigbeeNode *node);

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

#endif // INTEGRATIONPLUGINZIGBEE_H
