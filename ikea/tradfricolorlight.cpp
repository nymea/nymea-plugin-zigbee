#include "tradfricolorlight.h"
#include "extern-plugininfo.h"

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
    }

    qCDebug(dcZigbee()) << "Output clusters";
    foreach (ZigbeeCluster *cluster, m_endpoint->outputClusters()) {
        qCDebug(dcZigbee()) << " -" << cluster;
    }

    connect(m_network, &ZigbeeNetwork::stateChanged, this, &TradfriColorLight::onNetworkStateChanged);
    connect(m_endpoint, &ZigbeeNodeEndpoint::clusterAttributeChanged, this, &TradfriColorLight::onClusterAttributeChanged);

    //configureReporting();
}

void TradfriColorLight::identify()
{
    m_endpoint->identify(1);
}

void TradfriColorLight::setPower(bool power)
{
    qCDebug(dcZigbee()) << m_device << "set power" << power;
    m_endpoint->sendOnOffClusterCommand(power ? ZigbeeCluster::OnOffClusterCommandOn : ZigbeeCluster::OnOffClusterCommandOff);
    device()->setStateValue(tradfriColorLightPowerStateTypeId, power);
    readAttribute();
}

void TradfriColorLight::setBrightness(int brightness)
{
    if (brightness > 100)
        brightness = 100;

    if (brightness < 0)
        brightness = 0;

    quint8 level = static_cast<quint8>(qRound(255.0 * brightness / 100.0));
    // Note: time 20 = 2s
    m_endpoint->sendLevelCommand(ZigbeeCluster::LevelClusterCommandMoveToLevel, level, true, 20);
    device()->setStateValue(tradfriColorLightBrightnessStateTypeId, brightness);
}

void TradfriColorLight::setColorTemperature(int colorTemperature)
{
    // Note: time 20 = 2s
    m_endpoint->sendMoveToColorTemperature(static_cast<quint16>(colorTemperature), 20);
    device()->setStateValue(tradfriColorLightColorTemperatureStateTypeId, colorTemperature);
}

void TradfriColorLight::setColor(const QColor &color)
{
    // Note: time 20 = 2s
    m_endpoint->sendMoveToHue(static_cast<quint8>(qRound(color.hue() * 360.0 / 254)), 20);
    m_endpoint->sendMoveToSaturation(static_cast<quint8>(qRound(color.saturation() / 254.0)), 20);
    device()->setStateValue(tradfriColorLightColorStateTypeId, color);
}

void TradfriColorLight::readAttribute()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
        }

        if (cluster->clusterId() == Zigbee::ClusterIdLevelControl) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::LevelClusterAttributeCurrentLevel });
        }

        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
            m_endpoint->readAttribute(cluster, { ZigbeeCluster::ColorControlClusterAttributeColorMode,
                                                 ZigbeeCluster::ColorControlClusterAttributeCurrentX,
                                                 ZigbeeCluster::ColorControlClusterAttributeCurrentY });
        }
    }
}

void TradfriColorLight::configureReporting()
{
    foreach (ZigbeeCluster *cluster, m_endpoint->inputClusters()) {
        if (cluster->clusterId() == Zigbee::ClusterIdOnOff) {
            m_endpoint->configureReporting(cluster, { ZigbeeCluster::OnOffClusterAttributeOnOff });
        }

        if (cluster->clusterId() == Zigbee::ClusterIdLevelControl) {
            m_endpoint->configureReporting(cluster, { ZigbeeCluster::LevelClusterAttributeCurrentLevel });
        }

        if (cluster->clusterId() == Zigbee::ClusterIdColorControl) {
            m_endpoint->configureReporting(cluster, { ZigbeeCluster::ColorControlClusterAttributeColorMode,
                                                      ZigbeeCluster::ColorControlClusterAttributeColorTemperatureMireds });
        }
    }
}

void TradfriColorLight::onNetworkStateChanged(ZigbeeNetwork::State state)
{
    if (state == ZigbeeNetwork::StateRunning) {
        device()->setStateValue(tradfriColorLightConnectedStateTypeId, true);
        readAttribute();
    } else {
        device()->setStateValue(tradfriColorLightConnectedStateTypeId, false);
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
        device()->setStateValue(tradfriColorLightBrightnessStateTypeId, qRound(100.0 * currentLevel / 255.0));
    } else if (cluster->clusterId() == Zigbee::ClusterIdColorControl && attribute.id() == ZigbeeCluster::ColorControlClusterAttributeColorCapabilities) {
//        QByteArray data = attribute.data();
//        quint16 colorTemperature = 0;
//        QDataStream stream(&data, QIODevice::ReadOnly);
//        stream >> colorTemperature;
//        device()->setStateValue(tradfriColorLightColorTemperatureStateTypeId, colorTemperature);
    }
}
