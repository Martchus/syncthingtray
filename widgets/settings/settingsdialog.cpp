#include "./settingsdialog.h"

#include "../misc/syncthinglauncher.h"

#include "../../connector/syncthingconfig.h"
#include "../../connector/syncthingconnection.h"
#include "../../connector/syncthingprocess.h"
#include "../../connector/utils.h"
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
#include "../../connector/syncthingservice.h"
#include "../../model/colors.h"
#include "../../model/syncthingicons.h"
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
#include "ui_webviewoptionpage.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

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

#include <QFileDialog>
#include <QHostAddress>
#include <QMessageBox>
#if defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)
#include <QStandardPaths>
#elif defined(PLATFORM_WINDOWS)
#include <QSettings>
#endif
#include <QApplication>
#include <QFontDatabase>
#include <QStringBuilder>
#include <QStyle>
#include <QTextCursor>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Settings;
using namespace Data;
using namespace CppUtilities;
using namespace QtUtilities;

namespace QtGui {

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

QWidget *ConnectionOptionPage::setupWidget()
{
    auto *const widget = ConnectionOptionPageBase::setupWidget();
    ui()->certPathSelection->provideCustomFileMode(QFileDialog::ExistingFile);
    ui()->certPathSelection->lineEdit()->setPlaceholderText(
        QCoreApplication::translate("QtGui::ConnectionOptionPage", "Auto-detected for local instance"));
    ui()->instanceNoteIcon->setPixmap(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(32, 32));
    QObject::connect(m_connection, &SyncthingConnection::statusChanged, bind(&ConnectionOptionPage::updateConnectionStatus, this));
    QObject::connect(ui()->connectPushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::applyAndReconnect, this));
    QObject::connect(ui()->insertFromConfigFilePushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::insertFromConfigFile, this));
    QObject::connect(ui()->selectionComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        bind(&ConnectionOptionPage::showConnectionSettings, this, _1));
    QObject::connect(ui()->selectionComboBox, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::editTextChanged),
        bind(&ConnectionOptionPage::saveCurrentConfigName, this, _1));
    QObject::connect(ui()->downPushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::moveSelectedConfigDown, this));
    QObject::connect(ui()->upPushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::moveSelectedConfigUp, this));
    QObject::connect(ui()->addPushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::addNewConfig, this));
    QObject::connect(ui()->removePushButton, &QPushButton::clicked, bind(&ConnectionOptionPage::removeSelectedConfig, this));
    return widget;
}

