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

#include "plugininfo.h"
#include "nymeasettings.h"
#include "devicepluginzigbee.h"

#include <QDateTime>
#include <QSerialPortInfo>

DevicePluginZigbee::DevicePluginZigbee()
{

}

void DevicePluginZigbee::init()
{       

}

void DevicePluginZigbee::startMonitoringAutoDevices()
{
    // Start seaching for devices which can be discovered and added automatically
}

void DevicePluginZigbee::postSetupDevice(Device *device)
{
    qCDebug(dcZigbee()) << "Post setup device" << device->name() << device->params();

    if (device->deviceClassId() == xiaomiTemperatureHumidityDeviceClassId) {
        XiaomiTemperatureSensor *sensor = m_xiaomiTemperatureSensors.value(device);
        device->setStateValue(xiaomiTemperatureHumidityConnectedStateTypeId, sensor->connected());
        device->setStateValue(xiaomiTemperatureHumidityTemperatureStateTypeId, sensor->temperature());
        device->setStateValue(xiaomiTemperatureHumidityHumidityStateTypeId, sensor->humidity());
    }

    if (device->deviceClassId() == xiaomiMagnetSensorDeviceClassId) {
        XiaomiMagnetSensor *sensor = m_xiaomiMagnetSensors.value(device);
        device->setStateValue(xiaomiMagnetSensorConnectedStateTypeId, sensor->connected());
        device->setStateValue(xiaomiMagnetSensorClosedStateTypeId, sensor->closed());
    }

    if (device->deviceClassId() == xiaomiButtonSensorDeviceClassId) {
        XiaomiButtonSensor *sensor = m_xiaomiButtonSensors.value(device);
        device->setStateValue(xiaomiButtonSensorConnectedStateTypeId, sensor->connected());
        //device->setStateValue(xiaomiButtonSensorPressedStateTypeId, sensor->pressed());
    }

    if (device->deviceClassId() == xiaomiMotionSensorDeviceClassId) {
        XiaomiMotionSensor *sensor = m_xiaomiMotionSensors.value(device);
        device->setStateValue(xiaomiMotionSensorConnectedStateTypeId, sensor->connected());
        device->setStateValue(xiaomiMotionSensorIsPresentStateTypeId, sensor->present());
    }

    if (device->deviceClassId() == xiaomiRemoteSwitchSingleRockerDeviceClassId) {
        XiaomiRemoteSwitch *remoteSwitch = m_xiaomiRemoteSwitches.value(device);
        device->setStateValue(xiaomiRemoteSwitchSingleRockerConnectedStateTypeId, remoteSwitch->connected());
    }

    if (device->deviceClassId() == xiaomiRemoteSwitchDoubleRockerDeviceClassId) {
        XiaomiRemoteSwitch *remoteSwitch = m_xiaomiRemoteSwitches.value(device);
        device->setStateValue(xiaomiRemoteSwitchDoubleRockerConnectedStateTypeId, remoteSwitch->connected());
    }
}

void DevicePluginZigbee::deviceRemoved(Device *device)
{
    qCDebug(dcZigbee()) << "Remove device" << device->name() << device->params();

    if (device->deviceClassId() == zigbeeControllerDeviceClassId) {
        ZigbeeNetworkManager *zigbeeNetworkManager = m_zigbeeControllers.take(device);
        if (zigbeeNetworkManager) {
            zigbeeNetworkManager->deleteLater();
        }
    }

    if (device->deviceClassId() == xiaomiTemperatureHumidityDeviceClassId) {
        XiaomiTemperatureSensor *sensor = m_xiaomiTemperatureSensors.take(device);
        sensor->deleteLater();
    }

    if (device->deviceClassId() == xiaomiMagnetSensorDeviceClassId) {
        XiaomiMagnetSensor *sensor = m_xiaomiMagnetSensors.take(device);
        sensor->deleteLater();
    }

    if (device->deviceClassId() == xiaomiButtonSensorDeviceClassId) {
        XiaomiButtonSensor *sensor = m_xiaomiButtonSensors.take(device);
        sensor->deleteLater();
    }

    if (device->deviceClassId() == xiaomiMotionSensorDeviceClassId) {
        XiaomiMotionSensor *sensor = m_xiaomiMotionSensors.take(device);
        sensor->deleteLater();
    }

    if (device->deviceClassId() == xiaomiRemoteSwitchSingleRockerDeviceClassId ||
            device->deviceClassId() == xiaomiRemoteSwitchDoubleRockerDeviceClassId) {
        XiaomiRemoteSwitch *remoteSwitch = m_xiaomiRemoteSwitches.take(device);
        remoteSwitch->deleteLater();
    }
}

