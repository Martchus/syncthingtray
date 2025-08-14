#include "./settingsdialog.h"
#include "./settings.h"
#include "./wizard.h"

#include "../misc/syncthinglauncher.h"

#include <qtutilities/misc/compat.h>

#ifdef SYNCTHINGWIDGETS_SETUP_TOOLS_ENABLED
#include <qtutilities/setup/updater.h>
#endif

#include <syncthingconnector/syncthingconfig.h>
#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingprocess.h>
#include <syncthingconnector/utils.h>
#include <syncthingmodel/syncthingstatuscomputionmodel.h>
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include <syncthingconnector/syncthingservice.h>
#include <syncthingmodel/colors.h>
#include <syncthingmodel/syncthingicons.h>
#endif

#include "ui_appearanceoptionpage.h"
#include "ui_autostartoptionpage.h"
#include "ui_connectionoptionpage.h"
#include "ui_iconsoptionpage.h"
#include "ui_launcheroptionpage.h"
#include "ui_notificationsoptionpage.h"
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include "ui_systemdoptionpage.h"
#endif
#include "ui_builtinwebviewoptionpage.h"
#include "ui_generalwebviewoptionpage.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/misc/compat.h>
#include <qtutilities/misc/desktoputils.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/paletteeditor/colorbutton.h>
#include <qtutilities/settingsdialog/optioncategory.h>
#include <qtutilities/settingsdialog/optioncategorymodel.h>
#include <qtutilities/settingsdialog/qtsettings.h>
#include <qtutilities/widgets/iconbutton.h>
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
#include <qtutilities/misc/dbusnotification.h>
#endif
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include <c++utilities/chrono/datetime.h>
#include <qtutilities/misc/dialogutils.h>
#endif

#include <QDesktopServices>
#include <QFileDialog>
#include <QHostAddress>
#include <QInputDialog>
#include <QMessageBox>
#if defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStandardPaths>
#elif defined(PLATFORM_WINDOWS)
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSettings>
#elif defined(PLATFORM_MAC)
#include <QFileInfo>
#endif
#include <QApplication>
#include <QFontDatabase>
#include <QMenu>
#include <QStringBuilder>
#include <QStyle>
#include <QTextBlock>
#include <QTextCursor>

#include <functional>
#include <initializer_list>

using namespace std;
using namespace std::placeholders;
using namespace Data;
using namespace CppUtilities;
using namespace QtUtilities;

namespace QtGui {

/// \brief Returns the tooltip text for the specified \a isMetered value.
static QString meteredToolTip(std::optional<bool> isMetered)
{
    return isMetered.has_value()
        ? (isMetered.value() ? QCoreApplication::translate("QtGui", "The network connection is currently considered metered.")
                             : QCoreApplication::translate("QtGui", "The network connection is currently not considered metered."))
        : QCoreApplication::translate("QtGui", "Unable to determine whether the network connection is metered; assuming an unmetered connection.");
}

/// \brief Configures the specified \a checkBox for the specified \a isMetered value.
static void configureMeteredCheckbox(QCheckBox *checkBox, std::optional<bool> isMetered)
{
    checkBox->setEnabled(isMetered.has_value());
    checkBox->setToolTip(meteredToolTip(isMetered));
}

// ConnectionOptionPage
ConnectionOptionPage::ConnectionOptionPage(Data::SyncthingConnection *connection, QWidget *parentWidget)
    : ConnectionOptionPageBase(parentWidget)
    , m_connection(connection)
    , m_currentIndex(0)
{
}

ConnectionOptionPage::~ConnectionOptionPage()
{
}

void ConnectionOptionPage::hideConnectionStatus()
{
    m_connection = nullptr;
    if (ui()) {
        ui()->statusTextLabel->setHidden(true);
        ui()->statusLabel->setHidden(true);
        ui()->connectPushButton->setHidden(true);
    }
}

QWidget *ConnectionOptionPage::setupWidget()
{
    auto *const widget = ConnectionOptionPageBase::setupWidget();
    m_statusComputionModel = new SyncthingStatusComputionModel(widget);
    ui()->certPathSelection->provideCustomFileMode(QFileDialog::ExistingFile);
    ui()->certPathSelection->lineEdit()->setPlaceholderText(
        QCoreApplication::translate("QtGui::ConnectionOptionPage", "Auto-detected for local instance"));
    ui()->instanceNoteIcon->setPixmap(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(32, 32));
    ui()->pollTrafficLabel->setToolTip(ui()->pollTrafficSpinBox->toolTip());
    ui()->pollDevStatsLabel->setToolTip(ui()->pollDevStatsSpinBox->toolTip());
    ui()->pollErrorsLabel->setToolTip(ui()->pollErrorsSpinBox->toolTip());
    ui()->reconnectLabel->setToolTip(ui()->reconnectSpinBox->toolTip());
    if (m_connection) {
        QObject::connect(m_connection, &SyncthingConnection::statusChanged, widget, bind(&ConnectionOptionPage::updateConnectionStatus, this));
    } else {
        hideConnectionStatus();
    }
    ui()->statusComputionFlagsListView->setModel(m_statusComputionModel);
    QObject::connect(ui()->connectPushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::applyAndReconnect, this));
    QObject::connect(ui()->insertFromConfigFilePushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::insertFromConfigFile, this, false));
    QObject::connect(
        ui()->insertFromCustomConfigFilePushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::insertFromConfigFile, this, true));
    QObject::connect(ui()->selectionComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        bind(&ConnectionOptionPage::showConnectionSettings, this, _1));
    QObject::connect(ui()->selectionComboBox, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::editTextChanged),
        bind(&ConnectionOptionPage::saveCurrentConfigName, this, _1));
    QObject::connect(ui()->downPushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::moveSelectedConfigDown, this));
    QObject::connect(ui()->upPushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::moveSelectedConfigUp, this));
    QObject::connect(ui()->addPushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::addNewConfig, this));
    QObject::connect(ui()->removePushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::removeSelectedConfig, this));
    QObject::connect(ui()->advancedCheckBox, &QCheckBox::toggled, bind(&ConnectionOptionPage::toggleAdvancedSettings, this, std::placeholders::_1));
    const auto *const launcher = SyncthingLauncher::mainInstance();
    configureMeteredCheckbox(ui()->pauseOnMeteredConnectionCheckBox, launcher ? launcher->isNetworkConnectionMetered() : std::nullopt);
    if (launcher) {
        QObject::connect(launcher, &SyncthingLauncher::networkConnectionMeteredChanged,
            bind(&configureMeteredCheckbox, ui()->pauseOnMeteredConnectionCheckBox, std::placeholders::_1));
    }
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    ui()->timeoutSpinBox->setEnabled(false);
#endif
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
    ui()->localPathLineEdit->setEnabled(false);
#endif
    toggleAdvancedSettings(false);
    return widget;
}

void ConnectionOptionPage::insertFromConfigFile(bool forceFileSelection)
{
    auto configFile(forceFileSelection ? QString() : SyncthingConfig::locateConfigFile());
    if (configFile.isEmpty()) {
        // allow user to select config file manually if it could not be located
        configFile = QFileDialog::getOpenFileName(
            widget(), QCoreApplication::translate("QtGui::ConnectionOptionPage", "Select Syncthing config file") + QStringLiteral(" - " APP_NAME));
    }
    if (configFile.isEmpty()) {
        return;
    }
    SyncthingConfig config;
    if (!config.restore(configFile)) {
        QMessageBox::critical(widget(), widget()->windowTitle() + QStringLiteral(" - " APP_NAME),
            QCoreApplication::translate("QtGui::ConnectionOptionPage", "Unable to parse the Syncthing config file."));
        return;
    }

    if (!config.guiAddress.isEmpty()) {
        const auto portStart(config.guiAddress.indexOf(QChar(':')));
        auto guiHost(config.guiAddress.mid(0, portStart));
        const auto guiPort = portStart > 0 ? QtUtilities::midRef(config.guiAddress, portStart) : QtUtilities::StringView();
        const QHostAddress guiAddress(guiHost);
        // assume local connection if address is eg. 0.0.0.0
        auto localConnection = true;
        if (guiAddress == QHostAddress::AnyIPv4) {
            guiHost = QStringLiteral("127.0.0.1");
        } else if (guiAddress == QHostAddress::AnyIPv6) {
            guiHost = QStringLiteral("[::1]");
        } else if (!isLocal(guiHost, guiAddress)) {
            localConnection = false;
        }
        const QString guiProtocol((config.guiEnforcesSecureConnection || !localConnection) ? QStringLiteral("https://") : QStringLiteral("http://"));

        ui()->urlLineEdit->selectAll();
        ui()->urlLineEdit->insert(guiProtocol % guiHost % guiPort);
    }
    if (!config.guiUser.isEmpty() || !config.guiPasswordHash.isEmpty()) {
        ui()->authCheckBox->setChecked(true);
        ui()->userNameLineEdit->selectAll();
        ui()->userNameLineEdit->insert(config.guiUser);
    } else {
        ui()->authCheckBox->setChecked(false);
    }
    if (!config.guiApiKey.isEmpty()) {
        ui()->apiKeyLineEdit->selectAll();
        ui()->apiKeyLineEdit->insert(config.guiApiKey);
    }
}

void ConnectionOptionPage::updateConnectionStatus()
{
    if (m_connection) {
        ui()->statusLabel->setText(m_connection->statusText());
    }
}

bool ConnectionOptionPage::showConnectionSettings(int index)
{
    if (index == m_currentIndex) {
        return true;
    }
    if (!cacheCurrentSettings(false)) {
        ui()->selectionComboBox->setCurrentIndex(m_currentIndex);
        return false;
    }
    const SyncthingConnectionSettings &connectionSettings = (index == 0 ? m_primarySettings : m_secondarySettings[static_cast<size_t>(index - 1)]);
    ui()->urlLineEdit->setText(connectionSettings.syncthingUrl);
    ui()->localPathLineEdit->setText(connectionSettings.localPath);
    ui()->authCheckBox->setChecked(connectionSettings.authEnabled);
    ui()->userNameLineEdit->setText(connectionSettings.userName);
    ui()->passwordLineEdit->setText(connectionSettings.password);
    ui()->apiKeyLineEdit->setText(connectionSettings.apiKey);
#ifndef QT_NO_SSL
    ui()->certPathSelection->lineEdit()->setText(connectionSettings.httpsCertPath);
#endif
    ui()->timeoutSpinBox->setValue(connectionSettings.requestTimeout);
    ui()->longPollingSpinBox->setValue(connectionSettings.longPollingTimeout);
    ui()->diskEventLimitSpinBox->setValue(connectionSettings.diskEventLimit);
    ui()->pollTrafficSpinBox->setValue(connectionSettings.trafficPollInterval);
    ui()->pollDevStatsSpinBox->setValue(connectionSettings.devStatsPollInterval);
    ui()->pollErrorsSpinBox->setValue(connectionSettings.errorsPollInterval);
    ui()->reconnectSpinBox->setValue(connectionSettings.reconnectInterval);
    ui()->autoConnectCheckBox->setChecked(connectionSettings.autoConnect);
    ui()->pauseOnMeteredConnectionCheckBox->setChecked(connectionSettings.pauseOnMeteredConnection);
    m_statusComputionModel->setStatusComputionFlags(connectionSettings.statusComputionFlags);
    setCurrentIndex(index);
    return true;
}

