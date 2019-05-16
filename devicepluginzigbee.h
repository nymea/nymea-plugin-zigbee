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

#include "devicemanager.h"
#include "plugin/deviceplugin.h"
#include "zigbeenetworkmanager.h"

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

    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;

private:
    QHash<Device *, ZigbeeNetworkManager *> m_zigbeeControllers;

    ZigbeeNetworkManager *findParentController(Device *device) const;
    ZigbeeNetworkManager *findNodeController(ZigbeeNode *node) const;
    Device *findNodeDevice(ZigbeeNode *node);

    void createDeviceForNode(Device *parentDevice, ZigbeeNode *node);
    void createGenericNodeDeviceForNode(Device *parentDevice, ZigbeeNode *node);

private slots:
    void onZigbeeNodeStateChanged(ZigbeeNode::State nodeState);
    void onZigbeeNodeClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute);

    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);

    void onZigbeeControllerStateChanged(ZigbeeNetwork::State state);
    void onZigbeeControllerChannelChanged(uint channel);
    void onZigbeeControllerPanIdChanged(quint64 extendedPanId);
    void onZigbeeControllerPermitJoiningChanged(bool permitJoining);
    void onZigbeeControllerNodeAdded(ZigbeeNode *node);
    void onZigbeeControllerNodeRemoved(ZigbeeNode *node);
};

#endif // DEVICEPLUGINZIGBEE_H
