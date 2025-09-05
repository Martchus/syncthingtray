#include "./wizard.h"
#include "./settingsdialog.h"
#include "./setupdetection.h"

#include "../misc/otherdialogs.h"
#include "../misc/statusinfo.h"
#include "../misc/syncthinglauncher.h"
#include "../misc/utils.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include "ui_applywizardpage.h"
#include "ui_autostartwizardpage.h"
#include "ui_mainconfigwizardpage.h"

#include <qtutilities/misc/dialogutils.h>

#include <QApplication>
#include <QCheckBox>
#include <QCommandLinkButton>
#include <QDesktopServices>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QStyle>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

#include <initializer_list>

#if !defined(SYNCTHINGWIDGETS_AUTOSTART_DISABLED)                                                                                                    \
    && ((defined(PLATFORM_LINUX) && !defined(Q_OS_ANDROID)) || defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC))
#define SETTINGS_WIZARD_AUTOSTART
#endif

namespace QtGui {

constexpr int syncthingPollTimeout = 60 * 1000;
constexpr int syncthingPollInterval = 2000;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
constexpr int systemdPollTimeout = 10 * 1000;
#endif

Wizard *Wizard::s_instance = nullptr;

Wizard::Wizard(QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags)
{
#ifdef Q_OS_WINDOWS
    // avoid using QWizard::AeroStyle when not also the Qt Widgets style "windowsvista" is used
    // note: Otherwise the wizard is quite unusable with a dark or otherwise customized color palette.
    //       The Windows 11 style also supports dark/custom palettes so also then the fallback makes
    //       sense.
    auto fallbackToModernStyle = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
    if (const auto *const style = QApplication::style()) {
        if (style->name().compare(QLatin1String("windowsvista"), Qt::CaseInsensitive) != 0) {
            setWizardStyle(QWizard::ModernStyle);
            fallbackToModernStyle = true;
        }
    }
#endif
#endif

    setWindowTitle(tr("Setup wizard - ") + QStringLiteral(APP_NAME));

    auto icon = QIcon(QString::fromUtf8(":/icons/hicolor/scalable/app/syncthingtray.svg"));
#ifdef Q_OS_WINDOWS
    if (!fallbackToModernStyle) {
        // workaround displaying issues with bigger icons when QWizard::AeroStyle is used
        icon = icon.pixmap(16);
    }
#endif
    setWindowIcon(icon);

    setMinimumSize(770, 550);
    resize(1024, 720);

    auto *const welcomePage = new WelcomeWizardPage(this);
    auto *const detectionPage = new DetectionWizardPage(this);
    auto *const mainConfigPage = new MainConfigWizardPage(this);
    auto *const applyPage = new ApplyWizardPage(this);
    auto *const finalPage = new FinalWizardPage(this);
    connect(mainConfigPage, &MainConfigWizardPage::retry, detectionPage, &DetectionWizardPage::showCheckAgainButton);
    connect(mainConfigPage, &MainConfigWizardPage::configurationSelected, this, &Wizard::handleConfigurationSelected);
    connect(this, &Wizard::configApplied, finalPage, &FinalWizardPage::completeChanged);
    connect(this, &Wizard::configApplied, finalPage, &FinalWizardPage::showResults);
    addPage(welcomePage);
    addPage(detectionPage);
    addPage(mainConfigPage);
#ifdef SETTINGS_WIZARD_AUTOSTART
    if (!Settings::values().isPlasmoid) {
        auto *const autostartPage = new AutostartWizardPage(this);
        connect(autostartPage, &AutostartWizardPage::autostartSelected, this, &Wizard::handleAutostartSelected);
        addPage(autostartPage);
    }
#endif
    addPage(applyPage);
    addPage(finalPage);

    connect(this, &QWizard::customButtonClicked, this, &Wizard::showDetailsFromSetupDetection);
}

Wizard::~Wizard()
{
    auto &settings = Settings::values();
    settings.firstLaunch = settings.fakeFirstLaunch = false;
    if (this == s_instance) {
        s_instance = nullptr;
    }
}

Wizard *Wizard::instance()
{
    if (!s_instance) {
        s_instance = new Wizard();
        s_instance->setAttribute(Qt::WA_DeleteOnClose, true);
    }
    return s_instance;
}

SetupDetection &Wizard::setupDetection()
{
    if (!m_setupDetection) {
        m_setupDetection = std::make_unique<SetupDetection>();
    }
    return *m_setupDetection;
}

bool Wizard::changeSettings()
{
    const auto &detection = setupDetection();
    auto &settings = Settings::values();

    // clear state/error from possible previous attempt
    m_configApplied = false;
    m_configError.clear();

    // set settings for configured "main config"
    switch (mainConfig()) {
    case MainConfiguration::None:
        break;
    case MainConfiguration::CurrentlyRunning:
        // apply changes to current primary config if necessary
        settings.connection.addConfigFromWizard(detection.config);
        break;
    case MainConfiguration::LauncherExternal:
        // configure to use external Syncthing executable
        settings.launcher.useLibSyncthing = false;
        settings.launcher.syncthingPath = detection.launcherSettings.syncthingPath;
        // restore args to defaults that are known to work (maybe warn about this if user had custom args configured?)
        settings.launcher.syncthingArgs = detection.defaultSyncthingArgs;
        break;
    case MainConfiguration::LauncherBuiltIn:
        // configure to use built-in Syncthing executable
        settings.launcher.useLibSyncthing = true;
#ifdef SYNCTHINGWIDGETS_USE_LIBSYNCTHING
        settings.launcher.libSyncthing.configDir = QFileInfo(detection.configFilePath).canonicalPath();
#endif
        break;
    case MainConfiguration::SystemdUserUnit:
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        // configure systemd integration for user unit
        settings.systemd.syncthingUnit = detection.userService.unitName();
        settings.systemd.systemUnit = false;
#endif
        break;
    case MainConfiguration::SystemdSystemUnit:
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        // configure systemd integration for system unit
        settings.systemd.syncthingUnit = detection.systemService.unitName();
        settings.systemd.systemUnit = true;
#endif
        break;
    }

    // enable/disable integrations accordingly
    if (mainConfig() != MainConfiguration::CurrentlyRunning) {
        settings.launcher.considerForReconnect = settings.launcher.showButton = settings.launcher.autostartEnabled
            = (mainConfig() == MainConfiguration::LauncherExternal || mainConfig() == MainConfiguration::LauncherBuiltIn);
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    settings.systemd.considerForReconnect = settings.systemd.showButton = extraConfig() && ExtraConfiguration::SystemdIntegration;
#endif

    // enable/disable auto start
#ifdef SETTINGS_WIZARD_AUTOSTART
    if (!settings.isPlasmoid && !keepExistingAutoStartEntry()) {
        setAutostartEnabled(autoStart(), true);
    }
#endif

    // invoke next step
    switch (mainConfig()) {
    case MainConfiguration::None:
        // just conclude the wizard immediately
        handleConfigurationApplied();
        return true;
    case MainConfiguration::CurrentlyRunning:
        // let the tray widget / plasmoid apply the settings immediately
        // note: The tray widget / plasmoid is expected to call handleConfigurationApplied().
        emit settingsChanged();
        return true;
    case MainConfiguration::LauncherExternal:
    case MainConfiguration::LauncherBuiltIn:
        // launch Syncthing via launcher (before trying to connect to it)
        if (auto *const launcher = Data::SyncthingLauncher::mainInstance()) {
            launcher->launch(settings.launcher);
        } else {
            handleConfigurationApplied(tr("The internal launcher has not been initialized."));
            return true;
        }
        break;
    case MainConfiguration::SystemdUserUnit:
    case MainConfiguration::SystemdSystemUnit:
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        if (auto *const service = Data::SyncthingService::mainInstance()) {
            settings.systemd.setupService(*service);
            service->start();
            service->enable();
        } else {
            handleConfigurationApplied(tr("The service handler has not been initialized."));
            return true;
        }
#endif
        break;
    }

    // poll until Syncthing config with API key has been created
    m_elapsedPollTime = 0;
    pollForSyncthingConfig();
    return true;
}

void Wizard::showDetailsFromSetupDetection()
{
    auto const &detection = setupDetection();
    auto info = QString();
    auto addParagraph = [&info](const QString &text) {
        info.append(QStringLiteral("<p><b>"));
        info.append(text);
        info.append(QStringLiteral("</b></p>"));
    };
    auto addListRo = [&info](const QStringList &items) {
        info.append(QStringLiteral("<ul><li>"));
        info.append(items.join(QStringLiteral("</li><li>")));
        info.append(QStringLiteral("</li></ul>"));
    };
    auto addList = [&addListRo](QStringList &items) {
        addListRo(items);
        items.clear();
    };
    auto infoItems = QStringList();
    if (detection.configFilePath.isEmpty()) {
        infoItems << tr("Unable to locate Syncthing config file.");
    } else {
        infoItems << tr("Located Syncthing config file: ") + detection.configFilePath;
        if (detection.hasConfig()) {
            infoItems << tr("Syncthing config file looks ok.");
        } else {
            infoItems << tr("Syncthing config file looks invalid/incomplete.");
        }
    }
    addParagraph(tr("Syncthing configuration:"));
    addList(infoItems);

    // add connection info
    if (detection.connection.isConnected()) {
        auto statusInfo = StatusInfo();
        statusInfo.updateConnectionStatus(detection.connection);
        statusInfo.updateConnectedDevices(detection.connection);
        infoItems << tr("Could connect to Syncthing under: ") + detection.connection.syncthingUrl();
        infoItems << tr("Syncthing version: ") + detection.connection.syncthingVersion();
        infoItems << tr("Syncthing device ID: ") + detection.connection.myId();
        infoItems << tr("Syncthing status: ") + statusInfo.statusText();
        if (!statusInfo.additionalStatusText().isEmpty()) {
            infoItems << tr("Additional Syncthing status info: ") + statusInfo.additionalStatusText();
        }
    } else {
        infoItems << tr("Could NOT connect to Syncthing under: ") + detection.connection.syncthingUrl();
    }
    addParagraph(tr("API connection:"));
    addList(infoItems);
    if (!detection.connectionErrors.isEmpty()) {
        addParagraph(tr("API connection errors:"));
        addListRo(detection.connectionErrors);
    }

    // add systemd service info
    addParagraph(tr("Systemd:"));
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    infoItems << tr("State of user unit file \"%1\": ").arg(detection.userService.unitName()) + detection.userService.unitFileState();
    infoItems << tr("State of system unit file \"%1\": ").arg(detection.systemService.unitName()) + detection.systemService.unitFileState();
    addList(infoItems);
#else
    addListRo(QStringList(tr("No available")));
#endif

    // add launcher info
    const auto successfulTestLaunch = detection.launcherExitCode.has_value() && detection.launcherExitStatus.value() == QProcess::NormalExit;
    if (successfulTestLaunch) {
        infoItems << tr("Could test-launch Syncthing successfully, exit code: ") + QString::number(detection.launcherExitCode.value());
        infoItems << tr("Syncthing version returned from test-launch: ") + QString::fromLocal8Bit(detection.launcherOutput.trimmed());
    } else {
        infoItems << tr("Unable to test-launch Syncthing: ") + detection.launcher.errorString();
    }
    infoItems << tr("Built-in Syncthing available: ") + (Data::SyncthingLauncher::isLibSyncthingAvailable() ? tr("yes") : tr("no"));
    addParagraph(tr("Launcher:"));
    addList(infoItems);

#ifdef SETTINGS_WIZARD_AUTOSTART
    addParagraph(tr("Autostart:"));
    infoItems << tr("Currently %1").arg(detection.autostartEnabled ? tr("enabled") : tr("disabled"));
    if (detection.autostartEnabled && detection.autostartConfiguredPath.has_value()) {
        infoItems << tr("Points to \"%1\"").arg(detection.autostartConfiguredPath.value());
    }
    addList(infoItems);
#endif

    // show info in dialog
    auto dlg = QDialog(this);
    dlg.setWindowFlags(Qt::Tool);
    dlg.setWindowTitle(tr("Details from setup detection - ") + QStringLiteral(APP_NAME));
    dlg.resize(500, 400);
    QtUtilities::centerWidgetAvoidingOverflow(&dlg);
    auto *const layout = new QBoxLayout(QBoxLayout::Up, &dlg);
    layout->setContentsMargins(0, 0, 0, 0);
    auto textEdit = QTextEdit(this);
    textEdit.setHtml(info);
    layout->addWidget(&textEdit);
    dlg.exec();
}

void Wizard::handleConfigurationSelected(MainConfiguration mainConfig, ExtraConfiguration extraConfig)
{
    m_configApplied = false;
    m_mainConfig = mainConfig;
    m_extraConfig = extraConfig;
}

void Wizard::handleAutostartSelected(bool autostartEnabled, bool keepExisting)
{
    m_configApplied = false;
    m_autoStart = autostartEnabled;
    m_autoStartKeepExisting = keepExisting;
}

void Wizard::pollForSyncthingConfig()
{
    // check if Syncthing is still running (if not that's an error and we can stop polling)
    switch (mainConfig()) {
    case MainConfiguration::LauncherExternal:
    case MainConfiguration::LauncherBuiltIn:
        // launch Syncthing via launcher (before trying to connect to it)
        if (auto *const launcher = Data::SyncthingLauncher::mainInstance()) {
            if (!launcher->isRunning()) {
                handleConfigurationApplied(tr("The Syncthing process exited prematurely. ") + hintAboutSyncthingLog());
                return;
            }
        }
        break;
    case MainConfiguration::SystemdUserUnit:
    case MainConfiguration::SystemdSystemUnit:
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        if (m_elapsedPollTime < systemdPollTimeout) {
            break; // don't consider the systemd unit status immediately; also give it a short time to adjust
        }
        if (auto *const service = Data::SyncthingService::mainInstance()) {
            if (!service->isRunning()) {
                handleConfigurationApplied(tr("The Syncthing service stopped prematurely. ") + hintAboutSyncthingLog());
                return;
            }
        }
#endif
        break;
    default:;
    }

    // check for config and if present let the tray widget / plasmoid apply the settings
    // note: The tray widget / plasmoid is expected to call handleConfigurationApplied().
    auto &detection = setupDetection();
    detection.determinePaths();
    if (!detection.configFilePath.isEmpty()) {
        detection.restoreConfig();
        if (detection.hasConfig()) {
            Settings::values().connection.addConfigFromWizard(detection.config);
            emit settingsChanged();
            return;
        }
    }

    // keep polling
    if (m_elapsedPollTime > syncthingPollTimeout) {
        handleConfigurationApplied(tr("Ran into timeout while waiting for Syncthing to create config file. "
                                      "Maybe Syncthing created its config file under an unexpected location. ")
            + hintAboutSyncthingLog());
        return;
    }
    m_elapsedPollTime += syncthingPollInterval;
    QTimer::singleShot(syncthingPollInterval, Qt::VeryCoarseTimer, this, &Wizard::pollForSyncthingConfig);
}

QString Wizard::hintAboutSyncthingLog() const
{
    auto res = tr("Checkout Syncthing's log for details.");
    switch (mainConfig()) {
    case MainConfiguration::LauncherExternal:
    case MainConfiguration::LauncherBuiltIn:
        res += tr(" It can be accessed within the <a href=\"openLauncherSettings\">launcher settings</a>.");
        break;
    case MainConfiguration::SystemdUserUnit:
    case MainConfiguration::SystemdSystemUnit:
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        res += tr(" It is normally written to the system journal (and can be accessed via e.g. journalctl).");
#endif
        break;
    default:;
    }
    return res;
}

void Wizard::handleConfigurationApplied(const QString &configError, Data::SyncthingConnection *connection)
{
    m_configApplied = true;
    if (m_configError.isEmpty()) {
        m_configError = configError;
    }
    if ((m_appliedConnection = connection)) {
        connect(connection, &Data::SyncthingConnection::destroyed, this, [this] { m_appliedConnection = nullptr; });
    }
    emit configApplied();
}

WelcomeWizardPage::WelcomeWizardPage(QWidget *parent)
    : QWizardPage(parent)
{
    const auto readmeUrl = QtGui::readmeUrl();
    auto *const infoLabel = new QLabel(this);
    auto infoText = QString();
    const auto &settings = Settings::values();
#if !defined(Q_OS_WINDOWS) && !defined(Q_OS_DARWIN) && !defined(Q_OS_ANDROID)
    if (!settings.isPlasmoid && qEnvironmentVariable("XDG_CURRENT_DESKTOP").split(QChar(':')).contains(QStringLiteral("KDE"))) {
        infoText += tr("<p><strong>You have started the standalone version of Syncthing Tray. It looks like you are on KDE Plasma, though. "
                       "On KDE the preferred version of Syncthing Tray is the Plasmoid which is supposed to be used <em>instead</em> of "
                       "the standalone version.</strong> Checkout the documentation about <a href=\"%1\">configuring the Plasmoid</a> for the "
                       "best way to use Syncthing Tray on KDE. After adding Syncthing Tray as Plasmoid, you can reopen this wizard from the "
                       "Plasmoid via its settings dialog.</p>")
                        .arg(readmeUrl + QStringLiteral("#configuring-plasmoid"));
    }
#endif
    if (settings.firstLaunch || settings.fakeFirstLaunch) {
        setTitle(tr("Welcome to Syncthing Tray"));
        setSubTitle(tr("It looks like you're launching Syncthing Tray for the first time."));
        infoText += tr("<p>You must configure how to connect to Syncthing and how to launch Syncthing (if that's wanted) when using Syncthing "
                       "Tray the first time.</p>"
                       "<p>You can always reopen this wizard from the settings dialog.</p>");
    } else {
        setTitle(tr("Wizard's start page"));
        setSubTitle(tr("This wizard will help you configuring Syncthing Tray."));
    }
    infoLabel->setWordWrap(true);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setText(infoText);

    auto *const startWizardCommand = new QCommandLinkButton(this);
    startWizardCommand->setObjectName(QStringLiteral("startWizardCommand"));
    startWizardCommand->setText(tr("Start guided setup"));
    startWizardCommand->setDescription(
        tr("Allows to configure Syncthing Tray automatically for the local Syncthing instance and helps you starting Syncthing if wanted."));
    startWizardCommand->setIcon(
        QIcon::fromTheme(QStringLiteral("quickwizard"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/tools-wizard.svg"))));
    connect(startWizardCommand, &QCommandLinkButton::clicked, this, [this] { this->wizard()->next(); });

    auto *const showSettingsCommand = new QCommandLinkButton(this);
    showSettingsCommand->setObjectName(QStringLiteral("showSettingsCommand"));
    if (settings.firstLaunch) {
        showSettingsCommand->setText(tr("Configure connection and launcher settings manually"));
    } else {
        showSettingsCommand->setText(tr("Head back to settings to configure connection and launcher manually"));
    }
    showSettingsCommand->setDescription(
        tr("Note that the connection settings allow importing URL, credentials and API-key from the local Syncthing configuration."));
    showSettingsCommand->setIcon(
        QIcon::fromTheme(QStringLiteral("preferences-other"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/preferences-other.svg"))));
    connect(showSettingsCommand, &QCommandLinkButton::clicked, this, [this] {
        if (auto *const wizard = qobject_cast<Wizard *>(this->wizard())) {
            emit wizard->settingsDialogRequested();
            wizard->close();
        }
    });

    auto *const line = new QFrame(this);
    line->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    auto *const showDocsCommand = new QCommandLinkButton(this);
    showDocsCommand->setText(tr("Show Syncthing's documentation"));
    showDocsCommand->setDescription(tr("It contains general information about configuring Syncthing."));
    showDocsCommand->setIcon(
        QIcon::fromTheme(QStringLiteral("help-contents"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/system-help.svg"))));
    connect(showDocsCommand, &QCommandLinkButton::clicked, this, [] { QDesktopServices::openUrl(QStringLiteral("https://docs.syncthing.net/")); });

    auto *const showReadmeCommand = new QCommandLinkButton(this);
    showReadmeCommand->setText(tr("Show Syncthing Tray's README"));
    showReadmeCommand->setDescription(tr("It contains documentation about this GUI integration specifically."));
    showReadmeCommand->setIcon(showDocsCommand->icon());
    connect(showReadmeCommand, &QCommandLinkButton::clicked, this, [readmeUrl] { QDesktopServices::openUrl(readmeUrl); });

    auto *const layout = new QVBoxLayout(this);
    layout->addWidget(infoLabel);
    layout->addWidget(startWizardCommand);
    layout->addWidget(showSettingsCommand);
    layout->addStretch();
    layout->addWidget(line);
    layout->addWidget(showDocsCommand);
    layout->addWidget(showReadmeCommand);
}

bool WelcomeWizardPage::isComplete() const
{
    return false;
}

DetectionWizardPage::DetectionWizardPage(QWidget *parent)
    : QWizardPage(parent)
    , m_setupDetection(nullptr)
{
    setTitle(m_defaultTitle = tr("Checking current Syncthing setup"));
    setSubTitle(m_defaultSubTitle = tr("Checking Syncthing configuration and whether Syncthing is already running or can be started …"));

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(0);
    m_checkAgainButton = new QPushButton(this);
    m_checkAgainButton->setText(tr("Check again"));
    m_checkAgainButton->hide();
    m_checkAgainButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_checkAgainButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    connect(m_checkAgainButton, &QPushButton::clicked, this, &DetectionWizardPage::refresh);

    auto *const layout = new QVBoxLayout(this);
    auto *const buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_checkAgainButton);
    buttonLayout->addStretch();
    layout->addWidget(m_progressBar);
    layout->addLayout(buttonLayout);
}

bool DetectionWizardPage::isComplete() const
{
    return m_setupDetection && m_setupDetection->isDone();
}

void DetectionWizardPage::initializePage()
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    if (!wizard) {
        return;
    }
    if (!m_setupDetection) {
        m_setupDetection = &wizard->setupDetection();
    }

    m_setupDetection->reset();
    emit completeChanged();
    QTimer::singleShot(0, this, &DetectionWizardPage::tryToConnect);
}

void DetectionWizardPage::cleanupPage()
{
    if (m_setupDetection) {
        m_setupDetection->disconnect(this);
        m_setupDetection->reset();
    }
}

void DetectionWizardPage::refresh()
{
    if (m_setupDetection && !m_setupDetection->isDone()) {
        return;
    }
    initializePage();
}

void DetectionWizardPage::showCheckAgainButton()
{
    setTitle(tr("Re-visit setup detection"));
    setSubTitle(tr("You might trigger checking the Syncthing setup again"));
    m_progressBar->hide();
    m_checkAgainButton->show();
}

void DetectionWizardPage::tryToConnect()
{
    // skip if the wizard has been closed
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    if (!wizard || wizard->isHidden()) {
        return;
    }

    setTitle(m_defaultTitle);
    setSubTitle(m_defaultSubTitle);
    m_progressBar->show();
    m_checkAgainButton->hide();

    // determine path of Syncthing's config file, possibly ask user to select it
    m_setupDetection->determinePaths();
    if (m_setupDetection->configFilePath.isEmpty()) {
        auto msgbox = QMessageBox(wizard);
        auto yesButton = QPushButton(tr("Yes, continue configuration"));
        auto noButton = QPushButton(tr("No, let me select Syncthing's configuration file manually"));
        msgbox.setIcon(QMessageBox::Question);
        msgbox.setText(
            tr("It looks like Syncthing has not been running on this system before as its configuration cannot be found. Is that correct?"));
        msgbox.addButton(&yesButton, QMessageBox::YesRole);
        msgbox.addButton(&noButton, QMessageBox::NoRole);
        msgbox.exec();
        if (msgbox.clickedButton() == &noButton) {
            m_setupDetection->configFilePath = QFileDialog::getOpenFileName(
                wizard, tr("Select Syncthing's configuration file"), QString(), QStringLiteral("XML files (*.xml);All files (*.*)"));
        }
    }

    // start setup detection tests
    connect(m_setupDetection, &SetupDetection::done, this, &DetectionWizardPage::continueIfDone, Qt::QueuedConnection);
    m_setupDetection->startTest();
}

void DetectionWizardPage::continueIfDone()
{
    m_setupDetection->disconnect(this);
    emit completeChanged();
    wizard()->next();
}

MainConfigWizardPage::MainConfigWizardPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MainConfigWizardPage)
    , m_configSelected(false)
{
    setTitle(tr("Select what configuration to apply"));
    setSubTitle(tr("Something when wrong when checking the Syncthing setup."));
    setButtonText(QWizard::CustomButton1, tr("Show details from setup detection"));
    m_ui->setupUi(this);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    m_cfgSystemdUserUnitText = m_ui->cfgSystemdUserUnitRadioButton->text();
    m_cfgSystemdSystemUnitText = m_ui->cfgSystemdSystemUnitRadioButton->text();
#endif

    // connect signals & slots
    for (auto *const option : std::initializer_list<QRadioButton *>{ m_ui->cfgCurrentlyRunningRadioButton, m_ui->cfgLauncherExternalRadioButton,
             m_ui->cfgLauncherBuiltInRadioButton, m_ui->cfgSystemdUserUnitRadioButton, m_ui->cfgSystemdSystemUnitRadioButton,
             m_ui->cfgNoneRadioButton }) {
        connect(option, &QRadioButton::toggled, this, &MainConfigWizardPage::handleSelectionChanged);
    }
}

MainConfigWizardPage::~MainConfigWizardPage()
{
}

bool MainConfigWizardPage::isComplete() const
{
    return m_configSelected;
}

void MainConfigWizardPage::initializePage()
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    if (!wizard) {
        return;
    }

    // enable button to show details from setup detection
    wizard->setOption(QWizard::HaveCustomButton1, true);

    // hide all configuration options as a starting point
    for (auto *const option : std::initializer_list<QWidget *>{ m_ui->cfgCurrentlyRunningRadioButton, m_ui->cfgLauncherExternalRadioButton,
             m_ui->cfgLauncherBuiltInRadioButton, m_ui->cfgSystemdUserUnitRadioButton, m_ui->cfgSystemdSystemUnitRadioButton,
             m_ui->enableSystemdIntegrationCheckBox }) {
        option->hide();
    }

    // offer enabling Systemd integration if at least one unit is available
    auto const &detection = wizard->setupDetection();
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    if (detection.userService.isUnitAvailable() || detection.systemService.isUnitAvailable()) {
        m_ui->enableSystemdIntegrationCheckBox->show();
        m_ui->enableSystemdIntegrationCheckBox->setChecked(true);
    }
#endif

    // add short summary as sub title and offer configurations that make sense
    auto warnings = QStringList();
    if (detection.connection.isConnected()) {
        // do not propose any options to launch Syncthing if it is already running
        setSubTitle(tr("Looks like Syncthing is already running and Syncthing Tray can be configured accordingly automatically."));
        m_ui->cfgCurrentlyRunningRadioButton->show();
        m_ui->cfgCurrentlyRunningRadioButton->setChecked(true);
        m_ui->invalidConfigLabel->hide();
    } else {
        // propose options to launch Syncthing if it is not running
        auto launchOptions = QStringList();
        launchOptions.reserve(2);

        // enable options to launch Syncthing via Systemd if Systemd units have been found
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        const auto canLaunchViaUserUnit = detection.userService.canEnableOrStart();
        const auto canLaunchViaSystemUnit = detection.systemService.canEnableOrStart();
        const auto userUnitRunning = detection.userService.isRunning();
        const auto systemUnitRunning = detection.systemService.isRunning();
        if (canLaunchViaUserUnit) {
            m_ui->cfgSystemdUserUnitRadioButton->show();
            m_ui->cfgSystemdUserUnitRadioButton->setText(m_cfgSystemdUserUnitText.arg(detection.userService.unitName()));
        }
        if (canLaunchViaSystemUnit) {
            m_ui->cfgSystemdSystemUnitRadioButton->show();
            m_ui->cfgSystemdSystemUnitRadioButton->setText(m_cfgSystemdSystemUnitText.arg(detection.systemService.unitName()));
        }
        if (canLaunchViaUserUnit && !systemUnitRunning) {
            m_ui->cfgSystemdUserUnitRadioButton->setChecked(true);
        } else if (canLaunchViaSystemUnit && !userUnitRunning) {
            m_ui->cfgSystemdSystemUnitRadioButton->setChecked(true);
        }
        if (!detection.connection.isConnected()) {
            auto runningUnits = QStringList();
            runningUnits.reserve(2);
            if (userUnitRunning) {
                runningUnits << detection.userService.displayName();
            }
            if (systemUnitRunning) {
                runningUnits << detection.systemService.displayName();
            }
            if (!runningUnits.isEmpty()) {
                warnings << QStringLiteral("<li>")
                        % tr("Syncthing has been started via the systemd %1 but the automatic setup detection was not able to connect via the "
                             "REST-API. If you want Syncthing Tray to connect to this instance of Syncthing you will have to configure this "
                             "manually. Otherwise you should probably stop this unit before proceeding with launching Syncthing in a different way.")
                              .arg(runningUnits.join(tr(" and ")))
                        % QStringLiteral("</li>");
            }
        }
        if (canLaunchViaUserUnit || canLaunchViaSystemUnit) {
            launchOptions << tr("Systemd");
        }
#endif

        // enable options to launch Syncthing via built-in launcher if Syncthing executable found or libsyncthing available
        const auto successfulTestLaunch = detection.launcherExitCode.has_value() && detection.launcherExitStatus.value() == QProcess::NormalExit;
        if (!Settings::values().isPlasmoid && (successfulTestLaunch || Data::SyncthingLauncher::isLibSyncthingAvailable())) {
            launchOptions << tr("Syncthing Tray's launcher");
            if (successfulTestLaunch) {
                m_ui->cfgLauncherExternalRadioButton->show();
            }
            if (Data::SyncthingLauncher::isLibSyncthingAvailable()) {
                m_ui->cfgLauncherBuiltInRadioButton->show();
            }
            if (launchOptions.isEmpty()) {
                if (successfulTestLaunch) {
                    m_ui->cfgLauncherExternalRadioButton->setChecked(true);
                } else {
                    m_ui->cfgLauncherBuiltInRadioButton->setChecked(true);
                }
            }
        }

        if (!launchOptions.isEmpty()) {
            setSubTitle(tr("Looks like Syncthing is not running yet. You can launch it via %1.").arg(launchOptions.join(tr(" and "))));
        } else {
            setSubTitle(tr("Looks like Syncthing is not running yet and needs to be installed before Syncthing Tray can be configured."));
        }

        if (detection.configFilePath.isEmpty() || detection.configOk) {
            m_ui->invalidConfigLabel->hide();
        } else {
            m_ui->invalidConfigLabel->setText(tr("<b>The Syncthing config could be located under \"%1\" but it seems invalid/incomplete.</b> Hence "
                                                 "Syncthing is assumed to be not running.")
                    .arg(detection.configFilePath));
            m_ui->invalidConfigLabel->show();
        }
    }

#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    if (detection.userService.isRunningOrEnabled() && detection.systemService.isRunningOrEnabled()) {
        warnings << QStringLiteral("<li>")
                % tr("The systemd %1 and %2 have both be detected as enabled/running. This is probably a misconfiguration.")
                      .arg(detection.userService.displayName(), detection.systemService.displayName())
                % QStringLiteral("</li>");
    }
#endif

    if (warnings.isEmpty()) {
        m_ui->warningLabel->hide();
    } else {
        m_ui->warningLabel->setText(QStringLiteral("<p><strong>") % tr("Warnings:") % QStringLiteral("</strong></p><ul>") % warnings.join(QString())
            % QStringLiteral("</ul>"));
        m_ui->warningLabel->show();
    }

    handleSelectionChanged();
}

void MainConfigWizardPage::cleanupPage()
{
    wizard()->setOption(QWizard::HaveCustomButton1, false);
    emit retry();
}

bool MainConfigWizardPage::validatePage()
{
    auto mainConfig = MainConfiguration::None;
    if (m_ui->cfgCurrentlyRunningRadioButton->isChecked()) {
        mainConfig = MainConfiguration::CurrentlyRunning;
    } else if (m_ui->cfgLauncherExternalRadioButton->isChecked()) {
        mainConfig = MainConfiguration::LauncherExternal;
    } else if (m_ui->cfgLauncherBuiltInRadioButton->isChecked()) {
        mainConfig = MainConfiguration::LauncherBuiltIn;
    } else if (m_ui->cfgSystemdUserUnitRadioButton->isChecked()) {
        mainConfig = MainConfiguration::SystemdUserUnit;
    } else if (m_ui->cfgSystemdSystemUnitRadioButton->isChecked()) {
        mainConfig = MainConfiguration::SystemdSystemUnit;
    }

    auto extraConfig = ExtraConfiguration::None;
    CppUtilities::modFlagEnum(extraConfig, ExtraConfiguration::SystemdIntegration, m_ui->enableSystemdIntegrationCheckBox->isChecked());

    emit configurationSelected(mainConfig, extraConfig);
    return true;
}

void MainConfigWizardPage::handleSelectionChanged()
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    if (!wizard) {
        return;
    }

    // enable/disable option for Systemd integration
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    const auto &detection = wizard->setupDetection();
    if (!m_ui->cfgCurrentlyRunningRadioButton->isHidden()) {
        const auto &systemdSettings = Settings::values().systemd;
        m_ui->enableSystemdIntegrationCheckBox->setChecked(systemdSettings.showButton || systemdSettings.considerForReconnect
            || detection.userService.isRunning() || detection.systemService.isRunning());
    } else {
        if ((!m_ui->cfgSystemdUserUnitRadioButton->isHidden() && m_ui->cfgSystemdUserUnitRadioButton->isChecked())
            || ((!m_ui->cfgSystemdSystemUnitRadioButton->isHidden() && m_ui->cfgSystemdSystemUnitRadioButton->isChecked()))) {
            m_ui->enableSystemdIntegrationCheckBox->setChecked(true);
            m_ui->enableSystemdIntegrationCheckBox->setEnabled(true);
        } else {
            m_ui->enableSystemdIntegrationCheckBox->setChecked(false);
            m_ui->enableSystemdIntegrationCheckBox->setEnabled(true);
        }
    }
#endif

    // set completed state according to selection
    auto configSelected = false;
    for (auto *const option : std::initializer_list<QRadioButton *>{ m_ui->cfgCurrentlyRunningRadioButton, m_ui->cfgLauncherExternalRadioButton,
             m_ui->cfgLauncherBuiltInRadioButton, m_ui->cfgSystemdUserUnitRadioButton, m_ui->cfgSystemdSystemUnitRadioButton,
             m_ui->cfgNoneRadioButton }) {
        if ((configSelected = !option->isHidden() && option->isChecked())) {
            break;
        }
    }
    configSelected = configSelected || (m_ui->enableSystemdIntegrationCheckBox->isEnabled() && m_ui->enableSystemdIntegrationCheckBox->isChecked());
    if (configSelected != m_configSelected) {
        m_configSelected = configSelected;
        emit completeChanged();
    }
}

AutostartWizardPage::AutostartWizardPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::AutostartWizardPage)
    , m_configSelected(false)
{
    setTitle(tr("Configure autostart"));
    setSubTitle(tr("Select whether to start Syncthing Tray automatically"));
    m_ui->setupUi(this);
}

AutostartWizardPage::~AutostartWizardPage()
{
}

bool AutostartWizardPage::isComplete() const
{
    return true;
}

void AutostartWizardPage::initializePage()
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    if (!wizard) {
        return;
    }

    for (auto *const widget : std::initializer_list<QWidget *>{ m_ui->launcherEnabledLabel, m_ui->systemdEnabledLabel, m_ui->launcherDisabledLabel,
             m_ui->launcherAlreadyEnabledLabel, m_ui->generalLabel }) {
        widget->hide();
    }

    switch (wizard->mainConfig()) {
    case MainConfiguration::None:
        m_ui->generalLabel->show();
        break;
    case MainConfiguration::CurrentlyRunning: {
        auto *const launcher = Data::SyncthingLauncher::mainInstance();
        if (launcher && launcher->guiUrl().port(-1) == QUrl(wizard->setupDetection().config.syncthingUrl()).port(-2)) {
            m_ui->launcherAlreadyEnabledLabel->show();
        } else {
            m_ui->launcherDisabledLabel->show();
        }
        break;
    }
    case MainConfiguration::LauncherExternal:
    case MainConfiguration::LauncherBuiltIn:
        m_ui->launcherEnabledLabel->show();
        break;
    case MainConfiguration::SystemdUserUnit:
    case MainConfiguration::SystemdSystemUnit:
        m_ui->systemdEnabledLabel->show();
        break;
    }

    const auto &detection = wizard->setupDetection();
    const auto hasExistingEntry = detection.autostartConfiguredPath.has_value() && !detection.autostartConfiguredPath.value().isEmpty()
        && detection.autostartConfiguredPath.value() != detection.autostartSupposedPath;
    m_ui->keepExistingCheckBox->setVisible(hasExistingEntry);
    if (hasExistingEntry) {
        m_ui->keepExistingCheckBox->setText(
            tr("Do not modify the existing autostart entry for\n\"%1\"").arg(detection.autostartConfiguredPath.value()));
    } else {
        m_ui->keepExistingCheckBox->setChecked(false);
    }
    m_ui->enableAutostartCheckBox->setChecked(detection.autostartEnabled);
}

void AutostartWizardPage::cleanupPage()
{
}

bool AutostartWizardPage::validatePage()
{
    emit autostartSelected(m_ui->enableAutostartCheckBox->isChecked(), m_ui->keepExistingCheckBox->isChecked());
    return true;
}

ApplyWizardPage::ApplyWizardPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::ApplyWizardPage)
{
    const auto applyText = tr("Apply");
    setTitle(tr("Apply selected configuration"));
    setSubTitle(tr("Review the summary of the configuration changes before applying them"));
    setButtonText(QWizard::NextButton, applyText);
    setButtonText(QWizard::CommitButton, applyText);
    setButtonText(QWizard::FinishButton, applyText);
    m_ui->setupUi(this);
}

ApplyWizardPage::~ApplyWizardPage()
{
}

bool ApplyWizardPage::isComplete() const
{
    return true;
}

void ApplyWizardPage::initializePage()
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    if (!wizard) {
        return;
    }
    const auto &detection = wizard->setupDetection();
    const auto &currentSettings = Settings::values();
    auto html = QStringLiteral("<p><b>%1</b></p><ul>").arg(tr("Summary:"));
    auto makeListItem = [](QString &output, const QString &text) {
        output.append(QStringLiteral("<li>"));
        output.append(text);
        output.append(QStringLiteral("</li>"));
    };
    auto makeNestedListItem = [](QString &output, const QString &text) {
        output.append(QStringLiteral("<ul><li>"));
        output.append(text);
        output.append(QStringLiteral("</li></ul>"));
    };
    auto addListItem = [&makeListItem, &html](const QString &text) { makeListItem(html, text); };
    auto logFeature
        = [&makeNestedListItem, &addListItem](const QString &feature, bool enabled, bool enabledBefore, const QString &remark = QString()) {
              auto text = enabled == enabledBefore ? (tr("Keep %1 %2").arg(feature, enabled ? tr("enabled") : tr("disabled")))
                                                   : (tr("%1 %2").arg(enabled ? tr("Enable") : tr("Disable"), feature));
              if (!remark.isEmpty()) {
                  makeNestedListItem(text, remark);
              }
              addListItem(text);
          };
    auto mainConfig = QString();
    auto extraInfo = QString();
    switch (wizard->mainConfig()) {
    case MainConfiguration::None:
        mainConfig = tr("Keep connection and launcher configuration as-is");
        break;
    case MainConfiguration::CurrentlyRunning:
        mainConfig = tr("Configure Syncthing Tray to use the currently running Syncthing instance");
        extraInfo = tr("Do <i>not</i> change how Syncthing is launched");
        break;
    case MainConfiguration::LauncherExternal:
    case MainConfiguration::LauncherBuiltIn:
        mainConfig = tr("Start Syncthing via Syncthing Tray's launcher");
        extraInfo = wizard->mainConfig() == MainConfiguration::LauncherExternal
            ? tr("executable from PATH as separate process, \"%1\"").arg(QString::fromLocal8Bit(detection.launcherOutput.trimmed()))
            : tr("built-in Syncthing library, \"%1\"").arg(Data::SyncthingLauncher::libSyncthingVersionInfo());
        break;
    case MainConfiguration::SystemdUserUnit:
    case MainConfiguration::SystemdSystemUnit:
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        mainConfig = tr("Start Syncthing by enabling and starting its systemd unit");
        extraInfo = wizard->mainConfig() == MainConfiguration::SystemdUserUnit
            ? tr("Using user unit \"%1\"").arg(detection.userService.unitName())
            : tr("Using system unit \"%1\"").arg(detection.systemService.unitName());
#endif
        break;
    }
    if (!extraInfo.isEmpty()) {
        makeNestedListItem(mainConfig, extraInfo);
    }
    addListItem(mainConfig);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    logFeature(tr("systemd integration"), wizard->extraConfig() && ExtraConfiguration::SystemdIntegration,
        currentSettings.systemd.showButton && currentSettings.systemd.considerForReconnect);
#endif
#ifdef SETTINGS_WIZARD_AUTOSTART
    if (!currentSettings.isPlasmoid) {
        if (wizard->keepExistingAutoStartEntry()) {
            addListItem(tr("Preserve existing autostart entry for \"%1\"").arg(detection.autostartConfiguredPath.value_or(QStringLiteral("?"))));
        } else {
            auto remark = QString();
            auto action = QString();
            if (detection.autostartConfiguredPath.has_value() && !detection.autostartConfiguredPath.value().isEmpty()
                && detection.autostartConfiguredPath.value() != detection.autostartSupposedPath) {
                action = wizard->autoStart() ? tr("Override") : tr("Delete");
                remark = tr("%1 existing autostart entry for \"%2\"").arg(action, detection.autostartConfiguredPath.value());
            }
            logFeature(tr("autostart of Syncthing Tray"), wizard->autoStart(), detection.autostartEnabled, remark);
        }
    }
#endif
    html.append(QStringLiteral("</ul><p><b>%1</b></p><ul><li>%2</li><li>%3</li><li>%4</li></ul>")
            .arg(tr("Further information:"), tr("Click on \"Show details from setup detection\" for further details."),
                tr("If you want to do amendments, you can head back one or more steps."), tr("If you abort now, nothing will be changed.")));
    m_ui->summaryTextBrowser->setHtml(html);
}

bool ApplyWizardPage::validatePage()
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    return wizard ? wizard->changeSettings() : false;
}