bool ConnectionOptionPage::cacheCurrentSettings(bool applying)
{
    if (m_currentIndex < 0) {
        return true;
    }

    SyncthingConnectionSettings &connectionSettings
        = (m_currentIndex == 0 ? m_primarySettings : m_secondarySettings[static_cast<size_t>(m_currentIndex - 1)]);
    connectionSettings.syncthingUrl = ui()->urlLineEdit->text();
    connectionSettings.localPath = ui()->localPathLineEdit->text();
    connectionSettings.authEnabled = ui()->authCheckBox->isChecked();
    connectionSettings.userName = ui()->userNameLineEdit->text();
    connectionSettings.password = ui()->passwordLineEdit->text();
    connectionSettings.apiKey = ui()->apiKeyLineEdit->text().toUtf8();
#ifndef QT_NO_SSL
    connectionSettings.expectedSslErrors.clear();
    connectionSettings.httpsCertPath = ui()->certPathSelection->lineEdit()->text();
#endif
    connectionSettings.requestTimeout = ui()->timeoutSpinBox->value();
    connectionSettings.longPollingTimeout = ui()->longPollingSpinBox->value();
    connectionSettings.diskEventLimit = ui()->diskEventLimitSpinBox->value();
    connectionSettings.trafficPollInterval = ui()->pollTrafficSpinBox->value();
    connectionSettings.devStatsPollInterval = ui()->pollDevStatsSpinBox->value();
    connectionSettings.errorsPollInterval = ui()->pollErrorsSpinBox->value();
    connectionSettings.reconnectInterval = ui()->reconnectSpinBox->value();
    connectionSettings.autoConnect = ui()->autoConnectCheckBox->isChecked();
    connectionSettings.pauseOnMeteredConnection = ui()->pauseOnMeteredConnectionCheckBox->isChecked();
    connectionSettings.statusComputionFlags = m_statusComputionModel->statusComputionFlags();
#ifndef QT_NO_SSL
    if (!connectionSettings.loadHttpsCert()) {
        const QString errorMessage = QCoreApplication::translate("QtGui::ConnectionOptionPage", "Unable to load specified certificate \"%1\".")
                                         .arg(connectionSettings.httpsCertPath);
        if (!applying) {
            QMessageBox::critical(widget(), QCoreApplication::applicationName(), errorMessage);
        } else {
            errors() << errorMessage;
        }
        return false;
    }
#endif
    return true;
}

void ConnectionOptionPage::saveCurrentConfigName(const QString &name)
{
    const int index = ui()->selectionComboBox->currentIndex();
    if (index == m_currentIndex && index >= 0) {
        (index == 0 ? m_primarySettings : m_secondarySettings[static_cast<size_t>(index - 1)]).label = name;
        ui()->selectionComboBox->setItemText(index, name);
    }
}

void ConnectionOptionPage::addNewConfig()
{
    m_secondarySettings.emplace_back();
    m_secondarySettings.back().label
        = QCoreApplication::translate("QtGui::ConnectionOptionPage", "Instance %1").arg(ui()->selectionComboBox->count() + 1);
    ui()->selectionComboBox->addItem(m_secondarySettings.back().label);
    ui()->selectionComboBox->setCurrentIndex(ui()->selectionComboBox->count() - 1);
    ui()->removePushButton->setEnabled(true);
}

void ConnectionOptionPage::removeSelectedConfig()
{
    if (m_secondarySettings.empty()) {
        return;
    }
    const int index = ui()->selectionComboBox->currentIndex();
    if (index < 0 || static_cast<unsigned>(index) > m_secondarySettings.size()) {
        return;
    }

    if (index == 0) {
        m_primarySettings = std::move(m_secondarySettings.front());
        m_secondarySettings.erase(m_secondarySettings.begin());
    } else {
        m_secondarySettings.erase(m_secondarySettings.begin() + (index - 1));
    }
    m_currentIndex = -1;
    ui()->selectionComboBox->removeItem(index);
    ui()->removePushButton->setEnabled(!m_secondarySettings.empty());
}

void ConnectionOptionPage::moveSelectedConfigDown()
{
    if (m_secondarySettings.empty()) {
        return;
    }
    const int index = ui()->selectionComboBox->currentIndex();
    if (index < 0) {
        return;
    }

    if (index == 0) {
        swap(m_primarySettings, m_secondarySettings.front());
        ui()->selectionComboBox->setItemText(0, m_primarySettings.label);
        ui()->selectionComboBox->setItemText(1, m_secondarySettings.front().label);
        setCurrentIndex(1);
    } else if (static_cast<unsigned>(index) < m_secondarySettings.size()) {
        SyncthingConnectionSettings &current = m_secondarySettings[static_cast<unsigned>(index) - 1];
        SyncthingConnectionSettings &exchange = m_secondarySettings[static_cast<unsigned>(index)];
        swap(current, exchange);
        ui()->selectionComboBox->setItemText(index, current.label);
        ui()->selectionComboBox->setItemText(index + 1, exchange.label);
        setCurrentIndex(index + 1);
    }
    ui()->selectionComboBox->setCurrentIndex(m_currentIndex);
}

void ConnectionOptionPage::moveSelectedConfigUp()
{
    if (m_secondarySettings.empty()) {
        return;
    }
    const int index = ui()->selectionComboBox->currentIndex();
    if (index <= 0) {
        return;
    }

    if (index == 1) {
        swap(m_primarySettings, m_secondarySettings.front());
        ui()->selectionComboBox->setItemText(0, m_primarySettings.label);
        ui()->selectionComboBox->setItemText(1, m_secondarySettings.front().label);
        setCurrentIndex(0);
    } else if (static_cast<unsigned>(index) - 1 < m_secondarySettings.size()) {
        SyncthingConnectionSettings &current = m_secondarySettings[static_cast<unsigned>(index) - 1];
        SyncthingConnectionSettings &exchange = m_secondarySettings[static_cast<unsigned>(index) - 2];
        swap(current, exchange);
        ui()->selectionComboBox->setItemText(index, current.label);
        ui()->selectionComboBox->setItemText(index - 1, exchange.label);
        setCurrentIndex(index - 1);
    }
    ui()->selectionComboBox->setCurrentIndex(m_currentIndex);
}

void ConnectionOptionPage::setCurrentIndex(int currentIndex)
{
    m_currentIndex = currentIndex;
    ui()->downPushButton->setEnabled(currentIndex >= 0 && static_cast<unsigned>(currentIndex) < m_secondarySettings.size());
    ui()->upPushButton->setEnabled(currentIndex > 0 && static_cast<unsigned>(currentIndex) - 1 < m_secondarySettings.size());
}