Device::DeviceError DevicePluginZigbee::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    Q_UNUSED(params)

    QList<DeviceDescriptor> deviceDescriptors;
    if (deviceClassId == zigbeeControllerDeviceClassId) {
        // Scan serial ports
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
            qCDebug(dcZigbee()) << "Found serial port" << info.portName();
            qCDebug(dcZigbee()) << "   Description:" << info.description();
            qCDebug(dcZigbee()) << "   System location:" << info.systemLocation();
            qCDebug(dcZigbee()) << "   Manufacturer:" << info.manufacturer();
            qCDebug(dcZigbee()) << "   Serialnumber:" << info.serialNumber();
            if (info.hasProductIdentifier()) {
                qCDebug(dcZigbee()) << "   Product identifier:" << info.productIdentifier();
            }
            if (info.hasVendorIdentifier()) {
                qCDebug(dcZigbee()) << "   Vendor identifier:" << info.vendorIdentifier();
            }

            uint baudrate = 115200;
            ParamList params;
            params.append(Param(zigbeeControllerDeviceSerialPortParamTypeId, info.systemLocation()));
            params.append(Param(zigbeeControllerDeviceBaudrateParamTypeId, baudrate));
            params.append(Param(zigbeeControllerDeviceSerialNumberParamTypeId, info.serialNumber()));

            qCDebug(dcZigbee()) << "Using baudrate param" << params.paramValue(zigbeeControllerDeviceBaudrateParamTypeId);

            DeviceDescriptor descriptor(zigbeeControllerDeviceClassId);
            //Check if this stick is already added, needed for device rediscovery e.g. when the systemlocation has changed
            foreach(Device *existingDevice, myDevices().filterByParam(zigbeeControllerDeviceSerialNumberParamTypeId, info.serialNumber())) {
                descriptor.deviceId() = existingDevice->id();
                break;
            }

            descriptor.setTitle(info.manufacturer() + " - " + info.description());
            descriptor.setDescription(info.systemLocation());
            descriptor.setParams(params);
            deviceDescriptors.append(descriptor);
        }
    }

    emit devicesDiscovered(deviceClassId, deviceDescriptors);
    return Device::DeviceErrorAsync;
}