FinalWizardPage::FinalWizardPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ownDeviceIdWidget(nullptr)
{
    m_layout = new QVBoxLayout(this);
    m_layout->addWidget(m_progressBar = new QProgressBar());
    m_layout->addWidget(m_label = new QLabel());

    m_label->setObjectName(QStringLiteral("label"));
    m_label->setWordWrap(true);
    m_progressBar->setMaximum(0);
    m_progressBar->setObjectName(QStringLiteral("progressBar"));

    connect(m_label, &QLabel::linkActivated, this, &FinalWizardPage::handleLinkActivated);
}

FinalWizardPage::~FinalWizardPage()
{
}

bool FinalWizardPage::isComplete() const
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    return wizard != nullptr && wizard->isConfigApplied();
}

void FinalWizardPage::initializePage()
{
    showResults();
}

bool FinalWizardPage::validatePage()
{
    // keep config file on disk in-sync with new settings
    Settings::save();
    return true;
}

void FinalWizardPage::showResults()
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    if (!wizard) {
        return;
    }

    // delete possibly previously shown own device ID in any case
    delete m_ownDeviceIdWidget;

    if (!wizard->isConfigApplied()) {
        setTitle(tr("Waiting for configuration wizard completed"));
        setSubTitle(tr("Changes are being applied"));
        m_label->hide();
        m_progressBar->show();
        return;
    }

    setTitle(tr("Configuration wizard completed"));
    m_label->show();
    m_progressBar->hide();
    if (wizard->configError().isEmpty()) {
        setSubTitle(tr("All changes have been applied"));
        if (wizard->mainConfig() == MainConfiguration::None) {
            m_label->setText(tr("<p>The configuration has been changed successfully. The way Syncthing Tray connects to and starts Syncthing "
                                "has not changed, though. You may configure this manually in the settings.</p>"));
        } else {
            m_label->setText(tr("<p>The configuration has been changed successfully. You can close the wizard and <a href=\"openSyncthing\">open "
                                "Syncthing</a> to pair remote devices and add folders for sharing. If you need further help, read the "
                                "<a href=\"openDocs\">documentation to get started</a>.</p>"
                                "<p>To initiate the pairing from another device, the device ID of this Syncthing device is displayed below.</p>"));
        }
        if (wizard->appliedConnection()) {
            m_layout->addWidget(m_ownDeviceIdWidget = ownDeviceIdWidget(*(wizard->appliedConnection()), 256));
        }
    } else {
        setSubTitle(tr("Not all changes could be applied"));
        m_label->setText(QStringLiteral("<p>%1</p><p>%2</p>")
                .arg(wizard->configError(),
                    tr("You may try to head back one or more steps and try again or finish the wizard and configure Syncthing Tray manually.")));
    }
}

void FinalWizardPage::handleLinkActivated(const QString &href)
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    if (wizard && href == QLatin1String("openSyncthing")) {
        emit wizard->openSyncthingRequested();
    } else if (wizard && href == QLatin1String("openLauncherSettings")) {
        emit wizard->openLauncherSettingsRequested();
    } else if (href == QLatin1String("openDocs")) {
        QDesktopServices::openUrl(QStringLiteral("https://docs.syncthing.net/intro/getting-started.html#configuring"));
    }
}

} // namespace QtGui