void ConnectionOptionPage::toggleAdvancedSettings(bool show)
{
    if (!ui()) {
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    for (auto *const widget : std::initializer_list<QWidget *>{ ui()->localPathLabel, ui()->authLabel, ui()->userNameLabel, ui()->passwordLabel,
             ui()->timeoutLabel, ui()->longPollingLabel, ui()->diskEventLimitLabel, ui()->pollLabel, ui()->pauseOnMeteredConnectionCheckBox }) {
        ui()->formLayout->setRowVisible(widget, show);
    }
#else
    for (auto *const widget : std::initializer_list<QWidget *>{ ui()->localPathLabel, ui()->localPathLineEdit, ui()->authLabel, ui()->authCheckBox,
             ui()->userNameLabel, ui()->userNameLineEdit, ui()->passwordLabel, ui()->passwordLineEdit, ui()->timeoutLabel, ui()->timeoutSpinBox,
             ui()->longPollingLabel, ui()->longPollingSpinBox, ui()->diskEventLimitLabel, ui()->diskEventLimitSpinBox, ui()->pollLabel,
             ui()->pollDevStatsLabel, ui()->pollDevStatsSpinBox, ui()->pollErrorsLabel, ui()->pollErrorsSpinBox, ui()->pollTrafficLabel,
             ui()->pollTrafficSpinBox, ui()->reconnectLabel, ui()->reconnectSpinBox, ui()->pauseOnMeteredConnectionCheckBox }) {
        widget->setVisible(show);
    }
#endif
}

bool ConnectionOptionPage::apply()
{
    if (!cacheCurrentSettings(true)) {
        return false;
    }
    auto &values = Settings::values();
    values.connection.primary = m_primarySettings;
    values.connection.secondary = m_secondarySettings;
    return true;
}

void ConnectionOptionPage::reset()
{
    const auto &values = Settings::values();
    m_primarySettings = values.connection.primary;
    m_secondarySettings = values.connection.secondary;
    m_currentIndex = -1;

    QStringList itemTexts;
    itemTexts.reserve(1 + static_cast<int>(m_secondarySettings.size()));
    itemTexts << m_primarySettings.label;
    for (const SyncthingConnectionSettings &settings : m_secondarySettings) {
        itemTexts << settings.label;
    }
    ui()->selectionComboBox->clear();
    ui()->selectionComboBox->addItems(itemTexts);
    ui()->selectionComboBox->setCurrentIndex(0);

    updateConnectionStatus();
}

void ConnectionOptionPage::applyAndReconnect()
{
    apply();
    if (m_connection) {
        m_connection->reconnect((m_currentIndex == 0 ? m_primarySettings : m_secondarySettings[static_cast<size_t>(m_currentIndex - 1)]));
    }
}

// NotificationsOptionPage
NotificationsOptionPage::NotificationsOptionPage(GuiType guiType, QWidget *parentWidget)
    : NotificationsOptionPageBase(parentWidget)
    , m_guiType(guiType)
{
}

NotificationsOptionPage::~NotificationsOptionPage()
{
}

QWidget *NotificationsOptionPage::setupWidget()
{
    auto *const widget = NotificationsOptionPageBase::setupWidget();
    switch (m_guiType) {
    case GuiType::TrayWidget:
        break;
    case GuiType::Plasmoid:
        ui()->apiGroupBox->setHidden(true);
        break;
    }
    return widget;
}

bool NotificationsOptionPage::apply()
{
    bool ok = true;
    auto &settings(Settings::values());
    auto &notifyOn(settings.notifyOn);
    notifyOn.disconnect = ui()->notifyOnDisconnectCheckBox->isChecked();
    notifyOn.internalErrors = ui()->notifyOnErrorsCheckBox->isChecked();
    notifyOn.launcherErrors = ui()->notifyOnLauncherErrorsCheckBox->isChecked();
    notifyOn.localSyncComplete = ui()->notifyOnLocalSyncCompleteCheckBox->isChecked();
    notifyOn.remoteSyncComplete = ui()->notifyOnRemoteSyncCompleteCheckBox->isChecked();
    notifyOn.syncthingErrors = ui()->showSyncthingNotificationsCheckBox->isChecked();
    notifyOn.newDeviceConnects = ui()->notifyOnNewDevConnectsCheckBox->isChecked();
    notifyOn.newDirectoryShared = ui()->notifyOnNewDirSharedCheckBox->isChecked();
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    if ((settings.dbusNotifications = ui()->dbusRadioButton->isChecked()) && !DBusNotification::isAvailable()) {
        errors() << QCoreApplication::translate(
            "QtGui::NotificationsOptionPage", "Configured to use D-Bus notifications but D-Bus notification daemon seems unavailabe.");
        ok = false;
    }
#endif
    settings.ignoreInavailabilityAfterStart = static_cast<unsigned int>(ui()->ignoreInavailabilityAfterStartSpinBox->value());
    return ok;
}

void NotificationsOptionPage::reset()
{
    auto &settings(Settings::values());
    const auto &notifyOn = settings.notifyOn;
    ui()->notifyOnDisconnectCheckBox->setChecked(notifyOn.disconnect);
    ui()->notifyOnErrorsCheckBox->setChecked(notifyOn.internalErrors);
    ui()->notifyOnLauncherErrorsCheckBox->setChecked(notifyOn.launcherErrors);
    ui()->notifyOnLocalSyncCompleteCheckBox->setChecked(notifyOn.localSyncComplete);
    ui()->notifyOnRemoteSyncCompleteCheckBox->setChecked(notifyOn.remoteSyncComplete);
    ui()->showSyncthingNotificationsCheckBox->setChecked(notifyOn.syncthingErrors);
    ui()->notifyOnNewDevConnectsCheckBox->setChecked(notifyOn.newDeviceConnects);
    ui()->notifyOnNewDirSharedCheckBox->setChecked(notifyOn.newDirectoryShared);
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    (settings.dbusNotifications ? ui()->dbusRadioButton : ui()->qtRadioButton)->setChecked(true);
#else
    ui()->dbusRadioButton->setEnabled(false);
    ui()->qtRadioButton->setChecked(true);
#endif
    ui()->ignoreInavailabilityAfterStartSpinBox->setValue(static_cast<int>(settings.ignoreInavailabilityAfterStart));
}

// AppearanceOptionPage
AppearanceOptionPage::AppearanceOptionPage(QWidget *parentWidget)
    : AppearanceOptionPageBase(parentWidget)
{
}

AppearanceOptionPage::~AppearanceOptionPage()
{
}

bool AppearanceOptionPage::apply()
{
    auto &v = Settings::values();
    auto &settings = v.appearance;
    settings.windowType = ui()->windowTypeComboBox->currentIndex();
    settings.trayMenuSize.setWidth(ui()->widthSpinBox->value());
    settings.trayMenuSize.setHeight(ui()->heightSpinBox->value());
    settings.showTraffic = ui()->showTrafficCheckBox->isChecked();
    settings.showDownloads = ui()->showDownloadsCheckBox->isChecked();
    settings.showTabTexts = ui()->showTabTextsCheckBox->isChecked();
    v.icons.preferIconsFromTheme = ui()->preferIconsFromThemeCheckBox->isChecked();
    int style;
    switch (ui()->frameShapeComboBox->currentIndex()) {
    case 0:
        style = QFrame::NoFrame;
        break;
    case 1:
        style = QFrame::Box;
        break;
    case 2:
        style = QFrame::Panel;
        break;
    default:
        style = QFrame::StyledPanel;
    }
    switch (ui()->frameShadowComboBox->currentIndex()) {
    case 0:
        style |= QFrame::Plain;
        break;
    case 1:
        style |= QFrame::Raised;
        break;
    default:
        style |= QFrame::Sunken;
    }
    settings.frameStyle = style;
    settings.tabPosition = ui()->tabPosComboBox->currentIndex();

    settings.positioning.useCursorPosition = ui()->useCursorPosCheckBox->isChecked();
    settings.positioning.useAssumedIconPosition = ui()->assumeIconPosCheckBox->isChecked();
    settings.positioning.assumedIconPosition = QPoint(ui()->xPosSpinBox->value(), ui()->yPosSpinBox->value());
    return true;
}

void AppearanceOptionPage::resetPositioningSettings()
{
    const auto &v = Settings::values();
    const auto &settings = v.appearance;
    ui()->widthSpinBox->setValue(settings.trayMenuSize.width());
    ui()->heightSpinBox->setValue(settings.trayMenuSize.height());
    ui()->xPosSpinBox->setValue(settings.positioning.assumedIconPosition.x());
    ui()->yPosSpinBox->setValue(settings.positioning.assumedIconPosition.y());
}

void AppearanceOptionPage::reset()
{
    const auto &v = Settings::values();
    const auto &settings = v.appearance;
    resetPositioningSettings();
    ui()->windowTypeComboBox->setCurrentIndex(settings.windowType);
    ui()->showTrafficCheckBox->setChecked(settings.showTraffic);
    ui()->showDownloadsCheckBox->setChecked(settings.showDownloads);
    ui()->showTabTextsCheckBox->setChecked(settings.showTabTexts);
    ui()->preferIconsFromThemeCheckBox->setChecked(v.icons.preferIconsFromTheme);
    auto index = int();
    switch (settings.frameStyle & QFrame::Shape_Mask) {
    case QFrame::NoFrame:
        index = 0;
        break;
    case QFrame::Box:
        index = 1;
        break;
    case QFrame::Panel:
        index = 2;
        break;
    default:
        index = 3;
    }
    ui()->frameShapeComboBox->setCurrentIndex(index);
    switch (settings.frameStyle & QFrame::Shadow_Mask) {
    case QFrame::Plain:
        index = 0;
        break;
    case QFrame::Raised:
        index = 1;
        break;
    default:
        index = 2;
    }
    ui()->frameShadowComboBox->setCurrentIndex(index);
    ui()->tabPosComboBox->setCurrentIndex(settings.tabPosition);

    ui()->useCursorPosCheckBox->setChecked(settings.positioning.useCursorPosition);
    ui()->assumeIconPosCheckBox->setChecked(settings.positioning.useAssumedIconPosition);
}

// IconsOptionPage
IconsOptionPage::IconsOptionPage(Context context, QWidget *parentWidget)
    : IconsOptionPageBase(parentWidget)
    , m_context(context)
{
}

IconsOptionPage::~IconsOptionPage()
{
}

QWidget *IconsOptionPage::setupWidget()
{
    auto *const widget = IconsOptionPageBase::setupWidget();

    // set context-specific elements
    switch (m_context) {
    case Context::Combined:
        ui()->contextLabel->hide();
        ui()->contextCheckBox->hide();
        break;
    case Context::UI:
        widget->setWindowTitle(QCoreApplication::translate("QtGui::IconsOptionPageBase", "UI icons"));
        ui()->contextLabel->setText(
            QCoreApplication::translate("QtGui::IconsOptionPageBase", "These icon settings are used within Syncthing Tray's UI."));
        ui()->contextCheckBox->hide();
        break;
    case Context::System:
        widget->setWindowTitle(QCoreApplication::translate("QtGui::IconsOptionPageBase", "System icons"));
        ui()->contextLabel->setText(QCoreApplication::translate(
            "QtGui::IconsOptionPageBase", "These icon settings are used for the system tray icon and the notifications."));
        ui()->contextCheckBox->setText(QCoreApplication::translate("QtGui::IconsOptionPageBase", "Use same settings as for UI icons"));
        break;
    }

    // allow changing stroke thickness
    QObject::connect(ui()->thickStrokeWidthCheckBox, &QCheckBox::toggled, widget,
        [this](bool thick) { m_settings.strokeWidth = thick ? StatusIconStrokeWidth::Thick : StatusIconStrokeWidth::Normal; });

    // populate form for status icon colors
    auto *const gridLayout = ui()->gridLayout;
    auto *const statusIconsGroupBox = ui()->statusIconsGroupBox;
    auto index = int();
    for (auto &colorMapping : m_settings.colorMapping()) {
        // populate widgets array
        auto &widgetsForColor = m_widgets[index++] = {
            {
                new ColorButton(statusIconsGroupBox),
                new ColorButton(statusIconsGroupBox),
                new ColorButton(statusIconsGroupBox),
            },
            new QLabel(statusIconsGroupBox),
            &colorMapping.setting,
            colorMapping.defaultEmblem,
        };
        widgetsForColor.previewLabel->setMaximumSize(QSize(32, 32));

        // add label for color name
        gridLayout->addWidget(new QLabel(colorMapping.colorName, statusIconsGroupBox), index, 0, Qt::AlignRight | Qt::AlignVCenter);

        // setup preview
        gridLayout->addWidget(widgetsForColor.previewLabel, index, 4, Qt::AlignCenter);
        const auto updatePreview = [this, &widgetsForColor] {
            widgetsForColor.previewLabel->setPixmap(renderSvgImage(makeSyncthingIcon(
                                                                       StatusIconColorSet{
                                                                           widgetsForColor.colorButtons[0]->color(),
                                                                           widgetsForColor.colorButtons[1]->color(),
                                                                           widgetsForColor.colorButtons[2]->color(),
                                                                       },
                                                                       widgetsForColor.statusEmblem, m_settings.strokeWidth),
                widgetsForColor.previewLabel->maximumSize()));
        };
        for (const auto &colorButton : widgetsForColor.colorButtons) {
            QObject::connect(colorButton, &ColorButton::colorChanged, widget, updatePreview);
        }
        QObject::connect(ui()->thickStrokeWidthCheckBox, &QCheckBox::toggled, widget, updatePreview);

        // setup color buttons
        widgetsForColor.colorButtons[0]->setColor(colorMapping.setting.backgroundStart);
        widgetsForColor.colorButtons[1]->setColor(colorMapping.setting.backgroundEnd);
        widgetsForColor.colorButtons[2]->setColor(colorMapping.setting.foreground);
        gridLayout->addWidget(widgetsForColor.colorButtons[0], index, 1);
        gridLayout->addWidget(widgetsForColor.colorButtons[1], index, 2);
        gridLayout->addWidget(widgetsForColor.colorButtons[2], index, 3);

        if (index >= StatusIconSettings::distinguishableColorCount) {
            break;
        }
    }

    // setup presets menu
    auto *const presetsMenu = new QMenu(widget);
    presetsMenu->addAction(QCoreApplication::translate("QtGui::IconsOptionPageBase", "Colorful background with gradient (default)"), widget, [this] {
        m_settings = Data::StatusIconSettings();
        m_usePalette = false;
        update(true);
    });
    presetsMenu->addAction(
        QCoreApplication::translate("QtGui::IconsOptionPageBase", "Transparent background and dark foreground (for bright themes)"), widget, [this] {
            m_settings = Data::StatusIconSettings(Data::StatusIconSettings::BrightTheme{});
            m_usePalette = false;
            update(true);
        });
    presetsMenu->addAction(
        QCoreApplication::translate("QtGui::IconsOptionPageBase", "Transparent background and bright foreground (for dark themes)"), widget, [this] {
            m_settings = Data::StatusIconSettings(Data::StatusIconSettings::DarkTheme{});
            m_usePalette = false;
            update(true);
        });
    m_paletteAction = presetsMenu->addAction(QString(), widget, [this] {
        m_usePalette = !m_usePalette;
        update(true);
    });

    // setup additional buttons
    ui()->restoreDefaultsPushButton->setMenu(presetsMenu);
    QObject::connect(ui()->restorePreviousPushButton, &QPushButton::clicked, widget, [this] { reset(); });

    // setup slider
    QObject::connect(ui()->renderingSizeSlider, &QSlider::valueChanged, widget, [this](int value) {
        m_settings.renderSize = QSize(value, value);
        auto *const label = ui()->renderingSizeLabel;
        if (const auto scaleFactor = label->devicePixelRatioF(); scaleFactor == 1.0) {
            label->setText(QString::number(value) + QStringLiteral(" px"));
        } else {
            label->setText(QCoreApplication::translate("QtGui::IconsOptionPageBase", "%1 px (scaled to %2 px)")
                    .arg(QString::number(value), QString::number(static_cast<qreal>(value) * scaleFactor, 'f', 0)));
        }
    });

    return widget;
}

bool IconsOptionPage::apply()
{
    for (auto &widgetsForColor : m_widgets) {
        *widgetsForColor.setting = StatusIconColorSet{
            widgetsForColor.colorButtons[0]->color(),
            widgetsForColor.colorButtons[1]->color(),
            widgetsForColor.colorButtons[2]->color(),
        };
    }
    auto &iconSettings = Settings::values().icons;
    switch (m_context) {
    case Context::Combined:
    case Context::UI:
        iconSettings.status = m_settings;
        iconSettings.usePaletteForStatus = m_usePalette;
        break;
    case Context::System:
        iconSettings.tray = m_settings;
        iconSettings.usePaletteForTray = m_usePalette;
        iconSettings.distinguishTrayIcons = !ui()->contextCheckBox->isChecked();
        break;
    }
    return true;
}

void IconsOptionPage::update(bool preserveSize)
{
    if (preserveSize) {
        const auto size = ui()->renderingSizeSlider->value();
        m_settings.renderSize = QSize(size, size);
    } else {
        ui()->renderingSizeSlider->setValue(std::max(m_settings.renderSize.width(), m_settings.renderSize.height()));
    }
    m_paletteAction->setText(m_usePalette
            ? QCoreApplication::translate("QtGui::IconsOptionPageBase", "Select colors manually (no longer follow system palette)")
            : QCoreApplication::translate("QtGui::IconsOptionPageBase", "Transparent background and foreground depending on system palette"));
    ui()->gridWidget->setDisabled(m_usePalette);
    ui()->thickStrokeWidthCheckBox->setChecked(m_settings.strokeWidth == StatusIconStrokeWidth::Thick);
    for (auto &widgetsForColor : m_widgets) {
        widgetsForColor.colorButtons[0]->setColor(widgetsForColor.setting->backgroundStart);
        widgetsForColor.colorButtons[1]->setColor(widgetsForColor.setting->backgroundEnd);
        widgetsForColor.colorButtons[2]->setColor(widgetsForColor.setting->foreground);
    }
}

void IconsOptionPage::reset()
{
    const auto &iconSettings = Settings::values().icons;
    switch (m_context) {
    case Context::Combined:
    case Context::UI:
        m_settings = iconSettings.status;
        m_usePalette = iconSettings.usePaletteForStatus;
        break;
    case Context::System:
        m_settings = iconSettings.tray;
        m_usePalette = iconSettings.usePaletteForTray;
        ui()->contextCheckBox->setChecked(!iconSettings.distinguishTrayIcons);
        break;
    }
    update();
}

// AutostartOptionPage
AutostartOptionPage::AutostartOptionPage(QWidget *parentWidget)
    : AutostartOptionPageBase(parentWidget)
{
}

AutostartOptionPage::~AutostartOptionPage()
{
}

QWidget *AutostartOptionPage::setupWidget()
{
    auto *widget = AutostartOptionPageBase::setupWidget();
    auto *style = QApplication::style();

    ui()->infoIconLabel->setPixmap(
        style->standardIcon(QStyle::SP_MessageBoxInformation, nullptr, ui()->infoIconLabel).pixmap(ui()->infoIconLabel->size()));
    ui()->pathWarningIconLabel->setPixmap(
        style->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, ui()->pathWarningIconLabel).pixmap(ui()->pathWarningIconLabel->size()));
    QObject::connect(ui()->deleteExistingEntryPushButton, &QPushButton::clicked, widget, [this] {
        setAutostartPath(QString());
        reset();
    });
#if !defined(SYNCTHINGWIDGETS_AUTOSTART_DISABLED) && defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)
    ui()->platformNoteLabel->setText(QCoreApplication::translate("QtGui::AutostartOptionPage",
        "This is achieved by adding a *.desktop file under <i>~/.config/autostart</i> so the setting only affects the current user."));
#elif !defined(SYNCTHINGWIDGETS_AUTOSTART_DISABLED) && defined(PLATFORM_WINDOWS)
    ui()->platformNoteLabel->setText(QCoreApplication::translate("QtGui::AutostartOptionPage",
        "This is achieved by adding a registry key under <i>HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run</i> so the setting "
        "only affects the current user. Note that the startup entry is invalidated when moving <i>syncthingtray.exe</i>."));
#elif !defined(SYNCTHINGWIDGETS_AUTOSTART_DISABLED) && defined(PLATFORM_MAC)
    ui()->platformNoteLabel->setText(QCoreApplication::translate("QtGui::AutostartOptionPage",
        "This is achieved by adding a *.plist file under <i>~/Library/LaunchAgents</i> so the setting only affects the current user."));
#else
    ui()->platformNoteLabel->setText(
        QCoreApplication::translate("QtGui::AutostartOptionPage", "This feature has not been implemented for your platform (yet)."));
    m_unsupported = true;
    ui()->pathWidget->setVisible(false);
    ui()->autostartCheckBox->setVisible(false);
#endif
    return widget;
}

