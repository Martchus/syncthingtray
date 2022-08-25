#include "./syncthingapplet.h"
#include "./settingsdialog.h"

#include <syncthingconnector/syncthingservice.h>
#include <syncthingconnector/utils.h>

#include <syncthingwidgets/misc/direrrorsdialog.h>
#include <syncthingwidgets/misc/internalerrorsdialog.h>
#include <syncthingwidgets/misc/otherdialogs.h>
#include <syncthingwidgets/misc/textviewdialog.h>
#include <syncthingwidgets/settings/settings.h>
#include <syncthingwidgets/settings/settingsdialog.h>
#include <syncthingwidgets/webview/webviewdialog.h>

#include <syncthingmodel/syncthingicons.h>

#include <syncthingconnector/utils.h>

#include "resources/config.h"
#include "resources/qtconfig.h"

#include <qtforkawesome/utils.h>
#include <qtquickforkawesome/imageprovider.h>

#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/resources/resources.h>

#include <c++utilities/application/argumentparser.h>
#include <c++utilities/conversion/stringconversion.h>

#include <KConfigGroup>

#include <Plasma/Theme>
#include <plasma/plasma_export.h>
#include <plasma_version.h>

#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QMap>
#include <QNetworkReply>
#include <QPalette>
#include <QQmlEngine>
#include <QStringBuilder>

using namespace std;
using namespace Data;
using namespace Plasma;
using namespace CppUtilities;
using namespace QtUtilities;
using namespace QtGui;