void ConnectionOptionPage::insertFromConfigFile()
{
    auto configFile(SyncthingConfig::locateConfigFile());
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
        QString guiHost(config.guiAddress.mid(0, portStart));
        const QStringRef guiPort(portStart > 0 ? config.guiAddress.midRef(portStart) : QStringRef());
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
    ui()->statusLabel->setText(m_connection->statusText());
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
    ui()->authCheckBox->setChecked(connectionSettings.authEnabled);
    ui()->userNameLineEdit->setText(connectionSettings.userName);
    ui()->passwordLineEdit->setText(connectionSettings.password);
    ui()->apiKeyLineEdit->setText(connectionSettings.apiKey);
    ui()->certPathSelection->lineEdit()->setText(connectionSettings.httpsCertPath);
    ui()->pollTrafficSpinBox->setValue(connectionSettings.trafficPollInterval);
    ui()->pollDevStatsSpinBox->setValue(connectionSettings.devStatsPollInterval);
    ui()->pollErrorsSpinBox->setValue(connectionSettings.errorsPollInterval);
    ui()->reconnectSpinBox->setValue(connectionSettings.reconnectInterval);
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
    connectionSettings.authEnabled = ui()->authCheckBox->isChecked();
    connectionSettings.userName = ui()->userNameLineEdit->text();
    connectionSettings.password = ui()->passwordLineEdit->text();
    connectionSettings.apiKey = ui()->apiKeyLineEdit->text().toUtf8();
    connectionSettings.expectedSslErrors.clear();
    connectionSettings.httpsCertPath = ui()->certPathSelection->lineEdit()->text();
    connectionSettings.trafficPollInterval = ui()->pollTrafficSpinBox->value();
    connectionSettings.devStatsPollInterval = ui()->pollDevStatsSpinBox->value();
    connectionSettings.errorsPollInterval = ui()->pollErrorsSpinBox->value();
    connectionSettings.reconnectInterval = ui()->reconnectSpinBox->value();
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
        m_primarySettings = move(m_secondarySettings.front());
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

bool ConnectionOptionPage::apply()
{
    if (!cacheCurrentSettings(true)) {
        return false;
    }
    values().connection.primary = m_primarySettings;
    values().connection.secondary = m_secondarySettings;
    return true;
}

void ConnectionOptionPage::reset()
{
    m_primarySettings = values().connection.primary;
    m_secondarySettings = values().connection.secondary;
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
    m_connection->reconnect((m_currentIndex == 0 ? m_primarySettings : m_secondarySettings[static_cast<size_t>(m_currentIndex - 1)]));
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
    auto &settings(values());
    auto &notifyOn(settings.notifyOn);
    notifyOn.disconnect = ui()->notifyOnDisconnectCheckBox->isChecked();
    notifyOn.internalErrors = ui()->notifyOnErrorsCheckBox->isChecked();
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
    values().ignoreInavailabilityAfterStart = static_cast<unsigned int>(ui()->ignoreInavailabilityAfterStartSpinBox->value());
    return ok;
}

void NotificationsOptionPage::reset()
{
    const auto &notifyOn = values().notifyOn;
    ui()->notifyOnDisconnectCheckBox->setChecked(notifyOn.disconnect);
    ui()->notifyOnErrorsCheckBox->setChecked(notifyOn.internalErrors);
    ui()->notifyOnLocalSyncCompleteCheckBox->setChecked(notifyOn.localSyncComplete);
    ui()->notifyOnRemoteSyncCompleteCheckBox->setChecked(notifyOn.remoteSyncComplete);
    ui()->showSyncthingNotificationsCheckBox->setChecked(notifyOn.syncthingErrors);
    ui()->notifyOnNewDevConnectsCheckBox->setChecked(notifyOn.newDeviceConnects);
    ui()->notifyOnNewDirSharedCheckBox->setChecked(notifyOn.newDirectoryShared);
#ifdef QT_UTILITIES_SUPPORT_DBUS_NOTIFICATIONS
    (values().dbusNotifications ? ui()->dbusRadioButton : ui()->qtRadioButton)->setChecked(true);
#else
    ui()->dbusRadioButton->setEnabled(false);
    ui()->qtRadioButton->setChecked(true);
#endif
    ui()->ignoreInavailabilityAfterStartSpinBox->setValue(static_cast<int>(values().ignoreInavailabilityAfterStart));
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
    auto &settings = values().appearance;
    settings.trayMenuSize.setWidth(ui()->widthSpinBox->value());
    settings.trayMenuSize.setHeight(ui()->heightSpinBox->value());
    settings.showTraffic = ui()->showTrafficCheckBox->isChecked();
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

    settings.brightTextColors = ui()->brightTextColorsCheckBox->isChecked();
    return true;
}

void AppearanceOptionPage::reset()
{
    const auto &settings = values().appearance;
    ui()->widthSpinBox->setValue(settings.trayMenuSize.width());
    ui()->heightSpinBox->setValue(settings.trayMenuSize.height());
    ui()->showTrafficCheckBox->setChecked(settings.showTraffic);
    int index;
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
    ui()->brightTextColorsCheckBox->setChecked(settings.brightTextColors);
}

// IconsOptionPage
IconsOptionPage::IconsOptionPage(QWidget *parentWidget)
    : IconsOptionPageBase(parentWidget)
{
}

IconsOptionPage::~IconsOptionPage()
{
}

QWidget *IconsOptionPage::setupWidget()
{
    auto *const widget = IconsOptionPageBase::setupWidget();

    // populate form for status icon colors
    auto *const gridLayout = ui()->gridLayout;
    auto *const statusIconsGroupBox = ui()->statusIconsGroupBox;
    int index = 0;
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

        // add label for color name
        gridLayout->addWidget(new QLabel(colorMapping.colorName, statusIconsGroupBox), index, 0, Qt::AlignRight | Qt::AlignVCenter);

        // setup preview
        gridLayout->addWidget(widgetsForColor.previewLabel, index, 4, Qt::AlignCenter);
        const auto updatePreview = [&widgetsForColor] {
            widgetsForColor.previewLabel->setPixmap(renderSvgImage(makeSyncthingIcon(
                                                                       StatusIconColorSet{
                                                                           widgetsForColor.colorButtons[0]->color(),
                                                                           widgetsForColor.colorButtons[1]->color(),
                                                                           widgetsForColor.colorButtons[2]->color(),
                                                                       },
                                                                       widgetsForColor.statusEmblem),
                QSize(32, 32)));
        };
        for (const auto &colorButton : widgetsForColor.colorButtons) {
            QObject::connect(colorButton, &ColorButton::colorChanged, updatePreview);
        }

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

    // setup additional buttons
    QObject::connect(ui()->restoreDefaultsPushButton, &QPushButton::clicked, [this] {
        m_settings = Data::StatusIconSettings();
        update();
    });
    QObject::connect(ui()->restorePreviousPushButton, &QPushButton::clicked, [this] { reset(); });

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
    values().statusIcons = m_settings;
    return true;
}

void IconsOptionPage::update()
{
    for (auto &widgetsForColor : m_widgets) {
        widgetsForColor.colorButtons[0]->setColor(widgetsForColor.setting->backgroundStart);
        widgetsForColor.colorButtons[1]->setColor(widgetsForColor.setting->backgroundEnd);
        widgetsForColor.colorButtons[2]->setColor(widgetsForColor.setting->foreground);
    }
}

void IconsOptionPage::reset()
{
    m_settings = values().statusIcons;
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

    ui()->infoIconLabel->setPixmap(
        QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation, nullptr, ui()->infoIconLabel).pixmap(ui()->infoIconLabel->size()));
#if defined(PLATFORM_LINUX) && !defined(PLATFORM_ANDROID)
    ui()->platformNoteLabel->setText(QCoreApplication::translate("QtGui::AutostartOptionPage",
        "This is achieved by adding a *.desktop file under <i>~/.config/autostart</i> so the setting only affects the current user."));
#elif defined(PLATFORM_WINDOWS)
    ui()->platformNoteLabel->setText(QCoreApplication::translate("QtGui::AutostartOptionPage",
        "This is achieved by adding a registry key under <i>HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run</i> so the setting "
        "only affects the current user. Note that the startup entry is invalidated when moving <i>syncthingtray.exe</i>."));
#else
    ui()->platformNoteLabel->setText(
        QCoreApplication::translate("QtGui::AutostartOptionPage", "This feature has not been implemented for your platform (yet)."));
    ui()->autostartCheckBox->setEnabled(false);
#endif
    return widget;
}