#if (defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)) || (defined(PLATFORM_WINDOWS))
/// \cond
static std::optional<QString> readQuotedPath(const QRegularExpression &regex, const QString &data)
{
    auto match = regex.match(data);
    auto captured = match.captured(2);
    if (captured.isNull()) {
        captured = match.captured(3);
    }
    return captured.isNull() ? std::nullopt : std::make_optional(captured);
}
/// \endcond
#endif

/*!
 * \brief Returns the currently configured autostart path or an empty string if autostart is disabled; returns std::nullopt when the
 *        path cannot be determined.
 * \remarks
 * - At this point the path cannot be determined on MacOS.
 * - Use isAutostartEnabled() if you just need to know whether autostart is generally enabled as this is a simpler check. This
 *   function is also generally useful as a fallback.
 */
std::optional<QString> configuredAutostartPath()
{
    if (qEnvironmentVariableIsSet(PROJECT_VARNAME_UPPER "_AUTOSTART_PATH_MOCK")) {
        auto mockedPath = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_AUTOSTART_PATH_MOCK");
        return mockedPath.isEmpty() ? std::nullopt : std::make_optional(mockedPath);
    }
#if defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)
    auto desktopFile = QFile(QStandardPaths::locate(QStandardPaths::ConfigLocation, QStringLiteral("autostart/" PROJECT_NAME ".desktop")));
    // check whether the file can be opened and whether it is enabled but prevent reading large files
    if (!desktopFile.open(QFile::ReadOnly)) {
        return QString();
    }
    if (desktopFile.size() > (5 * 1024)) {
        return std::nullopt;
    }
    const auto data = QString::fromUtf8(desktopFile.readAll());
    if (data.contains(QLatin1String("Hidden=true"))) {
        return QString();
    }
    static const auto regex = QRegularExpression(QStringLiteral("Exec=(\"([^\"\\n]*)\"|([^\\s\\n]*))"));
    return readQuotedPath(regex, data);
#elif defined(PLATFORM_WINDOWS)
    static const auto regex = QRegularExpression(QStringLiteral("(\"([^\"\\n]*)\"|([^\\s\\n]*))"));
    return readQuotedPath(regex,
        QSettings(QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat)
            .value(QStringLiteral(PROJECT_NAME))
            .toString());
#else
    return std::nullopt;
#endif
}

/*!
 * \brief Returns the autostart path that will be configured by invoking setAutostartEnabled(true).
 */
QString supposedAutostartPath()
{
#if defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)
#ifndef SYNCTHINGWIDGETS_AUTOSTART_EXEC_PATH
#define SYNCTHINGWIDGETS_AUTOSTART_EXEC_PATH QCoreApplication::applicationFilePath()
#endif
    return qEnvironmentVariable("APPIMAGE", SYNCTHINGWIDGETS_AUTOSTART_EXEC_PATH);
#elif defined(PLATFORM_WINDOWS)
    return QCoreApplication::applicationFilePath().replace(QChar('/'), QChar('\\'));
#else
    return QCoreApplication::applicationFilePath();
#endif
}

