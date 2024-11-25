#include "./syncthingdir.h"

#include <c++utilities/conversion/stringconversion.h>

#include <QCoreApplication>
#include <QHash>
#include <QJsonObject>
#include <QStringBuilder>

using namespace CppUtilities;

namespace Data {

QString statusString(SyncthingDirStatus status)
{
    switch (status) {
    case SyncthingDirStatus::Unknown:
        return QCoreApplication::translate("SyncthingDir", "Unknown");
    case SyncthingDirStatus::Idle:
        return QCoreApplication::translate("SyncthingDir", "Up to Date");
    case SyncthingDirStatus::WaitingToScan:
        return QCoreApplication::translate("SyncthingDir", "Waiting to Scan");
    case SyncthingDirStatus::Scanning:
        return QCoreApplication::translate("SyncthingDir", "Scanning");
    case SyncthingDirStatus::WaitingToSync:
        return QCoreApplication::translate("SyncthingDir", "Waiting to Sync");
    case SyncthingDirStatus::PreparingToSync:
        return QCoreApplication::translate("SyncthingDir", "Preparing to Sync");
    case SyncthingDirStatus::Synchronizing:
        return QCoreApplication::translate("SyncthingDir", "Syncing");
    case SyncthingDirStatus::Cleaning:
        return QCoreApplication::translate("SyncthingDir", "Cleaning Versions");
    case SyncthingDirStatus::WaitingToClean:
        return QCoreApplication::translate("SyncthingDir", "Waiting to Clean");
    case SyncthingDirStatus::OutOfSync:
        return QCoreApplication::translate("SyncthingDir", "Out of Sync");
    }
    return QString();
}

QString dirTypeString(SyncthingDirType dirType)
{
    switch (dirType) {
    case SyncthingDirType::Unknown:
        return QCoreApplication::translate("SyncthingDirType", "Unknown");
    case SyncthingDirType::SendReceive:
        return QCoreApplication::translate("SyncthingDirType", "Send & Receive");
    case SyncthingDirType::SendOnly:
        return QCoreApplication::translate("SyncthingDirType", "Send Only");
    case SyncthingDirType::ReceiveOnly:
        return QCoreApplication::translate("SyncthingDirType", "Receive Only");
    case SyncthingDirType::ReceiveEncrypted:
        return QCoreApplication::translate("SyncthingDirType", "Receive Encrypted");
    }
    return QString();
}

bool SyncthingDir::checkWhetherStatusUpdateRelevant(SyncthingEventId eventId, DateTime time)
{
    // ignore old updates
    if (lastStatusUpdateEvent > eventId) {
        return false;
    }
    lastStatusUpdateEvent = eventId;
    lastStatusUpdateTime = time;
    return true;
}

bool SyncthingDir::finalizeStatusUpdate(SyncthingDirStatus newStatus, SyncthingEventId eventId, DateTime time)
{
    // handle obsoletion of out-of-sync items: no FolderErrors are accepted older than the last "sync" state are accepted
    if (newStatus == SyncthingDirStatus::PreparingToSync || newStatus == SyncthingDirStatus::Synchronizing) {
        // update time of last "sync" state and obsolete currently assigned errors
        lastSyncStartedEvent = eventId;
        lastSyncStartedTime = time; // used internally and not displayed, hence keep it GMT
        itemErrors.clear();
        pullErrorCount = 0;
    } else if (lastSyncStartedTime.isNull() && newStatus != SyncthingDirStatus::OutOfSync) {
        // prevent adding new errors from "before the first status" if the time of the last "sync" state is unknown
        lastSyncStartedEvent = eventId;
        lastSyncStartedTime = time;
    }

    // clear global error if not out-of-sync anymore
    if (newStatus != SyncthingDirStatus::OutOfSync) {
        globalError.clear();
    }

    // consider the directory still as out-of-sync if there are still pull errors
    // note: Syncthing reports status changes to "idle" despite pull errors. This means we can only rely on reading
    //       a "FolderSummary" event without pull errors for clearing the out-of-sync status.
    if (pullErrorCount && (newStatus == SyncthingDirStatus::Unknown || newStatus == SyncthingDirStatus::Idle)) {
        newStatus = SyncthingDirStatus::OutOfSync;
    }

    if (newStatus == status) {
        return false;
    }

    // update last scan time if the previous status was scanning
    if (status == SyncthingDirStatus::Scanning) {
        // FIXME: better use \a time and convert it from GMT to local time
        lastScanTime = DateTime::now();
    }

    status = newStatus;
    return true;
}

/*!
 * \brief Assigns the status from the specified status string.
 * \returns Returns whether the status has actually changed.
 * \remarks
 * The status mapping is defined by `lib/model/folderstate.go`. Additional statuses are made up on UI-level
 * in `gui/default/syncthing/core/syncthingController.js` but those are not handled by this function.
 */
bool SyncthingDir::assignStatus(const QString &statusStr, SyncthingEventId eventId, CppUtilities::DateTime time)
{
    if (!checkWhetherStatusUpdateRelevant(eventId, time)) {
        return false;
    }

    static const auto statusMapping = QHash<QString, SyncthingDirStatus>{
        { QStringLiteral("idle"), SyncthingDirStatus::Idle },
        { QStringLiteral("scanning"), SyncthingDirStatus::Scanning },
        { QStringLiteral("scan-waiting"), SyncthingDirStatus::WaitingToScan },
        { QStringLiteral("sync-waiting"), SyncthingDirStatus::WaitingToSync },
        { QStringLiteral("sync-preparing"), SyncthingDirStatus::PreparingToSync },
        { QStringLiteral("syncing"), SyncthingDirStatus::Synchronizing },
        { QStringLiteral("cleaning"), SyncthingDirStatus::Cleaning },
        { QStringLiteral("clean-waiting"), SyncthingDirStatus::WaitingToClean },
        { QStringLiteral("error"), SyncthingDirStatus::OutOfSync },
    };

    const auto i = statusMapping.find(statusStr);
    const auto newStatus = i != statusMapping.cend() ? *i : SyncthingDirStatus::Unknown;
    switch (newStatus) {
    case SyncthingDirStatus::Idle:
    case SyncthingDirStatus::OutOfSync:
        completionPercentage = 0;
        break;
    case Data::SyncthingDirStatus::WaitingToSync:
    case Data::SyncthingDirStatus::PreparingToSync:
        // ensure status changed signal is emitted
        if (!itemErrors.empty()) {
            status = SyncthingDirStatus::Unknown;
        }
        break;
    default:;
    }

    rawStatus = statusStr;
    return finalizeStatusUpdate(newStatus, eventId, time);
}

bool SyncthingDir::assignDirType(const QString &dirTypeStr)
{
    static const auto typeMapping = QHash<QString, SyncthingDirType>{
        { QStringLiteral("sendreceive"), SyncthingDirType::SendReceive },
        { QStringLiteral("readwrite"), SyncthingDirType::SendReceive },
        { QStringLiteral("sendonly"), SyncthingDirType::SendOnly },
        { QStringLiteral("readonly"), SyncthingDirType::SendOnly },
        { QStringLiteral("receiveonly"), SyncthingDirType::ReceiveOnly },
        { QStringLiteral("receiveencrypted"), SyncthingDirType::ReceiveEncrypted },
    };
    const auto i = typeMapping.find(dirTypeStr);
    dirType = i != typeMapping.cend() ? *i : SyncthingDirType::Unknown;
    return dirType != SyncthingDirType::Unknown;
}

/*!
 * \brief Returns a status string for the directory.
 * \remarks
 * This function does not only take the SyncthingDirStatus into account but also other properties of \a dir. This function is therefore
 * similar to the function `$scope.folderStatus` of Syncthing's official UI (see `gui/default/syncthing/core/syncthingController.js`).
 */
QString SyncthingDir::statusString() const
{
    if (paused) {
        return QCoreApplication::translate("SyncthingDir", "Paused");
    }
    if (isUnshared()) {
        return QCoreApplication::translate("SyncthingDir", "Unshared");
    }
    switch (status) {
    case SyncthingDirStatus::Unknown:
        if (!rawStatus.isEmpty()) {
            return rawStatus;
        }
        break;
    case SyncthingDirStatus::Idle:
        if (receiveOnlyStats.total > 0) {
            switch (dirType) {
            case SyncthingDirType::ReceiveOnly:
                return QCoreApplication::translate("SyncthingDir", "Local Additions");
            case SyncthingDirType::ReceiveEncrypted:
                return QCoreApplication::translate("SyncthingDir", "Unexpected Items");
            default:;
            }
        }
        break;
    case SyncthingDirStatus::Scanning:
        if (scanningPercentage > 0) {
            if (scanningRate != 0.0) {
                return QCoreApplication::translate("SyncthingDir", "Scanning (%1 %, %2)")
                    .arg(scanningPercentage)
                    .arg(bitrateToString(scanningRate * 0.008, true).data());
            }
            return QCoreApplication::translate("SyncthingDir", "Scanning (%1 %)").arg(scanningPercentage);
        }
        break;
    case SyncthingDirStatus::Synchronizing:
        if (completionPercentage > 0) {
            return QCoreApplication::translate("SyncthingDir", "Syncing (%1 %)").arg(completionPercentage);
        }
        break;
    default:;
    }
    return Data::statusString(status);
}

QtUtilities::StringView SyncthingDir::pathWithoutTrailingSlash() const
{
    auto dirPath = QtUtilities::makeStringView(path);
    while (dirPath.endsWith(QChar('/'))) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
        dirPath.chop(1);
#else
        dirPath = dirPath.left(dirPath.size() - 1);
#endif
    }
    return dirPath;
}

