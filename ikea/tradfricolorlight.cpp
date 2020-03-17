#include "tradfricolorlight.h"
#include "extern-plugininfo.h"
#include "nymea-zigbee/zigbeeutils.h"

TradfriColorLight::TradfriColorLight(ZigbeeNetwork *network, ZigbeeAddress ieeeAddress, Device *device, QObject *parent) :
    ZigbeeDevice(network, ieeeAddress, device, parent)
{
    Q_ASSERT_X(m_node, "ZigbeeDevice", "ZigbeeDevice created but the node is not here yet.");

    // Initialize the endpoint
    foreach (ZigbeeNodeEndpoint *endpoit, m_node->endpoints()) {
        if (endpoit->deviceId() == Zigbee::LightLinkDeviceColourLight) {
            m_endpoint = endpoit;
            break;
        }
    }

    Q_ASSERT_X(m_endpoint, "ZigbeeDevice", "ZigbeeDevice created but the endpoint could not be found.");

    qCDebug(dcZigbee()) << m_device << m_endpoint;
    qCDebug(dcZigbee()) << "Input clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
        foreach(const ZigbeeClusterAttribute &attribute, cluster->attributes()) {
            qCDebug(dcZigbee()) << "   - " << attribute;
        }
    }

    qCDebug(dcZigbee()) << "Output clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
        foreach(const ZigbeeClusterAttribute &attribute, cluster->attributes()) {
            qCDebug(dcZigbee()) << "   - " << attribute;
        }
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriColorLight::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &TradfriColorLight::onClusterAttributeChanged);

    //configureReporting();
}

void TradfriColorLight::identify()
{
    m_endpoint->identify(1);
}

void TradfriColorLight::removeFromNetwork()
{
    m_node->leaveNetworkRequest();
}

void TradfriColorLight::checkOnlineStatus()
{
    if (m_network->state() == ZigbeeNetwork::StateRunning) {
        device()->setStateValue(tradfriColorLightConnectedStateTypeId, true);
        device()->setStateValue(tradfriColorLightVersionStateTypeId, m_endpoint->softwareBuildId());
    } else {
        device()->setStateValue(tradfriColorLightConnectedStateTypeId, false);
    }
}

void TradfriColorLight::setPower(bool power)
{
    qCDebug(dcZigbee()) << m_device << "set power" << power;
    m_endpoint->sendOnOffClusterCommand(power ? ZigbeeCluster::OnOffClusterCommandOn : ZigbeeCluster::OnOffClusterCommandOff);
    device()->setStateValue(tradfriColorLightPowerStateTypeId, power);
    readOnOffState();
}

void TradfriColorLight::setBrightness(int brightness)
{
    if (brightness > 100)
        brightness = 100;

    if (brightness < 0)
        brightness = 0;

    quint8 level = static_cast<quint8>(qRound(255.0 * brightness / 100.0));
    // Note: time unit is 1/10 s
    m_endpoint->sendLevelCommand(ZigbeeCluster::LevelClusterCommandMoveToLevel, level, true, 5);
    device()->setStateValue(tradfriColorLightBrightnessStateTypeId, brightness);
    // Note: due to triggersOnOff is true
    device()->setStateValue(tradfriColorLightPowerStateTypeId, (level > 0));
}

void TradfriColorLight::setColorTemperature(int colorTemperature)
{
    // Note: the color temperature command/attribute is not supported. It does support only xy, so we have to interpolate the colors

    int minValue = device()->deviceClass().getStateType(tradfriColorLightColorTemperatureStateTypeId).minValue().toInt();
    int maxValue = device()->deviceClass().getStateType(tradfriColorLightColorTemperatureStateTypeId).maxValue().toInt();
    QColor temperatureColor = ZigbeeUtils::interpolateColorFromColorTemperature(colorTemperature, minValue, maxValue);
    QPointF temperatureColorXy = ZigbeeUtils::convertColorToXY(temperatureColor);
    m_endpoint->sendMoveToColor(temperatureColorXy.x(), temperatureColorXy.y(), 5);
    device()->setStateValue(tradfriColorLightColorTemperatureStateTypeId, colorTemperature);
    readColorXy();
}