/*!
 * \brief Sets the \a path of the application's autostart entry or removes the entry if \a path is empty.
 * \sa See https://learn.microsoft.com/en-us/windows/win32/setupapi/run-and-runonce-registry-keys for Windows implementation.
 */
bool setAutostartPath(const QString &path)
{
    if (qEnvironmentVariableIsSet(PROJECT_VARNAME_UPPER "_AUTOSTART_PATH_MOCK")) {
        return qputenv(PROJECT_VARNAME_UPPER "_AUTOSTART_PATH_MOCK", path.toLocal8Bit());
    }
#if defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)
    const auto configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (configPath.isEmpty()) {
        return false;
    }
    if (!path.isEmpty() && !QDir().mkpath(configPath + QStringLiteral("/autostart"))) {
        return false;
    }
    auto desktopFile = QFile(configPath + QStringLiteral("/autostart/" PROJECT_NAME ".desktop"));
    if (!path.isEmpty()) {
        if (!desktopFile.open(QFile::WriteOnly | QFile::Truncate)) {
            return false;
        }
        desktopFile.write("[Desktop Entry]\n"
                          "Name=" APP_NAME "\n"
                          "Exec=\"");
        desktopFile.write(path.toUtf8());
        desktopFile.write("\" qt-widgets-gui --single-instance --wait\nComment=" APP_DESCRIPTION "\n"
                          "Icon=" PROJECT_NAME "\n"
                          "Type=Application\n"
                          "Terminal=false\n"
                          "X-GNOME-Autostart-Delay=0\n"
                          "X-GNOME-Autostart-enabled=true\n"
                          "X-LXQt-Need-Tray=true\n");
        return desktopFile.error() == QFile::NoError && desktopFile.flush();

    } else {
        return !desktopFile.exists() || desktopFile.remove();
    }

#elif defined(PLATFORM_WINDOWS)
    auto settings = QSettings(QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
    if (!path.isEmpty()) {
        settings.setValue(QStringLiteral(PROJECT_NAME), QStringLiteral("\"%1\"").arg(path));
    } else {
        settings.remove(QStringLiteral(PROJECT_NAME));
    }
    settings.sync();
    return true;

#elif defined(PLATFORM_MAC)
    const auto libraryPath = QDir::home().filePath(QStringLiteral("Library"));
    if (!path.isEmpty() && !QDir().mkpath(libraryPath + QStringLiteral("/LaunchAgents"))) {
        return false;
    }
    auto launchdPlistFile = QFile(libraryPath + QStringLiteral("/LaunchAgents/" PROJECT_NAME ".plist"));
    if (!path.isEmpty()) {
        if (!launchdPlistFile.open(QFile::WriteOnly | QFile::Truncate)) {
            return false;
        }
        launchdPlistFile.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                               "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                               "<plist version=\"1.0\">\n"
                               "    <dict>\n"
                               "        <key>Label</key>\n"
                               "        <string>" PROJECT_NAME "</string>\n"
                               "        <key>ProgramArguments</key>\n"
                               "        <array>\n"
                               "            <string>");
        launchdPlistFile.write(path.toUtf8());
        launchdPlistFile.write("</string>\n"
                               "        </array>\n"
                               "        <key>KeepAlive</key>\n"
                               "        <true/>\n"
                               "    </dict>\n"
                               "</plist>\n");
        return launchdPlistFile.error() == QFile::NoError && launchdPlistFile.flush();
    } else {
        return !launchdPlistFile.exists() || launchdPlistFile.remove();
    }
#else
    return false;
#endif
}

/*!
 * \brief Returns whether the application is launched on startup.
 * \remarks
 * - Only implemented under Linux/Windows/Mac. Always returns false on other platforms.
 * - Does not check whether the startup entry is functional (e.g. whether the specified path is still valid and points to the
 *   currently running instance of the application).
 */
bool isAutostartEnabled()
{
#if defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)
    auto desktopFile = QFile(QStandardPaths::locate(QStandardPaths::ConfigLocation, QStringLiteral("autostart/" PROJECT_NAME ".desktop")));
    // check whether the file can be opened and whether it is enabled but prevent reading large files
    if (desktopFile.open(QFile::ReadOnly) && (desktopFile.size() > (5 * 1024) || !desktopFile.readAll().contains("Hidden=true"))) {
        return true;
    }
    return false;
#elif defined(PLATFORM_WINDOWS)
    return QSettings(QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat)
        .contains(QStringLiteral(PROJECT_NAME));
#elif defined(PLATFORM_MAC)
    return QFileInfo(QDir::home(), QStringLiteral("Library/LaunchAgents/" PROJECT_NAME ".plist")).isReadable();
#else
    return false;
#endif
}

/*!
 * \brief Sets whether the application is launched on startup.
 * \remarks
 * - Only implemented under Linux/Windows/Mac. Does nothing on other platforms.
 * - If a startup entry already exists and \a enabled is true, this function will not touch the existing entry - even if it points
 *   to another application. Delete the existing entry first if it is no longer wanted or set \a force to true.
 *     - Note that is might not be possible to determine the currently configured path. If the path cannot be determined an
 *       existing autostart entry will always be overridden (despite \a force being false).
 * - If no startup entry could be detected via isAutostartEnabled() and \a enabled is false this function doesn't touch anything.
 */
bool setAutostartEnabled(bool enabled, bool force)
{
    const auto configuredPath = configuredAutostartPath();
    if (!(configuredPath.has_value() ? !configuredPath.value().isEmpty() : isAutostartEnabled()) && !enabled) {
        return true;
    }
    const auto supposedPath = supposedAutostartPath();
    if (!force && enabled && configuredPath.has_value() && !configuredPath.value().isEmpty() && configuredPath.value() != supposedPath) {
        return true; // don't touch existing entry
    }
    return setAutostartPath(enabled ? supposedPath : QString());
}

bool AutostartOptionPage::apply()
{
    if (m_unsupported) {
        return true; // don't treat this as an error
    }
    if (!setAutostartEnabled(ui()->autostartCheckBox->isChecked())) {
        errors() << QCoreApplication::translate("QtGui::AutostartOptionPage", "unable to modify startup entry");
        return false;
    }
    return true;
}

void AutostartOptionPage::reset()
{
    if (!hasBeenShown() || m_unsupported) {
        return;
    }
    const auto configuredPath = configuredAutostartPath();
    if (!configuredPath.has_value()) { // we can't determine the currently configured path
        ui()->pathWidget->setVisible(false);
        ui()->autostartCheckBox->setEnabled(true);
        ui()->autostartCheckBox->setChecked(isAutostartEnabled());
        return;
    }
    const auto autostartEnabled = !configuredPath.value().isEmpty();
    ui()->autostartCheckBox->setChecked(autostartEnabled);
    if (!autostartEnabled) {
        ui()->pathWidget->setVisible(false);
        ui()->autostartCheckBox->setEnabled(true);
        return;
    }
    const auto supposedPath = supposedAutostartPath();
    const auto pathMismatch = configuredPath != supposedPath;
    ui()->pathWidget->setVisible(pathMismatch);
    ui()->autostartCheckBox->setEnabled(!pathMismatch);
    if (pathMismatch) {
        ui()->pathWarningLabel->setText(QCoreApplication::translate("QtGui::AutostartOptionPage",
            "There is already an autostart entry for \"%1\". "
            "It will not be overridden when applying changes unless you delete it first.")
                .arg(configuredPath.value()));
    }
}

// LauncherOptionPage
LauncherOptionPage::LauncherOptionPage(QWidget *parentWidget)
    : QObject(parentWidget)
    , LauncherOptionPageBase(parentWidget)
    , m_process(nullptr)
    , m_launcher(SyncthingLauncher::mainInstance())
    , m_kill(false)
{
}

LauncherOptionPage::LauncherOptionPage(const QString &tool, const QString &toolName, const QString &windowTitle, QWidget *parentWidget)
    : QObject(parentWidget)
    , LauncherOptionPageBase(parentWidget)
    , m_process(&Settings::Launcher::toolProcess(tool))
    , m_launcher(nullptr)
    , m_restoreArgsAction(nullptr)
    , m_kill(false)
    , m_tool(tool)
    , m_toolName(toolName)
    , m_windowTitle(windowTitle)
{
}

