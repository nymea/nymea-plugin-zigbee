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

#include "plugininfo.h"
#include "nymeasettings.h"
#include "integrationpluginzigbee.h"

#include "zigbeeutils.h"
#include "zigbeenetworkkey.h"

#include <QDateTime>
#include <QSerialPortInfo>

IntegrationPluginZigbee::IntegrationPluginZigbee()
{

}

void IntegrationPluginZigbee::init()
{       

}

void IntegrationPluginZigbee::startMonitoringAutoThings()
{
    // Start seaching for devices which can be discovered and added automatically
}

void IntegrationPluginZigbee::postSetupThing(Thing *thing)
{
    qCDebug(dcZigbee()) << "Post setup device" << thing->name() << thing->params();

    if (m_zigbeeDevices.contains(thing)) {
        ZigbeeDevice *zigbeeDevice = m_zigbeeDevices.value(thing);
        zigbeeDevice->checkOnlineStatus();
    }
}

void IntegrationPluginZigbee::thingRemoved(Thing *thing)
{
    qCDebug(dcZigbee()) << "Remove device" << thing->name() << thing->params();

    if (thing->thingClassId() == zigbeeControllerThingClassId) {
        ZigbeeNetwork *zigbeeNetwork = m_zigbeeNetworks.take(thing);
        if (zigbeeNetwork) {
            zigbeeNetwork->deleteLater();
        }
    } else {
        ZigbeeDevice *zigbeeDevice = m_zigbeeDevices.take(thing);
        if (zigbeeDevice)
            delete zigbeeDevice;
    }
}

