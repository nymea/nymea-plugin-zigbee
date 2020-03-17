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

#include <integrations/integrationplugin.h>
#include <zigbeenetworkmanager.h>

#include "ikea/tradfriremote.h"
#include "ikea/tradfrionoffswitch.h"
#include "ikea/tradfricolorlight.h"
#include "ikea/tradfripowersocket.h"
#include "ikea/tradfricolortemperaturelight.h"
#include "ikea/tradfrirangeextender.h"

#include "feibit/feibitonofflight.h"

#include "lumi/lumimagnetsensor.h"
#include "lumi/lumibuttonsensor.h"
#include "lumi/lumimotionsensor.h"
#include "lumi/lumitemperaturesensor.h"


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
    QHash<Thing *, ZigbeeNetwork *> m_zigbeeNetworks;
    QHash<Thing *, ZigbeeDevice *> m_zigbeeDevices;

    ZigbeeNetwork *findParentNetwork(Thing *thing) const;
    ZigbeeDevice *findNodeZigbeeDevice(ZigbeeNode *node);

private slots:
    void onZigbeeNetworkStateChanged(ZigbeeNetwork::State state);
    void onZigbeeNetworkChannelChanged(uint channel);
    void onZigbeeNetworkPanIdChanged(quint64 extendedPanId);
    void onZigbeeNetworkPermitJoiningChanged(bool permitJoining);
    void onZigbeeNetworkNodeAdded(ZigbeeNode *node);
    void onZigbeeNetworkNodeRemoved(ZigbeeNode *node);
};

#endif // INTEGRATIONPLUGINZIGBEE_H