Device::DeviceSetupStatus DevicePluginZigbee::setupDevice(Device *device)
{
    qCDebug(dcZigbee()) << "Setup device" << device->name() << device->params();

    if (device->deviceClassId() == zigbeeControllerDeviceClassId) {
        qCDebug(dcZigbee()) << "Create zigbee network manager for controller" << device;
        ZigbeeNetworkManager *zigbeeNetworkManager = new ZigbeeNetworkManager(this);
        zigbeeNetworkManager->setSerialPortName(device->paramValue(zigbeeControllerDeviceSerialPortParamTypeId).toString());
        zigbeeNetworkManager->setSerialBaudrate(static_cast<qint32>(device->paramValue(zigbeeControllerDeviceBaudrateParamTypeId).toUInt()));
        zigbeeNetworkManager->setSettingsFileName(NymeaSettings::settingsPath() + "/nymea-zigbee.conf");

        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::stateChanged, this, &DevicePluginZigbee::onZigbeeControllerStateChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::channelChanged, this, &DevicePluginZigbee::onZigbeeControllerChannelChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::extendedPanIdChanged, this, &DevicePluginZigbee::onZigbeeControllerPanIdChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::permitJoiningChanged, this, &DevicePluginZigbee::onZigbeeControllerPermitJoiningChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::nodeAdded, this, &DevicePluginZigbee::onZigbeeControllerNodeAdded);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::nodeRemoved, this, &DevicePluginZigbee::onZigbeeControllerNodeRemoved);

        m_zigbeeControllers.insert(device, zigbeeNetworkManager);

        zigbeeNetworkManager->startNetwork();
    }

    if (device->deviceClassId() == xiaomiTemperatureHumidityDeviceClassId) {
        qCDebug(dcZigbee()) << "Xiaomi temperature humidiry sensor" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiTemperatureHumidityDeviceIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(device);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);

        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return Device::DeviceSetupStatusFailure;
        }

        XiaomiTemperatureSensor *sensor = new XiaomiTemperatureSensor(node, this);
        connect(sensor, &XiaomiTemperatureSensor::connectedChanged, this, &DevicePluginZigbee::onXiaomiTemperatureSensorConnectedChanged);
        connect(sensor, &XiaomiTemperatureSensor::temperatureChanged, this, &DevicePluginZigbee::onXiaomiTemperatureSensorTemperatureChanged);
        connect(sensor, &XiaomiTemperatureSensor::humidityChanged, this, &DevicePluginZigbee::onXiaomiTemperatureSensorHumidityChanged);
        m_xiaomiTemperatureSensors.insert(device, sensor);
    }


    if (device->deviceClassId() == xiaomiMagnetSensorDeviceClassId) {
        qCDebug(dcZigbee()) << "Xiaomi magnet sensor" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiMagnetSensorDeviceIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(device);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return Device::DeviceSetupStatusFailure;
        }

        XiaomiMagnetSensor *sensor = new XiaomiMagnetSensor(node, this);
        connect(sensor, &XiaomiMagnetSensor::connectedChanged, this, &DevicePluginZigbee::onXiaomiMagnetSensorConnectedChanged);
        connect(sensor, &XiaomiMagnetSensor::closedChanged, this, &DevicePluginZigbee::onXiaomiMagnetSensorClosedChanged);

        m_xiaomiMagnetSensors.insert(device, sensor);
    }

    if (device->deviceClassId() == xiaomiButtonSensorDeviceClassId) {
        qCDebug(dcZigbee()) << "Xiaomi button sensor" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiButtonSensorDeviceIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(device);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return Device::DeviceSetupStatusFailure;
        }

        XiaomiButtonSensor *sensor = new XiaomiButtonSensor(node, this);
        connect(sensor, &XiaomiButtonSensor::connectedChanged, this, &DevicePluginZigbee::onXiaomiButtonSensorConnectedChanged);
        connect(sensor, &XiaomiButtonSensor::pressedChanged, this, &DevicePluginZigbee::onXiaomiButtonSensorPressedChanged);
        connect(sensor, &XiaomiButtonSensor::buttonPressed, this, &DevicePluginZigbee::onXiaomiButtonSensorPressed);
        connect(sensor, &XiaomiButtonSensor::buttonLongPressed, this, &DevicePluginZigbee::onXiaomiButtonSensorLongPressed);

        m_xiaomiButtonSensors.insert(device, sensor);
    }

    if (device->deviceClassId() == xiaomiMotionSensorDeviceClassId) {
        qCDebug(dcZigbee()) << "Xiaomi motion sensor" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiMotionSensorDeviceIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(device);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return Device::DeviceSetupStatusFailure;
        }

        XiaomiMotionSensor *sensor = new XiaomiMotionSensor(node, this);
        connect(sensor, &XiaomiMotionSensor::connectedChanged, this, &DevicePluginZigbee::onXiaomiMotionSensorConnectedChanged);
        connect(sensor, &XiaomiMotionSensor::presentChanged, this, &DevicePluginZigbee::onXiaomiMotionSensorPresentChanged);
        connect(sensor, &XiaomiMotionSensor::motionDetected, this, &DevicePluginZigbee::onXiaomiMotionSensorMotionDetected);

        m_xiaomiMotionSensors.insert(device, sensor);
    }

    if (device->deviceClassId() == xiaomiRemoteSwitchSingleRockerDeviceClassId) {
        qCDebug(dcZigbee()) << "Xiaomi remote switch single rocker" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiRemoteSwitchSingleRockerDeviceIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(device);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return Device::DeviceSetupStatusFailure;
        }

        XiaomiRemoteSwitch *remoteSwitch = new XiaomiRemoteSwitch(node, this);
        connect(remoteSwitch, &XiaomiRemoteSwitch::connectedChanged, this, &DevicePluginZigbee::onXiaomiRemoteSwitchConnectedChanged);
        connect(remoteSwitch, &XiaomiRemoteSwitch::pressedChanged, this, &DevicePluginZigbee::onXiaomiRemoteSwitchPressedChanged);
        connect(remoteSwitch, &XiaomiRemoteSwitch::buttonPressed, this, &DevicePluginZigbee::onXiaomiRemoteSwitchPressed);
        connect(remoteSwitch, &XiaomiRemoteSwitch::buttonLongPressed, this, &DevicePluginZigbee::onXiaomiRemoteSwitchLongPressed);

        m_xiaomiRemoteSwitches.insert(device, remoteSwitch);
    }

    if (device->deviceClassId() == xiaomiRemoteSwitchDoubleRockerDeviceClassId) {
        qCDebug(dcZigbee()) << "Xiaomi remote switch double rocker" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiRemoteSwitchDoubleRockerDeviceIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(device);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return Device::DeviceSetupStatusFailure;
        }

        XiaomiRemoteSwitch *remoteSwitch = new XiaomiRemoteSwitch(node, this);
        connect(remoteSwitch, &XiaomiRemoteSwitch::connectedChanged, this, &DevicePluginZigbee::onXiaomiRemoteSwitchConnectedChanged);
        connect(remoteSwitch, &XiaomiRemoteSwitch::pressedChanged, this, &DevicePluginZigbee::onXiaomiRemoteSwitchPressedChanged);
        connect(remoteSwitch, &XiaomiRemoteSwitch::buttonPressed, this, &DevicePluginZigbee::onXiaomiRemoteSwitchPressed);
        connect(remoteSwitch, &XiaomiRemoteSwitch::buttonLongPressed, this, &DevicePluginZigbee::onXiaomiRemoteSwitchLongPressed);

        m_xiaomiRemoteSwitches.insert(device, remoteSwitch);
    }
    return Device::DeviceSetupStatusSuccess;
}

