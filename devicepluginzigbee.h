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

#ifndef DEVICEPLUGINZIGBEE_H
#define DEVICEPLUGINZIGBEE_H

#include "devices/devicemanager.h"
#include "devices/deviceplugin.h"
#include "nymea-zigbee/zigbeenetworkmanager.h"

#include "xiaomi/xiaomibuttonsensor.h"
#include "xiaomi/xiaomimotionsensor.h"
#include "xiaomi/xiaomimagnetsensor.h"
#include "xiaomi/xiaomitemperaturesensor.h"
#include "xiaomi/xiaomiremoteswitch.h"

class DevicePluginZigbee: public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginzigbee.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginZigbee();

    void init() override;
    void startMonitoringAutoDevices() override;
    void postSetupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;

    Device::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    Device::DeviceSetupStatus setupDevice(Device *device) override;
    Device::DeviceError executeAction(Device *device, const Action &action) override;

private:
    QHash<Device *, ZigbeeNetworkManager *> m_zigbeeControllers;
    QHash<Device *, XiaomiTemperatureSensor *> m_xiaomiTemperatureSensors;
    QHash<Device *, XiaomiMagnetSensor *> m_xiaomiMagnetSensors;
    QHash<Device *, XiaomiButtonSensor *> m_xiaomiButtonSensors;
    QHash<Device *, XiaomiMotionSensor *> m_xiaomiMotionSensors;
    QHash<Device *, XiaomiRemoteSwitch *> m_xiaomiRemoteSwitches;

    ZigbeeNetworkManager *findParentController(Device *device) const;
    ZigbeeNetworkManager *findNodeController(ZigbeeNode *node) const;

    Device *findNodeDevice(ZigbeeNode *node);

    void createDeviceForNode(Device *parentDevice, ZigbeeNode *node);
    void createGenericNodeDeviceForNode(Device *parentDevice, ZigbeeNode *node);

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


    // Xiaomi remote switch
    void onXiaomiRemoteSwitchConnectedChanged(bool connected);
    void onXiaomiRemoteSwitchPressedChanged(bool pressed);
    void onXiaomiRemoteSwitchPressed();
    void onXiaomiRemoteSwitchLongPressed();

};

#endif // DEVICEPLUGINZIGBEE_H
