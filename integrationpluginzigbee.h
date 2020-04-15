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

#include "lumi/lumimagnetsensor.h"
#include "lumi/lumibuttonsensor.h"
#include "lumi/lumimotionsensor.h"
#include "lumi/lumitemperaturesensor.h"

#include "generic/genericonofflight.h"
#include "generic/genericpowersocket.h"
#include "generic/genericcolorlight.h"
#include "generic/genericcolortemperaturelight.h"

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

    bool createIkeaDevice(Thing *networkManagerDevice, ZigbeeNode *node);
    bool createLumiDevice(Thing *networkManagerDevice, ZigbeeNode *node);
    bool createGenericDevice(Thing *networkManagerDevice, ZigbeeNode *node);

private slots:
    void onZigbeeNetworkStateChanged(ZigbeeNetwork::State state);
    void onZigbeeNetworkChannelChanged(uint channel);
    void onZigbeeNetworkPanIdChanged(quint16 panId);
    void onZigbeeNetworkPermitJoiningChanged(bool permitJoining);

    void onZigbeeNetworkNodeAdded(ZigbeeNode *node);
    void onZigbeeNetworkNodeRemoved(ZigbeeNode *node);
};

#endif // INTEGRATIONPLUGINZIGBEE_H
