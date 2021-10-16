#include "./syncthingdiractions.h"
#include "./syncthingfileitemactionstaticdata.h"

#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/syncthingdir.h>
#include <syncthingconnector/utils.h>

using namespace Data;

SyncthingDirActions::SyncthingDirActions(const SyncthingDir &dir, const SyncthingFileItemActionStaticData *data, QObject *parent)
    : QObject(parent)
    , m_dirId(dir.id)
{
    Q_UNUSED(data)
    const auto &icons = commonForkAwesomeIcons();
    m_infoAction.setSeparator(true);
    m_infoAction.setIcon(icons.folder);
    m_globalStatusAction.setIcon(icons.globe);
    m_localStatusAction.setIcon(icons.home);
    m_lastScanAction.setIcon(icons.clock);
    m_rescanIntervalAction.setIcon(icons.refresh);
    m_errorsAction.setIcon(icons.exclamationTriangle);
    updateStatus(dir);
}

void SyncthingDirActions::updateStatus(const std::vector<SyncthingDir> &dirs)
{
    for (const SyncthingDir &dir : dirs) {
        if (updateStatus(dir)) {
            return;
        }
    }
    m_statusAction.setText(tr("Status: not available anymore"));
    m_statusAction.setIcon(statusIcons().disconnected);
}

bool SyncthingDirActions::updateStatus(const SyncthingDir &dir)
{
    if (dir.id != m_dirId) {
        return false;
    }
    m_infoAction.setText(tr("Directory info for %1").arg(dir.displayName()));
    m_statusAction.setText(tr("Status: ") + dir.statusString());
    if (dir.paused && dir.status != SyncthingDirStatus::OutOfSync) {
        m_statusAction.setIcon(statusIcons().pause);
    } else if (dir.isUnshared()) {
        m_statusAction.setIcon(statusIcons().disconnected);
    } else {
        switch (dir.status) {
        case SyncthingDirStatus::Unknown:
            m_statusAction.setIcon(statusIcons().disconnected);
            break;
        case SyncthingDirStatus::Idle:
        case SyncthingDirStatus::WaitingToClean:
        case SyncthingDirStatus::Cleaning:
            m_statusAction.setIcon(statusIcons().idling);
            break;
        case SyncthingDirStatus::WaitingToScan:
        case SyncthingDirStatus::Scanning:
            m_statusAction.setIcon(statusIcons().scanninig);
            break;
        case SyncthingDirStatus::WaitingToSync:
        case SyncthingDirStatus::PreparingToSync:
        case SyncthingDirStatus::Synchronizing:
            m_statusAction.setIcon(statusIcons().sync);
            break;
        case SyncthingDirStatus::OutOfSync:
            m_statusAction.setIcon(statusIcons().error);
            break;
        }
    }
    m_globalStatusAction.setText(tr("Global: ") + directoryStatusString(dir.globalStats));
    m_localStatusAction.setText(tr("Local: ") + directoryStatusString(dir.localStats));
    m_lastScanAction.setText(tr("Last scan time: ") + agoString(dir.lastScanTime));
    m_rescanIntervalAction.setText(tr("Rescan interval: %1 seconds").arg(dir.rescanInterval));
    if (!dir.pullErrorCount) {
        m_errorsAction.setVisible(false);
    } else {
        m_errorsAction.setVisible(true);
        m_errorsAction.setText(tr("%1 item(s) out-of-sync", nullptr, trQuandity(dir.pullErrorCount)).arg(dir.pullErrorCount));
    }
    return true;
}

QList<QAction *> &operator<<(QList<QAction *> &actions, SyncthingDirActions &dirActions)
{
    return actions << &dirActions.m_infoAction << &dirActions.m_statusAction << &dirActions.m_globalStatusAction << &dirActions.m_localStatusAction
                   << &dirActions.m_lastScanAction << &dirActions.m_rescanIntervalAction << &dirActions.m_errorsAction;
}