LauncherOptionPage::~LauncherOptionPage()
{
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
static LibSyncthing::LogLevel comboBoxIndexToLogLevel(int index)
{
    switch (index) {
    case 0:
        return LibSyncthing::LogLevel::Debug;
    case 1:
        return LibSyncthing::LogLevel::Info;
    case 2:
        return LibSyncthing::LogLevel::Warning;
    case 3:
        return LibSyncthing::LogLevel::Error;
    default:
        return LibSyncthing::LogLevel::Default;
    }
}

static int logLevelToComboBoxIndex(LibSyncthing::LogLevel logLevel)
{
    switch (logLevel) {
    case LibSyncthing::LogLevel::Debug:
        return 0;
    case LibSyncthing::LogLevel::Info:
        return 1;
    case LibSyncthing::LogLevel::Warning:
        return 2;
    case LibSyncthing::LogLevel::Error:
        return 3;
    default:
        return 1;
    }
}
#endif

QWidget *LauncherOptionPage::setupWidget()
{
    auto *const widget = LauncherOptionPageBase::setupWidget();

    // adjust labels to use name of additional tool instead of "Syncthing"
    const auto isSyncthing = m_tool.isEmpty();
    if (!isSyncthing) {
        widget->setWindowTitle(m_windowTitle.isEmpty() ? tr("%1-launcher").arg(m_tool) : m_windowTitle);
        ui()->enabledCheckBox->setText(tr("Launch %1 when starting the tray icon").arg(m_toolName.isEmpty() ? m_tool : m_toolName));
        auto toolNameStartingSentence = m_toolName.isEmpty() ? m_tool : m_toolName;
        toolNameStartingSentence[0] = toolNameStartingSentence[0].toUpper();
        ui()->syncthingPathLabel->setText(tr("%1 executable").arg(toolNameStartingSentence));
        ui()->logLabel->setText(tr("%1 log (interleaved stdout/stderr)").arg(toolNameStartingSentence));

        // hide "consider for reconnect" and "show start/stop button on tray" checkboxes for tools
        ui()->considerForReconnectCheckBox->setVisible(false);
        ui()->showButtonCheckBox->setVisible(false);
        ui()->stopOnMeteredCheckBox->setVisible(false);
    }

    // set placeholder texts in path selections
    for (auto *const pathSelection :
        std::initializer_list<QtUtilities::PathSelection *>{ ui()->configDirPathSelection, ui()->dataDirPathSelection }) {
        pathSelection->lineEdit()->setPlaceholderText(tr("Leave empty for default path"));
    }

    // hide libsyncthing-controls by default (as the checkbox is unchecked by default)
    for (auto *const lstWidget : std::initializer_list<QWidget *>{ ui()->configDirLabel, ui()->configDirPathSelection, ui()->dataDirLabel,
             ui()->dataDirPathSelection, ui()->logLevelLabel, ui()->logLevelComboBox, ui()->optionsLabel, ui()->expandEnvCheckBox }) {
        lstWidget->setVisible(false);
    }

    // add "restore to defaults" action for Syncthing arguments
    if (isSyncthing) {
        m_restoreArgsAction = new QAction(ui()->argumentsLineEdit);
        m_restoreArgsAction->setText(tr("Restore default"));
        m_restoreArgsAction->setIcon(
            QIcon::fromTheme(QStringLiteral("edit-undo"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-paste.svg"))));
        connect(m_restoreArgsAction, &QAction::triggered, this, &LauncherOptionPage::restoreDefaultArguments);
        ui()->argumentsLineEdit->addCustomAction(m_restoreArgsAction);
        m_syncthingDownloadAction = new QAction(ui()->syncthingPathSelection);
        m_syncthingDownloadAction->setText(tr("Show Syncthing releases/downloads"));
        m_syncthingDownloadAction->setIcon(
            QIcon::fromTheme(QStringLiteral("download"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/download.svg"))));
        connect(m_syncthingDownloadAction, &QAction::triggered,
            [] { QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/syncthing/syncthing/releases"))); });
        ui()->syncthingPathSelection->lineEdit()->addCustomAction(m_syncthingDownloadAction);
        ui()->configDirPathSelection->provideCustomFileMode(QFileDialog::Directory);
        ui()->dataDirPathSelection->provideCustomFileMode(QFileDialog::Directory);
    }

    // setup other widgets
    ui()->syncthingPathSelection->provideCustomFileMode(QFileDialog::ExistingFile);
    ui()->logTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    const auto running = isRunning();
    ui()->launchNowPushButton->setHidden(running);
    ui()->stopPushButton->setHidden(!running);
    ui()->useBuiltInVersionCheckBox->setVisible(isSyncthing && SyncthingLauncher::isLibSyncthingAvailable());
    if (isSyncthing) {
        ui()->useBuiltInVersionCheckBox->setToolTip(SyncthingLauncher::libSyncthingVersionInfo());
    }

    // setup process/launcher
    if (m_process) {
        connect(m_process, &SyncthingProcess::readyRead, this, &LauncherOptionPage::handleSyncthingReadyRead, Qt::QueuedConnection);
        connect(m_process, static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
            &LauncherOptionPage::handleSyncthingExited, Qt::QueuedConnection);
        connect(m_process, &SyncthingProcess::errorOccurred, this, &LauncherOptionPage::handleSyncthingError, Qt::QueuedConnection);
        configureMeteredCheckbox(ui()->stopOnMeteredCheckBox, std::nullopt);
    } else if (m_launcher) {
        connect(m_launcher, &SyncthingLauncher::runningChanged, this, &LauncherOptionPage::handleSyncthingLaunched);
        connect(m_launcher, &SyncthingLauncher::outputAvailable, this, &LauncherOptionPage::handleSyncthingOutputAvailable, Qt::QueuedConnection);
        connect(m_launcher, &SyncthingLauncher::exited, this, &LauncherOptionPage::handleSyncthingExited, Qt::QueuedConnection);
        connect(m_launcher, &SyncthingLauncher::errorOccurred, this, &LauncherOptionPage::handleSyncthingError, Qt::QueuedConnection);
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
        connect(ui()->logLevelComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &LauncherOptionPage::updateLibSyncthingLogLevel);
#endif
        configureMeteredCheckbox(ui()->stopOnMeteredCheckBox, m_launcher->isNetworkConnectionMetered());
        connect(m_launcher, &SyncthingLauncher::networkConnectionMeteredChanged, this,
            std::bind(&configureMeteredCheckbox, ui()->stopOnMeteredCheckBox, std::placeholders::_1));

        m_launcher->setEmittingOutput(true);
    }
    connect(ui()->launchNowPushButton, &QPushButton::clicked, this, &LauncherOptionPage::launch);
    connect(ui()->stopPushButton, &QPushButton::clicked, this, &LauncherOptionPage::stop);

    return widget;
}

bool LauncherOptionPage::apply()
{
    auto &settings = Settings::values().launcher;
    if (m_tool.isEmpty()) {
        settings.autostartEnabled = ui()->enabledCheckBox->isChecked();
        settings.useLibSyncthing = ui()->useBuiltInVersionCheckBox->isChecked();
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
        settings.libSyncthing.configDir = ui()->configDirPathSelection->lineEdit()->text();
        settings.libSyncthing.dataDir = ui()->dataDirPathSelection->lineEdit()->text();
        settings.libSyncthing.logLevel = comboBoxIndexToLogLevel(ui()->logLevelComboBox->currentIndex());
        settings.libSyncthing.expandPaths = ui()->expandEnvCheckBox->isChecked();
#endif
        settings.syncthingPath = ui()->syncthingPathSelection->lineEdit()->text();
        settings.syncthingArgs = ui()->argumentsLineEdit->text();
        settings.considerForReconnect = ui()->considerForReconnectCheckBox->isChecked();
        settings.showButton = ui()->showButtonCheckBox->isChecked();
        settings.stopOnMeteredConnection = ui()->stopOnMeteredCheckBox->isChecked();
        if (m_launcher) {
            m_launcher->setStoppingOnMeteredConnection(settings.stopOnMeteredConnection);
        }
    } else {
        auto &params = settings.tools[m_tool];
        params.autostart = ui()->enabledCheckBox->isChecked();
        params.path = ui()->syncthingPathSelection->lineEdit()->text();
        params.args = ui()->argumentsLineEdit->text();
    }
    return true;
}

void LauncherOptionPage::reset()
{
    const auto &settings = Settings::values().launcher;
    if (m_tool.isEmpty()) {
        ui()->enabledCheckBox->setChecked(settings.autostartEnabled);
        ui()->useBuiltInVersionCheckBox->setChecked(settings.useLibSyncthing);
        ui()->useBuiltInVersionCheckBox->setVisible(settings.useLibSyncthing || SyncthingLauncher::isLibSyncthingAvailable());
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
        ui()->configDirPathSelection->lineEdit()->setText(settings.libSyncthing.configDir);
        ui()->dataDirPathSelection->lineEdit()->setText(settings.libSyncthing.dataDir);
        ui()->logLevelComboBox->setCurrentIndex(logLevelToComboBoxIndex(settings.libSyncthing.logLevel));
        ui()->expandEnvCheckBox->setChecked(settings.libSyncthing.expandPaths);
#endif
        ui()->syncthingPathSelection->lineEdit()->setText(settings.syncthingPath);
        ui()->argumentsLineEdit->setText(settings.syncthingArgs);
        ui()->considerForReconnectCheckBox->setChecked(settings.considerForReconnect);
        ui()->showButtonCheckBox->setChecked(settings.showButton);
        ui()->stopOnMeteredCheckBox->setChecked(settings.stopOnMeteredConnection);
    } else {
        const auto params = settings.tools.value(m_tool);
        ui()->useBuiltInVersionCheckBox->setChecked(false);
        ui()->useBuiltInVersionCheckBox->setVisible(false);
        ui()->enabledCheckBox->setChecked(params.autostart);
        ui()->syncthingPathSelection->lineEdit()->setText(params.path);
        ui()->argumentsLineEdit->setText(params.args);
    }
}

void LauncherOptionPage::handleSyncthingLaunched(bool running)
{
    if (!running) {
        return; // Syncthing being stopped is handled elsewhere
    }
    ui()->launchNowPushButton->hide();
    ui()->stopPushButton->show();
    ui()->stopPushButton->setText(tr("Stop launched instance"));
    m_kill = false;
}

void LauncherOptionPage::handleSyncthingReadyRead()
{
    handleSyncthingOutputAvailable(m_process->readAll());
}

void LauncherOptionPage::handleSyncthingOutputAvailable(const QByteArray &output)
{
    if (!hasBeenShown()) {
        return;
    }
    QTextCursor cursor(ui()->logTextEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QString::fromUtf8(output));
    if (ui()->ensureCursorVisibleCheckBox->isChecked()) {
        ui()->logTextEdit->moveCursor(QTextCursor::End);
        ui()->logTextEdit->ensureCursorVisible();
    }
}

void LauncherOptionPage::handleSyncthingExited(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!hasBeenShown()) {
        return;
    }

    QTextCursor cursor(ui()->logTextEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock();

    switch (exitStatus) {
    case QProcess::NormalExit:
        cursor.insertText(tr("%1 exited with exit code %2").arg(m_tool.isEmpty() ? QStringLiteral("Syncthing") : m_tool, QString::number(exitCode)));
        break;
    case QProcess::CrashExit:
        cursor.insertText(tr("%1 crashed with exit code %2").arg(m_tool.isEmpty() ? QStringLiteral("Syncthing") : m_tool, QString::number(exitCode)));
        break;
    }
    cursor.insertBlock();

    if (ui()->ensureCursorVisibleCheckBox->isChecked()) {
        ui()->logTextEdit->moveCursor(QTextCursor::End);
        ui()->logTextEdit->ensureCursorVisible();
    }

    ui()->stopPushButton->hide();
    ui()->launchNowPushButton->show();
}

void LauncherOptionPage::handleSyncthingError(QProcess::ProcessError error)
{
    if (!hasBeenShown()) {
        return;
    }

    QTextCursor cursor(ui()->logTextEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock();

    auto errorString = QString();
    if (m_launcher) {
        errorString = m_launcher->errorString();
    } else if (m_process) {
        errorString = m_process->errorString();
    }
    if (errorString.isEmpty()) {
        switch (error) {
        case QProcess::FailedToStart:
            errorString = tr("failed to start (e.g. executable does not exist or not permission error)");
            break;
        case QProcess::Crashed:
            errorString = tr("process crashed");
            break;
        case QProcess::Timedout:
            errorString = tr("timeout error");
            break;
        case QProcess::ReadError:
            errorString = tr("read error");
            break;
        case QProcess::WriteError:
            errorString = tr("write error");
            break;
        default:
            errorString = tr("unknown process error");
        }
    }
    cursor.insertText(tr("An error occurred when running %1: %2").arg(m_tool.isEmpty() ? QStringLiteral("Syncthing") : m_tool, errorString));
    cursor.insertBlock();

    if ((m_launcher && !m_launcher->isRunning()) || (m_process && !m_process->isRunning())) {
        ui()->stopPushButton->hide();
        ui()->launchNowPushButton->show();
    }
}

bool LauncherOptionPage::isRunning() const
{
    return (m_process && m_process->isRunning()) || (m_launcher && m_launcher->isRunning());
}

void LauncherOptionPage::launch()
{
    if (!hasBeenShown()) {
        return;
    }
    apply();
    if (isRunning()) {
        return;
    }
    const auto &launcherSettings(Settings::values().launcher);
    if (m_tool.isEmpty()) {
        m_launcher->launch(launcherSettings);
        return;
    }
    const auto toolParams(launcherSettings.tools.value(m_tool));
    m_process->startSyncthing(toolParams.path, SyncthingProcess::splitArguments(toolParams.args));
    handleSyncthingLaunched(true);
}

#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
void LauncherOptionPage::updateLibSyncthingLogLevel()
{
    m_launcher->setLibSyncthingLogLevel(comboBoxIndexToLogLevel(ui()->logLevelComboBox->currentIndex()));
}
#endif

void LauncherOptionPage::stop()
{
    if (!hasBeenShown()) {
        return;
    }
    if (m_kill) {
        if (m_process) {
            m_process->killSyncthing();
        }
        if (m_launcher) {
            m_launcher->kill();
        }
    } else {
        ui()->stopPushButton->setText(tr("Kill launched instance"));
        m_kill = true;
        if (m_process) {
            m_process->stopSyncthing();
        }
        if (m_launcher) {
            m_launcher->terminate(Settings::Launcher::connectionForLauncher(m_launcher));
        }
    }
}

void LauncherOptionPage::restoreDefaultArguments()
{
    static const Settings::Launcher defaults;
    ui()->argumentsLineEdit->setText(defaults.syncthingArgs);
}

// SystemdOptionPage
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
SystemdOptionPage::SystemdOptionPage(QWidget *parentWidget)
    : SystemdOptionPageBase(parentWidget)
    , m_service(SyncthingService::mainInstance())
{
}

SystemdOptionPage::~SystemdOptionPage()
{
    QObject::disconnect(m_unitChangedConn);
    QObject::disconnect(m_descChangedConn);
    QObject::disconnect(m_statusChangedConn);
    QObject::disconnect(m_enabledChangedConn);
}

QWidget *SystemdOptionPage::setupWidget()
{
    auto *const widget = SystemdOptionPageBase::setupWidget();
    auto *const refreshAction = new QAction(QCoreApplication::translate("QtGui::SystemdOptionPage", "Reload all unit files"), widget);
    refreshAction->setIcon(
        QIcon::fromTheme(QStringLiteral("view-refresh"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/view-refresh.svg"))));
    ui()->syncthingUnitLineEdit->addCustomAction(refreshAction);
    if (!m_service) {
        ui()->stopOnMeteredCheckBox->setHidden(true);
        return widget;
    }
    QObject::connect(refreshAction, &QAction::triggered, m_service, &SyncthingService::reloadAllUnitFiles);
    QObject::connect(ui()->syncthingUnitLineEdit, &QLineEdit::textChanged, m_service, &SyncthingService::setUnitName);
    QObject::connect(ui()->startPushButton, &QPushButton::clicked, m_service, &SyncthingService::start);
    QObject::connect(ui()->stopPushButton, &QPushButton::clicked, m_service, &SyncthingService::stop);
    QObject::connect(ui()->enablePushButton, &QPushButton::clicked, m_service, &SyncthingService::enable);
    QObject::connect(ui()->disablePushButton, &QPushButton::clicked, m_service, &SyncthingService::disable);
    QObject::connect(ui()->stopOnMeteredCheckBox,
        &QCheckBox::
#if (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0))
            checkStateChanged
#else
            stateChanged
#endif
        ,
        m_service,
        [s = m_service](
#if (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0))
            Qt::CheckState
#else
            int
#endif
                checkState) { s->setStoppingOnMeteredConnection(checkState == Qt::Checked); });
    m_unitChangedConn
        = QObject::connect(ui()->systemUnitCheckBox, &QCheckBox::clicked, m_service, bind(&SystemdOptionPage::handleSystemUnitChanged, this));
    m_descChangedConn
        = QObject::connect(m_service, &SyncthingService::descriptionChanged, bind(&SystemdOptionPage::handleDescriptionChanged, this, _1));
    m_statusChangedConn
        = QObject::connect(m_service, &SyncthingService::stateChanged, bind(&SystemdOptionPage::handleStatusChanged, this, _1, _2, _3));
    m_enabledChangedConn
        = QObject::connect(m_service, &SyncthingService::unitFileStateChanged, bind(&SystemdOptionPage::handleEnabledChanged, this, _1));
    if (const auto *optionPageWidget = qobject_cast<OptionPageWidget *>(widget)) {
        QObject::connect(optionPageWidget, &OptionPageWidget::paletteChanged, std::bind(&SystemdOptionPage::updateColors, this));
    }
    configureMeteredCheckbox(ui()->stopOnMeteredCheckBox, m_service->isNetworkConnectionMetered());
    QObject::connect(m_service, &SyncthingService::networkConnectionMeteredChanged,
        std::bind(&configureMeteredCheckbox, ui()->stopOnMeteredCheckBox, std::placeholders::_1));
    return widget;
}

bool SystemdOptionPage::apply()
{
    auto &settings = Settings::values();
    auto &systemdSettings = settings.systemd;
    auto &launcherSettings = settings.launcher;
    systemdSettings.syncthingUnit = ui()->syncthingUnitLineEdit->text();
    systemdSettings.systemUnit = ui()->systemUnitCheckBox->isChecked();
    systemdSettings.showButton = ui()->showButtonCheckBox->isChecked();
    systemdSettings.considerForReconnect = ui()->considerForReconnectCheckBox->isChecked();
    systemdSettings.stopOnMeteredConnection = ui()->stopOnMeteredCheckBox->isChecked();
    auto result = true;
    if (systemdSettings.showButton && launcherSettings.showButton) {
        errors().append(QCoreApplication::translate("QtGui::SystemdOptionPage",
            "It is not possible to show the start/stop button for the systemd service and the internal launcher at the same time. The systemd "
            "service precedes."));
        result = false;
    }
    if (systemdSettings.considerForReconnect && launcherSettings.considerForReconnect) {
        errors().append(QCoreApplication::translate("QtGui::SystemdOptionPage",
            "It is not possible to consider the systemd service and the internal launcher for reconnects at the same time. The systemd service "
            "precedes."));
        result = false;
    }
    return result;
}

void SystemdOptionPage::reset()
{
    const auto &settings = Settings::values().systemd;
    ui()->syncthingUnitLineEdit->setText(settings.syncthingUnit);
    ui()->systemUnitCheckBox->setChecked(settings.systemUnit);
    ui()->showButtonCheckBox->setChecked(settings.showButton);
    ui()->considerForReconnectCheckBox->setChecked(settings.considerForReconnect);
    ui()->stopOnMeteredCheckBox->setChecked(settings.stopOnMeteredConnection);
    if (!m_service) {
        return;
    }
    handleDescriptionChanged(m_service->description());
    handleStatusChanged(m_service->activeState(), m_service->subState(), m_service->activeSince());
    handleEnabledChanged(m_service->unitFileState());
}

void SystemdOptionPage::handleSystemUnitChanged()
{
    m_service->setScope(ui()->systemUnitCheckBox->isChecked() ? SystemdScope::System : SystemdScope::User);
}

void SystemdOptionPage::handleDescriptionChanged(const QString &description)
{
    ui()->descriptionValueLabel->setText(description.isEmpty()
            ? QCoreApplication::translate("QtGui::SystemdOptionPage", "specified unit is either inactive or doesn't exist")
            : description);
}

static void setIndicatorColor(QWidget *indicator, const QColor &color)
{
    indicator->setStyleSheet(QStringLiteral("border-radius:8px;background-color:") + color.name());
}

void SystemdOptionPage::handleStatusChanged(const QString &activeState, const QString &subState, DateTime activeSince)
{
    m_status.clear();
    if (!activeState.isEmpty()) {
        m_status << activeState;
    }
    if (!subState.isEmpty()) {
        m_status << subState;
    }

    const bool isRunning = updateRunningColor();
    QString timeStamp;
    if (isRunning && !activeSince.isNull()) {
        timeStamp = QLatin1Char('\n') % QCoreApplication::translate("QtGui::SystemdOptionPage", "since ")
            % QString::fromUtf8(activeSince.toString(DateTimeOutputFormat::DateAndTime).data());
    }

    ui()->statusValueLabel->setText(
        m_status.isEmpty() ? QCoreApplication::translate("QtGui::SystemdOptionPage", "unknown") : m_status.join(QStringLiteral(" - ")) + timeStamp);
    ui()->startPushButton->setVisible(!isRunning);
    ui()->stopPushButton->setVisible(!m_status.isEmpty() && isRunning);
}

void SystemdOptionPage::handleEnabledChanged(const QString &unitFileState)
{
    const auto isEnabled = updateEnabledColor();
    ui()->unitFileStateValueLabel->setText(
        unitFileState.isEmpty() ? QCoreApplication::translate("QtGui::SystemdOptionPage", "unknown") : unitFileState);
    ui()->enablePushButton->setVisible(!isEnabled);
    ui()->disablePushButton->setVisible(!unitFileState.isEmpty() && isEnabled);
}

bool SystemdOptionPage::updateRunningColor()
{
    const bool isRunning = m_service && m_service->isRunning();
    const auto brightColors = isPaletteDark(widget()->palette());
    setIndicatorColor(ui()->statusIndicator,
        m_status.isEmpty() ? Colors::gray(brightColors) : (isRunning ? Colors::green(brightColors) : Colors::red(brightColors)));
    return isRunning;
}

bool SystemdOptionPage::updateEnabledColor()
{
    const auto isEnabled = m_service && m_service->isEnabled();
    const auto brightColors = isPaletteDark(widget()->palette());
    setIndicatorColor(ui()->enabledIndicator, isEnabled ? Colors::green(brightColors) : Colors::gray(brightColors));
    return isEnabled;
}

void SystemdOptionPage::updateColors()
{
    updateRunningColor();
    updateEnabledColor();
}
#endif

// GeneralWebViewOptionPage
GeneralWebViewOptionPage::GeneralWebViewOptionPage(QWidget *parentWidget)
    : GeneralWebViewOptionPageBase(parentWidget)
{
}

GeneralWebViewOptionPage::~GeneralWebViewOptionPage()
{
}

QWidget *GeneralWebViewOptionPage::setupWidget()
{
    auto *const widget = GeneralWebViewOptionPageBase::setupWidget();
    auto *const cfgToolButton = ui()->appModeCfgToolButton;
#ifdef SYNCTHINGWIDGETS_NO_WEBVIEW
    ui()->builtinRadioButton->setEnabled(false);
#endif
    const auto minHeight = cfgToolButton->height();
    ui()->builtinRadioButton->setMinimumHeight(minHeight);
    ui()->browserRadioButton->setMinimumHeight(minHeight);
    ui()->appModeRadioButton->setMinimumHeight(minHeight);
    QObject::connect(cfgToolButton, &QToolButton::clicked, cfgToolButton, [this] { showCustomCommandPrompt(); });
    return widget;
}

bool GeneralWebViewOptionPage::apply()
{
    auto &webView = Settings::values().webView;
    if (ui()->builtinRadioButton->isChecked()) {
        webView.mode = Settings::WebView::Mode::Builtin;
    } else if (ui()->browserRadioButton->isChecked()) {
        webView.mode = Settings::WebView::Mode::Browser;
    } else if (ui()->appModeRadioButton->isChecked()) {
        webView.mode = Settings::WebView::Mode::Command;
    }
    webView.customCommand = m_customCommand;
    return true;
}

void GeneralWebViewOptionPage::reset()
{
    const auto &webView = Settings::values().webView;
    switch (webView.mode) {
    case Settings::WebView::Mode::Builtin:
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
        ui()->builtinRadioButton->setChecked(true);
        break;
#endif
    case Settings::WebView::Mode::Browser:
        ui()->browserRadioButton->setChecked(true);
        break;
    case Settings::WebView::Mode::Command:
        ui()->appModeRadioButton->setChecked(true);
        break;
    }
    m_customCommand = webView.customCommand;
}

void GeneralWebViewOptionPage::showCustomCommandPrompt()
{
    auto dlg = QInputDialog();
    dlg.setInputMode(QInputDialog::TextInput);
    dlg.setWindowTitle(
        QCoreApplication::translate("QtGui::GeneralWebViewOptionPage", "Custom command to launch Syncthing's UI - ") + QStringLiteral(APP_NAME));
    dlg.setLabelText(QCoreApplication::translate("QtGui::GeneralWebViewOptionPage",
        "<p>Enter a custom command to launch Syncthing's UI. The expression <code>%SYNCTHING_URL%</code> will be replaced with the "
        "Syncthing-URL.</p><p>Leave the command empty to use the auto-detection.</p>"));
    dlg.setTextValue(m_customCommand);
    if (dlg.exec() == QDialog::Accepted) {
        m_customCommand = dlg.textValue();
    }
}

// BuiltinWebViewOptionPage
BuiltinWebViewOptionPage::BuiltinWebViewOptionPage(QWidget *parentWidget)
    : BuiltinWebViewOptionPageBase(parentWidget)
{
}

BuiltinWebViewOptionPage::~BuiltinWebViewOptionPage()
{
}

#ifdef SYNCTHINGWIDGETS_NO_WEBVIEW
QWidget *BuiltinWebViewOptionPage::setupWidget()
{
    auto *label = new QLabel;
    label->setWindowTitle(QCoreApplication::translate("QtGui::BuiltinWebViewOptionPage", "Built-in web view"));
    label->setAlignment(Qt::AlignCenter);
    label->setText(QCoreApplication::translate("QtGui::BuiltinWebViewOptionPage",
        "Syncthing Tray has not been built with vieb view support utilizing either Qt WebKit "
        "or Qt WebEngine."));
    return label;
}
#endif

bool BuiltinWebViewOptionPage::apply()
{
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    auto &webView = Settings::values().webView;
    webView.zoomFactor = ui()->zoomDoubleSpinBox->value();
    webView.keepRunning = ui()->keepRunningCheckBox->isChecked();
#endif
    return true;
}

void BuiltinWebViewOptionPage::reset()
{
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    const auto &webView = Settings::values().webView;
    ui()->zoomDoubleSpinBox->setValue(webView.zoomFactor);
    ui()->keepRunningCheckBox->setChecked(webView.keepRunning);
#endif
}

QtUtilities::RestartHandler *SettingsDialog::s_restartHandler = nullptr;

SettingsDialog::SettingsDialog(const QList<OptionCategory *> &categories, QWidget *parent)
    : QtUtilities::SettingsDialog(parent)
{
    categoryModel()->setCategories(categories);
    init();
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QtUtilities::SettingsDialog(parent)
{
    init();
}

#ifdef SYNCTHINGWIDGETS_SETUP_TOOLS_ENABLED
/// \cond
static void replaceProcess()
{
    auto *const updateHandler = QtUtilities::UpdateHandler::mainInstance();
    if (!updateHandler) {
        return;
    }
    auto *const process = new Data::SyncthingProcess();
    QObject::connect(process, &Data::SyncthingProcess::finished, process, &QObject::deleteLater);
    QObject::connect(process, &Data::SyncthingProcess::errorOccurred, process, [process] {
        auto messageBox = QMessageBox();
        messageBox.setWindowTitle(QStringLiteral("Syncthing"));
        messageBox.setWindowIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
        messageBox.setIcon(QMessageBox::Critical);
        messageBox.setText(QCoreApplication::translate("QtGui", "Unable to restart via \"%1\": %2").arg(process->program(), process->errorString()));
        messageBox.exec();
    });
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    process->start(updateHandler->updater()->storedPath(), QStringList({ QStringLiteral("qt-widgets-gui"), QStringLiteral("--replace") }));
}
/// \endcond
#endif

SettingsDialog::SettingsDialog(Data::SyncthingConnection *connection, QWidget *parent)
    : QtUtilities::SettingsDialog(parent)
{
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

#ifdef SYNCTHINGWIDGETS_SETUP_TOOLS_ENABLED
    // initialize option page for updating
    m_updateOptionPage = new UpdateOptionPage(QtUtilities::UpdateHandler::mainInstance(), this);
    if (Settings::values().isIndependentInstance) {
        if (!s_restartHandler) {
            s_restartHandler = new RestartHandler;
        }
        m_updateOptionPage->setRestartHandler(s_restartHandler->requester());
    } else {
        m_updateOptionPage->setRestartHandler(&replaceProcess);
    }
#endif

    // setup categories
    QList<OptionCategory *> categories;
    OptionCategory *category;

    category = new OptionCategory(this);
    translateCategory(category, [] { return tr("Tray"); });
    category->assignPages({ m_connectionsOptionPage = new ConnectionOptionPage(connection), new NotificationsOptionPage,
        m_appearanceOptionPage = new AppearanceOptionPage, new IconsOptionPage(IconsOptionPage::Context::UI),
        new IconsOptionPage(IconsOptionPage::Context::System) });
    category->setIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    categories << category;

    category = new OptionCategory(this);
    translateCategory(category, [] { return tr("Web view"); });
    category->assignPages({ new GeneralWebViewOptionPage, new BuiltinWebViewOptionPage });
    category->setIcon(
        QIcon::fromTheme(QStringLiteral("internet-web-browser"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/internet-web-browser.svg"))));
    categories << category;

    category = new OptionCategory(this);
    translateCategory(category, [] { return tr("Startup"); });
    category->assignPages({ new AutostartOptionPage, new LauncherOptionPage,
        new LauncherOptionPage(QStringLiteral("Process"), tr("additional tool"), tr("Extra launcher"))
#ifdef SYNCTHINGWIDGETS_SETUP_TOOLS_ENABLED
            ,
        m_updateOptionPage
#endif
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        ,
        new SystemdOptionPage
#endif
    });
    category->setIcon(QIcon::fromTheme(QStringLiteral("system-run"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/system-run.svg"))));
    m_launcherSettingsCategory = static_cast<int>(categories.size());
    m_launcherSettingsPageIndex = 1;
    m_updateSettingsCategory = m_launcherSettingsCategory;
    m_updateSettingsPageIndex = 3;
    categories << category;

    categories << Settings::values().qt.category();
    categoryModel()->setCategories(categories);
    init();
}

void SettingsDialog::respawnIfRestartRequested()
{
#ifdef SYNCTHINGWIDGETS_SETUP_TOOLS_ENABLED
    if (!s_restartHandler) {
        return;
    }
    s_restartHandler->respawnIfRestartRequested();
    delete s_restartHandler;
    s_restartHandler = nullptr;
#endif
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::init()
{
    resize(1100, 750);
    setWindowTitle(tr("Settings") + QStringLiteral(" - " APP_NAME));
    setWindowIcon(
        QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/preferences-other.svg"))));

    // add button for starting wizard
    auto *startWizardButton = new QPushButton(this);
    startWizardButton->setToolTip(tr("Start wizard"));
    startWizardButton->setIcon(
        QIcon::fromTheme(QStringLiteral("quickwizard"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/tools-wizard.svg"))));
    startWizardButton->setFlat(true);
    startWizardButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    connect(startWizardButton, &QPushButton::clicked, this, &SettingsDialog::wizardRequested);
    addHeadingWidget(startWizardButton);
}

void SettingsDialog::hideConnectionStatus()
{
    if (m_connectionsOptionPage) {
        m_connectionsOptionPage->hideConnectionStatus();
    }
}

void SettingsDialog::resetPositioningSettings()
{
    if (m_appearanceOptionPage && m_appearanceOptionPage->hasBeenShown()) {
        m_appearanceOptionPage->resetPositioningSettings();
    }
}

void SettingsDialog::selectLauncherSettings()
{
    if (m_launcherSettingsCategory >= 0 && m_launcherSettingsPageIndex >= 0) {
        selectPage(m_launcherSettingsCategory, m_launcherSettingsPageIndex);
    }
}

void SettingsDialog::selectUpdateSettings()
{
    if (m_updateSettingsCategory >= 0 && m_updateSettingsPageIndex >= 0) {
        selectPage(m_updateSettingsCategory, m_updateSettingsPageIndex);
    }
}

} // namespace QtGui

INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, ConnectionOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, NotificationsOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, AppearanceOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, IconsOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, AutostartOptionPage)
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, LauncherOptionPage)
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, SystemdOptionPage)
#endif
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, GeneralWebViewOptionPage)
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, BuiltinWebViewOptionPage)
#endif