Device::DeviceError DevicePluginZigbee::executeAction(Device *device, const Action &action)
{
    qCDebug(dcZigbee()) << "Executing action for device" << device->name() << action.actionTypeId().toString() << action.params();

    if (device->deviceClassId() == zigbeeControllerDeviceClassId) {
        ZigbeeNetworkManager *networkManager = m_zigbeeControllers.value(device);
        if (networkManager->state() != ZigbeeNetworkManager::StateRunning)
            return Device::DeviceErrorHardwareNotAvailable;

        if (action.actionTypeId() == zigbeeControllerFactoryResetActionTypeId)
            networkManager->factoryResetNetwork();

        //        if (action.actionTypeId() == zigbeeControllerTouchlinkActionTypeId)
        //            networkManager->controller()->commandInitiateTouchLink();

        //        if (action.actionTypeId() == zigbeeControllerTouchlinkResetActionTypeId)
        //            networkManager->controller()->commandTouchLinkFactoryReset();

        if (action.actionTypeId() == zigbeeControllerPermitJoinActionTypeId)
            networkManager->setPermitJoining(action.params().paramValue(zigbeeControllerPermitJoinActionPermitJoinParamTypeId).toBool());

    }

    if (device->deviceClassId() == zigbeeNodeDeviceClassId) {
        ZigbeeNetworkManager *networkManager = findParentController(device);

        if (!networkManager)
            return Device::DeviceErrorHardwareFailure;

        if (networkManager->state() != ZigbeeNetworkManager::StateRunning)
            return Device::DeviceErrorHardwareNotAvailable;

        quint16 shortAddress = static_cast<quint16>(device->paramValue(zigbeeNodeDeviceNwkAddressParamTypeId).toUInt());
        ZigbeeAddress extendedAddress = ZigbeeAddress(device->paramValue(zigbeeNodeDeviceIeeeAddressParamTypeId).toString());

        if (action.actionTypeId() == zigbeeNodeIdentifyActionTypeId) {
            qCDebug(dcZigbee()) << extendedAddress.toString();
        }

        if (action.actionTypeId() == zigbeeNodeLqiRequestActionTypeId) {
            networkManager->controller()->commandRequestLinkQuality(shortAddress);
        }
    }

    return Device::DeviceErrorNoError;
}

ZigbeeNetworkManager *DevicePluginZigbee::findParentController(Device *device) const
{
    foreach (Device *d, myDevices()) {
        if (d->deviceClassId() == zigbeeControllerDeviceClassId && d->id() == device->parentId()) {
            return m_zigbeeControllers.value(d);
        }
    }

    return nullptr;
}

ZigbeeNetworkManager *DevicePluginZigbee::findNodeController(ZigbeeNode *node) const
{
    foreach (ZigbeeNetworkManager *controller, m_zigbeeControllers.values()) {
        if (controller->nodes().contains(node)) {
            return controller;
        }
    }

    return nullptr;
}

