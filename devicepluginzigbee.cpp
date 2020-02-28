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

    if (device->deviceClassId() == zigbeeControllerDeviceClassId) {
        ZigbeeNetwork *zigbeeNetwork = m_zigbeeNetworks.value(device);
        zigbeeNetwork->startNetwork();
    }

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
}

void DevicePluginZigbee::deviceRemoved(Device *device)
{
    qCDebug(dcZigbee()) << "Remove device" << device->name() << device->params();

    if (device->deviceClassId() == zigbeeControllerDeviceClassId) {
        ZigbeeNetwork *zigbeeNetwork = m_zigbeeNetworks.take(device);
        if (zigbeeNetwork) {
            zigbeeNetwork->deleteLater();
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
}

void DevicePluginZigbee::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() == zigbeeControllerDeviceClassId) {
        // Scan serial ports
        foreach (const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()) {
            qCDebug(dcZigbee()) << "Found serial port" << serialPortInfo.portName();
            qCDebug(dcZigbee()) << "   Description:" << serialPortInfo.description();
            qCDebug(dcZigbee()) << "   System location:" << serialPortInfo.systemLocation();
            qCDebug(dcZigbee()) << "   Manufacturer:" << serialPortInfo.manufacturer();
            qCDebug(dcZigbee()) << "   Serialnumber:" << serialPortInfo.serialNumber();
            if (serialPortInfo.hasProductIdentifier()) {
                qCDebug(dcZigbee()) << "   Product identifier:" << serialPortInfo.productIdentifier();
            }
            if (serialPortInfo.hasVendorIdentifier()) {
                qCDebug(dcZigbee()) << "   Vendor identifier:" << serialPortInfo.vendorIdentifier();
            }

            uint baudrate = 115200;
            ParamList params;
            params.append(Param(zigbeeControllerDeviceSerialPortParamTypeId, serialPortInfo.systemLocation()));
            params.append(Param(zigbeeControllerDeviceBaudrateParamTypeId, baudrate));

            qCDebug(dcZigbee()) << "Using baudrate param" << params.paramValue(zigbeeControllerDeviceBaudrateParamTypeId);

            DeviceDescriptor descriptor(zigbeeControllerDeviceClassId);
            descriptor.setTitle(serialPortInfo.manufacturer() + " - " + serialPortInfo.description());
            descriptor.setDescription(serialPortInfo.systemLocation());
            descriptor.setParams(params);
            info->addDeviceDescriptor(descriptor);
        }
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginZigbee::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    qCDebug(dcZigbee()) << "Setup device" << device->name() << device->params();

    if (device->deviceClassId() == zigbeeControllerDeviceClassId) {
        qCDebug(dcZigbee()) << "Create zigbee network manager for controller" << device;

        QString serialPortName = device->paramValue(zigbeeControllerDeviceSerialPortParamTypeId).toString();
        qint32 baudrate = static_cast<qint32>(device->paramValue(zigbeeControllerDeviceBaudrateParamTypeId).toUInt());

        ZigbeeNetwork *zigbeeNetwork = ZigbeeNetworkManager::createZigbeeNetwork(ZigbeeNetworkManager::BackendTypeNxp, this);
        zigbeeNetwork->setSettingsFileName(NymeaSettings::settingsPath() + "/nymea-zigbee.conf");
        zigbeeNetwork->setSerialPortName(serialPortName);
        zigbeeNetwork->setSerialBaudrate(baudrate);

        connect(zigbeeNetwork, &ZigbeeNetwork::stateChanged, this, &DevicePluginZigbee::onZigbeeNetworkStateChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::channelChanged, this, &DevicePluginZigbee::onZigbeeNetworkChannelChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::extendedPanIdChanged, this, &DevicePluginZigbee::onZigbeeNetworkPanIdChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::permitJoiningChanged, this, &DevicePluginZigbee::onZigbeeNetworkPermitJoiningChanged);
//        connect(zigbeeNetworkManager->network(), &ZigbeeNetwork::nodeAdded, this, &DevicePluginZigbee::onZigbeeControllerNodeAdded);
//        connect(zigbeeNetworkManager->network(), &ZigbeeNetwork::nodeRemoved, this, &DevicePluginZigbee::onZigbeeControllerNodeRemoved);

        m_zigbeeNetworks.insert(device, zigbeeNetwork);
    }

    if (device->deviceClassId() == xiaomiTemperatureHumidityDeviceClassId) {
//        qCDebug(dcZigbee()) << "Xiaomi temperature humidiry sensor" << device;
//        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiTemperatureHumidityDeviceIeeeAddressParamTypeId).toString());
//        // Get the parent controller and node for this device
//        ZigbeeNetworkManager *zigbeeNetworkManager = findParentNetwork(device);
//        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);

//        if (!node) {
//            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
//            return info->finish(Device::DeviceErrorSetupFailed);
//        }

//        XiaomiTemperatureSensor *sensor = new XiaomiTemperatureSensor(node, this);
//        connect(sensor, &XiaomiTemperatureSensor::connectedChanged, this, &DevicePluginZigbee::onXiaomiTemperatureSensorConnectedChanged);
//        connect(sensor, &XiaomiTemperatureSensor::temperatureChanged, this, &DevicePluginZigbee::onXiaomiTemperatureSensorTemperatureChanged);
//        connect(sensor, &XiaomiTemperatureSensor::humidityChanged, this, &DevicePluginZigbee::onXiaomiTemperatureSensorHumidityChanged);
//        m_xiaomiTemperatureSensors.insert(device, sensor);
    }


    if (device->deviceClassId() == xiaomiMagnetSensorDeviceClassId) {
//        qCDebug(dcZigbee()) << "Xiaomi magnet sensor" << device;
//        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiMagnetSensorDeviceIeeeAddressParamTypeId).toString());
//        // Get the parent controller and node for this device
//        ZigbeeNetworkManager *zigbeeNetworkManager = findParentNetwork(device);
//        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
//        if (!node) {
//            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
//            return info->finish(Device::DeviceErrorSetupFailed);
//        }

//        XiaomiMagnetSensor *sensor = new XiaomiMagnetSensor(node, this);
//        connect(sensor, &XiaomiMagnetSensor::connectedChanged, this, &DevicePluginZigbee::onXiaomiMagnetSensorConnectedChanged);
//        connect(sensor, &XiaomiMagnetSensor::closedChanged, this, &DevicePluginZigbee::onXiaomiMagnetSensorClosedChanged);

//        m_xiaomiMagnetSensors.insert(device, sensor);
    }

    if (device->deviceClassId() == xiaomiButtonSensorDeviceClassId) {
//        qCDebug(dcZigbee()) << "Xiaomi button sensor" << device;
//        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiButtonSensorDeviceIeeeAddressParamTypeId).toString());
//        // Get the parent controller and node for this device
//        ZigbeeNetworkManager *zigbeeNetworkManager = findParentNetwork(device);
//        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
//        if (!node) {
//            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
//            return info->finish(Device::DeviceErrorSetupFailed);
//        }

//        XiaomiButtonSensor *sensor = new XiaomiButtonSensor(node, this);
//        connect(sensor, &XiaomiButtonSensor::connectedChanged, this, &DevicePluginZigbee::onXiaomiButtonSensorConnectedChanged);
//        connect(sensor, &XiaomiButtonSensor::pressedChanged, this, &DevicePluginZigbee::onXiaomiButtonSensorPressedChanged);
//        connect(sensor, &XiaomiButtonSensor::buttonPressed, this, &DevicePluginZigbee::onXiaomiButtonSensorPressed);
//        connect(sensor, &XiaomiButtonSensor::buttonLongPressed, this, &DevicePluginZigbee::onXiaomiButtonSensorLongPressed);

//        m_xiaomiButtonSensors.insert(device, sensor);
    }

    if (device->deviceClassId() == xiaomiMotionSensorDeviceClassId) {
//        qCDebug(dcZigbee()) << "Xiaomi motion sensor" << device;
//        ZigbeeAddress ieeeAddress(device->paramValue(xiaomiMotionSensorDeviceIeeeAddressParamTypeId).toString());
//        // Get the parent controller and node for this device
//        ZigbeeNetworkManager *zigbeeNetworkManager = findParentNetwork(device);
//        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
//        if (!node) {
//            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
//            return info->finish(Device::DeviceErrorSetupFailed);
//        }

//        XiaomiMotionSensor *sensor = new XiaomiMotionSensor(node, this);
//        connect(sensor, &XiaomiMotionSensor::connectedChanged, this, &DevicePluginZigbee::onXiaomiMotionSensorConnectedChanged);
//        connect(sensor, &XiaomiMotionSensor::presentChanged, this, &DevicePluginZigbee::onXiaomiMotionSensorPresentChanged);
//        connect(sensor, &XiaomiMotionSensor::motionDetected, this, &DevicePluginZigbee::onXiaomiMotionSensorMotionDetected);

//        m_xiaomiMotionSensors.insert(device, sensor);
    }

    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginZigbee::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    qCDebug(dcZigbee()) << "Executing action for device" << device->name() << action.actionTypeId().toString() << action.params();
    if (device->deviceClassId() == zigbeeControllerDeviceClassId) {
        ZigbeeNetwork *zigbeeNetwork = m_zigbeeNetworks.value(device);

        if (action.actionTypeId() == zigbeeControllerFactoryResetActionTypeId)
            zigbeeNetwork->factoryResetNetwork();


//        if (action.actionTypeId() == zigbeeControllerResetActionTypeId)
//            networkManager->resetController();


//        if (action.actionTypeId() == zigbeeControllerTouchlinkActionTypeId)
//            networkManager->controller()->commandInitiateTouchLink();

//        if (action.actionTypeId() == zigbeeControllerTouchlinkResetActionTypeId)
//            networkManager->controller()->commandTouchLinkFactoryReset();

        if (zigbeeNetwork->state() != ZigbeeNetwork::StateRunning)
            return info->finish(Device::DeviceErrorHardwareNotAvailable);

        if (action.actionTypeId() == zigbeeControllerPermitJoinActionTypeId)
            zigbeeNetwork->setPermitJoining(action.params().paramValue(zigbeeControllerPermitJoinActionPermitJoinParamTypeId).toBool());

    }

    if (device->deviceClassId() == zigbeeNodeDeviceClassId) {
//        ZigbeeNetworkManager *networkManager = findParentNetwork(device);

//        if (!networkManager)
//            return info->finish(Device::DeviceErrorHardwareFailure);

//        if (networkManager->state() != ZigbeeNetworkManager::StateRunning)
//            return info->finish(Device::DeviceErrorHardwareNotAvailable);

//        quint16 shortAddress = static_cast<quint16>(device->paramValue(zigbeeNodeDeviceNwkAddressParamTypeId).toUInt());
//        ZigbeeAddress extendedAddress = ZigbeeAddress(device->paramValue(zigbeeNodeDeviceIeeeAddressParamTypeId).toString());

//        if (action.actionTypeId() == zigbeeNodeIdentifyActionTypeId) {
//            qCDebug(dcZigbee()) << extendedAddress.toString();
//        }

//        if (action.actionTypeId() == zigbeeNodeLqiRequestActionTypeId) {
//            networkManager->controller()->commandRequestLinkQuality(shortAddress);
//        }
    }

    return info->finish(Device::DeviceErrorNoError);
}

ZigbeeNetwork *DevicePluginZigbee::findParentNetwork(Device *device) const
{
    foreach (Device *d, myDevices()) {
        if (d->deviceClassId() == zigbeeControllerDeviceClassId && d->id() == device->parentId()) {
            return m_zigbeeNetworks.value(d);
        }
    }

    return nullptr;
}

ZigbeeNetworkManager *DevicePluginZigbee::findNodeController(ZigbeeNode *node) const
{
    Q_UNUSED(node)
//    foreach (ZigbeeNetworkManager *controller, m_zigbeeControllers.values()) {
//        if (controller->nodes().contains(node)) {
//            return controller;
//        }
//    }

    return nullptr;
}

Device * DevicePluginZigbee::findNodeDevice(ZigbeeNode *node)
{
    foreach (Device *device, myDevices()) {
        ZigbeeAddress deviceIeeeAddress;
        if (device->deviceClassId() == zigbeeNodeDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress(device->paramValue(zigbeeNodeDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiTemperatureHumidityDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress(device->paramValue(xiaomiTemperatureHumidityDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiMagnetSensorDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress(device->paramValue(xiaomiMagnetSensorDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiButtonSensorDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress(device->paramValue(xiaomiButtonSensorDeviceIeeeAddressParamTypeId).toString());
        }

        if (device->deviceClassId() == xiaomiMotionSensorDeviceClassId) {
            deviceIeeeAddress = ZigbeeAddress(device->paramValue(xiaomiMotionSensorDeviceIeeeAddressParamTypeId).toString());
        }

        if (node->extendedAddress() == deviceIeeeAddress) {
            return device;
        }
    }

    return nullptr;
}

void DevicePluginZigbee::createDeviceForNode(Device *parentDevice, ZigbeeNode *node)
{
    Q_UNUSED(parentDevice)
    Q_UNUSED(node)
    // We already know this device ieee address has not already been added
    // Try to figure out which device this is from the node properties and cluster information

//    if (node->hasOutputCluster(Zigbee::ClusterIdBasic)) {
//        ZigbeeCluster *basicCluster = node->getOutputCluster(Zigbee::ClusterIdBasic);
//        if (basicCluster->hasAttribute(Zigbee::ClusterAttributeBasicModelIdentifier)) {
//            QString modelIdentifier = QString::fromUtf8(basicCluster->attribute(Zigbee::ClusterAttributeBasicModelIdentifier).data());

//            // Xiaomi HT Sensor
//            if (modelIdentifier.contains("lumi.sensor_ht")) {
//                qCDebug(dcZigbee()) << "Xiaomi humidity/temperature sensor added";

//                qCDebug(dcZigbee()) << "Output cluster:";
//                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
//                    qCDebug(dcZigbee()) << "    " << cluster;
//                }

//                qCDebug(dcZigbee()) << "Input cluster:";
//                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
//                    qCDebug(dcZigbee()) << "    " << cluster;
//                }

//                DeviceDescriptor descriptor(xiaomiTemperatureHumidityDeviceClassId);
//                descriptor.setParentDeviceId(parentDevice->id());
//                descriptor.setTitle(tr("Xiaomi temperature and humidity sensor"));

//                ParamList params;
//                params.append(Param(xiaomiTemperatureHumidityDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
//                descriptor.setParams(params);

//                emit autoDevicesAppeared({ descriptor });
//                return;
//            }

//            // Xiaomi Magnet Sensor
//            if (modelIdentifier.contains("lumi.sensor_magnet")) {
//                qCDebug(dcZigbee()) << "Xiaomi magnet sensor added";

//                qCDebug(dcZigbee()) << "Output cluster:";
//                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
//                    qCDebug(dcZigbee()) << "    " << cluster;
//                }

//                qCDebug(dcZigbee()) << "Input cluster:";
//                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
//                    qCDebug(dcZigbee()) << "    " << cluster;
//                }

//                DeviceDescriptor descriptor(xiaomiMagnetSensorDeviceClassId);
//                descriptor.setParentDeviceId(parentDevice->id());
//                descriptor.setTitle(tr("Xiaomi magnet sensor"));

//                ParamList params;
//                params.append(Param(xiaomiMagnetSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
//                descriptor.setParams(params);

//                emit autoDevicesAppeared({ descriptor });
//                return;
//            }

//            // Xiaomi Button Sensor
//            if (modelIdentifier.contains("lumi.sensor_switch")) {
//                qCDebug(dcZigbee()) << "Xiaomi button sensor added";

//                qCDebug(dcZigbee()) << "Output cluster:";
//                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
//                    qCDebug(dcZigbee()) << "    " << cluster;
//                }

//                qCDebug(dcZigbee()) << "Input cluster:";
//                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
//                    qCDebug(dcZigbee()) << "    " << cluster;
//                }

//                DeviceDescriptor descriptor(xiaomiButtonSensorDeviceClassId);
//                descriptor.setParentDeviceId(parentDevice->id());
//                descriptor.setTitle(tr("Xiaomi button"));

//                ParamList params;
//                params.append(Param(xiaomiButtonSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
//                descriptor.setParams(params);

//                emit autoDevicesAppeared({ descriptor });
//                return;
//            }

//            // Xiaomi Motion Sensor
//            if (modelIdentifier.contains("lumi.sensor_motion")) {
//                qCDebug(dcZigbee()) << "Xiaomi motion sensor added";

//                qCDebug(dcZigbee()) << "Output cluster:";
//                foreach (ZigbeeCluster *cluster, node->outputClusters()) {
//                    qCDebug(dcZigbee()) << "    " << cluster;
//                }

//                qCDebug(dcZigbee()) << "Input cluster:";
//                foreach (ZigbeeCluster *cluster, node->inputClusters()) {
//                    qCDebug(dcZigbee()) << "    " << cluster;
//                }

//                DeviceDescriptor descriptor(xiaomiMotionSensorDeviceClassId);
//                descriptor.setParentDeviceId(parentDevice->id());
//                descriptor.setTitle(tr("Xiaomi motion sensor"));

//                ParamList params;
//                params.append(Param(xiaomiMotionSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
//                descriptor.setParams(params);

//                emit autoDevicesAppeared({ descriptor });
//                return;
//            }
//        }
//    }

//    qCDebug(dcZigbee()) << "Unhandled zigbee node added. Create a generic node device for it";

//    // If nothing recognized this device, create the generic node device
//    createGenericNodeDeviceForNode(parentDevice, node);
}

void DevicePluginZigbee::createGenericNodeDeviceForNode(Device *parentDevice, ZigbeeNode *node)
{
    if (!parentDevice) {
        qCWarning(dcZigbee()) << "No parent device passed";
        return;
    }

    if (!node) {
        qCWarning(dcZigbee()) << "No node passed";
        return;
    }

    DeviceDescriptor descriptor(zigbeeNodeDeviceClassId);
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

    emit autoDevicesAppeared({ descriptor });
}

void DevicePluginZigbee::onZigbeeNetworkStateChanged(ZigbeeNetwork::State state)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Device *device = m_zigbeeNetworks.key(zigbeeNetwork);
    if (!device) return;

    qCDebug(dcZigbee()) << "Controller state changed" << state << device;

    switch (state) {
    case ZigbeeNetwork::StateUninitialized:
        break;
    case ZigbeeNetwork::StateOffline:
        device->setStateValue(zigbeeControllerConnectedStateTypeId, false);
        break;
    case ZigbeeNetwork::StateRunning:
        device->setStateValue(zigbeeControllerConnectedStateTypeId, true);
//        device->setStateValue(zigbeeControllerVersionStateTypeId, zigbeeNetworkManager->controllerFirmwareVersion());
//        device->setStateValue(zigbeeControllerPanIdStateTypeId, zigbeeNetworkManager->extendedPanId());
//        device->setStateValue(zigbeeControllerChannelStateTypeId, zigbeeNetworkManager->channel());
//        device->setStateValue(zigbeeControllerPermitJoinStateTypeId, zigbeeNetworkManager->permitJoining());
//        device->setStateValue(zigbeeControllerIeeeAddressStateTypeId, zigbeeNetworkManager->coordinatorNode()->extendedAddress().toString());

//        // Initalize nodes
//        foreach (ZigbeeNode *node, zigbeeNetworkManager->nodes()) {
//            Device *device = findNodeDevice(node);
//            if (device) {
//                qCDebug(dcZigbee()) << "Devices for" << node << "already created." << device;
//                break;
//            }

//            createDeviceForNode(device, node);
//        }

        break;
    case ZigbeeNetwork::StateStarting:
        //device->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        break;
    case ZigbeeNetwork::StateStopping:
        //device->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        break;
    }

}

void DevicePluginZigbee::onZigbeeNetworkChannelChanged(uint channel)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Device *device = m_zigbeeNetworks.key(zigbeeNetwork);
    qCDebug(dcZigbee()) << "Zigbee network channel changed" << channel << device;
    device->setStateValue(zigbeeControllerChannelStateTypeId, channel);
}

void DevicePluginZigbee::onZigbeeNetworkPanIdChanged(quint64 extendedPanId)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Device *device = m_zigbeeNetworks.key(zigbeeNetwork);
    qCDebug(dcZigbee()) << "Zigbee network PAN id changed" << extendedPanId << device;
    device->setStateValue(zigbeeControllerPanIdStateTypeId, extendedPanId);
}

void DevicePluginZigbee::onZigbeeNetworkPermitJoiningChanged(bool permitJoining)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Device *device = m_zigbeeNetworks.key(zigbeeNetwork);
    qCDebug(dcZigbee()) << device << "permit joining changed" << permitJoining;
    device->setStateValue(zigbeeControllerPermitJoinStateTypeId, permitJoining);
}

void DevicePluginZigbee::onZigbeeNetworkNodeAdded(ZigbeeNode *node)
{
    Q_UNUSED(node)
//    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
//    Device *device = m_zigbeeControllers.key(zigbeeNetworkManager);
//    qCDebug(dcZigbee()) <<  device << "node added" << device << node;

//    if (findNodeDevice(node)) {
//        qCDebug(dcZigbee()) << "Device for" << node << "already created." << device;
//        return;
//    }

//    createDeviceForNode(device, node);
}

void DevicePluginZigbee::onZigbeeNetworkNodeRemoved(ZigbeeNode *node)
{
    Q_UNUSED(node)
//    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
//    Device *device = m_zigbeeControllers.key(zigbeeNetworkManager);
//    qCDebug(dcZigbee()) << device << "node removed" << node;
//    Device * nodeDevice = findNodeDevice(node);
//    if (!nodeDevice) {
//        qCWarning(dcZigbee()) << "There is no nymea device for this node" << node;
//        return;
//    }

//    emit autoDeviceDisappeared(nodeDevice->id());
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
