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
#include "integrationpluginzigbee.h"

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

    if (thing->thingClassId() == xiaomiTemperatureHumidityThingClassId) {
        XiaomiTemperatureSensor *sensor = m_xiaomiTemperatureSensors.value(thing);
        thing->setStateValue(xiaomiTemperatureHumidityConnectedStateTypeId, sensor->connected());
        thing->setStateValue(xiaomiTemperatureHumidityTemperatureStateTypeId, sensor->temperature());
        thing->setStateValue(xiaomiTemperatureHumidityHumidityStateTypeId, sensor->humidity());
    }

    if (thing->thingClassId() == xiaomiMagnetSensorThingClassId) {
        XiaomiMagnetSensor *sensor = m_xiaomiMagnetSensors.value(thing);
        thing->setStateValue(xiaomiMagnetSensorConnectedStateTypeId, sensor->connected());
        thing->setStateValue(xiaomiMagnetSensorClosedStateTypeId, sensor->closed());
    }

    if (thing->thingClassId() == xiaomiButtonSensorThingClassId) {
        XiaomiButtonSensor *sensor = m_xiaomiButtonSensors.value(thing);
        thing->setStateValue(xiaomiButtonSensorConnectedStateTypeId, sensor->connected());
        //thing->setStateValue(xiaomiButtonSensorPressedStateTypeId, sensor->pressed());
    }

    if (thing->thingClassId() == xiaomiMotionSensorThingClassId) {
        XiaomiMotionSensor *sensor = m_xiaomiMotionSensors.value(thing);
        thing->setStateValue(xiaomiMotionSensorConnectedStateTypeId, sensor->connected());
        thing->setStateValue(xiaomiMotionSensorIsPresentStateTypeId, sensor->present());
    }
}

