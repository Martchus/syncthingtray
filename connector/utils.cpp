#include "./utils.h"
#include "./syncthingconnection.h"

#include <c++utilities/chrono/datetime.h>
#include <c++utilities/conversion/stringconversion.h>

#include <QCoreApplication>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkInterface>
#include <QString>
#include <QUrl>

using namespace ChronoUtilities;
using namespace ConversionUtilities;

namespace Data {

/*!
 * \brief Returns a string like "2 min 45 s ago" for the specified \a dateTime.
 */
QString agoString(DateTime dateTime)
{
    const TimeSpan delta(DateTime::now() - dateTime);
    if (!delta.isNegative() && static_cast<uint64>(delta.totalTicks()) > (TimeSpan::ticksPerMinute / 4uL)) {
        return QCoreApplication::translate("Data::Utils", "%1 ago")
            .arg(QString::fromUtf8(delta.toString(TimeSpanOutputFormat::WithMeasures, true).data()));
    } else {
        return QCoreApplication::translate("Data::Utils", "right now");
    }
}

/*!
 * \brief Returns the "traffic string" for the specified \a total bytes and the specified \a rate.
 *
 * Eg. "10.2 GiB (45 kib/s)" or only "10.2 GiB" if rate is unknown or "unknown" if both values are unknown.
 */
QString trafficString(uint64 total, double rate)
{
    static const QString unknownStr(QCoreApplication::translate("Data::Utils", "unknown"));
    if (rate != 0.0) {
        return total != SyncthingConnection::unknownTraffic
            ? QStringLiteral("%1 (%2)").arg(QString::fromUtf8(bitrateToString(rate, true).data()), QString::fromUtf8(dataSizeToString(total).data()))
            : QString::fromUtf8(bitrateToString(rate, true).data());
    } else if (total != SyncthingConnection::unknownTraffic) {
        return QString::fromUtf8(dataSizeToString(total).data());
    }
    return unknownStr;
}

/*!
 * \brief Returns whether the host specified by the given \a url is the local machine.
 */
bool isLocal(const QUrl &url)
{
    const QString host(url.host());
    const QHostAddress hostAddress(host);
    return host.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0 || hostAddress.isLoopback()
        || QNetworkInterface::allAddresses().contains(hostAddress);
}

/*!
 * \brief Alters the specified \a syncthingConfig so that the dirs with specified IDs are paused or not.
 * \returns Returns whether the config has been altered (all dirs might have been already paused/unpaused).
 */
bool setDirectoriesPaused(QJsonObject &syncthingConfig, const QStringList &dirIds, bool paused)
{
    bool altered = false;
    QJsonValueRef folders = syncthingConfig.find(QLatin1String("folders")).value();
    if (folders.isArray()) {
        QJsonArray foldersArray = folders.toArray();
        for (QJsonValueRef folder : foldersArray) {
            QJsonObject folderObj = folder.toObject();
            if (dirIds.isEmpty() || dirIds.contains(folderObj.value(QLatin1String("id")).toString())) {
                QJsonValueRef pausedValue = folderObj.find(QLatin1String("paused")).value();
                if (pausedValue.toBool(false) != paused) {
                    pausedValue = paused;
                    folder = folderObj;
                    altered = true;
                }
            }
        }
        if (altered) {
            folders = foldersArray;
        }
    }
    return altered;
}

/*!
 * \brief Alters the specified \a syncthingConfig so that the devs with the specified IDs are paused or not.
 * \returns Returns whether the config has been altered (all devs might have been already paused/unpaused).
 */
bool setDevicesPaused(QJsonObject &syncthingConfig, const QStringList &devIds, bool paused)
{
    bool altered = false;
    QJsonValueRef devices = syncthingConfig.find(QLatin1String("devices")).value();
    if (devices.isArray()) {
        QJsonArray devicesArray = devices.toArray();
        for (QJsonValueRef device : devicesArray) {
            QJsonObject deviceObj = device.toObject();
            if (devIds.isEmpty() || devIds.contains(deviceObj.value(QLatin1String("deviceID")).toString())) {
                QJsonValueRef pausedValue = deviceObj.find(QLatin1String("paused")).value();
                if (pausedValue.toBool(false) != paused) {
                    pausedValue = paused;
                    device = deviceObj;
                    altered = true;
                }
            }
        }
        if (altered) {
            devices = devicesArray;
        }
    }
    return altered;
}
}