/*!
 * \brief Returns whether the application is launched on startup.
 * \remarks
 * - Only implemented under Linux/Windows. Always returns false on other platforms.
 * - Does not check whether the startup entry is functional (eg. the specified path is still valid).
 */
bool isAutostartEnabled()
{
#if defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)
    QFile desktopFile(QStandardPaths::locate(QStandardPaths::ConfigLocation, QStringLiteral("autostart/" PROJECT_NAME ".desktop")));
    // check whether the file can be opeed and whether it is enabled but prevent reading large files
    if (desktopFile.open(QFile::ReadOnly) && (desktopFile.size() > (5 * 1024) || !desktopFile.readAll().contains("Hidden=true"))) {
        return true;
    }
    return false;
#elif defined(PLATFORM_WINDOWS)
    QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
    return settings.contains(QStringLiteral(PROJECT_NAME));
#else
    return false;
#endif
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
/*!
 * \brief Provides a fallback for qEnvironmentVariable() when using old Qt version.
 */
QString qEnvironmentVariable(const char *varName, const QString &defaultValue)
{
    const auto val(qgetenv(varName));
    return !val.isEmpty() ? QString::fromLocal8Bit(val) : defaultValue;
}
#endif

/*!
 * \brief Sets whether the application is launchedc on startup.
 * \remarks
 * - Only implemented under Linux/Windows. Does nothing on other platforms.
 * - If a startup entry already exists and \a enabled is true, this function will ensure the path of the existing entry is valid.
 * - If no startup entry could be detected via isAutostartEnabled() and \a enabled is false this function doesn't touch anything.
 */
bool setAutostartEnabled(bool enabled)
{
    if (!isAutostartEnabled() && !enabled) {
        return true;
    }
#if defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)
    const QString configPath(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    if (configPath.isEmpty()) {
        return !enabled;
    }
    if (enabled && !QDir().mkpath(configPath + QStringLiteral("/autostart"))) {
        return false;
    }
    QFile desktopFile(configPath + QStringLiteral("/autostart/" PROJECT_NAME ".desktop"));
    if (enabled) {
        if (!desktopFile.open(QFile::WriteOnly | QFile::Truncate)) {
            return false;
        }
        desktopFile.write("[Desktop Entry]\n"
                          "Name=" APP_NAME "\n"
                          "Exec=\"");
        desktopFile.write(qEnvironmentVariable("APPIMAGE", QCoreApplication::applicationFilePath()).toUtf8().data());
        desktopFile.write("\"\nComment=" APP_DESCRIPTION "\n"
                          "Icon=" PROJECT_NAME "\n"
                          "Type=Application\n"
                          "Terminal=false\n"
                          "X-GNOME-Autostart-Delay=0\n"
                          "X-GNOME-Autostart-enabled=true");
        return desktopFile.error() == QFile::NoError && desktopFile.flush();

    } else {
        return !desktopFile.exists() || desktopFile.remove();
    }

#elif defined(PLATFORM_WINDOWS)
    QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
    if (enabled) {
        settings.setValue(QStringLiteral(PROJECT_NAME), QCoreApplication::applicationFilePath().replace(QChar('/'), QChar('\\')));
    } else {
        settings.remove(QStringLiteral(PROJECT_NAME));
    }
    settings.sync();
    return true;
#endif
}

bool AutostartOptionPage::apply()
{
    if (!setAutostartEnabled(ui()->autostartCheckBox->isChecked())) {
        errors() << QCoreApplication::translate("QtGui::AutostartOptionPage", "unable to modify startup entry");
        return false;
    }
    return true;
}

void AutostartOptionPage::reset()
{
    if (hasBeenShown()) {
        ui()->autostartCheckBox->setChecked(isAutostartEnabled());
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

LauncherOptionPage::LauncherOptionPage(const QString &tool, QWidget *parentWidget)
    : QObject(parentWidget)
    , LauncherOptionPageBase(parentWidget)
    , m_process(&Launcher::toolProcess(tool))
    , m_launcher(nullptr)
    , m_restoreArgsButton(nullptr)
    , m_kill(false)
    , m_tool(tool)
{
}

LauncherOptionPage::~LauncherOptionPage()
{
    for (const QMetaObject::Connection &connection : m_connections) {
        QObject::disconnect(connection);
    }
}

QWidget *LauncherOptionPage::setupWidget()
{
    auto *const widget = LauncherOptionPageBase::setupWidget();

    // adjust labels to use name of additional tool instead of "Syncthing"
    if (!m_tool.isEmpty()) {
        widget->setWindowTitle(QCoreApplication::translate("QtGui::LauncherOptionPage", "%1-launcher").arg(m_tool));
        ui()->enabledCheckBox->setText(QCoreApplication::translate("QtGui::LauncherOptionPage", "Launch %1 when starting the tray icon").arg(m_tool));
        ui()->syncthingPathLabel->setText(QCoreApplication::translate("QtGui::LauncherOptionPage", "%1 executable").arg(m_tool));
        ui()->logLabel->setText(QCoreApplication::translate("QtGui::LauncherOptionPage", "%1 log (interleaved stdout/stderr)").arg(m_tool));
    }

    // hide "consider for reconnect" checkbox for tools
    ui()->considerForReconnectCheckBox->setVisible(m_tool.isEmpty());

    // add "restore to defaults" action for arguments
    if (m_tool.isEmpty()) {
        m_restoreArgsButton = new IconButton(ui()->argumentsLineEdit);
        m_restoreArgsButton->setPixmap(
            QIcon::fromTheme(QStringLiteral("edit-undo"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/edit-paste.svg"))).pixmap(16));
        m_restoreArgsButton->setToolTip(QCoreApplication::translate("QtGui::LauncherOptionPage", "Restore default"));
        QObject::connect(m_restoreArgsButton, &IconButton::clicked, this, &LauncherOptionPage::restoreDefaultArguments);
        ui()->argumentsLineEdit->insertCustomButton(0, m_restoreArgsButton);
    }

    // setup other widgets
    ui()->syncthingPathSelection->provideCustomFileMode(QFileDialog::ExistingFile);
    ui()->logTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    const auto running(isRunning());
    ui()->launchNowPushButton->setHidden(running);
    ui()->stopPushButton->setHidden(!running);
    ui()->useBuiltInVersionCheckBox->setHidden(!SyncthingLauncher::isLibSyncthingAvailable());

    // connect signals & slots
    if (m_process) {
        m_connections << connect(m_process, &SyncthingProcess::readyRead, this, &LauncherOptionPage::handleSyncthingReadyRead, Qt::QueuedConnection);
        m_connections << connect(m_process,
            static_cast<void (SyncthingProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&SyncthingProcess::finished), this,
            &LauncherOptionPage::handleSyncthingExited, Qt::QueuedConnection);
    } else if (m_launcher) {
        m_connections << connect(
            m_launcher, &SyncthingLauncher::outputAvailable, this, &LauncherOptionPage::handleSyncthingOutputAvailable, Qt::QueuedConnection);
        m_connections << connect(m_launcher, &SyncthingLauncher::exited, this, &LauncherOptionPage::handleSyncthingExited, Qt::QueuedConnection);
    }
    QObject::connect(ui()->launchNowPushButton, &QPushButton::clicked, this, &LauncherOptionPage::launch);
    QObject::connect(ui()->stopPushButton, &QPushButton::clicked, this, &LauncherOptionPage::stop);

    return widget;
}

bool LauncherOptionPage::apply()
{
    auto &settings = values().launcher;
    if (m_tool.isEmpty()) {
        settings.enabled = ui()->enabledCheckBox->isChecked();
        settings.useLibSyncthing = ui()->useBuiltInVersionCheckBox->isChecked();
        settings.syncthingPath = ui()->syncthingPathSelection->lineEdit()->text();
        settings.syncthingArgs = ui()->argumentsLineEdit->text();
        settings.considerForReconnect = ui()->considerForReconnectCheckBox->isChecked();
    } else {
        ToolParameter &params = settings.tools[m_tool];
        params.autostart = ui()->enabledCheckBox->isChecked();
        params.path = ui()->syncthingPathSelection->lineEdit()->text();
        params.args = ui()->argumentsLineEdit->text();
    }
    return true;
}

void LauncherOptionPage::reset()
{
    const auto &settings = values().launcher;
    if (m_tool.isEmpty()) {
        ui()->enabledCheckBox->setChecked(settings.enabled);
        ui()->useBuiltInVersionCheckBox->setChecked(settings.useLibSyncthing);
        ui()->useBuiltInVersionCheckBox->setVisible(settings.useLibSyncthing || SyncthingLauncher::isLibSyncthingAvailable());
        ui()->syncthingPathSelection->lineEdit()->setText(settings.syncthingPath);
        ui()->argumentsLineEdit->setText(settings.syncthingArgs);
        ui()->considerForReconnectCheckBox->setChecked(settings.considerForReconnect);
    } else {
        const ToolParameter params = settings.tools.value(m_tool);
        ui()->useBuiltInVersionCheckBox->setChecked(false);
        ui()->useBuiltInVersionCheckBox->setVisible(false);
        ui()->enabledCheckBox->setChecked(params.autostart);
        ui()->syncthingPathSelection->lineEdit()->setText(params.path);
        ui()->argumentsLineEdit->setText(params.args);
    }
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
    if (cursor.positionInBlock()) {
        cursor.insertBlock();
    }
    switch (exitStatus) {
    case QProcess::NormalExit:
        cursor.insertText(QCoreApplication::translate("QtGui::LauncherOptionPage", "%1 exited with exit code %2\n")
                              .arg(m_tool.isEmpty() ? QStringLiteral("Syncthing") : m_tool, QString::number(exitCode)));
        break;
    case QProcess::CrashExit:
        cursor.insertText(QCoreApplication::translate("QtGui::LauncherOptionPage", "%1 crashed with exit code %2\n")
                              .arg(m_tool.isEmpty() ? QStringLiteral("Syncthing") : m_tool, QString::number(exitCode)));
        break;
    }
    ui()->stopPushButton->hide();
    ui()->launchNowPushButton->show();
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
    ui()->launchNowPushButton->hide();
    ui()->stopPushButton->show();
    ui()->stopPushButton->setText(QCoreApplication::translate("QtGui::LauncherOptionPage", "Stop launched instance"));
    m_kill = false;
    const auto launcherSettings(values().launcher);
    if (m_tool.isEmpty()) {
        m_launcher->launch(launcherSettings.useLibSyncthing ? QString() : launcherSettings.syncthingPath,
            SyncthingProcess::splitArguments(launcherSettings.syncthingArgs));
    } else {
        const auto toolParams(launcherSettings.tools.value(m_tool));
        m_process->startSyncthing(toolParams.path, SyncthingProcess::splitArguments(toolParams.args));
    }
}

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
        ui()->stopPushButton->setText(QCoreApplication::translate("QtGui::LauncherOptionPage", "Kill launched instance"));
        m_kill = true;
        if (m_process) {
            m_process->stopSyncthing();
        }
        if (m_launcher) {
            m_launcher->terminate();
        }
    }
}

void LauncherOptionPage::restoreDefaultArguments()
{
    static const ::Settings::Launcher defaults;
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
}

QWidget *SystemdOptionPage::setupWidget()
{
    auto *const widget = SystemdOptionPageBase::setupWidget();
    if (!m_service) {
        return widget;
    }
    QObject::connect(ui()->syncthingUnitLineEdit, &QLineEdit::textChanged, m_service, &SyncthingService::setUnitName);
    QObject::connect(ui()->startPushButton, &QPushButton::clicked, m_service, &SyncthingService::start);
    QObject::connect(ui()->stopPushButton, &QPushButton::clicked, m_service, &SyncthingService::stop);
    QObject::connect(ui()->enablePushButton, &QPushButton::clicked, m_service, &SyncthingService::enable);
    QObject::connect(ui()->disablePushButton, &QPushButton::clicked, m_service, &SyncthingService::disable);
    QObject::connect(m_service, &SyncthingService::descriptionChanged, bind(&SystemdOptionPage::handleDescriptionChanged, this, _1));
    QObject::connect(m_service, &SyncthingService::stateChanged, bind(&SystemdOptionPage::handleStatusChanged, this, _1, _2, _3));
    QObject::connect(m_service, &SyncthingService::unitFileStateChanged, bind(&SystemdOptionPage::handleEnabledChanged, this, _1));
    return widget;
}

bool SystemdOptionPage::apply()
{
    auto &settings = values().systemd;
    settings.syncthingUnit = ui()->syncthingUnitLineEdit->text();
    settings.showButton = ui()->showButtonCheckBox->isChecked();
    settings.considerForReconnect = ui()->considerForReconnectCheckBox->isChecked();
    return true;
}

void SystemdOptionPage::reset()
{
    const auto &settings = values().systemd;
    ui()->syncthingUnitLineEdit->setText(settings.syncthingUnit);
    ui()->showButtonCheckBox->setChecked(settings.showButton);
    ui()->considerForReconnectCheckBox->setChecked(settings.considerForReconnect);
    if (!m_service) {
        return;
    }
    handleDescriptionChanged(m_service->description());
    handleStatusChanged(m_service->activeState(), m_service->subState(), m_service->activeSince());
    handleEnabledChanged(m_service->unitFileState());
}

void SystemdOptionPage::handleDescriptionChanged(const QString &description)
{
    ui()->descriptionValueLabel->setText(description.isEmpty()
            ? QCoreApplication::translate("QtGui::SystemdOptionPage", "specified unit is either inactive or doesn't exist")
            : description);
}

void setIndicatorColor(QWidget *indicator, const QColor &color)
{
    indicator->setStyleSheet(QStringLiteral("border-radius:8px;background-color:") + color.name());
}

void SystemdOptionPage::handleStatusChanged(const QString &activeState, const QString &subState, DateTime activeSince)
{
    QStringList status;
    if (!activeState.isEmpty()) {
        status << activeState;
    }
    if (!subState.isEmpty()) {
        status << subState;
    }

    const bool isRunning = m_service && m_service->isRunning();
    QString timeStamp;
    if (isRunning && !activeSince.isNull()) {
        timeStamp = QLatin1Char('\n') % QCoreApplication::translate("QtGui::SystemdOptionPage", "since ")
            % QString::fromUtf8(activeSince.toString(DateTimeOutputFormat::DateAndTime).data());
    }

    ui()->statusValueLabel->setText(
        status.isEmpty() ? QCoreApplication::translate("QtGui::SystemdOptionPage", "unknown") : status.join(QStringLiteral(" - ")) + timeStamp);
    setIndicatorColor(ui()->statusIndicator,
        status.isEmpty() ? Colors::gray(values().appearance.brightTextColors)
                         : (isRunning ? Colors::green(values().appearance.brightTextColors) : Colors::red(values().appearance.brightTextColors)));
    ui()->startPushButton->setVisible(!isRunning);
    ui()->stopPushButton->setVisible(!status.isEmpty() && isRunning);
}

void SystemdOptionPage::handleEnabledChanged(const QString &unitFileState)
{
    const bool isEnabled = m_service && m_service->isEnabled();
    ui()->unitFileStateValueLabel->setText(
        unitFileState.isEmpty() ? QCoreApplication::translate("QtGui::SystemdOptionPage", "unknown") : unitFileState);
    setIndicatorColor(
        ui()->enabledIndicator, isEnabled ? Colors::green(values().appearance.brightTextColors) : Colors::gray(values().appearance.brightTextColors));
    ui()->enablePushButton->setVisible(!isEnabled);
    ui()->disablePushButton->setVisible(!unitFileState.isEmpty() && isEnabled);
}
#endif

// WebViewOptionPage
WebViewOptionPage::WebViewOptionPage(QWidget *parentWidget)
    : WebViewOptionPageBase(parentWidget)
{
}

WebViewOptionPage::~WebViewOptionPage()
{
}

#ifdef SYNCTHINGWIDGETS_NO_WEBVIEW
QWidget *WebViewOptionPage::setupWidget()
{
    auto *label = new QLabel;
    label->setWindowTitle(QCoreApplication::translate("QtGui::WebViewOptionPage", "General"));
    label->setAlignment(Qt::AlignCenter);
    label->setText(QCoreApplication::translate("QtGui::WebViewOptionPage",
        "Syncthing Tray has not been built with vieb view support utilizing either Qt WebKit "
        "or Qt WebEngine.\nThe Web UI will be opened in the default web browser instead."));
    return label;
}
#endif

bool WebViewOptionPage::apply()
{
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    auto &webView = values().webView;
    webView.disabled = ui()->disableCheckBox->isChecked();
    webView.zoomFactor = ui()->zoomDoubleSpinBox->value();
    webView.keepRunning = ui()->keepRunningCheckBox->isChecked();
#endif
    return true;
}

void WebViewOptionPage::reset()
{
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
    const auto &webView = values().webView;
    ui()->disableCheckBox->setChecked(webView.disabled);
    ui()->zoomDoubleSpinBox->setValue(webView.zoomFactor);
    ui()->keepRunningCheckBox->setChecked(webView.keepRunning);
#endif
}

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

SettingsDialog::SettingsDialog(Data::SyncthingConnection *connection, QWidget *parent)
    : QtUtilities::SettingsDialog(parent)
{
    // setup categories
    QList<OptionCategory *> categories;
    OptionCategory *category;

    category = new OptionCategory(this);
    category->setDisplayName(tr("Tray"));
    category->assignPages(QList<OptionPage *>() << new ConnectionOptionPage(connection) << new NotificationsOptionPage << new AppearanceOptionPage
                                                << new IconsOptionPage);
    category->setIcon(QIcon(QStringLiteral(":/icons/hicolor/scalable/app/syncthingtray.svg")));
    categories << category;

    category = new OptionCategory(this);
    category->setDisplayName(tr("Web view"));
    category->assignPages(QList<OptionPage *>() << new WebViewOptionPage);
    category->setIcon(
        QIcon::fromTheme(QStringLiteral("internet-web-browser"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/internet-web-browser.svg"))));
    categories << category;

    category = new OptionCategory(this);
    category->setDisplayName(tr("Startup"));
    category->assignPages(
        QList<OptionPage *>() << new AutostartOptionPage << new LauncherOptionPage << new LauncherOptionPage(QStringLiteral("Inotify"))
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
                              << new SystemdOptionPage
#endif
    );
    category->setIcon(QIcon::fromTheme(QStringLiteral("system-run"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/system-run.svg"))));
    categories << category;

    categories << values().qt.category();
    categoryModel()->setCategories(categories);
    init();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::init()
{
    resize(860, 620);
    setWindowTitle(tr("Settings") + QStringLiteral(" - " APP_NAME));
    setWindowIcon(
        QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/preferences-other.svg"))));

    // some settings could be applied without restarting the application, good idea?
    //connect(this, &Dialogs::SettingsDialog::applied, bind(&Dialogs::QtSettings::apply, &Settings::qtSettings()));
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
#ifndef SYNCTHINGWIDGETS_NO_WEBVIEW
INSTANTIATE_UI_FILE_BASED_OPTION_PAGE_NS(QtGui, WebViewOptionPage)
#endif
