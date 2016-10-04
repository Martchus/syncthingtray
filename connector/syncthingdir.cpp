#include "./syncthingdir.h"

#include <c++utilities/conversion/stringconversion.h>

#include <QStringBuilder>
#include <QJsonObject>

using namespace ChronoUtilities;
using namespace ConversionUtilities;

namespace Data {

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
        newStatus = errors.empty() ? SyncthingDirStatus::Idle : SyncthingDirStatus::OutOfSync;
    } else if(statusStr == QLatin1String("scanning")) {
        newStatus = SyncthingDirStatus::Scanning;
    } else if(statusStr == QLatin1String("syncing")) {
        if(!errors.empty()) {
            errors.clear(); // errors become obsolete
            status = SyncthingDirStatus::Unknown; // ensure status changed signal is emitted
        }
        newStatus = SyncthingDirStatus::Synchronizing;
    } else if(statusStr == QLatin1String("error")) {
        progressPercentage = 0;
        newStatus = SyncthingDirStatus::OutOfSync;
    } else {
        newStatus = errors.empty() ? SyncthingDirStatus::Idle : SyncthingDirStatus::OutOfSync;
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
    case SyncthingDirStatus::Idle:
    case SyncthingDirStatus::Unknown:
        if(!errors.empty()) {
            newStatus = SyncthingDirStatus::OutOfSync;
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