Device * DevicePluginZigbee::findNodeDevice(ZigbeeNode *node)
{
    foreach (Device *device, myDevices()) {
        ZigbeeAddress deviceIeeeAddress;
        if (device->deviceClassId() == zigbeeNodeDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress (device->paramValue(zigbeeNodeDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiTemperatureHumidityDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress (device->paramValue(xiaomiTemperatureHumidityDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiMagnetSensorDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress (device->paramValue(xiaomiMagnetSensorDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiButtonSensorDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress (device->paramValue(xiaomiButtonSensorDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiMotionSensorDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress (device->paramValue(xiaomiMotionSensorDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiRemoteSwitchDoubleRockerDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress (device->paramValue(xiaomiRemoteSwitchSingleRockerDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiRemoteSwitchDoubleRockerDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress (device->paramValue(xiaomiRemoteSwitchDoubleRockerDeviceIeeeAddressParamTypeId).toString());
        }

        if (node->extendedAddress() == deviceIeeeAddress) {
            return device;
        }
    }

    return nullptr;
}

void DevicePluginZigbee::createDeviceForNode(Device *parentDevice, ZigbeeNode *node)
{
    // We already know this device ieee address has not already been added
    // Try to figure out which device this is from the node properties and cluster information

    if (node->hasOutputCluster(Zigbee::ClusterIdBasic)) {
        ZigbeeCluster *basicCluster = node->getOutputCluster(Zigbee::ClusterIdBasic);
        if (basicCluster->hasAttribute(Zigbee::ClusterAttributeBasicModelIdentifier)) {
            QString modelIdentifier = QString::fromUtf8(basicCluster->attribute(Zigbee::ClusterAttributeBasicModelIdentifier).data());

            qWarning(dcZigbee()) << "Model identifier" << modelIdentifier;

            // Xiaomi HT Sensor
            if (modelIdentifier.contains("lumi.sensor_ht")) {
                qCDebug(dcZigbee()) << "Xiaomi humidity/temperature sensor added";

                qCDebug(dcZigbee()) << "Output cluster:";
                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                qCDebug(dcZigbee()) << "Input cluster:";
                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                DeviceDescriptor descriptor;
                descriptor.setParentDeviceId(parentDevice->id());
                descriptor.setTitle(tr("Xiaomi temperature and humidity sensor"));

                ParamList params;
                params.append(Param(xiaomiTemperatureHumidityDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoDevicesAppeared(xiaomiTemperatureHumidityDeviceClassId, { descriptor });
                return;
            }

            // Xiaomi Magnet Sensor
            if (modelIdentifier.contains("lumi.sensor_magnet")) {
                qCDebug(dcZigbee()) << "Xiaomi magnet sensor added";

                qCDebug(dcZigbee()) << "Output cluster:";
                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                qCDebug(dcZigbee()) << "Input cluster:";
                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                DeviceDescriptor descriptor;
                descriptor.setParentDeviceId(parentDevice->id());
                descriptor.setTitle(tr("Xiaomi magnet sensor"));

                ParamList params;
                params.append(Param(xiaomiMagnetSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoDevicesAppeared(xiaomiMagnetSensorDeviceClassId, { descriptor });
                return;
            }

            // Xiaomi Button Sensor
            if (modelIdentifier.contains("lumi.sensor_switch")) {
                qCDebug(dcZigbee()) << "Xiaomi button sensor added";

                qCDebug(dcZigbee()) << "Output cluster:";
                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                qCDebug(dcZigbee()) << "Input cluster:";
                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                DeviceDescriptor descriptor;
                descriptor.setParentDeviceId(parentDevice->id());
                descriptor.setTitle(tr("Xiaomi button"));

                ParamList params;
                params.append(Param(xiaomiButtonSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoDevicesAppeared(xiaomiButtonSensorDeviceClassId, { descriptor });
                return;
            }

            // Xiaomi Motion Sensor
            if (modelIdentifier.contains("lumi.sensor_motion")) {
                qCDebug(dcZigbee()) << "Xiaomi motion sensor added";

                qCDebug(dcZigbee()) << "Output cluster:";
                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                qCDebug(dcZigbee()) << "Input cluster:";
                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                DeviceDescriptor descriptor;
                descriptor.setParentDeviceId(parentDevice->id());
                descriptor.setTitle(tr("Xiaomi motion sensor"));

                ParamList params;
                params.append(Param(xiaomiMotionSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoDevicesAppeared(xiaomiMotionSensorDeviceClassId, { descriptor });
                return;
            }

            // Xiaomi Remote Switch Single Rocker
            if (modelIdentifier.contains("lumi.remote.b1")) {
                qCDebug(dcZigbee()) << "Xiaomi remote switch single rocker added";

                qCDebug(dcZigbee()) << "Output cluster:";
                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                qCDebug(dcZigbee()) << "Input cluster:";
                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                DeviceDescriptor descriptor;
                descriptor.setParentDeviceId(parentDevice->id());
                descriptor.setTitle(tr("Xiaomi remote switch single rocker"));

                ParamList params;
                params.append(Param(xiaomiRemoteSwitchSingleRockerDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoDevicesAppeared(xiaomiRemoteSwitchSingleRockerDeviceClassId, { descriptor });
                return;
            }

            // Xiaomi Remote Switch Double Rocker
            if (modelIdentifier.contains("lumi.remote.b2")) {
                qCDebug(dcZigbee()) << "Xiaomi remote switch double rocker added";

                qCDebug(dcZigbee()) << "Output cluster:";
                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                qCDebug(dcZigbee()) << "Input cluster:";
                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
                    qCDebug(dcZigbee()) << "    " << cluster;
                }

                DeviceDescriptor descriptor;
                descriptor.setParentDeviceId(parentDevice->id());
                descriptor.setTitle(tr("Xiaomi remote switch"));

                ParamList params;
                params.append(Param(xiaomiRemoteSwitchDoubleRockerDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoDevicesAppeared(xiaomiRemoteSwitchDoubleRockerDeviceClassId, { descriptor });
                return;
            }
        }
    }

    // If nothing recognized this device, create the generic node device
    //createGenericNodeDeviceForNode(parentDevice, node);
}

void DevicePluginZigbee::createGenericNodeDeviceForNode(Device *parentDevice, ZigbeeNode *node)
{
    DeviceDescriptor descriptor;
    descriptor.setParentDeviceId(parentDevice->id());

    if (node->shortAddress() == 0) {
        descriptor.setTitle("Zigbee node (coordinator)");
    } else {
        descriptor.setTitle("Zigbee node");
    }

    ParamList params;
    params.append(Param(zigbeeNodeDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
    params.append(Param(zigbeeNodeDeviceNwkAddressParamTypeId, QVariant::fromValue(node->shortAddress())));
    descriptor.setParams(params);

    emit autoDevicesAppeared(zigbeeNodeDeviceClassId, { descriptor });
}

void DevicePluginZigbee::onZigbeeControllerStateChanged(ZigbeeNetwork::State state)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Device *controllerDevice = m_zigbeeControllers.key(zigbeeNetworkManager);
    if (!controllerDevice) return;

    qCDebug(dcZigbee()) << "Controller state changed" << state << controllerDevice;

    switch (state) {
    case ZigbeeNetwork::StateUninitialized:
        break;
    case ZigbeeNetwork::StateDisconnected:
        controllerDevice->setStateValue(zigbeeControllerConnectedStateTypeId, false);
        break;
    case ZigbeeNetwork::StateRunning:
        controllerDevice->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        controllerDevice->setStateValue(zigbeeControllerVersionStateTypeId, zigbeeNetworkManager->controllerFirmwareVersion());
        controllerDevice->setStateValue(zigbeeControllerPanIdStateTypeId, zigbeeNetworkManager->extendedPanId());
        controllerDevice->setStateValue(zigbeeControllerChannelStateTypeId, zigbeeNetworkManager->channel());
        controllerDevice->setStateValue(zigbeeControllerPermitJoinStateTypeId, zigbeeNetworkManager->permitJoining());
        controllerDevice->setStateValue(zigbeeControllerIeeeAddressStateTypeId, zigbeeNetworkManager->coordinatorNode()->extendedAddress().toString());

        // Initalize nodes
        foreach (ZigbeeNode *node, zigbeeNetworkManager->nodes()) {
            Device *nodeDevice = findNodeDevice(node);
            if (nodeDevice) {
                qCDebug(dcZigbee()) << "Devices for" << node << "already created." << nodeDevice;
                break;
            }
            createDeviceForNode(controllerDevice, node);
        }

        break;
    case ZigbeeNetwork::StateStarting:
        //device->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        break;
    case ZigbeeNetwork::StateStopping:
        //device->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        break;
    }

}

void DevicePluginZigbee::onZigbeeControllerChannelChanged(uint channel)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Device *device = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << "Zigbee channel changed" << channel << device;
    device->setStateValue(zigbeeControllerChannelStateTypeId, channel);
}

void DevicePluginZigbee::onZigbeeControllerPanIdChanged(quint64 extendedPanId)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Device *device = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << "Zigbee extended PAN id changed" << extendedPanId << device;
    device->setStateValue(zigbeeControllerPanIdStateTypeId, extendedPanId);
}

void DevicePluginZigbee::onZigbeeControllerPermitJoiningChanged(bool permitJoining)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Device *device = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << device << "permit joining changed" << permitJoining;
    device->setStateValue(zigbeeControllerPermitJoinStateTypeId, permitJoining);
}

void DevicePluginZigbee::onZigbeeControllerNodeAdded(ZigbeeNode *node)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Device *device = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) <<  device << "node added" << device << node;

    if (findNodeDevice(node)) {
        qCDebug(dcZigbee()) << "Device for" << node << "already created." << device;
        return;
    }

    createDeviceForNode(device, node);
}

void DevicePluginZigbee::onZigbeeControllerNodeRemoved(ZigbeeNode *node)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Device *device = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << device << "node removed" << node;
    Device * nodeDevice = findNodeDevice(node);
    if (!nodeDevice) {
        qCWarning(dcZigbee()) << "There is no nymea device for this node" << node;
        return;
    }

    emit autoDeviceDisappeared(nodeDevice->id());
}

void DevicePluginZigbee::onXiaomiTemperatureSensorConnectedChanged(bool connected)
{
    XiaomiTemperatureSensor *sensor = static_cast<XiaomiTemperatureSensor *>(sender());
    Device *device = m_xiaomiTemperatureSensors.key(sensor);
    device->setStateValue(xiaomiTemperatureHumidityConnectedStateTypeId, connected);
}

void DevicePluginZigbee::onXiaomiTemperatureSensorTemperatureChanged(double temperature)
{
    XiaomiTemperatureSensor *sensor = static_cast<XiaomiTemperatureSensor *>(sender());
    Device *device = m_xiaomiTemperatureSensors.key(sensor);
    device->setStateValue(xiaomiTemperatureHumidityTemperatureStateTypeId, temperature);
    qCDebug(dcZigbee()) << device << "temperature changed" << temperature << "°C";
}

void DevicePluginZigbee::onXiaomiTemperatureSensorHumidityChanged(double humidity)
{
    XiaomiTemperatureSensor *sensor = static_cast<XiaomiTemperatureSensor *>(sender());
    Device *device = m_xiaomiTemperatureSensors.key(sensor);
    device->setStateValue(xiaomiTemperatureHumidityHumidityStateTypeId, humidity);
    qCDebug(dcZigbee()) << device << "humidity changed" << humidity << "%";
}

void DevicePluginZigbee::onXiaomiMagnetSensorConnectedChanged(bool connected)
{
    XiaomiMagnetSensor *sensor = static_cast<XiaomiMagnetSensor *>(sender());
    Device *device = m_xiaomiMagnetSensors.key(sensor);
    device->setStateValue(xiaomiMagnetSensorConnectedStateTypeId, connected);
}

void DevicePluginZigbee::onXiaomiMagnetSensorClosedChanged(bool closed)
{
    XiaomiMagnetSensor *sensor = static_cast<XiaomiMagnetSensor *>(sender());
    Device *device = m_xiaomiMagnetSensors.key(sensor);
    device->setStateValue(xiaomiMagnetSensorClosedStateTypeId, closed);
    qCDebug(dcZigbee()) << device << (closed ? "closed" : "opened");
}

void DevicePluginZigbee::onXiaomiButtonSensorConnectedChanged(bool connected)
{
    XiaomiButtonSensor *sensor = static_cast<XiaomiButtonSensor *>(sender());
    Device *device = m_xiaomiButtonSensors.key(sensor);
    device->setStateValue(xiaomiButtonSensorConnectedStateTypeId, connected);
}

void DevicePluginZigbee::onXiaomiButtonSensorPressedChanged(bool pressed)
{
    XiaomiButtonSensor *sensor = static_cast<XiaomiButtonSensor *>(sender());
    Device *device = m_xiaomiButtonSensors.key(sensor);
    //device->setStateValue(xiaomiButtonSensorPressedStateTypeId, pressed);
    qCDebug(dcZigbee()) << device << "Button" << (pressed ? "pressed" : "released");
}

void DevicePluginZigbee::onXiaomiButtonSensorPressed()
{
    XiaomiButtonSensor *sensor = static_cast<XiaomiButtonSensor *>(sender());
    Device *device = m_xiaomiButtonSensors.key(sensor);
    emitEvent(Event(xiaomiButtonSensorPressedEventTypeId, device->id()));
    qCDebug(dcZigbee()) << device << "Button clicked";
}

void DevicePluginZigbee::onXiaomiButtonSensorLongPressed()
{
    XiaomiButtonSensor *sensor = static_cast<XiaomiButtonSensor *>(sender());
    Device *device = m_xiaomiButtonSensors.key(sensor);
    emitEvent(Event(xiaomiButtonSensorLongPressedEventTypeId, device->id()));
    qCDebug(dcZigbee()) << device << "Button long pressed";
}

void DevicePluginZigbee::onXiaomiMotionSensorConnectedChanged(bool connected)
{
    XiaomiMotionSensor *sensor = static_cast<XiaomiMotionSensor *>(sender());
    Device *device = m_xiaomiMotionSensors.key(sensor);
    device->setStateValue(xiaomiMotionSensorConnectedStateTypeId, connected);
}

void DevicePluginZigbee::onXiaomiMotionSensorPresentChanged(bool present)
{
    XiaomiMotionSensor *sensor = static_cast<XiaomiMotionSensor *>(sender());
    Device *device = m_xiaomiMotionSensors.key(sensor);
    device->setStateValue(xiaomiMotionSensorIsPresentStateTypeId, present);
    qCDebug(dcZigbee()) << device << "present changed" << present;
}

void DevicePluginZigbee::onXiaomiMotionSensorMotionDetected()
{
    XiaomiMotionSensor *sensor = static_cast<XiaomiMotionSensor *>(sender());
    Device *device = m_xiaomiMotionSensors.key(sensor);
    device->setStateValue(xiaomiMotionSensorLastSeenTimeStateTypeId, QDateTime::currentDateTimeUtc().toTime_t());
    qCDebug(dcZigbee()) << device << "motion detected" << QDateTime::currentDateTimeUtc().toTime_t();
}

void DevicePluginZigbee::onXiaomiRemoteSwitchConnectedChanged(bool connected)
{
    XiaomiRemoteSwitch *remoteSwitch = static_cast<XiaomiRemoteSwitch *>(sender());
    Device *device = m_xiaomiRemoteSwitches.key(remoteSwitch);
    if (device->deviceClassId() == xiaomiRemoteSwitchSingleRockerDeviceClassId) {
        device->setStateValue(xiaomiRemoteSwitchSingleRockerConnectedStateTypeId, connected);
    }

    if (device->deviceClassId() == xiaomiRemoteSwitchDoubleRockerDeviceClassId) {
        device->setStateValue(xiaomiRemoteSwitchDoubleRockerConnectedStateTypeId, connected);
    }
}

void DevicePluginZigbee::onXiaomiRemoteSwitchPressedChanged(bool pressed)
{
    XiaomiRemoteSwitch *sensor = static_cast<XiaomiRemoteSwitch *>(sender());
    Device *device = m_xiaomiRemoteSwitches.key(sensor);
    qCDebug(dcZigbee()) << device << "Remote Switch" << (pressed ? "pressed" : "released") << "Button:";
}

void DevicePluginZigbee::onXiaomiRemoteSwitchPressed()
{
    XiaomiRemoteSwitch *sensor = static_cast<XiaomiRemoteSwitch *>(sender());
    Device *device = m_xiaomiRemoteSwitches.key(sensor);
    if (device->deviceClassId() == xiaomiRemoteSwitchSingleRockerDeviceClassId) {
        emitEvent(Event(xiaomiRemoteSwitchSingleRockerPressedEventTypeId, device->id()));
    }

    if (device->deviceClassId() == xiaomiRemoteSwitchDoubleRockerDeviceClassId) {
        if (true) { //TODO
            emitEvent(Event(xiaomiRemoteSwitchDoubleRockerPressedEventTypeId, device->id(), ParamList() << Param(xiaomiRemoteSwitchDoubleRockerPressedEventButtonNameParamTypeId, "left")));
        }else {
            emitEvent(Event(xiaomiRemoteSwitchDoubleRockerPressedEventTypeId, device->id(), ParamList() << Param(xiaomiRemoteSwitchDoubleRockerPressedEventButtonNameParamTypeId, "right")));
        }
    }
    qCDebug(dcZigbee()) << device << "remote switch pressed";
}

void DevicePluginZigbee::onXiaomiRemoteSwitchLongPressed()
{
    XiaomiRemoteSwitch *remoteSwitch = static_cast<XiaomiRemoteSwitch *>(sender());
    Device *device = m_xiaomiRemoteSwitches.key(remoteSwitch);
    if (device->deviceClassId() == xiaomiRemoteSwitchSingleRockerDeviceClassId) {
        emitEvent(Event(xiaomiRemoteSwitchSingleRockerLongPressedEventTypeId, device->id()));
    }

    if (device->deviceClassId() == xiaomiRemoteSwitchDoubleRockerDeviceClassId) {
        if (true) { //TODO
            emitEvent(Event(xiaomiRemoteSwitchDoubleRockerLongPressedEventTypeId, device->id(), ParamList() << Param(xiaomiRemoteSwitchDoubleRockerLongPressedEventButtonNameParamTypeId, "left")));
        }else {
            emitEvent(Event(xiaomiRemoteSwitchDoubleRockerLongPressedEventTypeId, device->id(), ParamList() << Param(xiaomiRemoteSwitchDoubleRockerLongPressedEventButtonNameParamTypeId, "right")));
        }
    }
    qCDebug(dcZigbee()) << device << "remote switch long pressed";
}