void TradfriColorLight::setColor(const QColor &color)
{
    QPointF xyColor = ZigbeeUtils::convertColorToXY(color);
    // Note: time unit is 1/10 s
    m_endpoint->sendMoveToColor(xyColor.x(), xyColor.y(), 5);
    readColorXy();
}

void TradfriColorLight::readColorCapabilities()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
            // Note: set the color once both attribute read
            m_colorAttributesArrived = 0;
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::ColorControlClusterAttributeColorMode,
                                                 ZigbeeCluster::ColorControlClusterAttributeColorCapabilities });
        }
    }
}

void TradfriColorLight::readOnOffState()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
        }
    }
}

void TradfriColorLight::readLevelValue()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdLevelControl) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::LevelClusterAttributeCurrentLevel });
        }
    }
}

void TradfriColorLight::readColorXy()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
            // Note: set the color once both attribute read
            m_colorAttributesArrived = 0;
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::ColorControlClusterAttributeCurrentX,
                                                 ZigbeeCluster::ColorControlClusterAttributeCurrentY });
        }
    }
}

void TradfriColorLight::configureReporting()
{
    //    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
    //        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
    //            m_endpoint->configureReporting(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
    //        }

    //        if (cluster->clusterId() == Zigbee::ClusterIdLevelControl) {
    //            m_endpoint->configureReporting(cluster, { ZigbeeCluster::LevelClusterAttributeCurrentLevel });
    //        }

    //        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
    //            m_endpoint->configureReporting(cluster, { ZigbeeCluster::ColorControlClusterAttributeColorMode,
    //                                                      ZigbeeCluster::ColorControlClusterAttributeColorTemperatureMireds });
    //        }
    //    }
}

void TradfriColorLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    checkOnlineStatus();
    if (state == ZigbeeNetwork::StateRunning) {
        readOnOffState();
        readLevelValue();
        readColorXy();
    }
}

void TradfriColorLight::onClusterAttributeChanged(ZigbeeCluster *cluster, const ZigbeeClusterAttribute &attribute)
{
    qCDebug(dcZigbee()) << device() << "cluster attribute changed" << cluster << attribute;

    if (cluster->clusterId() == Zigbee::ClusterIdOnOff && attribute.id() == ZigbeeCluster::OnOffClusterAttributeOnOff) {
        bool power = static_cast<bool>(attribute.data().at(0));
        device()->setStateValue(tradfriColorLightPowerStateTypeId, power);
    } else if (cluster->clusterId() == Zigbee::ClusterIdLevelControl && attribute.id() == ZigbeeCluster::LevelClusterAttributeCurrentLevel) {
        quint8 currentLevel = static_cast<quint8>(attribute.data().at(0));
        device()->setStateValue(tradfriColorLightBrightnessStateTypeId, qRound(currentLevel * 100.0 / 255.0));
    } else if (cluster->clusterId() == Zigbee::ClusterIdColorControl && attribute.id() == ZigbeeCluster::ColorControlClusterAttributeCurrentX) {
        QByteArray data = attribute.data();
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream >> m_currentX;
        m_colorAttributesArrived++;
        if (m_colorAttributesArrived >= 2) {
            m_colorAttributesArrived = 0;
            // Color x and y read. Calculate color and update state
            QColor color = ZigbeeUtils::convertXYToColor(m_currentX, m_currentY);
            device()->setStateValue(tradfriColorLightColorStateTypeId, color);
        }
    } else if (cluster->clusterId() == Zigbee::ClusterIdColorControl && attribute.id() == ZigbeeCluster::ColorControlClusterAttributeCurrentY) {
        QByteArray data = attribute.data();
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream >> m_currentY;
        m_colorAttributesArrived++;
        if (m_colorAttributesArrived >= 2) {
            m_colorAttributesArrived = 0;
            // Color x and y read. Calculate color and update state
            QColor color = ZigbeeUtils::convertXYToColor(m_currentX, m_currentY);
            device()->setStateValue(tradfriColorLightColorStateTypeId, color);
        }
    }
}