void IntegrationPluginZigbee::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() == zigbeeControllerThingClassId) {
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
            params.append(Param(zigbeeControllerThingSerialPortParamTypeId, serialPortInfo.systemLocation()));
            params.append(Param(zigbeeControllerThingBaudrateParamTypeId, baudrate));

            qCDebug(dcZigbee()) << "Using baudrate param" << params.paramValue(zigbeeControllerThingBaudrateParamTypeId);

            ThingDescriptor descriptor(zigbeeControllerThingClassId);
            descriptor.setTitle(serialPortInfo.manufacturer() + " - " + serialPortInfo.description());
            descriptor.setDescription(serialPortInfo.systemLocation());
            descriptor.setParams(params);
            info->addThingDescriptor(descriptor);
        }
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbee::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    qCDebug(dcZigbee()) << "Setup device" << thing->name() << thing->params();

    if (thing->thingClassId() == zigbeeControllerThingClassId) {
        qCDebug(dcZigbee()) << "Create zigbee network manager for controller" << thing;

        QString serialPortName = thing->paramValue(zigbeeControllerThingSerialPortParamTypeId).toString();
        qint32 baudrate = static_cast<qint32>(thing->paramValue(zigbeeControllerThingBaudrateParamTypeId).toUInt());

        ZigbeeNetwork *zigbeeNetwork = ZigbeeNetworkManager::createZigbeeNetwork(ZigbeeNetworkManager::BackendTypeNxp, this);
        zigbeeNetwork->setSettingsFileName(NymeaSettings::settingsPath() + "/nymea-zigbee.conf");
        zigbeeNetwork->setSerialPortName(serialPortName);
        zigbeeNetwork->setSerialBaudrate(baudrate);

        connect(zigbeeNetwork->bridgeController(), &ZigbeeBridgeController::firmwareVersionChanged, this, [thing](const QString &firmwareVersion){
            thing->setStateValue(zigbeeControllerVersionStateTypeId, firmwareVersion);
        });

        connect(zigbeeNetwork, &ZigbeeNetwork::stateChanged, this, &IntegrationPluginZigbee::onZigbeeNetworkStateChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::channelChanged, this, &IntegrationPluginZigbee::onZigbeeNetworkChannelChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::extendedPanIdChanged, this, &IntegrationPluginZigbee::onZigbeeNetworkPanIdChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::permitJoiningChanged, this, &IntegrationPluginZigbee::onZigbeeNetworkPermitJoiningChanged);
        connect(zigbeeNetwork, &ZigbeeNetwork::nodeAdded, this, &IntegrationPluginZigbee::onZigbeeNetworkNodeAdded);
        connect(zigbeeNetwork, &ZigbeeNetwork::nodeRemoved, this, &IntegrationPluginZigbee::onZigbeeNetworkNodeRemoved);

        m_zigbeeNetworks.insert(thing, zigbeeNetwork);
        zigbeeNetwork->startNetwork();
    }

    // Ikea
    if (thing->thingClassId() == tradfriRemoteThingClassId) {
        qCDebug(dcZigbee()) << "Tradfri remote" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(tradfriRemoteThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        TradfriRemote *remote = new TradfriRemote(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, remote);
    }

    if (thing->thingClassId() == tradfriOnOffSwitchThingClassId) {
        qCDebug(dcZigbee()) << "Tradfri on/off remote" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(tradfriOnOffSwitchThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        TradfriOnOffSwitch *remote = new TradfriOnOffSwitch(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, remote);
    }

    if (thing->thingClassId() == tradfriColorLightThingClassId) {
        qCDebug(dcZigbee()) << "Tradfri colour light" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(tradfriColorLightThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        TradfriColorLight *light = new TradfriColorLight(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, light);
    }

    if (thing->thingClassId() == tradfriColorTemperatureLightThingClassId) {
        qCDebug(dcZigbee()) << "Tradfri colour light" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(tradfriColorTemperatureLightThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        TradfriColorTemperatureLight *light = new TradfriColorTemperatureLight(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, light);
    }

    if (thing->thingClassId() == tradfriPowerSocketThingClassId) {
        qCDebug(dcZigbee()) << "Tradfri power socket" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(tradfriPowerSocketThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        TradfriPowerSocket *socket = new TradfriPowerSocket(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, socket);
    }

    if (thing->thingClassId() == tradfriRangeExtenderThingClassId) {
        qCDebug(dcZigbee()) << "Tradfri range extender" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(tradfriRangeExtenderThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        TradfriRangeExtender *extender = new TradfriRangeExtender(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, extender);
    }

    // Lumi
    if (thing->thingClassId() == lumiTemperatureHumidityThingClassId) {
        qCDebug(dcZigbee()) << "Lumi temperature humidity" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(lumiTemperatureHumidityThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        LumiTemperatureSensor *sensor = new LumiTemperatureSensor(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, sensor);
    }

    if (thing->thingClassId() == lumiMagnetSensorThingClassId) {
        qCDebug(dcZigbee()) << "Lumi magnet sensor" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(lumiMagnetSensorThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        LumiMagnetSensor *sensor = new LumiMagnetSensor(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, sensor);
    }

    if (thing->thingClassId() == lumiButtonSensorThingClassId) {
        qCDebug(dcZigbee()) << "Lumi button sensor" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(lumiButtonSensorThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        LumiButtonSensor *sensor = new LumiButtonSensor(network, ieeeAddress, thing, this);
        connect(sensor, &LumiButtonSensor::buttonPressed, this, [this, thing](){
            emit emitEvent(Event(lumiButtonSensorPressedEventTypeId, thing->id()));
        });
        connect(sensor, &LumiButtonSensor::buttonLongPressed, this, [this, thing](){
            emit emitEvent(Event(lumiButtonSensorLongPressedEventTypeId, thing->id()));
        });

        m_zigbeeDevices.insert(thing, sensor);
    }

    if (thing->thingClassId() == lumiMotionSensorThingClassId) {
        qCDebug(dcZigbee()) << "Lumi motion sensor" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(lumiMotionSensorThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        LumiMotionSensor *sensor = new LumiMotionSensor(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, sensor);
    }

    // Generic
    if (thing->thingClassId() == genericOnOffLightThingClassId) {
        qCDebug(dcZigbee()) << "On/OFF light" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(genericOnOffLightThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        GenericOnOffLight *light = new GenericOnOffLight(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, light);
    }

    if (thing->thingClassId() == genericPowerSocketThingClassId) {
        qCDebug(dcZigbee()) << "Power socket" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(genericPowerSocketThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        GenericPowerSocket *socket = new GenericPowerSocket(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, socket);
    }

    if (thing->thingClassId() == genericColorLightThingClassId) {
        qCDebug(dcZigbee()) << "Color light" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(genericColorLightThingIeeeAddressParamTypeId).toString());
        ZigbeeNetwork *network = findParentNetwork(thing);
        GenericColorLight *socket = new GenericColorLight(network, ieeeAddress, thing, this);
        m_zigbeeDevices.insert(thing, socket);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbee::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcZigbee()) << "Executing action for device" << thing->name() << action.actionTypeId().toString() << action.params();
    if (thing->thingClassId() == zigbeeControllerThingClassId) {
        ZigbeeNetwork *zigbeeNetwork = m_zigbeeNetworks.value(thing);

        // Note: following actions do not require a running network
        if (action.actionTypeId() == zigbeeControllerFactoryResetActionTypeId)
            zigbeeNetwork->factoryResetNetwork();

        if (action.actionTypeId() == zigbeeControllerResetActionTypeId)
            zigbeeNetwork->reset();

        // Note: following actions require a running network
        if (zigbeeNetwork->state() != ZigbeeNetwork::StateRunning)
            return info->finish(Thing::ThingErrorHardwareNotAvailable);

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
    if (thing->thingClassId() == tradfriRemoteThingClassId) {
        TradfriRemote *remote = qobject_cast<TradfriRemote *>(m_zigbeeDevices.value(thing));
        remote->executeAction(info);
        return;
    }

    // Tradfri range extender
    if (thing->thingClassId() == tradfriRangeExtenderThingClassId) {
        TradfriRangeExtender *extender = qobject_cast<TradfriRangeExtender *>(m_zigbeeDevices.value(thing));
        extender->executeAction(info);
        return;
    }

    // Tradfri on/off switch
    if (thing->thingClassId() == tradfriOnOffSwitchThingClassId) {
        TradfriOnOffSwitch *remote = qobject_cast<TradfriOnOffSwitch *>(m_zigbeeDevices.value(thing));
        remote->executeAction(info);
        return;
    }

    // Tradfri power socket
    if (thing->thingClassId() == tradfriPowerSocketThingClassId) {
        TradfriPowerSocket *socket = qobject_cast<TradfriPowerSocket *>(m_zigbeeDevices.value(thing));
        socket->executeAction(info);
        return;
    }

    // Tradfri color temperature light
    if (thing->thingClassId() == tradfriColorTemperatureLightThingClassId) {
        TradfriColorTemperatureLight *light = qobject_cast<TradfriColorTemperatureLight *>(m_zigbeeDevices.value(thing));
        light->executeAction(info);
        return;
    }

    // Tradfri color light
    if (thing->thingClassId() == tradfriColorLightThingClassId) {
        TradfriColorLight *light = qobject_cast<TradfriColorLight *>(m_zigbeeDevices.value(thing));
        light->executeAction(info);
        return;
    }

    // Lumi temperature/humidity sensor
    if (thing->thingClassId() == lumiTemperatureHumidityThingClassId) {
        LumiTemperatureSensor *sensor = qobject_cast<LumiTemperatureSensor *>(m_zigbeeDevices.value(thing));
        sensor->executeAction(info);
        return;
    }

    // Lumi magnet sensor
    if (thing->thingClassId() == lumiMagnetSensorThingClassId) {
        LumiMagnetSensor *sensor = qobject_cast<LumiMagnetSensor *>(m_zigbeeDevices.value(thing));
        sensor->executeAction(info);
        return;
    }

    // Lumi button sensor
    if (thing->thingClassId() == lumiButtonSensorThingClassId) {
        LumiButtonSensor *sensor = qobject_cast<LumiButtonSensor *>(m_zigbeeDevices.value(thing));
        sensor->executeAction(info);
        return;
    }

    // Lumi motion sensor
    if (thing->thingClassId() == lumiMotionSensorThingClassId) {
        LumiMotionSensor *sensor = qobject_cast<LumiMotionSensor *>(m_zigbeeDevices.value(thing));
        sensor->executeAction(info);
        return;
    }

    // Generic on/off light
    if (thing->thingClassId() == genericOnOffLightThingClassId) {
        GenericOnOffLight *light = qobject_cast<GenericOnOffLight *>(m_zigbeeDevices.value(thing));
        light->executeAction(info);
        return;
    }

    // Generic color temperature light
    if (thing->thingClassId() == genericColorTemperatureLightThingClassId) {
        GenericColorTemperatureLight *light = qobject_cast<GenericColorTemperatureLight *>(m_zigbeeDevices.value(thing));
        light->executeAction(info);
        return;
    }

    // Generic color light
    if (thing->thingClassId() == genericColorLightThingClassId) {
        GenericColorLight *light = qobject_cast<GenericColorLight *>(m_zigbeeDevices.value(thing));
        light->executeAction(info);
        return;
    }

    // Generic power socket
    if (thing->thingClassId() == genericPowerSocketThingClassId) {
        GenericPowerSocket *socket = qobject_cast<GenericPowerSocket *>(m_zigbeeDevices.value(thing));
        socket->executeAction(info);
        return;
    }
}

ZigbeeNetwork *IntegrationPluginZigbee::findParentNetwork(Thing *thing) const
{
    foreach (Thing *t, myThings()) {
        if (t->thingClassId() == zigbeeControllerThingClassId && t->id() == thing->parentId()) {
            return m_zigbeeNetworks.value(t);
        }
    }

    return nullptr;
}

ZigbeeDevice *IntegrationPluginZigbee::findNodeZigbeeDevice(ZigbeeNode *node)
{
    foreach (ZigbeeDevice *zigbeeDevice, m_zigbeeDevices.values()) {
        if (zigbeeDevice->ieeeAddress() == node->extendedAddress()) {
            return zigbeeDevice;
        }
    }

    return nullptr;
}

bool IntegrationPluginZigbee::createIkeaDevice(Thing *networkManagerDevice, ZigbeeNode *node)
{
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink &&
                endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceNonColourSceneController) {

            qCDebug(dcZigbee()) << "Found Ikea Tradfri Remote";
            // Check if node already added
            if (myThings().filterByThingClassId(tradfriRemoteThingClassId)
                    .filterByParam(tradfriRemoteThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new tradfri remote";
                ThingDescriptor descriptor(tradfriRemoteThingClassId);
                descriptor.setTitle(supportedThings().findById(tradfriRemoteThingClassId).displayName());
                ParamList params;
                params.append(Param(tradfriRemoteThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                endpoint->deviceId() == Zigbee::HomeAutomationDevice::HomeAutomationDeviceNonColourController) {

            qCDebug(dcZigbee()) << "Found Ikea Tradfri On/Off remote";
            // Check if node already added
            if (myThings().filterByThingClassId(tradfriOnOffSwitchThingClassId)
                    .filterByParam(tradfriOnOffSwitchThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new tradfri on/off remote";
                ThingDescriptor descriptor(tradfriOnOffSwitchThingClassId);
                descriptor.setTitle(supportedThings().findById(tradfriOnOffSwitchThingClassId).displayName());
                ParamList params;
                params.append(Param(tradfriOnOffSwitchThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        if ( (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceExtendedColourLight) ||
             (endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceColourLight)) {

            qCDebug(dcZigbee()) << "Found Ikea Tradfri Colour Light";
            // Check if node already added
            if (myThings().filterByThingClassId(tradfriColorLightThingClassId)
                    .filterByParam(tradfriColorLightThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new tradfri colour light";
                ThingDescriptor descriptor(tradfriColorLightThingClassId);
                descriptor.setTitle(supportedThings().findById(tradfriColorLightThingClassId).displayName());
                ParamList params;
                params.append(Param(tradfriColorLightThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOffPlugin) {
            qCDebug(dcZigbee()) << "Found Ikea tradfri power socket";
            // Check if node already added
            if (myThings().filterByThingClassId(tradfriPowerSocketThingClassId)
                    .filterByParam(tradfriPowerSocketThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new tradfri power socket";
                ThingDescriptor descriptor(tradfriPowerSocketThingClassId);
                descriptor.setTitle(supportedThings().findById(tradfriPowerSocketThingClassId).displayName());
                ParamList params;
                params.append(Param(tradfriPowerSocketThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        if ( (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceColourTemperatureLight) ||
             (endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceColourTemperatureLight)) {

            qCDebug(dcZigbee()) << "Found Ikea tradfri color temperature light";
            // Check if node already added
            if (myThings().filterByThingClassId(tradfriColorTemperatureLightThingClassId)
                    .filterByParam(tradfriColorTemperatureLightThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new tradfri color temperature light";
                ThingDescriptor descriptor(tradfriColorTemperatureLightThingClassId);
                descriptor.setTitle(supportedThings().findById(tradfriColorTemperatureLightThingClassId).displayName());
                ParamList params;
                params.append(Param(tradfriColorTemperatureLightThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        if (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation &&
                endpoint->deviceId() == Zigbee::HomeAutomationDeviceRangeExtender) {

            qCDebug(dcZigbee()) << "Found Ikea tradfri range extender";
            // Check if node already added
            if (myThings().filterByThingClassId(tradfriRangeExtenderThingClassId)
                    .filterByParam(tradfriRangeExtenderThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new tradfri range extender";
                ThingDescriptor descriptor(tradfriRangeExtenderThingClassId);
                descriptor.setTitle(supportedThings().findById(tradfriRangeExtenderThingClassId).displayName());
                ParamList params;
                params.append(Param(tradfriRangeExtenderThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }
    }

    return false;
}

bool IntegrationPluginZigbee::createLumiDevice(Thing *networkManagerDevice, ZigbeeNode *node)
{
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
            if (myThings().filterByThingClassId(lumiTemperatureHumidityThingClassId)
                    .filterByParam(lumiTemperatureHumidityThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new lumi temperature humidity sensor";
                ThingDescriptor descriptor(lumiTemperatureHumidityThingClassId);
                descriptor.setTitle(supportedThings().findById(lumiTemperatureHumidityThingClassId).displayName());
                ParamList params;
                params.append(Param(lumiTemperatureHumidityThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                modelIdentifier.startsWith("lumi.sensor_magnet")) {

            qCDebug(dcZigbee()) << "This device is a lumi magnet sensor";
            if (myThings().filterByThingClassId(lumiMagnetSensorThingClassId)
                    .filterByParam(lumiMagnetSensorThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new lumi magnet sensor";
                ThingDescriptor descriptor(lumiMagnetSensorThingClassId);
                descriptor.setTitle(supportedThings().findById(lumiMagnetSensorThingClassId).displayName());
                ParamList params;
                params.append(Param(lumiMagnetSensorThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }

            return true;
        }

        // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                modelIdentifier.startsWith("lumi.sensor_switch")) {

            qCDebug(dcZigbee()) << "This device is a lumi button sensor";
            if (myThings().filterByThingClassId(lumiButtonSensorThingClassId)
                    .filterByParam(lumiButtonSensorThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new lumi button sensor";
                ThingDescriptor descriptor(lumiButtonSensorThingClassId);
                descriptor.setTitle(supportedThings().findById(lumiButtonSensorThingClassId).displayName());
                ParamList params;
                params.append(Param(lumiButtonSensorThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation &&
                modelIdentifier.startsWith("lumi.sensor_motion")) {

            qCDebug(dcZigbee()) << "This device is a lumi motion sensor";
            if (myThings().filterByThingClassId(lumiMotionSensorThingClassId)
                    .filterByParam(lumiMotionSensorThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new lumi motion sensor";
                ThingDescriptor descriptor(lumiMotionSensorThingClassId);
                descriptor.setTitle(supportedThings().findById(lumiMotionSensorThingClassId).displayName());
                ParamList params;
                params.append(Param(lumiMotionSensorThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }
    }

    return false;
}

bool IntegrationPluginZigbee::createGenericDevice(Thing *networkManagerDevice, ZigbeeNode *node)
{
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        if ((endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceOnOffPlugin) ||
                (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceOnOffPlugin)) {

            // Create generic power socket
            qCDebug(dcZigbee()) << "This device is an power socket";
            if (myThings().filterByThingClassId(genericPowerSocketThingClassId)
                    .filterByParam(genericPowerSocketThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new generic power socket";
                ThingDescriptor descriptor(genericPowerSocketThingClassId);
                QString deviceClassName = supportedThings().findById(genericPowerSocketThingClassId).displayName();
                descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));
                ParamList params;
                params.append(Param(genericPowerSocketThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                params.append(Param(genericPowerSocketThingManufacturerParamTypeId, endpoint->manufacturerName()));
                params.append(Param(genericPowerSocketThingModelParamTypeId, endpoint->modelIdentifier()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        if (endpoint->profile() == Zigbee::ZigbeeProfile::ZigbeeProfileLightLink &&
                endpoint->deviceId() == Zigbee::LightLinkDevice::LightLinkDeviceOnOffLight) {

            // Create generic on/off light
            qCDebug(dcZigbee()) << "This device is an on/off light";
            if (myThings().filterByThingClassId(genericOnOffLightThingClassId)
                    .filterByParam(genericOnOffLightThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new generic on/off light";
                ThingDescriptor descriptor(genericOnOffLightThingClassId);
                QString deviceClassName = supportedThings().findById(genericOnOffLightThingClassId).displayName();
                descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));
                ParamList params;
                params.append(Param(genericOnOffLightThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                params.append(Param(genericOnOffLightThingManufacturerParamTypeId, endpoint->manufacturerName()));
                params.append(Param(genericOnOffLightThingModelParamTypeId, endpoint->modelIdentifier()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        if ((endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceColourTemperatureLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceColourTemperatureLight)) {

            // Create generic power socket
            qCDebug(dcZigbee()) << "This device is an color temperature light";
            if (myThings().filterByThingClassId(genericColorTemperatureLightThingClassId)
                    .filterByParam(genericPowerSocketThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new generic generic color temperature light";
                ThingDescriptor descriptor(genericColorTemperatureLightThingClassId);
                QString deviceClassName = supportedThings().findById(genericColorTemperatureLightThingClassId).displayName();
                descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));
                ParamList params;
                params.append(Param(genericColorTemperatureLightThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                params.append(Param(genericColorTemperatureLightThingManufacturerParamTypeId, endpoint->manufacturerName()));
                params.append(Param(genericColorTemperatureLightThingModelParamTypeId, endpoint->modelIdentifier()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }

        if ((endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceColourLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileLightLink && endpoint->deviceId() == Zigbee::LightLinkDeviceExtendedColourLight) ||
                (endpoint->profile() == Zigbee::ZigbeeProfileHomeAutomation && endpoint->deviceId() == Zigbee::HomeAutomationDeviceExtendedColourLight)) {

            // Create generic color light
            qCDebug(dcZigbee()) << "This device is an color light";
            if (myThings().filterByThingClassId(genericColorLightThingClassId)
                    .filterByParam(genericColorLightThingIeeeAddressParamTypeId, node->extendedAddress().toString())
                    .isEmpty()) {
                qCDebug(dcZigbee()) << "Adding new generic color light";
                ThingDescriptor descriptor(genericColorLightThingClassId);
                QString deviceClassName = supportedThings().findById(genericColorLightThingClassId).displayName();
                descriptor.setTitle(QString("%1 (%2 - %3)").arg(deviceClassName).arg(endpoint->manufacturerName()).arg(endpoint->modelIdentifier()));
                ParamList params;
                params.append(Param(genericColorLightThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                params.append(Param(genericColorLightThingManufacturerParamTypeId, endpoint->manufacturerName()));
                params.append(Param(genericColorLightThingModelParamTypeId, endpoint->modelIdentifier()));
                descriptor.setParams(params);
                descriptor.setParentId(networkManagerDevice->id());
                emit autoThingsAppeared({descriptor});
            } else {
                qCDebug(dcZigbee()) << "The device for this node has already been created.";
            }
            return true;
        }
    }

    return false;
}

void IntegrationPluginZigbee::onZigbeeNetworkStateChanged(ZigbeeNetwork::State state)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Thing *thing = m_zigbeeNetworks.key(zigbeeNetwork);
    if (!thing) return;

    qCDebug(dcZigbee()) << "Controller state changed" << state << thing;

    switch (state) {
    case ZigbeeNetwork::StateUninitialized:
        break;
    case ZigbeeNetwork::StateOffline:
        thing->setStateValue(zigbeeControllerConnectedStateTypeId, false);
        break;
    case ZigbeeNetwork::StateRunning:
        thing->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        thing->setStateValue(zigbeeControllerVersionStateTypeId, zigbeeNetwork->bridgeController()->firmwareVersion());
        thing->setStateValue(zigbeeControllerPanIdStateTypeId, zigbeeNetwork->extendedPanId());
        thing->setStateValue(zigbeeControllerChannelStateTypeId, zigbeeNetwork->channel());
        thing->setStateValue(zigbeeControllerPermitJoinStateTypeId, zigbeeNetwork->permitJoining());
        thing->setStateValue(zigbeeControllerIeeeAddressStateTypeId, zigbeeNetwork->coordinatorNode()->extendedAddress().toString());
        break;
    case ZigbeeNetwork::StateStarting:
        //device->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        break;
    case ZigbeeNetwork::StateStopping:
        //device->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        break;
    }
}

void IntegrationPluginZigbee::onZigbeeNetworkChannelChanged(uint channel)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Thing *thing = m_zigbeeNetworks.key(zigbeeNetwork);
    qCDebug(dcZigbee()) << "Zigbee network channel changed" << channel << thing;
    thing->setStateValue(zigbeeControllerChannelStateTypeId, channel);
}

void IntegrationPluginZigbee::onZigbeeNetworkPanIdChanged(quint64 extendedPanId)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Thing *thing = m_zigbeeNetworks.key(zigbeeNetwork);
    qCDebug(dcZigbee()) << "Zigbee network PAN id changed" << extendedPanId << thing;
    thing->setStateValue(zigbeeControllerPanIdStateTypeId, extendedPanId);
}

void IntegrationPluginZigbee::onZigbeeNetworkPermitJoiningChanged(bool permitJoining)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Thing *thing = m_zigbeeNetworks.key(zigbeeNetwork);
    qCDebug(dcZigbee()) << thing << "permit joining changed" << permitJoining;
    thing->setStateValue(zigbeeControllerPermitJoinStateTypeId, permitJoining);
}

void IntegrationPluginZigbee::onZigbeeNetworkNodeAdded(ZigbeeNode *node)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Thing *networkManagerDevice = m_zigbeeNetworks.key(zigbeeNetwork);
    if (!networkManagerDevice) return;

    qCDebug(dcZigbee()) << "Node added. Check if we recognize this node" << node;
    foreach (ZigbeeNodeEndpoint *endpoint, node->endpoints()) {
        qCDebug(dcZigbee()) << endpoint;
        if (!endpoint->manufacturerName().isEmpty()) {
            qCDebug(dcZigbee()) << "  Manufacturer" << endpoint->manufacturerName();
            qCDebug(dcZigbee()) << "  Model" << endpoint->modelIdentifier();
            qCDebug(dcZigbee()) << "  Version" << endpoint->softwareBuildId();
        }
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

    // Lets see who recognizes this device, if we have no matching device class for this, create a generic node with some information

    // Check ikea devices
    if (node->manufacturerCode() == Zigbee::Manufacturer::Ikea) {
        qCDebug(dcZigbee()) << "This device is from Ikea";
        if (createIkeaDevice(networkManagerDevice, node)) {
            // Recognized and created or already created
            return;
        }
    }

    // Check if this is Lumi
    if (node->manufacturerCode() == 0x1037) {
        // Note: Lumi / Xiaomi / Aquara devices are not in the specs, so no enum here
        qCDebug(dcZigbee()) << "This device is from Lumi";
        if (createLumiDevice(networkManagerDevice, node)) {
            return;
        }
    }

    // Check if we can create a generic device from it
    qCDebug(dcZigbee()) << "Try to create a generic device for this node";
    if (createGenericDevice(networkManagerDevice, node)) {
        // Recognized and created or already created
        return;
    }

    qCWarning(dcZigbee()) << "Could not create a device for this node. Please send the cluster node information to nymea, maybe we can change this :)";
}

void IntegrationPluginZigbee::onZigbeeNetworkNodeRemoved(ZigbeeNode *node)
{
    ZigbeeNetwork *zigbeeNetwork = static_cast<ZigbeeNetwork *>(sender());
    Thing *networkDevice = m_zigbeeNetworks.key(zigbeeNetwork);

    qCDebug(dcZigbee()) << networkDevice << "removed" << node;

    ZigbeeDevice * zigbeeDevice = findNodeZigbeeDevice(node);
    if (!zigbeeDevice) {
        qCWarning(dcZigbee()) << "There is no nymea device for this node" << node;
        return;
    }

    // Clean up
    Thing *thing = m_zigbeeDevices.key(zigbeeDevice);
    m_zigbeeDevices.remove(thing);
    delete zigbeeDevice;

    emit autoThingDisappeared(thing->id());
}
