#include "./statusinfo.h"

#include "../../connector/syncthingconnection.h"
#include "../../connector/syncthingdev.h"
#include "../../connector/utils.h"
#include "../../model/syncthingicons.h"

#include <QCoreApplication>
#include <QIcon>
#include <QStringBuilder>

using namespace Data;

namespace QtGui {

StatusInfo::StatusInfo()
    : m_statusText(QCoreApplication::translate("QtGui::StatusInfo", "Initializing ..."))
    , m_statusIcon(&statusIcons().disconnected)
{
}

void StatusInfo::recomputeAdditionalStatusText()
{
    if (m_additionalStatusInfo.isEmpty()) {
        m_additionalStatusText = m_additionalDeviceInfo;
    } else if (m_additionalDeviceInfo.isEmpty()) {
        m_additionalStatusText = m_additionalStatusInfo;
    } else if (m_additionalStatusInfo.isEmpty() && m_additionalDeviceInfo.isEmpty()) {
        m_additionalStatusText.clear();
    } else {
        m_additionalStatusText = m_additionalStatusInfo % QChar('\n') % m_additionalDeviceInfo;
    }
}

void StatusInfo::updateConnectionStatus(const SyncthingConnection &connection)
{
    m_additionalStatusText.clear();

    switch (connection.status()) {
    case SyncthingStatus::Disconnected:
        if (connection.autoReconnectInterval() > 0) {
            m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Not connected to Syncthing");
            m_additionalStatusInfo
                = QCoreApplication::translate("QtGui::StatusInfo", "Trying to reconnect every %1 ms").arg(connection.autoReconnectInterval());
        } else {
            m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Not connected to Syncthing");
        }
        m_statusIcon = &statusIcons().disconnected;
        break;
    case SyncthingStatus::Reconnecting:
        m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Reconnecting ...");
        m_statusIcon = &statusIcons().disconnected;
        break;
    default:
        if (connection.hasOutOfSyncDirs()) {
            switch (connection.status()) {
            case SyncthingStatus::Synchronizing:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Synchronization is ongoing");
                m_additionalStatusInfo = QCoreApplication::translate("QtGui::StatusInfo", "At least one directory is out of sync");
                m_statusIcon = &statusIcons().errorSync;
                break;
            default:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "At least one directory is out of sync");
                m_statusIcon = &statusIcons().error;
            }
        } else if (connection.hasUnreadNotifications()) {
            m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Notifications available");
            m_statusIcon = &statusIcons().notify;
        } else {
            switch (connection.status()) {
            case SyncthingStatus::Idle:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Syncthing is idling");
                m_statusIcon = &statusIcons().idling;
                break;
            case SyncthingStatus::Scanning:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Syncthing is scanning");
                m_statusIcon = &statusIcons().scanninig;
                break;
            case SyncthingStatus::Paused:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "At least one device is paused");
                m_statusIcon = &statusIcons().pause;
                break;
            case SyncthingStatus::Synchronizing:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Synchronization is ongoing");
                m_statusIcon = &statusIcons().sync;
                break;
            default:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Status is unknown");
                m_statusIcon = &statusIcons().disconnected;
            }
        }
    }

    recomputeAdditionalStatusText();
}

void StatusInfo::updateConnectedDevices(const SyncthingConnection &connection)
{
    m_additionalDeviceInfo.clear();

    switch (connection.status()) {
    case SyncthingStatus::Idle:
    case SyncthingStatus::OutOfSync:
    case SyncthingStatus::Scanning:
    case SyncthingStatus::Synchronizing: {
        // find devices we're currently connected to
        const auto connectedDevices(connection.connectedDevices());

        // handle case when not connected to other devices
        if (connectedDevices.empty()) {
            m_additionalDeviceInfo = QCoreApplication::translate("QtGui::StatusInfo", "Not connected to other devices");
            return;
        }

        // get up to 2 device names
        const auto deviceCount = trQuandity(connectedDevices.size());
        const auto deviceNames = [&] {
            QStringList deviceNames;
            deviceNames.reserve(2);
            for (const auto *dev : connectedDevices) {
                if (dev->name.isEmpty()) {
                    continue;
                }
                deviceNames << dev->name;
                if (deviceNames.size() > 2) {
                    break;
                }
            }
            return deviceNames;
        }();

        // update status text
        if (deviceNames.empty()) {
            m_additionalDeviceInfo
                = QCoreApplication::translate("QtGui::StatusInfo", "Conntected to %1 devices", nullptr, deviceCount).arg(deviceCount);
        } else if (deviceNames.size() < deviceCount) {
            m_additionalDeviceInfo
                = QCoreApplication::translate("QtGui::StatusInfo", "Conntected to %1 and %2 other devices", nullptr, deviceCount - deviceNames.size())
                      .arg(deviceNames.join(QStringLiteral(", ")))
                      .arg(deviceCount - deviceNames.size());
        } else if (deviceNames.size() == 2) {
            m_additionalDeviceInfo = QCoreApplication::translate("QtGui::StatusInfo", "Conntected to %1 and %2", nullptr, deviceCount)
                                         .arg(deviceNames[0], deviceNames[1]);
        } else if (deviceNames.size() == 1) {
            m_additionalDeviceInfo = QCoreApplication::translate("QtGui::StatusInfo", "Conntected to %1", nullptr, deviceCount).arg(deviceNames[0]);
        }
        break;
    }
    default:;
    }

    recomputeAdditionalStatusText();
}

} // namespace QtGui