void IntegrationPluginZigbee::thingRemoved(Thing *thing)
{
    qCDebug(dcZigbee()) << "Remove device" << thing->name() << thing->params();

    if (thing->thingClassId() == zigbeeControllerThingClassId) {
        ZigbeeNetworkManager *zigbeeNetworkManager = m_zigbeeControllers.take(thing);
        if (zigbeeNetworkManager) {
            zigbeeNetworkManager->deleteLater();
        }
    }

    if (thing->thingClassId() == xiaomiTemperatureHumidityThingClassId) {
        XiaomiTemperatureSensor *sensor = m_xiaomiTemperatureSensors.take(thing);
        sensor->deleteLater();
    }

    if (thing->thingClassId() == xiaomiMagnetSensorThingClassId) {
        XiaomiMagnetSensor *sensor = m_xiaomiMagnetSensors.take(thing);
        sensor->deleteLater();
    }

    if (thing->thingClassId() == xiaomiButtonSensorThingClassId) {
        XiaomiButtonSensor *sensor = m_xiaomiButtonSensors.take(thing);
        sensor->deleteLater();
    }

    if (thing->thingClassId() == xiaomiMotionSensorThingClassId) {
        XiaomiMotionSensor *sensor = m_xiaomiMotionSensors.take(thing);
        sensor->deleteLater();
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
        ZigbeeNetworkManager *zigbeeNetworkManager = new ZigbeeNetworkManager(this);
        zigbeeNetworkManager->setSerialPortName(thing->paramValue(zigbeeControllerThingSerialPortParamTypeId).toString());
        zigbeeNetworkManager->setSerialBaudrate(static_cast<qint32>(thing->paramValue(zigbeeControllerThingBaudrateParamTypeId).toUInt()));
        zigbeeNetworkManager->setSettingsFileName(NymeaSettings::settingsPath() + "/nymea-zigbee.conf");

        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::stateChanged, this, &IntegrationPluginZigbee::onZigbeeControllerStateChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::channelChanged, this, &IntegrationPluginZigbee::onZigbeeControllerChannelChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::extendedPanIdChanged, this, &IntegrationPluginZigbee::onZigbeeControllerPanIdChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::permitJoiningChanged, this, &IntegrationPluginZigbee::onZigbeeControllerPermitJoiningChanged);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::nodeAdded, this, &IntegrationPluginZigbee::onZigbeeControllerNodeAdded);
        connect(zigbeeNetworkManager, &ZigbeeNetworkManager::nodeRemoved, this, &IntegrationPluginZigbee::onZigbeeControllerNodeRemoved);

        m_zigbeeControllers.insert(thing, zigbeeNetworkManager);

        zigbeeNetworkManager->startNetwork();
    }

    if (thing->thingClassId() == xiaomiTemperatureHumidityThingClassId) {
        qCDebug(dcZigbee()) << "Xiaomi temperature humidiry sensor" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(xiaomiTemperatureHumidityThingIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(thing);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);

        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        XiaomiTemperatureSensor *sensor = new XiaomiTemperatureSensor(node, this);
        connect(sensor, &XiaomiTemperatureSensor::connectedChanged, this, &IntegrationPluginZigbee::onXiaomiTemperatureSensorConnectedChanged);
        connect(sensor, &XiaomiTemperatureSensor::temperatureChanged, this, &IntegrationPluginZigbee::onXiaomiTemperatureSensorTemperatureChanged);
        connect(sensor, &XiaomiTemperatureSensor::humidityChanged, this, &IntegrationPluginZigbee::onXiaomiTemperatureSensorHumidityChanged);
        m_xiaomiTemperatureSensors.insert(thing, sensor);
    }


    if (thing->thingClassId() == xiaomiMagnetSensorThingClassId) {
        qCDebug(dcZigbee()) << "Xiaomi magnet sensor" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(xiaomiMagnetSensorThingIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(thing);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        XiaomiMagnetSensor *sensor = new XiaomiMagnetSensor(node, this);
        connect(sensor, &XiaomiMagnetSensor::connectedChanged, this, &IntegrationPluginZigbee::onXiaomiMagnetSensorConnectedChanged);
        connect(sensor, &XiaomiMagnetSensor::closedChanged, this, &IntegrationPluginZigbee::onXiaomiMagnetSensorClosedChanged);

        m_xiaomiMagnetSensors.insert(thing, sensor);
    }

    if (thing->thingClassId() == xiaomiButtonSensorThingClassId) {
        qCDebug(dcZigbee()) << "Xiaomi button sensor" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(xiaomiButtonSensorThingIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(thing);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        XiaomiButtonSensor *sensor = new XiaomiButtonSensor(node, this);
        connect(sensor, &XiaomiButtonSensor::connectedChanged, this, &IntegrationPluginZigbee::onXiaomiButtonSensorConnectedChanged);
        connect(sensor, &XiaomiButtonSensor::pressedChanged, this, &IntegrationPluginZigbee::onXiaomiButtonSensorPressedChanged);
        connect(sensor, &XiaomiButtonSensor::buttonPressed, this, &IntegrationPluginZigbee::onXiaomiButtonSensorPressed);
        connect(sensor, &XiaomiButtonSensor::buttonLongPressed, this, &IntegrationPluginZigbee::onXiaomiButtonSensorLongPressed);

        m_xiaomiButtonSensors.insert(thing, sensor);
    }

    if (thing->thingClassId() == xiaomiMotionSensorThingClassId) {
        qCDebug(dcZigbee()) << "Xiaomi motion sensor" << thing;
        ZigbeeAddress ieeeAddress(thing->paramValue(xiaomiMotionSensorThingIeeeAddressParamTypeId).toString());
        // Get the parent controller and node for this device
        ZigbeeNetworkManager *zigbeeNetworkManager = findParentController(thing);
        ZigbeeNode *node = zigbeeNetworkManager->getZigbeeNode(ieeeAddress);
        if (!node) {
            qCWarning(dcZigbee()) << "Could not find node for this device. The setup failed";
            return info->finish(Thing::ThingErrorSetupFailed);
        }

        XiaomiMotionSensor *sensor = new XiaomiMotionSensor(node, this);
        connect(sensor, &XiaomiMotionSensor::connectedChanged, this, &IntegrationPluginZigbee::onXiaomiMotionSensorConnectedChanged);
        connect(sensor, &XiaomiMotionSensor::presentChanged, this, &IntegrationPluginZigbee::onXiaomiMotionSensorPresentChanged);
        connect(sensor, &XiaomiMotionSensor::motionDetected, this, &IntegrationPluginZigbee::onXiaomiMotionSensorMotionDetected);

        m_xiaomiMotionSensors.insert(thing, sensor);
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZigbee::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    qCDebug(dcZigbee()) << "Executing action for device" << thing->name() << action.actionTypeId().toString() << action.params();

    if (thing->thingClassId() == zigbeeControllerThingClassId) {
        ZigbeeNetworkManager *networkManager = m_zigbeeControllers.value(thing);
        if (networkManager->state() != ZigbeeNetworkManager::StateRunning)
            return info->finish(Thing::ThingErrorHardwareNotAvailable);

        if (action.actionTypeId() == zigbeeControllerFactoryResetActionTypeId)
            networkManager->factoryResetNetwork();

//        if (action.actionTypeId() == zigbeeControllerTouchlinkActionTypeId)
//            networkManager->controller()->commandInitiateTouchLink();

//        if (action.actionTypeId() == zigbeeControllerTouchlinkResetActionTypeId)
//            networkManager->controller()->commandTouchLinkFactoryReset();

        if (action.actionTypeId() == zigbeeControllerPermitJoinActionTypeId)
            networkManager->setPermitJoining(action.params().paramValue(zigbeeControllerPermitJoinActionPermitJoinParamTypeId).toBool());

    }

    if (thing->thingClassId() == zigbeeNodeThingClassId) {
        ZigbeeNetworkManager *networkManager = findParentController(thing);

        if (!networkManager)
            return info->finish(Thing::ThingErrorHardwareFailure);

        if (networkManager->state() != ZigbeeNetworkManager::StateRunning)
            return info->finish(Thing::ThingErrorHardwareNotAvailable);

        quint16 shortAddress = static_cast<quint16>(thing->paramValue(zigbeeNodeThingNwkAddressParamTypeId).toUInt());
        ZigbeeAddress extendedAddress = ZigbeeAddress(thing->paramValue(zigbeeNodeThingIeeeAddressParamTypeId).toString());

        if (action.actionTypeId() == zigbeeNodeIdentifyActionTypeId) {
            qCDebug(dcZigbee()) << extendedAddress.toString();
        }

        if (action.actionTypeId() == zigbeeNodeLqiRequestActionTypeId) {
            networkManager->controller()->commandRequestLinkQuality(shortAddress);
        }
    }

    return info->finish(Thing::ThingErrorNoError);
}

ZigbeeNetworkManager *IntegrationPluginZigbee::findParentController(Thing *thing) const
{
    foreach (Thing *t, myThings()) {
        if (t->thingClassId() == zigbeeControllerThingClassId && t->id() == thing->parentId()) {
            return m_zigbeeControllers.value(t);
        }
    }

    return nullptr;
}

ZigbeeNetworkManager *IntegrationPluginZigbee::findNodeController(ZigbeeNode *node) const
{
    foreach (ZigbeeNetworkManager *controller, m_zigbeeControllers.values()) {
        if (controller->nodes().contains(node)) {
            return controller;
        }
    }

    return nullptr;
}

Thing *IntegrationPluginZigbee::findNodeThing(ZigbeeNode *node)
{
    foreach (Thing *thing, myThings()) {
        ZigbeeAddress deviceIeeeAddress;
        if (thing->thingClassId() == zigbeeNodeThingClassId) {
            deviceIeeeAddress = ZigbeeAddress (thing->paramValue(zigbeeNodeThingIeeeAddressParamTypeId).toString());
        }

        if (thing->thingClassId() == xiaomiTemperatureHumidityThingClassId) {
            deviceIeeeAddress = ZigbeeAddress (thing->paramValue(xiaomiTemperatureHumidityThingIeeeAddressParamTypeId).toString());
        }

        if (thing->thingClassId() == xiaomiMagnetSensorThingClassId) {
            deviceIeeeAddress = ZigbeeAddress (thing->paramValue(xiaomiMagnetSensorThingIeeeAddressParamTypeId).toString());
        }

        if (thing->thingClassId() == xiaomiButtonSensorThingClassId) {
            deviceIeeeAddress = ZigbeeAddress (thing->paramValue(xiaomiButtonSensorThingIeeeAddressParamTypeId).toString());
        }

        if (thing->thingClassId() == xiaomiMotionSensorThingClassId) {
            deviceIeeeAddress = ZigbeeAddress (thing->paramValue(xiaomiMotionSensorThingIeeeAddressParamTypeId).toString());
        }

        if (node->extendedAddress() == deviceIeeeAddress) {
            return thing;
        }
    }

    return nullptr;
}

void IntegrationPluginZigbee::createThingForNode(Thing *parent, ZigbeeNode *node)
{
    // We already know this device ieee address has not already been added
    // Try to figure out which device this is from the node properties and cluster information

    if (node->hasOutputCluster(Zigbee::ClusterIdBasic)) {
        ZigbeeCluster *basicCluster = node->getOutputCluster(Zigbee::ClusterIdBasic);
        if (basicCluster->hasAttribute(Zigbee::ClusterAttributeBasicModelIdentifier)) {
            QString modelIdentifier = QString::fromUtf8(basicCluster->attribute(Zigbee::ClusterAttributeBasicModelIdentifier).data());

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

                ThingDescriptor descriptor(xiaomiTemperatureHumidityThingClassId);
                descriptor.setParentId(parent->id());
                descriptor.setTitle(tr("Xiaomi temperature and humidity sensor"));

                ParamList params;
                params.append(Param(xiaomiTemperatureHumidityThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoThingsAppeared({ descriptor });
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

                ThingDescriptor descriptor(xiaomiMagnetSensorThingClassId);
                descriptor.setParentId(parent->id());
                descriptor.setTitle(tr("Xiaomi magnet sensor"));

                ParamList params;
                params.append(Param(xiaomiMagnetSensorThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoThingsAppeared({ descriptor });
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

                ThingDescriptor descriptor(xiaomiButtonSensorThingClassId);
                descriptor.setParentId(parent->id());
                descriptor.setTitle(tr("Xiaomi button"));

                ParamList params;
                params.append(Param(xiaomiButtonSensorThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoThingsAppeared({ descriptor });
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

                ThingDescriptor descriptor(xiaomiMotionSensorThingClassId);
                descriptor.setParentId(parent->id());
                descriptor.setTitle(tr("Xiaomi motion sensor"));

                ParamList params;
                params.append(Param(xiaomiMotionSensorThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
                descriptor.setParams(params);

                emit autoThingsAppeared({ descriptor });
                return;
            }
        }
    }

    // If nothing recognized this device, create the generic node device
    //createGenericNodeDeviceForNode(parentDevice, node);
}

void IntegrationPluginZigbee::createGenericNodeThingForNode(Thing *parent, ZigbeeNode *node)
{
    ThingDescriptor descriptor;
    descriptor.setParentId(parent->id());

    if (node->shortAddress() == 0) {
        descriptor.setTitle("Zigbee node (coordinator)");
    } else {
        descriptor.setTitle("Zigbee node");
    }

    ParamList params;
    params.append(Param(zigbeeNodeThingIeeeAddressParamTypeId, node->extendedAddress().toString()));
    params.append(Param(zigbeeNodeThingNwkAddressParamTypeId, QVariant::fromValue(node->shortAddress())));
    descriptor.setParams(params);

    emit autoThingsAppeared({ descriptor });
}

void IntegrationPluginZigbee::onZigbeeControllerStateChanged(ZigbeeNetwork::State state)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Thing *thing = m_zigbeeControllers.key(zigbeeNetworkManager);
    if (!thing) return;

    qCDebug(dcZigbee()) << "Controller state changed" << state << thing;

    switch (state) {
    case ZigbeeNetwork::StateUninitialized:
        break;
    case ZigbeeNetwork::StateDisconnected:
        thing->setStateValue(zigbeeControllerConnectedStateTypeId, false);
        break;
    case ZigbeeNetwork::StateRunning:
        thing->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        thing->setStateValue(zigbeeControllerVersionStateTypeId, zigbeeNetworkManager->controllerFirmwareVersion());
        thing->setStateValue(zigbeeControllerPanIdStateTypeId, zigbeeNetworkManager->extendedPanId());
        thing->setStateValue(zigbeeControllerChannelStateTypeId, zigbeeNetworkManager->channel());
        thing->setStateValue(zigbeeControllerPermitJoinStateTypeId, zigbeeNetworkManager->permitJoining());
        thing->setStateValue(zigbeeControllerIeeeAddressStateTypeId, zigbeeNetworkManager->coordinatorNode()->extendedAddress().toString());

        // Initalize nodes
        foreach (ZigbeeNode *node, zigbeeNetworkManager->nodes()) {
            Thing *thing = findNodeThing(node);
            if (thing) {
                qCDebug(dcZigbee()) << "Devices for" << node << "already created." << thing;
                break;
            }

            createThingForNode(thing, node);
        }

        break;
    case ZigbeeNetwork::StateStarting:
        //thing->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        break;
    case ZigbeeNetwork::StateStopping:
        //thing->setStateValue(zigbeeControllerConnectedStateTypeId, true);
        break;
    }

}

void IntegrationPluginZigbee::onZigbeeControllerChannelChanged(uint channel)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Thing *thing = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << "Zigbee channel changed" << channel << thing;
    thing->setStateValue(zigbeeControllerChannelStateTypeId, channel);
}

void IntegrationPluginZigbee::onZigbeeControllerPanIdChanged(quint64 extendedPanId)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Thing *thing = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << "Zigbee extended PAN id changed" << extendedPanId << thing;
    thing->setStateValue(zigbeeControllerPanIdStateTypeId, extendedPanId);
}

void IntegrationPluginZigbee::onZigbeeControllerPermitJoiningChanged(bool permitJoining)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Thing *thing = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << thing << "permit joining changed" << permitJoining;
    thing->setStateValue(zigbeeControllerPermitJoinStateTypeId, permitJoining);
}

void IntegrationPluginZigbee::onZigbeeControllerNodeAdded(ZigbeeNode *node)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Thing *thing = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << thing << "node added" << thing << node;

    if (findNodeThing(node)) {
        qCDebug(dcZigbee()) << "Device for" << node << "already created." << thing;
        return;
    }

    createThingForNode(thing, node);
}

void IntegrationPluginZigbee::onZigbeeControllerNodeRemoved(ZigbeeNode *node)
{
    ZigbeeNetworkManager *zigbeeNetworkManager = static_cast<ZigbeeNetworkManager *>(sender());
    Thing *thing = m_zigbeeControllers.key(zigbeeNetworkManager);
    qCDebug(dcZigbee()) << thing << "node removed" << node;
    Thing * nodeThing = findNodeThing(node);
    if (!nodeThing) {
        qCWarning(dcZigbee()) << "There is no nymea device for this node" << node;
        return;
    }

    emit autoThingDisappeared(nodeThing->id());
}

void IntegrationPluginZigbee::onXiaomiTemperatureSensorConnectedChanged(bool connected)
{
    XiaomiTemperatureSensor *sensor = static_cast<XiaomiTemperatureSensor *>(sender());
    Thing *thing = m_xiaomiTemperatureSensors.key(sensor);
    thing->setStateValue(xiaomiTemperatureHumidityConnectedStateTypeId, connected);
}

void IntegrationPluginZigbee::onXiaomiTemperatureSensorTemperatureChanged(double temperature)
{
    XiaomiTemperatureSensor *sensor = static_cast<XiaomiTemperatureSensor *>(sender());
    Thing *thing = m_xiaomiTemperatureSensors.key(sensor);
    thing->setStateValue(xiaomiTemperatureHumidityTemperatureStateTypeId, temperature);
    qCDebug(dcZigbee()) << thing << "temperature changed" << temperature << "°C";
}

void IntegrationPluginZigbee::onXiaomiTemperatureSensorHumidityChanged(double humidity)
{
    XiaomiTemperatureSensor *sensor = static_cast<XiaomiTemperatureSensor *>(sender());
    Thing *thing = m_xiaomiTemperatureSensors.key(sensor);
    thing->setStateValue(xiaomiTemperatureHumidityHumidityStateTypeId, humidity);
    qCDebug(dcZigbee()) << thing << "humidity changed" << humidity << "%";
}

void IntegrationPluginZigbee::onXiaomiMagnetSensorConnectedChanged(bool connected)
{
    XiaomiMagnetSensor *sensor = static_cast<XiaomiMagnetSensor *>(sender());
    Thing *thing = m_xiaomiMagnetSensors.key(sensor);
    thing->setStateValue(xiaomiMagnetSensorConnectedStateTypeId, connected);
}

void IntegrationPluginZigbee::onXiaomiMagnetSensorClosedChanged(bool closed)
{
    XiaomiMagnetSensor *sensor = static_cast<XiaomiMagnetSensor *>(sender());
    Thing *thing = m_xiaomiMagnetSensors.key(sensor);
    thing->setStateValue(xiaomiMagnetSensorClosedStateTypeId, closed);
    qCDebug(dcZigbee()) << thing << (closed ? "closed" : "opened");
}

void IntegrationPluginZigbee::onXiaomiButtonSensorConnectedChanged(bool connected)
{
    XiaomiButtonSensor *sensor = static_cast<XiaomiButtonSensor *>(sender());
    Thing *thing = m_xiaomiButtonSensors.key(sensor);
    thing->setStateValue(xiaomiButtonSensorConnectedStateTypeId, connected);
}

void IntegrationPluginZigbee::onXiaomiButtonSensorPressedChanged(bool pressed)
{
    XiaomiButtonSensor *sensor = static_cast<XiaomiButtonSensor *>(sender());
    Thing *thing = m_xiaomiButtonSensors.key(sensor);
    //thing->setStateValue(xiaomiButtonSensorPressedStateTypeId, pressed);
    qCDebug(dcZigbee()) << thing << "Button" << (pressed ? "pressed" : "released");
}

void IntegrationPluginZigbee::onXiaomiButtonSensorPressed()
{
    XiaomiButtonSensor *sensor = static_cast<XiaomiButtonSensor *>(sender());
    Thing *thing = m_xiaomiButtonSensors.key(sensor);
    emitEvent(Event(xiaomiButtonSensorPressedEventTypeId, thing->id()));
    qCDebug(dcZigbee()) << thing << "Button clicked";
}

void IntegrationPluginZigbee::onXiaomiButtonSensorLongPressed()
{
    XiaomiButtonSensor *sensor = static_cast<XiaomiButtonSensor *>(sender());
    Thing *thing = m_xiaomiButtonSensors.key(sensor);
    emitEvent(Event(xiaomiButtonSensorLongPressedEventTypeId, thing->id()));
    qCDebug(dcZigbee()) << thing << "Button long pressed";
}

void IntegrationPluginZigbee::onXiaomiMotionSensorConnectedChanged(bool connected)
{
    XiaomiMotionSensor *sensor = static_cast<XiaomiMotionSensor *>(sender());
    Thing *thing = m_xiaomiMotionSensors.key(sensor);
    thing->setStateValue(xiaomiMotionSensorConnectedStateTypeId, connected);
}

void IntegrationPluginZigbee::onXiaomiMotionSensorPresentChanged(bool present)
{
    XiaomiMotionSensor *sensor = static_cast<XiaomiMotionSensor *>(sender());
    Thing *thing = m_xiaomiMotionSensors.key(sensor);
    thing->setStateValue(xiaomiMotionSensorIsPresentStateTypeId, present);
    qCDebug(dcZigbee()) << thing << "present changed" << present;
}

void IntegrationPluginZigbee::onXiaomiMotionSensorMotionDetected()
{
    XiaomiMotionSensor *sensor = static_cast<XiaomiMotionSensor *>(sender());
    Thing *thing = m_xiaomiMotionSensors.key(sensor);
    thing->setStateValue(xiaomiMotionSensorLastSeenTimeStateTypeId, QDateTime::currentDateTimeUtc().toTime_t());
    qCDebug(dcZigbee()) << thing << "motion detected" << QDateTime::currentDateTimeUtc().toTime_t();
}
