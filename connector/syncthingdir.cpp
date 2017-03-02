#include "./syncthingdir.h"

#include <c++utilities/conversion/stringconversion.h>

#include <QStringBuilder>
#include <QJsonObject>
#include <QCoreApplication>

using namespace ChronoUtilities;
using namespace ConversionUtilities;

namespace Data {

QString statusString(SyncthingDirStatus status)
{
    switch(status) {
    case SyncthingDirStatus::Unknown:
        return QCoreApplication::translate("SyncthingDirStatus", "unknown");
    case SyncthingDirStatus::Idle:
        return QCoreApplication::translate("SyncthingDirStatus", "idle");
    case SyncthingDirStatus::Unshared:
        return QCoreApplication::translate("SyncthingDirStatus", "unshared");
    case SyncthingDirStatus::Scanning:
        return QCoreApplication::translate("SyncthingDirStatus", "scanning");
    case SyncthingDirStatus::Synchronizing:
        return QCoreApplication::translate("SyncthingDirStatus", "synchronizing");
    case SyncthingDirStatus::OutOfSync:
        return QCoreApplication::translate("SyncthingDirStatus", "out of sync");
    default:
        return QString();
    }
}

/*!
 * \brief Assigns the status from the specified status string.
 * \returns Returns whether the status has actually changed.
 */
bool SyncthingDir::assignStatus(const QString &statusStr, ChronoUtilities::DateTime time)
{
    if(lastStatusUpdate > time) {
        return false;
    } else {
        lastStatusUpdate = time;
    }
    SyncthingDirStatus newStatus;
    if(statusStr == QLatin1String("idle")) {
        progressPercentage = 0;
        newStatus = SyncthingDirStatus::Idle;
    } else if(statusStr == QLatin1String("scanning")) {
        newStatus = SyncthingDirStatus::Scanning;
    } else if(statusStr == QLatin1String("syncing")) {
        // ensure status changed signal is emitted
        if(!errors.empty()) {
            status = SyncthingDirStatus::Unknown;
        }
        // errors become obsolete; however errors must be kept as previous errors to be able
        // to identify new errors occuring during this sync attempt as known errors
        previousErrors.clear();
        previousErrors.swap(errors);
        newStatus = SyncthingDirStatus::Synchronizing;
    } else if(statusStr == QLatin1String("error")) {
        progressPercentage = 0;
        newStatus = SyncthingDirStatus::OutOfSync;
    } else {
        newStatus = SyncthingDirStatus::Idle;
    }
    if(newStatus == SyncthingDirStatus::Idle) {
        if(!errors.empty()) {
            newStatus = SyncthingDirStatus::OutOfSync;
        } else if(devices.size() < 2) {
            // FIXME: we can assume only own device is assigned, correct?
            newStatus = SyncthingDirStatus::Unshared;
        }
    }
    if(newStatus != status) {
        switch(status) {
        case SyncthingDirStatus::Scanning:
            lastScanTime = DateTime::now();
            break;
        default:
            ;
        }
        status = newStatus;
        return true;
    }
    return false;
}

bool SyncthingDir::assignStatus(SyncthingDirStatus newStatus, DateTime time)
{
    if(lastStatusUpdate > time) {
        return false;
    } else {
        lastStatusUpdate = time;
    }
    switch(newStatus) {
    case SyncthingDirStatus::Unknown:
    case SyncthingDirStatus::Idle:
    case SyncthingDirStatus::Unshared:
        if(!errors.empty()) {
            newStatus = SyncthingDirStatus::OutOfSync;
        } else if(devices.size() < 2) {
            // FIXME: we can assume only own device is assigned, correct?
            newStatus = SyncthingDirStatus::Unshared;
        }
        break;
    default:
        ;
    }
    if(newStatus != status) {
        switch(status) {
        case SyncthingDirStatus::Scanning:
            lastScanTime = DateTime::now();
            break;
        default:
            ;
        }
        status = newStatus;
        return true;
    }
    return false;
}

QString SyncthingDir::statusString() const
{
    if(paused) {
        return QCoreApplication::translate("SyncthingDir", "paused");
    } else {
        return ::Data::statusString(status);
    }
}

SyncthingItemDownloadProgress::SyncthingItemDownloadProgress(const QString &containingDirPath, const QString &relativeItemPath, const QJsonObject &values) :
    relativePath(relativeItemPath),
    fileInfo(containingDirPath % QChar('/') % QString(relativeItemPath).replace(QChar('\\'), QChar('/'))),
    blocksCurrentlyDownloading(values.value(QStringLiteral("Pulling")).toInt()),
    blocksAlreadyDownloaded(values.value(QStringLiteral("Pulled")).toInt()),
    totalNumberOfBlocks(values.value(QStringLiteral("Total")).toInt()),
    downloadPercentage((blocksAlreadyDownloaded > 0 && totalNumberOfBlocks > 0)
                       ? (static_cast<unsigned int>(blocksAlreadyDownloaded) * 100 / static_cast<unsigned int>(totalNumberOfBlocks))
                       : 0),
    blocksCopiedFromOrigin(values.value(QStringLiteral("CopiedFromOrigin")).toInt()),
    blocksCopiedFromElsewhere(values.value(QStringLiteral("CopiedFromElsewhere")).toInt()),
    blocksReused(values.value(QStringLiteral("Reused")).toInt()),
    bytesAlreadyHandled(values.value(QStringLiteral("BytesDone")).toInt()),
    totalNumberOfBytes(values.value(QStringLiteral("BytesTotal")).toInt()),
    label(QStringLiteral("%1 / %2 - %3 %").arg(
              QString::fromLatin1(dataSizeToString(blocksAlreadyDownloaded > 0 ? static_cast<uint64>(blocksAlreadyDownloaded) * syncthingBlockSize : 0).data()),
              QString::fromLatin1(dataSizeToString(totalNumberOfBlocks > 0 ? static_cast<uint64>(totalNumberOfBlocks) * syncthingBlockSize : 0).data()),
              QString::number(downloadPercentage))
          )
{}

} // namespace Data