namespace Plasmoid {

static inline QPalette paletteFromTheme(const Plasma::Theme &theme)
{
#if 0 && PLASMA_VERSION_MAJOR >= 5 && PLASMA_VERSION_MINOR >= 68
    return theme.palette();
#else
    auto p = QPalette();
    p.setColor(QPalette::Normal, QPalette::Text, theme.color(Plasma::Theme::TextColor));
    return p;
#endif
}

SyncthingApplet::SyncthingApplet(QObject *parent, const QVariantList &data)
    : Applet(parent, data)
    , m_faUrl(QStringLiteral("image://fa/"))
    , m_iconManager(IconManager::instance(&m_palette))
    , m_aboutDlg(nullptr)
    , m_connection()
    , m_notifier(m_connection)
    , m_dirModel(m_connection)
    , m_sortFilterDirModel(&m_dirModel)
    , m_devModel(m_connection)
    , m_sortFilterDevModel(&m_devModel)
    , m_downloadModel(m_connection)
    , m_recentChangesModel(m_connection)
    , m_settingsDlg(nullptr)
    , m_imageProvider(nullptr)
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    , m_webViewDlg(nullptr)
#endif
    , m_currentConnectionConfig(-1)
    , m_hasInternalErrors(false)
    , m_initialized(false)
{
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    m_notifier.setService(&m_service);
#endif
    m_sortFilterDirModel.sort(0, Qt::AscendingOrder);
    m_sortFilterDevModel.sort(0, Qt::AscendingOrder);
    qmlRegisterUncreatableMetaObject(Data::staticMetaObject, "martchus.syncthingplasmoid", 0, 6, "Data", QStringLiteral("only enums"));
}

SyncthingApplet::~SyncthingApplet()
{
    delete m_settingsDlg;
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    delete m_webViewDlg;
#endif
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    SyncthingService::setMainInstance(nullptr);
#endif
}

void SyncthingApplet::init()
{
    LOAD_QT_TRANSLATIONS;
    setupCommonQtApplicationAttributes();

    Applet::init();

    // connect signals and slots
    connect(&m_notifier, &SyncthingNotifier::statusChanged, this, &SyncthingApplet::handleConnectionStatusChanged);
    connect(&m_notifier, &SyncthingNotifier::syncComplete, &m_dbusNotifier, &DBusStatusNotifier::showSyncComplete);
    connect(&m_notifier, &SyncthingNotifier::disconnected, &m_dbusNotifier, &DBusStatusNotifier::showDisconnect);
    connect(&m_connection, &SyncthingConnection::newDevices, this, &SyncthingApplet::handleDevicesChanged);
    connect(&m_connection, &SyncthingConnection::devStatusChanged, this, &SyncthingApplet::handleDevicesChanged);
    connect(&m_connection, &SyncthingConnection::error, this, &SyncthingApplet::handleInternalError);
    connect(&m_connection, &SyncthingConnection::trafficChanged, this, &SyncthingApplet::trafficChanged);
    connect(&m_connection, &SyncthingConnection::dirStatisticsChanged, this, &SyncthingApplet::handleDirStatisticsChanged);
    connect(&m_connection, &SyncthingConnection::newNotification, this, &SyncthingApplet::handleNewNotification);
    connect(&m_notifier, &SyncthingNotifier::newDevice, &m_dbusNotifier, &DBusStatusNotifier::showNewDev);
    connect(&m_notifier, &SyncthingNotifier::newDir, &m_dbusNotifier, &DBusStatusNotifier::showNewDir);
    connect(&m_dbusNotifier, &DBusStatusNotifier::connectRequested, &m_connection,
        static_cast<void (SyncthingConnection::*)(void)>(&SyncthingConnection::connect));
    connect(&m_dbusNotifier, &DBusStatusNotifier::dismissNotificationsRequested, this, &SyncthingApplet::dismissNotifications);
    connect(&m_dbusNotifier, &DBusStatusNotifier::showNotificationsRequested, this, &SyncthingApplet::showNotificationsDialog);
    connect(&m_dbusNotifier, &DBusStatusNotifier::errorDetailsRequested, this, &SyncthingApplet::showInternalErrorsDialog);
    connect(&m_dbusNotifier, &DBusStatusNotifier::webUiRequested, this, &SyncthingApplet::showWebUI);
    connect(&m_iconManager, &IconManager::statusIconsChanged, this, &SyncthingApplet::connectionStatusChanged);
    connect(&m_theme, &Plasma::Theme::themeChanged, this, &SyncthingApplet::handleThemeChanged);

    // restore settings
    Settings::restore();

    // initialize systemd service support
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    SyncthingService::setMainInstance(&m_service);
    Settings::values().systemd.setupService(m_service);
    connect(&m_service, &SyncthingService::systemdAvailableChanged, this, &SyncthingApplet::handleSystemdStatusChanged);
    connect(&m_service, &SyncthingService::stateChanged, this, &SyncthingApplet::handleSystemdStatusChanged);
    connect(&m_service, &SyncthingService::errorOccurred, this, &SyncthingApplet::handleSystemdServiceError);
#endif

    // load primary connection config
    m_currentConnectionConfig = config().readEntry<int>("selectedConfig", 0);

    // apply settings and connect according to settings
    setBrightColors(isPaletteDark(paletteFromTheme(m_theme)));
    handleSettingsChanged();

    m_initialized = true;
}

void SyncthingApplet::initEngine(QObject *object)
{
    const auto engine = qmlEngine(object);
    if (!engine) {
        return;
    }
    const auto color = m_theme.color(Plasma::Theme::TextColor, Plasma::Theme::NormalColorGroup);
    m_imageProvider = new QtForkAwesome::QuickImageProvider(m_iconManager.forkAwesomeRenderer(), color);
    connect(engine, &QObject::destroyed, this, &SyncthingApplet::handleImageProviderDestroyed); // engine has ownership over image provider
    engine->addImageProvider(QStringLiteral("fa"), m_imageProvider);
}

QIcon SyncthingApplet::statusIcon() const
{
    return m_statusInfo.statusIcon();
}

QString SyncthingApplet::incomingTraffic() const
{
    return trafficString(m_connection.totalIncomingTraffic(), m_connection.totalIncomingRate());
}

bool SyncthingApplet::hasIncomingTraffic() const
{
    return m_connection.totalIncomingRate() > 0.0;
}

QString SyncthingApplet::outgoingTraffic() const
{
    return trafficString(m_connection.totalOutgoingTraffic(), m_connection.totalOutgoingRate());
}

bool SyncthingApplet::hasOutgoingTraffic() const
{
    return m_connection.totalOutgoingRate() > 0.0;
}

SyncthingStatistics SyncthingApplet::globalStatistics() const
{
    return m_overallStats.global;
}

SyncthingStatistics SyncthingApplet::localStatistics() const
{
    return m_overallStats.local;
}

QStringList SyncthingApplet::connectionConfigNames() const
{
    const auto &settings = Settings::values().connection;
    QStringList names;
    names.reserve(static_cast<int>(settings.secondary.size() + 1));
    names << settings.primary.label;
    for (const auto &setting : settings.secondary) {
        names << setting.label;
    }
    return names;
}

QString SyncthingApplet::currentConnectionConfigName() const
{
    const auto &settings = Settings::values().connection;
    if (m_currentConnectionConfig == 0) {
        return settings.primary.label;
    } else if (m_currentConnectionConfig > 0 && static_cast<unsigned>(m_currentConnectionConfig) <= settings.secondary.size()) {
        return settings.secondary[static_cast<unsigned>(m_currentConnectionConfig) - 1].label;
    }
    return QString();
}

Data::SyncthingConnectionSettings *SyncthingApplet::connectionConfig(int index)
{
    auto &connectionSettings = Settings::values().connection;
    if (index >= 0 && static_cast<unsigned>(index) <= connectionSettings.secondary.size()) {
        return index == 0 ? &connectionSettings.primary : &connectionSettings.secondary[static_cast<unsigned>(index) - 1];
    }
    return nullptr;
}

void SyncthingApplet::setCurrentConnectionConfigIndex(int index)
{
    auto &settings = Settings::values();
    bool reconnectRequired = false;
    if (index != m_currentConnectionConfig && index >= 0 && static_cast<unsigned>(index) <= settings.connection.secondary.size()) {
        auto &selectedConfig = index == 0 ? settings.connection.primary : settings.connection.secondary[static_cast<unsigned>(index) - 1];
        reconnectRequired = m_connection.applySettings(selectedConfig);
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
        if (m_webViewDlg) {
            m_webViewDlg->applySettings(selectedConfig, false);
        }
#endif
        config().writeEntry<int>("selectedConfig", index);
        emit currentConnectionConfigIndexChanged(m_currentConnectionConfig = index);
        emit localChanged();
    }

    // apply systemd settings, reconnect if required and possible
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto systemdConsideredForReconnect
        = settings.systemd.apply(m_connection, currentConnectionConfig(), reconnectRequired).consideredForReconnect;
#else
    const auto systemdConsideredForReconnect = false;
#endif
    if (!systemdConsideredForReconnect && (reconnectRequired || !m_connection.isConnected())) {
        m_connection.reconnect();
    }
}

bool SyncthingApplet::isStartStopEnabled() const
{
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    return Settings::values().systemd.showButton;
#else
    return false;
#endif
}

bool SyncthingApplet::hasInternalErrors() const
{
    return m_hasInternalErrors;
}

bool SyncthingApplet::areNotificationsAvailable() const
{
    return !m_notifications.empty();
}

void SyncthingApplet::setPassiveStates(const QList<QtUtilities::ChecklistItem> &passiveStates)
{
    m_passiveSelectionModel.setItems(passiveStates);
    const auto currentState = static_cast<int>(m_connection.status());
    setPassive(currentState >= 0 && currentState < passiveStates.size() && passiveStates.at(currentState).isChecked());
}

void SyncthingApplet::updateStatusIconAndTooltip()
{
    m_statusInfo.updateConnectionStatus(m_connection);
    m_statusInfo.updateConnectedDevices(m_connection);
    emit connectionStatusChanged();
}

QIcon SyncthingApplet::loadForkAwesomeIcon(const QString &name, int size) const
{
    const auto icon = QtForkAwesome::iconFromId(name);
    return QtForkAwesome::isIconValid(icon)
        ? QIcon(IconManager::instance().forkAwesomeRenderer().pixmap(icon, QSize(size, size), QGuiApplication::palette().color(QPalette::WindowText)))
        : QIcon();
}

QString SyncthingApplet::formatFileSize(quint64 fileSizeInByte) const
{
    return QString::fromStdString(dataSizeToString(fileSizeInByte));
}

QString SyncthingApplet::substituteTilde(const QString &path) const
{
    return Data::substituteTilde(path, m_connection.tilde(), m_connection.pathSeparator());
}

void SyncthingApplet::showSettingsDlg()
{
    if (!m_settingsDlg) {
        m_settingsDlg = new SettingsDialog(*this);
        // ensure settings take effect when applied
        connect(m_settingsDlg, &SettingsDialog::applied, this, &SyncthingApplet::handleSettingsChanged);
        // save plasmoid specific settings to disk when applied
        connect(m_settingsDlg, &SettingsDialog::applied, this, &SyncthingApplet::configChanged);
        // save global/general settings to disk when applied
        connect(m_settingsDlg, &SettingsDialog::applied, &Settings::save);
    }
    centerWidget(m_settingsDlg);
    m_settingsDlg->show();
    m_settingsDlg->activateWindow();
}

void SyncthingApplet::showWebUI()
{
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    if (Settings::values().webView.disabled) {
#endif
        QDesktopServices::openUrl(m_connection.syncthingUrl());
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    } else {
        if (!m_webViewDlg) {
            m_webViewDlg = new WebViewDialog;
            if (const auto *connectionConfig = currentConnectionConfig()) {
                m_webViewDlg->applySettings(*connectionConfig, true);
            }
            connect(m_webViewDlg, &WebViewDialog::destroyed, this, &SyncthingApplet::handleWebViewDeleted);
        }
        m_webViewDlg->show();
        m_webViewDlg->activateWindow();
    }
#endif
}

void SyncthingApplet::showLog()
{
    auto *const dlg = TextViewDialog::forLogEntries(m_connection);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
}

void SyncthingApplet::showOwnDeviceId()
{
    auto *const dlg = ownDeviceIdDialog(m_connection);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
}

void SyncthingApplet::showAboutDialog()
{
    if (!m_aboutDlg) {
        m_aboutDlg = new AboutDialog(nullptr, QStringLiteral(APP_NAME), aboutDialogAttribution(), QStringLiteral(APP_VERSION),
            CppUtilities::applicationInfo.dependencyVersions, QStringLiteral(APP_URL), QStringLiteral(APP_DESCRIPTION), aboutDialogImage());
        m_aboutDlg->setWindowTitle(tr("About") + QStringLiteral(" - " APP_NAME));
        m_aboutDlg->setWindowIcon(QIcon::fromTheme(QStringLiteral("syncthingtray")));
        m_aboutDlg->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_aboutDlg, &QObject::destroyed, this, &SyncthingApplet::handleAboutDialogDeleted);
    }
    centerWidget(m_aboutDlg);
    m_aboutDlg->show();
    m_aboutDlg->activateWindow();
}

