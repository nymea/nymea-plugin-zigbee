/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@nymea.io>                 *
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

DevicePluginZigbee::DevicePluginZigbee()
{
    connect(this, &DevicePluginZigbee::configValueChanged, this, &DevicePluginZigbee::onPluginConfigurationChanged);
}

void DevicePluginZigbee::init()
{       
    if (!configValue(zigbeePluginEnabledParamTypeId).toBool()) {
        qCDebug(dcZigbee()) << "Zigbee is disabled.";
        return;
    }

    m_zigbeeNetworkManager = new ZigbeeNetworkManager(this);
    m_zigbeeNetworkManager->setSerialPortName(configValue(zigbeePluginSerialPortParamTypeId).toString());
    m_zigbeeNetworkManager->setSerialBaudrate(static_cast<qint32>(configValue(zigbeePluginBaudrateParamTypeId).toUInt()));
    m_zigbeeNetworkManager->startNetwork();
}

void DevicePluginZigbee::startMonitoringAutoDevices()
{
    // Start seaching for devices which can be discovered and added automatically
}

void DevicePluginZigbee::postSetupDevice(Device *device)
{
    qCDebug(dcZigbee()) << "Post setup device" << device->name() << device->params();

    // This method will be called once the setup for device is finished
}

void DevicePluginZigbee::deviceRemoved(Device *device)
{
    qCDebug(dcZigbee()) << "Remove device" << device->name() << device->params();

    // Clean up all data related to this device
}

DeviceManager::DeviceSetupStatus DevicePluginZigbee::setupDevice(Device *device)
{
    qCDebug(dcZigbee()) << "Setup device" << device->name() << device->params();

    return DeviceManager::DeviceSetupStatusSuccess;
}

DeviceManager::DeviceError DevicePluginZigbee::executeAction(Device *device, const Action &action)
{
    qCDebug(dcZigbee()) << "Executing action for device" << device->name() << action.actionTypeId().toString() << action.params();

    return DeviceManager::DeviceErrorNoError;
}

void DevicePluginZigbee::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{

    qCDebug(dcZigbee()) << "Plugin configuration changed:" << paramTypeId.toString() << value;

}