bool SyncthingDir::areRemotesUpToDate() const
{
    for (const auto &completionForDev : completionByDevice) {
        if (!completionForDev.second.needed.isNull()) {
            return false;
        }
    }
    return true;
}

SyncthingItemDownloadProgress::SyncthingItemDownloadProgress(
    const QString &containingDirPath, const QString &relativeItemPath, const QJsonObject &values)
    : relativePath(relativeItemPath)
    , fileInfo(containingDirPath % QChar('/') % QString(relativeItemPath).replace(QChar('\\'), QChar('/')))
    , blocksCurrentlyDownloading(values.value(QLatin1String("Pulling")).toInt())
    , blocksAlreadyDownloaded(values.value(QLatin1String("Pulled")).toInt())
    , totalNumberOfBlocks(values.value(QLatin1String("Total")).toInt())
    , downloadPercentage((blocksAlreadyDownloaded > 0 && totalNumberOfBlocks > 0)
              ? (static_cast<unsigned int>(blocksAlreadyDownloaded) * 100 / static_cast<unsigned int>(totalNumberOfBlocks))
              : 0)
    , blocksCopiedFromOrigin(values.value(QLatin1String("CopiedFromOrigin")).toInt())
    , blocksCopiedFromElsewhere(values.value(QLatin1String("CopiedFromElsewhere")).toInt())
    , blocksReused(values.value(QLatin1String("Reused")).toInt())
    , bytesAlreadyHandled(values.value(QLatin1String("BytesDone")).toInt())
    , totalNumberOfBytes(values.value(QLatin1String("BytesTotal")).toInt())
    , label(QStringLiteral("%1 / %2 - %3 %")
                .arg(QString::fromLatin1(
                         dataSizeToString(blocksAlreadyDownloaded > 0 ? static_cast<std::uint64_t>(blocksAlreadyDownloaded) * syncthingBlockSize : 0)
                             .data()),
                    QString::fromLatin1(
                        dataSizeToString(totalNumberOfBlocks > 0 ? static_cast<std::uint64_t>(totalNumberOfBlocks) * syncthingBlockSize : 0).data()),
                    QString::number(downloadPercentage)))
{
}

SyncthingStatistics &SyncthingStatistics::operator+=(const SyncthingStatistics &other)
{
    bytes += other.bytes;
    deletes += other.deletes;
    dirs += other.dirs;
    files += other.files;
    symlinks += other.symlinks;
    total += other.total;
    return *this;
}

QString Data::SyncthingStatistics::bytesAsString() const
{
    return QString::fromStdString(CppUtilities::dataSizeToString(bytes));
}

/*!
 * \brief Computes overall statistics for the specified \a directories.
 */
SyncthingOverallDirStatistics::SyncthingOverallDirStatistics(const std::vector<SyncthingDir> &directories)
{
    for (const auto &dir : directories) {
        local += dir.localStats;
        global += dir.globalStats;
        needed += dir.neededStats;
    }
}

} // namespace Data