void SyncthingApplet::showNotificationsDialog()
{
    auto *const dlg = TextViewDialog::forLogEntries(m_notifications, tr("New notifications"));
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
    dismissNotifications();
}

void SyncthingApplet::dismissNotifications()
{
    m_connection.considerAllNotificationsRead();
    if (m_notifications.empty()) {
        return;
    }
    m_notifications.clear();
    emit notificationsAvailableChanged(false);

    // update status as well because having or not having notifications is relevant for status text/icon
    updateStatusIconAndTooltip();
}

void SyncthingApplet::showInternalErrorsDialog()
{
    auto *const errorViewDlg = InternalErrorsDialog::instance();
    connect(errorViewDlg, &InternalErrorsDialog::errorsCleared, this, &SyncthingApplet::handleErrorsCleared);
    centerWidget(errorViewDlg);
    errorViewDlg->show();
}

void SyncthingApplet::showDirectoryErrors(unsigned int directoryIndex)
{
    const auto &dirs = m_connection.dirInfo();
    if (directoryIndex >= dirs.size()) {
        return;
    }

    const auto &dir(dirs[directoryIndex]);
    m_connection.requestDirPullErrors(dir.id);

    auto *const dlg = new DirectoryErrorsDialog(m_connection, dir);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    centerWidget(dlg);
    dlg->show();
}

