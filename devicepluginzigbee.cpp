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
#include "devicepluginzigbee.h"

#include <QSerialPortInfo>

DevicePluginZigbee::DevicePluginZigbee()
{
    connect(this, &DevicePluginZigbee::configValueChanged, this, &DevicePluginZigbee::onPluginConfigurationChanged);
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
    Q_UNUSED(device)
    //qCDebug(dcZigbee()) << "Post setup device" << device->name() << device->params();
}

void DevicePluginZigbee::deviceRemoved(Device *device)
{
    qCDebug(dcZigbee()) << "Remove device" << device->name() << device->params();

    // Clean up all data related to this device
}

DeviceManager::DeviceError DevicePluginZigbee::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
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

            ParamList params;
            params.append(Param(zigbeeControllerDeviceSerialPortParamTypeId, info.systemLocation()));
            params.append(Param(zigbeeControllerDeviceBaudrateParamTypeId, 1000000));

            DeviceDescriptor descriptor(zigbeeControllerDeviceClassId);
            descriptor.setTitle(info.systemLocation());
            descriptor.setDescription(info.manufacturer() + " - " + info.description());
            descriptor.setParams(params);
            deviceDescriptors.append(descriptor);
        }
    }

    emit devicesDiscovered(deviceClassId, deviceDescriptors);
    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginZigbee::setupDevice(Device *device)
{
    qCDebug(dcZigbee()) << "Setup device" << device->name() << device->params();

    if (device->deviceClassId() == zigbeeControllerDeviceClassId) {
        qCDebug(dcZigbee()) << "Create zigbee network manager for controller" << device;
        ZigbeeNetworkManager *zigbeeNetworkManager = new ZigbeeNetworkManager(this);
        zigbeeNetworkManager->setSerialPortName(device->paramValue(zigbeeControllerDeviceSerialPortParamTypeId).toString());
        zigbeeNetworkManager->setSerialBaudrate(static_cast<qint32>(device->paramValue(zigbeeControllerDeviceBaudrateParamTypeId).toUInt()));

        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::stateChanged, this, &DevicePluginZigbee::onZigbeeControllerStateChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::channelChanged, this, &DevicePluginZigbee::onZigbeeControllerChannelChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::extendedPanIdChanged, this, &DevicePluginZigbee::onZigbeeControllerPanIdChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::permitJoiningChanged, this, &DevicePluginZigbee::onZigbeeControllerPermitJoiningChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::nodeAdded, this, &DevicePluginZigbee::onZigbeeControllerNodeAdded);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::nodeRemoved, this, &DevicePluginZigbee::onZigbeeControllerNodeRemoved);

        m_zigbeeControllers.insert(device, zigbeeNetworkManager);

        zigbeeNetworkManager->startNetwork();
    }

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginZigbee::executeAction(Device *device, const Action &action)
{
    qCDebug(dcZigbee()) << "Executing action for device" << device->name() << action.actionTypeId().toString() << action.params();

    if (device->deviceClassId() == zigbeeControllerDeviceClassId) {
        ZigbeeNetworkManager *networkManager = m_zigbeeControllers.value(device);
        if (networkManager->state() != ZigbeeNetworkManager::StateRunning)
            return DeviceManager::DeviceErrorHardwareNotAvailable;

        if (action.actionTypeId() == zigbeeControllerFactoryResetActionTypeId)
            networkManager->factoryResetNetwork();

        if (action.actionTypeId() == zigbeeControllerPermitJoinActionTypeId)
            networkManager->setPermitJoining(action.params().paramValue(zigbeeControllerPermitJoinActionPermitJoinParamTypeId).toBool());

    }

    if (device->deviceClassId() == zigbeeNodeDeviceClassId) {
        ZigbeeNetworkManager *networkManager = findParentController(device);

        if (!networkManager)
            return DeviceManager::DeviceErrorHardwareFailure;

        if (networkManager->state() != ZigbeeNetworkManager::StateRunning)
            return DeviceManager::DeviceErrorHardwareNotAvailable;

        quint16 shortAddress = static_cast<quint16>(device->paramValue(zigbeeNodeDeviceNwkAddressParamTypeId).toUInt());
        ZigbeeAddress extendedAddress = ZigbeeAddress(device->paramValue(zigbeeNodeDeviceIeeeAddressParamTypeId).toString());

        if (action.actionTypeId() == zigbeeNodeIdentifyActionTypeId) {
            qCDebug(dcZigbee()) << extendedAddress.toString();
        }

        if (action.actionTypeId() == zigbeeNodeLqiRequestActionTypeId) {
            networkManager->controller()->commandRequestLinkQuality(shortAddress);
        }
    }

    return DeviceManager::DeviceErrorNoError;
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


void DevicePluginZigbee::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    qCDebug(dcZigbee()) << "Plugin configuration changed:" << paramTypeId.toString() << value;
}

void DevicePluginZigbee::onZigbeeControllerStateChanged(ZigbeeNetwork::State state)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Device *device = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << "Controller state changed" << state << device;

    switch (state) {
    case ZigbeeNetwork::StateDisconnected:
        device->setStateValue(zigbeeControllerConnectedStateTypeId, false);

        // TODO: disconnected for all childs

        break;
    case ZigbeeNetwork::StateRunning:
        device->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        device->setStateValue(zigbeeControllerVersionStateTypeId, zigbeeNetworkManager->controllerFirmwareVersion());
        device->setStateValue(zigbeeControllerPanIdStateTypeId, zigbeeNetworkManager->extendedPanId());
        device->setStateValue(zigbeeControllerChannelStateTypeId, zigbeeNetworkManager->channel());
        device->setStateValue(zigbeeControllerPermitJoinStateTypeId, zigbeeNetworkManager->permitJoining());
        device->setStateValue(zigbeeControllerIeeeAddressStateTypeId, zigbeeNetworkManager->coordinatorNode()->extendedAddress().toString());

        // TODO: connected true for all childs

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

    DeviceDescriptor descriptor;
    descriptor.setTitle("Zigbee node");
    descriptor.setParentDeviceId(device->id());

    ParamList params;
    params.append(Param(zigbeeNodeDeviceIeeeAddressParamTypeId, node->extendedAddress().toString()));
    params.append(Param(zigbeeNodeDeviceNwkAddressParamTypeId, QVariant::fromValue(node->shortAddress())));
    descriptor.setParams(params);

    emit autoDevicesAppeared(zigbeeNodeDeviceClassId, { descriptor });
}

void DevicePluginZigbee::onZigbeeControllerNodeRemoved(ZigbeeNode *node)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Device *device = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << device << "node removed" << device << node;
}
