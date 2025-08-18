#include "./statusinfo.h"

#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingconnectionsettings.h>
#include <syncthingconnector/syncthingdev.h>
#include <syncthingconnector/utils.h>
#include <syncthingmodel/syncthingicons.h>

#include <QCoreApplication>
#include <QIcon>
#include <QStringBuilder>

using namespace Data;

namespace QtGui {

StatusInfo::StatusInfo(bool textOnly, bool clickToConnect)
    : m_statusText(QCoreApplication::translate("QtGui::StatusInfo", "Initializing …"))
    , m_statusIcon(textOnly ? nullptr : &trayIcons().disconnected)
    , m_textOnly(textOnly)
    , m_clickToConnect(clickToConnect)
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

void StatusInfo::updateConnectionStatus(const SyncthingConnection &connection, const QString &configurationName)
{
    m_additionalStatusInfo.clear();

    const auto *const icons = m_textOnly ? nullptr : &trayIcons();
    switch (connection.status()) {
    case SyncthingStatus::Disconnected:
        if (connection.isConnecting()) {
            m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Connecting to Syncthing …");
        } else {
            m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Not connected to Syncthing");
            if (connection.autoReconnectInterval() > 0) {
                m_additionalStatusInfo
                    = QCoreApplication::translate("QtGui::StatusInfo", "Trying to reconnect every %1 ms").arg(connection.autoReconnectInterval());
            }
            if (m_clickToConnect) {
                const auto newLine = m_additionalStatusInfo.isEmpty() ? QString() : QStringLiteral("\n");
                m_additionalStatusInfo.append(newLine + QCoreApplication::translate("QtGui::StatusInfo", "Tap to connect now"));
            }
        }
        m_statusIcon = icons ? &icons->disconnected : nullptr;
        break;
    case SyncthingStatus::Reconnecting:
        m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Reconnecting …");
        m_statusIcon = icons ? &icons->disconnected : nullptr;
        break;
    default:
        if ((connection.statusComputionFlags() && SyncthingStatusComputionFlags::OutOfSync) && connection.hasOutOfSyncDirs()) {
            switch (connection.status()) {
            case SyncthingStatus::Synchronizing:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Synchronization is ongoing");
                m_additionalStatusInfo = QCoreApplication::translate("QtGui::StatusInfo", "At least one folder is out of sync");
                m_statusIcon = icons ? &icons->errorSync : nullptr;
                break;
            default:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "At least one folder is out of sync");
                m_statusIcon = icons ? &icons->error : nullptr;
            }
        } else if (connection.hasErrors() && (connection.statusComputionFlags() && SyncthingStatusComputionFlags::UnreadNotifications)) {
            m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Notifications available");
            m_statusIcon = icons ? &icons->notify : nullptr;
        } else {
            switch (connection.status()) {
            case SyncthingStatus::Idle:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Syncthing is idling");
                m_statusIcon = icons ? &icons->idling : nullptr;
                break;
            case SyncthingStatus::Scanning:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Syncthing is scanning");
                m_statusIcon = icons ? &icons->scanninig : nullptr;
                break;
            case SyncthingStatus::Paused:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "At least one device is paused");
                m_statusIcon = icons ? &icons->pause : nullptr;
                break;
            case SyncthingStatus::Synchronizing:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Synchronization is ongoing");
                m_statusIcon = icons ? &icons->sync : nullptr;
                break;
            case SyncthingStatus::RemoteNotInSync:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "At least one remote folder is not in sync");
                m_statusIcon = icons ? &icons->sync : nullptr;
                break;
            case SyncthingStatus::NoRemoteConnected:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "No remote device connected");
                m_statusIcon = icons ? &icons->noRemoteConnected : nullptr;
                break;
            default:
                m_statusText = QCoreApplication::translate("QtGui::StatusInfo", "Status is unknown");
                m_statusIcon = icons ? &icons->disconnected : nullptr;
            }
        }
    }

    if (!configurationName.isEmpty()) {
        m_statusText = configurationName % QChar(':') % QChar(' ') % m_statusText;
    }

    recomputeAdditionalStatusText();
}

void StatusInfo::updateConnectedDevices(const SyncthingConnection &connection)
{
    m_additionalDeviceInfo.clear();

    if (connection.isConnected()) {
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
            QStringList names;
            names.reserve(2);
            for (const auto *dev : connectedDevices) {
                if (dev->name.isEmpty()) {
                    continue;
                }
                names << dev->name;
                if (names.size() > 2) {
                    break;
                }
            }
            return names;
        }();

        // update status text
        if (deviceNames.empty()) {
            m_additionalDeviceInfo
                = QCoreApplication::translate("QtGui::StatusInfo", "Connected to %1 devices", nullptr, deviceCount).arg(deviceCount);
        } else if (deviceNames.size() < deviceCount) {
            m_additionalDeviceInfo = QCoreApplication::translate(
                "QtGui::StatusInfo", "Connected to %1 and %2 other devices", nullptr, deviceCount - static_cast<int>(deviceNames.size()))
                                         .arg(deviceNames.join(QStringLiteral(", ")))
                                         .arg(deviceCount - deviceNames.size());
        } else if (deviceNames.size() == 2) {
            m_additionalDeviceInfo = QCoreApplication::translate("QtGui::StatusInfo", "Connected to %1 and %2", nullptr, deviceCount)
                                         .arg(deviceNames[0], deviceNames[1]);
        } else if (deviceNames.size() == 1) {
            m_additionalDeviceInfo = QCoreApplication::translate("QtGui::StatusInfo", "Connected to %1", nullptr, deviceCount).arg(deviceNames[0]);
        }
    }

    recomputeAdditionalStatusText();
}

} // namespace QtGui
