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

#include "nymea-zigbee/zigbeeutils.h"
#include "nymea-zigbee/zigbeenetworkkey.h"

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

    if (m_zigbeeDevices.contains(device)) {
        ZigbeeDevice *zigbeeDevice = m_zigbeeDevices.value(device);
        zigbeeDevice->checkOnlineStatus();
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
    } else {
        ZigbeeDevice *zigbeeDevice = m_zigbeeDevices.take(device);
        if (zigbeeDevice)
            delete zigbeeDevice;
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

        connect(zigbeeNetwork->bridgeController(), &ZigbeeBridgeController::firmwareVersionChanged, this, [device](const QString &firmwareVersion){
            device->setStateValue(zigbeeControllerVersionStateTypeId, firmwareVersion);
        });

        connect(zigbeeNetwork, &ZigbeeNetwork::stateChanged, this, &DevicePluginZigbee::onZigbeeNetworkStateChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::channelChanged, this, &DevicePluginZigbee::onZigbeeNetworkChannelChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::extendedPanIdChanged, this, &DevicePluginZigbee::onZigbeeNetworkPanIdChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::permitJoiningChanged, this, &DevicePluginZigbee::onZigbeeNetworkPermitJoiningChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::nodeAdded, this, &DevicePluginZigbee::onZigbeeNetworkNodeAdded);
        connect(zigbeeNetwork, &ZigbeeNetwork::nodeRemoved, this, &DevicePluginZigbee::onZigbeeNetworkNodeRemoved);

        m_zigbeeNetworks.insert(device, zigbeeNetwork);
        zigbeeNetwork->startNetwork();
    }

    if (device->deviceClassId() == tradfriRemoteDeviceClassId) {
        qCDebug(dcZigbee()) << "Tradfri remote" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(tradfriRemoteDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        TradfriRemote *remote = new TradfriRemote(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, remote);
    }

    if (device->deviceClassId() == tradfriOnOffSwitchDeviceClassId) {
        qCDebug(dcZigbee()) << "Tradfri on/off remote" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(tradfriOnOffSwitchDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        TradfriOnOffSwitch *remote = new TradfriOnOffSwitch(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, remote);
    }


    if (device->deviceClassId() == tradfriColorLightDeviceClassId) {
        qCDebug(dcZigbee()) << "Tradfri colour light" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(tradfriColorLightDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        TradfriColorLight *light = new TradfriColorLight(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, light);
    }

    if (device->deviceClassId() == tradfriColorTemperatureLightDeviceClassId) {
        qCDebug(dcZigbee()) << "Tradfri colour light" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(tradfriColorTemperatureLightDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        TradfriColorTemperatureLight *light = new TradfriColorTemperatureLight(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, light);
    }

    if (device->deviceClassId() == tradfriPowerSocketDeviceClassId) {
        qCDebug(dcZigbee()) << "Tradfri power socket" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(tradfriPowerSocketDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        TradfriPowerSocket *socket = new TradfriPowerSocket(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, socket);
    }

    if (device->deviceClassId() == tradfriRangeExtenderDeviceClassId) {
        qCDebug(dcZigbee()) << "Tradfri range extender" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(tradfriRangeExtenderDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        TradfriRangeExtender *extender = new TradfriRangeExtender(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, extender);
    }

    if (device->deviceClassId() == feibitOnOffLightDeviceClassId) {
        qCDebug(dcZigbee()) << "FeiBit On/OFF light" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(feibitOnOffLightDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        FeiBitOnOffLight *light = new FeiBitOnOffLight(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, light);
    }

    if (device->deviceClassId() == lumiTemperatureHumidityDeviceClassId) {
        qCDebug(dcZigbee()) << "Lumi temperature humidity" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(lumiTemperatureHumidityDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        LumiTemperatureSensor *sensor = new LumiTemperatureSensor(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, sensor);
    }

    if (device->deviceClassId() == lumiMagnetSensorDeviceClassId) {
        qCDebug(dcZigbee()) << "Lumi magnet sensor" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(lumiMagnetSensorDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        LumiMagnetSensor *sensor = new LumiMagnetSensor(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, sensor);
    }

    if (device->deviceClassId() == lumiButtonSensorDeviceClassId) {
        qCDebug(dcZigbee()) << "Lumi button sensor" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(lumiButtonSensorDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        LumiButtonSensor *sensor = new LumiButtonSensor(network, ieeeAddress, device, this);
        connect(sensor, &LumiButtonSensor::buttonPressed, this, [this, device](){
            emit emitEvent(Event(lumiButtonSensorPressedEventTypeId, device->id()));
        });
        connect(sensor, &LumiButtonSensor::buttonLongPressed, this, [this, device](){
            emit emitEvent(Event(lumiButtonSensorLongPressedEventTypeId, device->id()));
        });

        m_zigbeeDevices.insert(device, sensor);
    }

    if (device->deviceClassId() == lumiMotionSensorDeviceClassId) {
        qCDebug(dcZigbee()) << "Lumi motion sensor" << device;
        ZigbeeAddress ieeeAddress(device->paramValue(lumiMotionSensorDeviceIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(device);
        LumiMotionSensor *sensor = new LumiMotionSensor(network, ieeeAddress, device, this);
        m_zigbeeDevices.insert(device, sensor);
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

        // Note: following actions do not require a running network
        if (action.actionTypeId() == zigbeeControllerFactoryResetActionTypeId)
            zigbeeNetwork->factoryResetNetwork();

        if (action.actionTypeId() == zigbeeControllerResetActionTypeId)
            zigbeeNetwork->reset();

        // Note: following actions require a running network
        if (zigbeeNetwork->state() != ZigbeeNetwork::StateRunning)
            return info->finish(Device::DeviceErrorHardwareNotAvailable);

        if (action.actionTypeId() == zigbeeControllerPermitJoinActionTypeId)
            zigbeeNetwork->setPermitJoining(action.params().paramValue(zigbeeControllerPermitJoinActionPermitJoinParamTypeId).toBool());

        if (action.actionTypeId() == zigbeeControllerTest1ActionTypeId) {
            qCDebug(dcZigbee()) << "Test 1";
            ZigbeeNodeEndpoint *endpoint = zigbeeNetwork->coordinatorNode()->getEndpoint(0x01);
            if (!endpoint) {
                qCWarning(dcZigbee()) << "Could not find node endpoint";
            } else {
                endpoint->addGroup(01, 0x0000);
            }
        }

        if (action.actionTypeId() == zigbeeControllerTest2ActionTypeId) {
            qCDebug(dcZigbee()) << "Test 2";
        }
    }

    // Tradfri remote
    if (device->deviceClassId() == tradfriRemoteDeviceClassId) {
        TradfriRemote *remote = qobject_cast<TradfriRemote *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == tradfriRemoteIdentifyActionTypeId) {
            remote->identify();
        } else if (action.actionTypeId() == tradfriRemoteRemoveFromNetworkActionTypeId) {
            remote->removeFromNetwork();
        }
    }


    // Tradfri range extender
    if (device->deviceClassId() == tradfriRangeExtenderDeviceClassId) {
        TradfriRangeExtender *extender = qobject_cast<TradfriRangeExtender *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == tradfriRangeExtenderIdentifyActionTypeId) {
            extender->identify();
        } else if (action.actionTypeId() == tradfriRangeExtenderRemoveFromNetworkActionTypeId) {
            extender->removeFromNetwork();
        }
    }

    // Tradfri on/off switch
    if (device->deviceClassId() == tradfriOnOffSwitchDeviceClassId) {
        TradfriOnOffSwitch *remote = qobject_cast<TradfriOnOffSwitch *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == tradfriOnOffSwitchIdentifyActionTypeId) {
            remote->identify();
        } else if (action.actionTypeId() == tradfriOnOffSwitchFactoryResetActionTypeId) {
            remote->factoryResetNode();
        } else if (action.actionTypeId() == tradfriOnOffSwitchRemoveFromNetworkActionTypeId) {
            remote->removeFromNetwork();
        }
    }

    // Tradfri power socket
    if (device->deviceClassId() == tradfriPowerSocketDeviceClassId) {
        TradfriPowerSocket *socket = qobject_cast<TradfriPowerSocket *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == tradfriPowerSocketIdentifyActionTypeId) {
            socket->identify();
        } else if (action.actionTypeId() == tradfriPowerSocketPowerActionTypeId) {
            socket->setPower(action.param(tradfriPowerSocketPowerActionPowerParamTypeId).value().toBool());
        } else if (action.actionTypeId() == tradfriPowerSocketRemoveFromNetworkActionTypeId) {
            socket->removeFromNetwork();
        }
    }

    // Tradfri color light
    if (device->deviceClassId() == tradfriColorLightDeviceClassId) {
        TradfriColorLight *light = qobject_cast<TradfriColorLight *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == tradfriColorLightIdentifyActionTypeId) {
            light->identify();
        } else if (action.actionTypeId() == tradfriColorLightPowerActionTypeId) {
            light->setPower(action.param(tradfriColorLightPowerActionPowerParamTypeId).value().toBool());
        } else if (action.actionTypeId() == tradfriColorLightBrightnessActionTypeId) {
            light->setBrightness(action.param(tradfriColorLightBrightnessActionBrightnessParamTypeId).value().toInt());
        } else if (action.actionTypeId() == tradfriColorLightColorTemperatureActionTypeId) {
            light->setColorTemperature(action.param(tradfriColorLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt());
        } else if (action.actionTypeId() == tradfriColorLightColorActionTypeId) {
            light->setColor(action.param(tradfriColorLightColorActionColorParamTypeId).value().value<QColor>());
        } else if (action.actionTypeId() == tradfriColorLightRemoveFromNetworkActionTypeId) {
            light->removeFromNetwork();
        }
    }

    // Tradfri color temperature light
    if (device->deviceClassId() == tradfriColorTemperatureLightDeviceClassId) {
        TradfriColorTemperatureLight *light = qobject_cast<TradfriColorTemperatureLight *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == tradfriColorTemperatureLightIdentifyActionTypeId) {
            light->identify();
        } else if (action.actionTypeId() == tradfriColorTemperatureLightPowerActionTypeId) {
            light->setPower(action.param(tradfriColorTemperatureLightPowerActionPowerParamTypeId).value().toBool());
        } else if (action.actionTypeId() == tradfriColorTemperatureLightBrightnessActionTypeId) {
            light->setBrightness(action.param(tradfriColorTemperatureLightBrightnessActionBrightnessParamTypeId).value().toInt());
        } else if (action.actionTypeId() == tradfriColorTemperatureLightColorTemperatureActionTypeId) {
            light->setColorTemperature(action.param(tradfriColorTemperatureLightColorTemperatureActionColorTemperatureParamTypeId).value().toInt());
        } else if (action.actionTypeId() == tradfriColorTemperatureLightRemoveFromNetworkActionTypeId) {
            light->removeFromNetwork();
        }
    }

    // FeiBit on/off light switch
    if (device->deviceClassId() == feibitOnOffLightDeviceClassId) {
        FeiBitOnOffLight *light = qobject_cast<FeiBitOnOffLight *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == feibitOnOffLightIdentifyActionTypeId) {
            light->identify();
        } else if (action.actionTypeId() == feibitOnOffLightPowerActionTypeId) {
            light->setPower(action.param(feibitOnOffLightPowerActionPowerParamTypeId).value().toBool());
        } else if (action.actionTypeId() == feibitOnOffLightRemoveFromNetworkActionTypeId) {
            light->removeFromNetwork();
        }
    }

    // Lumi temperature/humidity sensor
    if (device->deviceClassId() == lumiTemperatureHumidityDeviceClassId) {
        LumiTemperatureSensor *sensor = qobject_cast<LumiTemperatureSensor *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == lumiTemperatureHumidityIdentifyActionTypeId) {
            sensor->identify();
        } else if (action.actionTypeId() == lumiTemperatureHumidityRemoveFromNetworkActionTypeId) {
            sensor->removeFromNetwork();
        }
    }

    // Lumi magnet sensor
    if (device->deviceClassId() == lumiMagnetSensorDeviceClassId) {
        LumiMagnetSensor *sensor = qobject_cast<LumiMagnetSensor *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == lumiMagnetSensorRemoveFromNetworkActionTypeId) {
            sensor->removeFromNetwork();
        }
    }

    // Lumi motion sensor
    if (device->deviceClassId() == lumiMotionSensorDeviceClassId) {
        LumiMotionSensor *sensor = qobject_cast<LumiMotionSensor *>(m_zigbeeDevices.value(device));
        if (action.actionTypeId() == lumiMotionSensorRemoveFromNetworkActionTypeId) {
            sensor->removeFromNetwork();
        }
    }

    info->finish(Device::DeviceErrorNoError);
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

ZigbeeDevice *DevicePluginZigbee::findNodeZigbeeDevice(ZigbeeNode *node)
{
    foreach (ZigbeeDevice *zigbeeDevice, m_zigbeeDevices.values()) {
        if (zigbeeDevice->ieeeAddress() == node->extendedAddress()) {
            return zigbeeDevice;
        }
    }

    return nullptr;
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
        device->setStateValue(zigbeeControllerVersionStateTypeId, zigbeeNetwork->bridgeController()->firmwareVersion());
        device->setStateValue(zigbeeControllerPanIdStateTypeId, zigbeeNetwork->extendedPanId());
        device->setStateValue(zigbeeControllerChannelStateTypeId, zigbeeNetwork->channel());
        device->setStateValue(zigbeeControllerPermitJoinStateTypeId, zigbeeNetwork->permitJoining());
        device->setStateValue(zigbeeControllerIeeeAddressStateTypeId, zigbeeNetwork->coordinatorNode()->extendedAddress().toString());
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
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Device *networkManagerDevice = m_zigbeeNetworks.key(zigbeeNetwork);
    if (!networkManagerDevice) return;

    qCDebug(dcZigbee()) << "Node added. Check if we recognize this node" << node;
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        qCDebug(dcZigbee()) << endpoint;
        qCDebug(dcZigbee()) << "  Manufacturer" << endpoint->manufacturerName();
        qCDebug(dcZigbee()) << "  Model" << endpoint->modelIdentifier();
        qCDebug(dcZigbee()) << "  Version" << endpoint->softwareBuildId();
        qCDebug(dcZigbee()) << "  Input clusters (" << endpoint->inputClusters().count() << ")";
        foreach (ZigbeeCluster *cluster, endpoint->inputClusters()) {
            qCDebug(dcZigbee()) << "   -" << cluster;
            foreach(const ZigbeeClusterAttribute &attribute, cluster->attributes()) {
                qCDebug(dcZigbee()) << "     - " << attribute;
            }
        }

        qCDebug(dcZigbee()) << "  Output clusters (" << endpoint->outputClusters().count() << ")";
        foreach (ZigbeeCluster *cluster, endpoint->outputClusters()) {
            qCDebug(dcZigbee()) << "   -" << cluster;
            foreach(const ZigbeeClusterAttribute &attribute, cluster->attributes()) {
                qCDebug(dcZigbee()) << "     - " << attribute;
            }
        }
    }

    // Check ikea devices
    if (node->manufacturerCode() == Zigbee::Manufacturer::Ikea) {
        qCDebug(dcZigbee()) << "This device is from Ikea";
        foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
            if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink &&
                    endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceNonColourSceneController) {

                qCDebug(dcZigbee()) << "Found Ikea Tradfri Remote";
                // Check if node already added
                if (myDevices().filterByDeviceClassId(tradfriRemoteDeviceClassId)
                        .filterByParam(tradfriRemoteDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new tradfri remote";
                    DeviceDescriptor descriptor(tradfriRemoteDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(tradfriRemoteDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(tradfriRemoteDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            } else if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                       endpoint->deviceId() == Zigbee::HomeAutomationDevice::HomeAutomationDeviceNonColourController) {

                qCDebug(dcZigbee()) << "Found Ikea Tradfri On/Off remote";
                // Check if node already added
                if (myDevices().filterByDeviceClassId(tradfriOnOffSwitchDeviceClassId)
                        .filterByParam(tradfriOnOffSwitchDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new tradfri on/off remote";
                    DeviceDescriptor descriptor(tradfriOnOffSwitchDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(tradfriOnOffSwitchDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(tradfriOnOffSwitchDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            } else if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink &&
                       endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceColourLight) {

                qCDebug(dcZigbee()) << "Found Ikea Tradfri Colour Light";
                // Check if node already added
                if (myDevices().filterByDeviceClassId(tradfriColorLightDeviceClassId)
                        .filterByParam(tradfriColorLightDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new tradfri colour light";
                    DeviceDescriptor descriptor(tradfriColorLightDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(tradfriColorLightDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(tradfriColorLightDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            } else if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                       endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOffPlugin) {

                qCDebug(dcZigbee()) << "Found Ikea tradfri power socket";
                // Check if node already added
                if (myDevices().filterByDeviceClassId(tradfriPowerSocketDeviceClassId)
                        .filterByParam(tradfriPowerSocketDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new tradfri power socket";
                    DeviceDescriptor descriptor(tradfriPowerSocketDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(tradfriPowerSocketDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(tradfriPowerSocketDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            } else if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                       endpoint->deviceId() == Zigbee::HomeAutomationDeviceColourTemperatureLight) {

                qCDebug(dcZigbee()) << "Found Ikea tradfri color temperature light";
                // Check if node already added
                if (myDevices().filterByDeviceClassId(tradfriColorTemperatureLightDeviceClassId)
                        .filterByParam(tradfriColorTemperatureLightDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new tradfri color temperature light";
                    DeviceDescriptor descriptor(tradfriColorTemperatureLightDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(tradfriColorTemperatureLightDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(tradfriColorTemperatureLightDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            } else if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                       endpoint->deviceId() == Zigbee::HomeAutomationDeviceRangeExtender) {

                qCDebug(dcZigbee()) << "Found Ikea tradfri range extender";
                // Check if node already added
                if (myDevices().filterByDeviceClassId(tradfriRangeExtenderDeviceClassId)
                        .filterByParam(tradfriRangeExtenderDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new tradfri range extender";
                    DeviceDescriptor descriptor(tradfriRangeExtenderDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(tradfriRangeExtenderDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(tradfriRangeExtenderDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            }
        }
    } else if (node->manufacturerCode() == Zigbee::Manufacturer::FeiBit) {
        qCDebug(dcZigbee()) << "This device is from FeiBit";
        foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
            if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink &&
                    endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceOnOffLight) {

                qCDebug(dcZigbee()) << "This device is a FeiBit on/off light";
                if (myDevices().filterByDeviceClassId(feibitOnOffLightDeviceClassId)
                        .filterByParam(feibitOnOffLightDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new feibit on/off light";
                    DeviceDescriptor descriptor(feibitOnOffLightDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(feibitOnOffLightDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(feibitOnOffLightDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            }
        }
    } else if (node->manufacturerCode() == 0x1037) {
        // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
        qCDebug(dcZigbee()) << "This device is from Lumi";
        foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {

            // Get the model identifier if present from the first endpoint. Also this is out of spec
            if (!endpoint->hasInputCluster(Zigbee::ClusterIdBasic)) {
                qCWarning(dcZigbee()) << "This lumi device does not have the basic input cluster yet.";
                continue;
            }

            ZigbeeCluster *basicCluster = endpoint->getInputCluster(Zigbee::ClusterIdBasic);
            if (!basicCluster->hasAttribute(ZigbeeCluster::BasicAttributeModelIdentifier)) {
                qCWarning(dcZigbee()) << "This lumi device does not have the model identifier yet.";
                continue;
            }

            QString modelIdentifier = QString::fromUtf8(basicCluster->attribute(ZigbeeCluster::BasicAttributeModelIdentifier).data());
            qCDebug(dcZigbee()) << "Model identifier" << modelIdentifier;

            // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
            if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                    modelIdentifier.startsWith("lumi.sensor_ht") &&
                    endpoint->hasOutputCluster(Zigbee::ClusterIdTemperatureMeasurement) &&
                    endpoint->hasOutputCluster(Zigbee::ClusterIdRelativeHumidityMeasurement)) {

                qCDebug(dcZigbee()) << "This device is a lumi temperature humidity sensor";
                if (myDevices().filterByDeviceClassId(lumiTemperatureHumidityDeviceClassId)
                        .filterByParam(lumiTemperatureHumidityDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new lumi temperature humidity sensor";
                    DeviceDescriptor descriptor(lumiTemperatureHumidityDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(lumiTemperatureHumidityDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(lumiTemperatureHumidityDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            }

            // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
            if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                    modelIdentifier.startsWith("lumi.sensor_magnet")) {

                qCDebug(dcZigbee()) << "This device is a lumi magnet sensor";
                if (myDevices().filterByDeviceClassId(lumiMagnetSensorDeviceClassId)
                        .filterByParam(lumiMagnetSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new lumi magnet sensor";
                    DeviceDescriptor descriptor(lumiMagnetSensorDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(lumiMagnetSensorDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(lumiMagnetSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            }

            // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
            if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                    modelIdentifier.startsWith("lumi.sensor_switch")) {

                qCDebug(dcZigbee()) << "This device is a lumi button sensor";
                if (myDevices().filterByDeviceClassId(lumiButtonSensorDeviceClassId)
                        .filterByParam(lumiButtonSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new lumi button sensor";
                    DeviceDescriptor descriptor(lumiButtonSensorDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(lumiButtonSensorDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(lumiButtonSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            }

            // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
            if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                    modelIdentifier.startsWith("lumi.sensor_motion")) {

                qCDebug(dcZigbee()) << "This device is a lumi motion sensor";
                if (myDevices().filterByDeviceClassId(lumiMotionSensorDeviceClassId)
                        .filterByParam(lumiMotionSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString())
                        .isEmpty()) {
                    qCDebug(dcZigbee()) << "Adding new lumi motion sensor";
                    DeviceDescriptor descriptor(lumiMotionSensorDeviceClassId);
                    descriptor.setTitle(supportedDevices().findById(lumiMotionSensorDeviceClassId).displayName());
                    ParamList params;
                    params.append(Param(lumiMotionSensorDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
                    descriptor.setParams(params);
                    descriptor.setParentDeviceId(networkManagerDevice->id());
                    emit autoDevicesAppeared({descriptor});
                } else {
                    qCDebug(dcZigbee()) << "The device for this node has already been created.";
                }
            }
        }
    }
}

void DevicePluginZigbee::onZigbeeNetworkNodeRemoved(ZigbeeNode *node)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Device *networkDevice = m_zigbeeNetworks.key(zigbeeNetwork);

    qCDebug(dcZigbee()) << networkDevice << "removed" << node;

    ZigbeeDevice * zigbeeDevice = findNodeZigbeeDevice(node);
    if (!zigbeeDevice) {
        qCWarning(dcZigbee()) << "There is no nymea device for this node" << node;
        return;
    }

    // Clean up
    Device *device = m_zigbeeDevices.key(zigbeeDevice);
    m_zigbeeDevices.remove(device);
    delete zigbeeDevice;

    emit autoDeviceDisappeared(device->id());
}