void SyncthingApplet::copyToClipboard(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

/*!
 * \brief Ensures settings take effect when applied via the settings dialog.
 * \remarks Does not save the settings to disk. This is done in Settings::save() and Applet::configChanged().
 */
void SyncthingApplet::handleSettingsChanged()
{
    const KConfigGroup config(this->config());
    const auto &settings(Settings::values());

    // apply notifiction settings
    settings.apply(m_notifier);

    // apply appearance settings
    setSize(config.readEntry<QSize>("size", QSize(25, 25)));
    IconManager::instance().applySettings(&settings.icons.status);

    // restore selected states
    // note: The settings dialog writes this to the Plasmoid's config like the other settings. However, it
    //       is simpler and more efficient to assign the states directly. Of course this is only possible if
    //       the dialog has already been shown.
    if (m_settingsDlg) {
        setPassiveStates(m_settingsDlg->appearanceOptionPage()->passiveStatusSelection()->items());
    } else {
        m_passiveSelectionModel.applyVariantList(config.readEntry("passiveStates", QVariantList()));
    }

    // apply connection config
    const int currentConfig = m_currentConnectionConfig;
    m_currentConnectionConfig = -1; // force update
    setCurrentConnectionConfigIndex(currentConfig);

    // update status icons and tooltip because the reconnect interval might have changed
    updateStatusIconAndTooltip();

    emit settingsChanged();
}

void SyncthingApplet::handleConnectionStatusChanged(Data::SyncthingStatus previousStatus, Data::SyncthingStatus newStatus)
{
    Q_UNUSED(previousStatus)
    if (!m_initialized) {
        return;
    }

    setPassive(static_cast<int>(newStatus) < passiveStates().size() && passiveStates().at(static_cast<int>(newStatus)).isChecked());
    updateStatusIconAndTooltip();
}

void SyncthingApplet::handleDevicesChanged()
{
    m_statusInfo.updateConnectedDevices(m_connection);
    emit connectionStatusChanged();
}

void SyncthingApplet::handleInternalError(
    const QString &errorMsg, SyncthingErrorCategory category, int networkError, const QNetworkRequest &request, const QByteArray &response)
{
    if (!InternalError::isRelevant(m_connection, category, networkError)) {
        return;
    }
    auto error = InternalError(errorMsg, request.url(), response);
    if (Settings::values().notifyOn.internalErrors) {
        m_dbusNotifier.showInternalError(error);
    }
    InternalErrorsDialog::addError(std::move(error));
    if (!m_hasInternalErrors) {
        emit hasInternalErrorsChanged(m_hasInternalErrors = true);
    }
}

void SyncthingApplet::handleDirStatisticsChanged()
{
    m_overallStats = m_connection.computeOverallDirStatistics();
    emit statisticsChanged();
}

void SyncthingApplet::handleErrorsCleared()
{
    emit hasInternalErrorsChanged(m_hasInternalErrors = false);
}

void SyncthingApplet::handleAboutDialogDeleted()
{
    m_aboutDlg = nullptr;
}

#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
void SyncthingApplet::handleWebViewDeleted()
{
    m_webViewDlg = nullptr;
}
#endif

void SyncthingApplet::handleNewNotification(DateTime when, const QString &msg)
{
    m_notifications.emplace_back(QString::fromLocal8Bit(when.toString(DateTimeOutputFormat::DateAndTime, true).data()), msg);
    if (Settings::values().notifyOn.syncthingErrors) {
        m_dbusNotifier.showSyncthingNotification(when, msg);
    }
    if (m_notifications.size() == 1) {
        emit notificationsAvailableChanged(true);
        // update status as well because having or not having notifications is relevant for status text/icon
        updateStatusIconAndTooltip();
    }
}

void SyncthingApplet::handleSystemdServiceError(const QString &context, const QString &name, const QString &message)
{
    handleInternalError(tr("D-Bus error - unable to ") % context % QChar('\n') % name % QChar(':') % message, SyncthingErrorCategory::SpecificRequest,
        QNetworkReply::NoError, QNetworkRequest(), QByteArray());
}

void Plasmoid::SyncthingApplet::handleImageProviderDestroyed()
{
    m_imageProvider = nullptr;
}

void SyncthingApplet::handleThemeChanged()
{
    // unset the fa-URL to provoke Qt Quick to reload the images
    emit faUrlChanged(m_faUrl = QString());

    // return to the event loop before setting the new theme color; otherwise Qt Quick does not update the images
    QTimer::singleShot(0, this, [this] {
        const auto palette = paletteFromTheme(m_theme);
        IconManager::instance().setPalette(palette);
        setBrightColors(isPaletteDark(palette));
        if (m_imageProvider) {
            m_imageProvider->setDefaultColor(m_theme.color(Plasma::Theme::TextColor, Plasma::Theme::NormalColorGroup));
        }
        emit faUrlChanged(m_faUrl = QStringLiteral("image://fa/"));
    });
}

void SyncthingApplet::setBrightColors(bool brightColors)
{
    m_dirModel.setBrightColors(brightColors);
    m_devModel.setBrightColors(brightColors);
    m_downloadModel.setBrightColors(brightColors);
    m_recentChangesModel.setBrightColors(brightColors);
}

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
void SyncthingApplet::handleSystemdStatusChanged()
{
    Settings::values().systemd.apply(m_connection, currentConnectionConfig());
}
#endif

} // namespace Plasmoid

K_EXPORT_PLASMA_APPLET_WITH_JSON(syncthing, Plasmoid::SyncthingApplet, "metadata.json")

#include "syncthingapplet.moc"
