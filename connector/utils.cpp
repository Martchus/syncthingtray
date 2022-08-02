#include "./utils.h"
#include "./syncthingconnection.h"

#include <qtutilities/misc/compat.h>

#include <c++utilities/chrono/datetime.h>
#include <c++utilities/conversion/stringconversion.h>

#include <QCoreApplication>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkInterface>
#include <QString>
#include <QStringBuilder>
#include <QUrl>

using namespace CppUtilities;

namespace Data {

/*!
 * \brief Returns a string like "2 min 45 s ago" for the specified \a dateTime.
 */
QString agoString(DateTime dateTime)
{
    const TimeSpan delta(DateTime::now() - dateTime);
    if (!delta.isNegative() && static_cast<std::uint64_t>(delta.totalTicks()) > (TimeSpan::ticksPerMinute / 4uL)) {
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
QString trafficString(std::uint64_t total, double rate)
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
 * \brief Returns the string for global/local directory status, eg. "5 files, 1 directory, 23.7 MiB".
 */
QString directoryStatusString(const SyncthingStatistics &stats)
{
    return QCoreApplication::translate("Data::Utils", "%1 file(s)", nullptr, trQuandity(stats.files)).arg(stats.files) % QChar(',') % QChar(' ')
        % QCoreApplication::translate("Data::Utils", "%1 dir(s)", nullptr, trQuandity(stats.dirs)).arg(stats.dirs) % QChar(',') % QChar(' ')
        % QString::fromUtf8(dataSizeToString(stats.bytes).data());
}

/*!
 * \brief Returns the "sync complete" notification message for the specified directories.
 */
QString syncCompleteString(const std::vector<const SyncthingDir *> &completedDirs, const SyncthingDev *remoteDevice)
{
    const auto devName(remoteDevice ? remoteDevice->displayName() : QString());
    switch (completedDirs.size()) {
    case 0:
        return QString();
    case 1:
        if (devName.isEmpty()) {
            return QCoreApplication::translate("Data::Utils", "Synchronization of local directory %1 complete")
                .arg(completedDirs.front()->displayName());
        }
        return QCoreApplication::translate("Data::Utils", "Synchronization of %1 on %2 complete").arg(completedDirs.front()->displayName(), devName);
    default:;
    }
    const auto names(things(completedDirs, [](const auto *dir) { return dir->displayName(); }));
    if (devName.isEmpty()) {
        return QCoreApplication::translate("Data::Utils", "Synchronization of the following local directories complete:\n")
            + names.join(QStringLiteral(", "));
    }
    return QCoreApplication::translate("Data::Utils", "Synchronization of the following directories on %1 complete:\n").arg(devName)
        + names.join(QStringLiteral(", "));
}

/*!
 * \brief Returns the string representation of the specified \a rescanInterval.
 */
QString rescanIntervalString(int rescanInterval, bool fileSystemWatcherEnabled)
{
    if (!rescanInterval) {
        if (!fileSystemWatcherEnabled) {
            return QCoreApplication::translate("Data::Utils", "file system watcher and periodic rescan disabled");
        }
        return QCoreApplication::translate("Data::Utils", "file system watcher active, periodic rescan disabled");
    }
    return QString::fromLatin1(TimeSpan::fromSeconds(rescanInterval).toString(TimeSpanOutputFormat::WithMeasures, true).data())
        + (fileSystemWatcherEnabled ? QCoreApplication::translate("Data::Utils", ", file system watcher enabled")
                                    : QCoreApplication::translate("Data::Utils", ", file system watcher disabled"));
}

/*!
 * \brief Returns whether the specified \a hostName is the local machine.
 */
bool isLocal(const QString &hostName)
{
    return isLocal(hostName, QHostAddress(hostName));
}

/*!
 * \brief Returns whether the specified \a hostName and \a hostAddress are the local machine.
 */
bool isLocal(const QString &hostName, const QHostAddress &hostAddress)
{
    return hostName.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0 || hostAddress.isLoopback()
        || QNetworkInterface::allAddresses().contains(hostAddress);
}

/*!
 * \brief Sets the key "paused" of the specified \a object to \a paused.
 * \returns Returns whether object has been altered.
 */
bool setPausedValue(QJsonObject &object, bool paused)
{
    const QJsonObject::Iterator pausedIterator(object.find(QLatin1String("paused")));
    if (pausedIterator == object.end()) {
        object.insert(QLatin1String("paused"), paused);
    } else {
        QJsonValueRef pausedValue = pausedIterator.value();
        if (pausedValue.toBool(false) == paused) {
            return false;
        }
        pausedValue = paused;
    }
    return true;
}

/*!
 * \brief Alters the specified \a syncthingConfig so that the dirs with specified IDs are paused or not.
 * \returns Returns whether the config has been altered (all dirs might have been already paused/unpaused).
 */
bool setDirectoriesPaused(QJsonObject &syncthingConfig, const QStringList &dirIds, bool paused)
{
    // get reference to folders array
    const QJsonObject::Iterator foldersIterator(syncthingConfig.find(QLatin1String("folders")));
    if (foldersIterator == syncthingConfig.end()) {
        return false;
    }
    QJsonValueRef folders = foldersIterator.value();
    if (!folders.isArray()) {
        return false;
    }

    // alter folders
    bool altered = false;
    QJsonArray foldersArray = folders.toArray();
    for (QJsonValueRef folder : foldersArray) {
        QJsonObject folderObj = folder.toObject();

        // skip devices not matching the specified IDs or are already paused/unpaused
        if (!dirIds.isEmpty() && !dirIds.contains(folderObj.value(QLatin1String("id")).toString())) {
            continue;
        }

        // alter paused value
        if (setPausedValue(folderObj, paused)) {
            folder = folderObj;
            altered = true;
        }
    }

    // re-assign altered folders to array reference
    if (altered) {
        folders = foldersArray;
    }

    return altered;
}

/*!
 * \brief Alters the specified \a syncthingConfig so that the devs with the specified IDs are paused or not.
 * \returns Returns whether the config has been altered (all devs might have been already paused/unpaused).
 */
bool setDevicesPaused(QJsonObject &syncthingConfig, const QStringList &devIds, bool paused)
{
    // get reference to devices array
    const QJsonObject::Iterator devicesIterator(syncthingConfig.find(QLatin1String("devices")));
    if (devicesIterator == syncthingConfig.end()) {
        return false;
    }
    QJsonValueRef devices = devicesIterator.value();
    if (!devices.isArray()) {
        return false;
    }

    // alter devices
    bool altered = false;
    QJsonArray devicesArray = devices.toArray();
    for (QJsonValueRef device : devicesArray) {
        QJsonObject deviceObj = device.toObject();

        // skip devices not matching the specified IDs
        if (!devIds.isEmpty() && !devIds.contains(deviceObj.value(QLatin1String("deviceID")).toString())) {
            continue;
        }

        // alter paused value
        if (setPausedValue(deviceObj, paused)) {
            device = deviceObj;
            altered = true;
        }
    }

    // re-assign altered devices to array reference
    if (altered) {
        devices = devicesArray;
    }

    return altered;
}

/*!
 * \brief Substitutes "~" as first element in \a path with \a tilde assuming the elements in \a path
 *        are separated by \a pathSeparator.
 */
QString substituteTilde(const QString &path, const QString &tilde, const QString &pathSeparator)
{
    if (tilde.isEmpty() || pathSeparator.isEmpty() || !path.startsWith(QChar('~'))) {
        return path;
    }
    if (path.size() < 2) {
        return tilde;
    }
    if (QtUtilities::midRef(path, 1).startsWith(pathSeparator)) {
        return tilde % pathSeparator % QtUtilities::midRef(path, 1 + pathSeparator.size());
    }
    return path;
}

} // namespace Data
