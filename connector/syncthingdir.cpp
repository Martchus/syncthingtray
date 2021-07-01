#include "./syncthingdir.h"

#include <c++utilities/conversion/stringconversion.h>

#include <QCoreApplication>
#include <QJsonObject>
#include <QStringBuilder>

using namespace CppUtilities;

namespace Data {

QString statusString(SyncthingDirStatus status)
{
    switch (status) {
    case SyncthingDirStatus::Unknown:
        return QCoreApplication::translate("SyncthingDirStatus", "unknown");
    case SyncthingDirStatus::Idle:
        return QCoreApplication::translate("SyncthingDirStatus", "idle");
    case SyncthingDirStatus::Scanning:
        return QCoreApplication::translate("SyncthingDirStatus", "scanning");
    case SyncthingDirStatus::WaitingToScan:
        return QCoreApplication::translate("SyncthingDirStatus", "waiting to scan");
    case SyncthingDirStatus::WaitingToSync:
        return QCoreApplication::translate("SyncthingDirStatus", "waiting to sync");
    case SyncthingDirStatus::PreparingToSync:
        return QCoreApplication::translate("SyncthingDirStatus", "preparing to sync");
    case SyncthingDirStatus::Synchronizing:
        return QCoreApplication::translate("SyncthingDirStatus", "synchronizing");
    case SyncthingDirStatus::Cleaning:
        return QCoreApplication::translate("SyncthingDirStatus", "cleaning");
    case SyncthingDirStatus::WaitingToClean:
        return QCoreApplication::translate("SyncthingDirStatus", "waiting to clean");
    case SyncthingDirStatus::OutOfSync:
        return QCoreApplication::translate("SyncthingDirStatus", "out of sync");
    }
    return QString();
}

QString dirTypeString(SyncthingDirType dirType)
{
    switch (dirType) {
    case SyncthingDirType::Unknown:
        return QCoreApplication::translate("SyncthingDirType", "unknown");
    case SyncthingDirType::SendReceive:
        return QCoreApplication::translate("SyncthingDirType", "Send & Receive");
    case SyncthingDirType::SendOnly:
        return QCoreApplication::translate("SyncthingDirType", "Send only");
    case SyncthingDirType::ReceiveOnly:
        return QCoreApplication::translate("SyncthingDirType", "Receive only");
    }
    return QString();
}

bool SyncthingDir::checkWhetherStatusUpdateRelevant(DateTime time)
{
    // ignore old updates
    if (lastStatusUpdate > time) {
        return false;
    }
    lastStatusUpdate = time;
    return true;
}

bool SyncthingDir::finalizeStatusUpdate(SyncthingDirStatus newStatus, DateTime time)
{
    // handle obsoletion of out-of-sync items: no FolderErrors are accepted older than the last "sync" state are accepted
    if (newStatus == SyncthingDirStatus::PreparingToSync || newStatus == SyncthingDirStatus::Synchronizing) {
        // update time of last "sync" state and obsolete currently assigned errors
        lastSyncStarted = time; // used internally and not displayed, hence keep it GMT
        itemErrors.clear();
        pullErrorCount = 0;
    } else if (lastSyncStarted.isNull() && newStatus != SyncthingDirStatus::OutOfSync) {
        // prevent adding new errors from "before the first status" if the time of the last "sync" state is unknown
        lastSyncStarted = time;
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
 */
bool SyncthingDir::assignStatus(const QString &statusStr, CppUtilities::DateTime time)
{
    if (!checkWhetherStatusUpdateRelevant(time)) {
        return false;
    }

    // identify statusStr
    SyncthingDirStatus newStatus;
    if (statusStr == QLatin1String("idle")) {
        completionPercentage = 0;
        newStatus = SyncthingDirStatus::Idle;
    } else if (statusStr == QLatin1String("scanning")) {
        newStatus = SyncthingDirStatus::Scanning;
    } else if (statusStr == QLatin1String("scan-waiting")) {
        newStatus = SyncthingDirStatus::WaitingToScan;
    } else if (statusStr == QLatin1String("sync-waiting")) {
        newStatus = SyncthingDirStatus::WaitingToSync;
    } else if (statusStr == QLatin1String("sync-preparing")) {
        // ensure status changed signal is emitted
        if (!itemErrors.empty()) {
            status = SyncthingDirStatus::Unknown;
        }
        newStatus = SyncthingDirStatus::PreparingToSync;
    } else if (statusStr == QLatin1String("syncing")) {
        // ensure status changed signal is emitted
        if (!itemErrors.empty()) {
            status = SyncthingDirStatus::Unknown;
        }
        newStatus = SyncthingDirStatus::Synchronizing;
    } else if (statusStr == QLatin1String("cleaning")) {
        newStatus = SyncthingDirStatus::Cleaning;
    } else if (statusStr == QLatin1String("clean-waiting")) {
        newStatus = SyncthingDirStatus::WaitingToClean;
    } else if (statusStr == QLatin1String("error")) {
        completionPercentage = 0;
        newStatus = SyncthingDirStatus::OutOfSync;
    } else {
        newStatus = SyncthingDirStatus::Idle;
    }

    rawStatus = statusStr;

    return finalizeStatusUpdate(newStatus, time);
}

bool SyncthingDir::assignDirType(const QString &dirTypeStr)
{
    if (dirTypeStr == QLatin1String("sendreceive") || dirTypeStr == QLatin1String("readwrite")) {
        dirType = SyncthingDirType::SendReceive;
    } else if (dirTypeStr == QLatin1String("sendonly") || dirTypeStr == QLatin1String("readonly")) {
        dirType = SyncthingDirType::SendOnly;
    } else if (dirTypeStr == QLatin1String("receiveonly")) {
        dirType = SyncthingDirType::ReceiveOnly;
    } else {
        dirType = SyncthingDirType::Unknown;
        return false;
    }
    return true;
}

QString SyncthingDir::statusString() const
{
    if (paused) {
        return QCoreApplication::translate("SyncthingDir", "paused");
    } else if (isUnshared()) {
        return QCoreApplication::translate("SyncthingDir", "unshared");
    } else if (status == SyncthingDirStatus::Unknown && !rawStatus.isEmpty()) {
        return QString(rawStatus);
    } else {
        return ::Data::statusString(status);
    }
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
    return *this;
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
