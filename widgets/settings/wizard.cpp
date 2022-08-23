#include "./wizard.h"
#include "./settings.h"

#include "../misc/statusinfo.h"
#include "../misc/syncthinglauncher.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <syncthingconnector/syncthingconfig.h>
#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingconnectionsettings.h>
#include <syncthingconnector/syncthingservice.h>

#include <QCommandLinkButton>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStringBuilder>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <initializer_list>
#include <string_view>

#if defined(LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD) && (defined(PLATFORM_UNIX) || defined(PLATFORM_MINGW) || defined(PLATFORM_CYGWIN))
#define PLATFORM_HAS_GETLOGIN
#include <unistd.h>
#endif

namespace QtGui {

Wizard *Wizard::s_instance = nullptr;

Wizard::Wizard(QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags)
{
    setWindowTitle(tr("Setup wizard - ") + QStringLiteral(APP_NAME));

    const auto &settings = Settings::values();
    if (settings.firstLaunch || settings.fakeFirstLaunch) {
        addPage(new WelcomeWizardPage(this));
    }
    addPage(m_detectionPage = new DetectionWizardPage(this));
}

Wizard::~Wizard()
{
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

WelcomeWizardPage::WelcomeWizardPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Welcome to Syncthing Tray"));
    setSubTitle(tr("It looks like you're launching Syncthing Tray for the first time."));

    auto *const infoLabel = new QLabel(this);
    infoLabel->setText(tr(
        "You must configure how to connect to Syncthing and how to launch Syncthing (if that's wanted) when using Syncthing Tray the first time.  A "
        "guided/automated setup is still in the works so the manual setup is currently the only option."));
    infoLabel->setWordWrap(true);

    QCommandLinkButton *startWizardCommand = nullptr;
    if (Settings::values().enableWipFeatures) {
        startWizardCommand = new QCommandLinkButton(this);
        startWizardCommand->setText(tr("Start guided setup"));
        startWizardCommand->setDescription(
            tr("Allows to configure Syncthing Tray automatically for the local Syncthing instance, helps you starting Syncthing if wanted"));
        startWizardCommand->setIcon(QIcon::fromTheme(QStringLiteral("quickwizard")));
        connect(startWizardCommand, &QCommandLinkButton::clicked, this, [this] {
            if (auto *const wizard = qobject_cast<Wizard *>(this->wizard())) {
                wizard->next();
            }
        });
    }

    auto *const showSettingsCommand = new QCommandLinkButton(this);
    showSettingsCommand->setText(tr("Configure connection and launcher settings manually"));
    showSettingsCommand->setDescription(
        tr("Note that the connection settings allow importing URL, credentials and API-key from the local Syncthing configuration."));
    showSettingsCommand->setIcon(QIcon::fromTheme(QStringLiteral("preferences-other")));
    connect(showSettingsCommand, &QCommandLinkButton::clicked, this, [this] {
        if (auto *const wizard = qobject_cast<Wizard *>(this->wizard())) {
            emit wizard->settingsRequested();
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
    showDocsCommand->setIcon(QIcon::fromTheme(QStringLiteral("help-contents")));
    connect(showDocsCommand, &QCommandLinkButton::clicked, this, [] { QDesktopServices::openUrl(QStringLiteral("https://docs.syncthing.net/")); });

    auto *const showReadmeCommand = new QCommandLinkButton(this);
    showReadmeCommand->setText(tr("Show Syncthing Tray's README"));
    showReadmeCommand->setDescription(tr("It contains documentation about this GUI integration specifically."));
    showReadmeCommand->setIcon(showDocsCommand->icon());
    connect(showReadmeCommand, &QCommandLinkButton::clicked, this, [] {
        if constexpr (std::string_view(APP_VERSION).find('-') == std::string_view::npos) {
            QDesktopServices::openUrl(QStringLiteral(APP_URL "/blob/v" APP_VERSION "/README.md"));
        } else {
            QDesktopServices::openUrl(QStringLiteral(APP_URL "/blob/master/README.md"));
        }
    });

    auto *const layout = new QVBoxLayout;
    layout->addWidget(infoLabel);
    if (startWizardCommand) {
        layout->addWidget(startWizardCommand);
    }
    layout->addWidget(showSettingsCommand);
    layout->addStretch();
    layout->addWidget(line);
    layout->addWidget(showDocsCommand);
    layout->addWidget(showReadmeCommand);
    setLayout(layout);
}

bool WelcomeWizardPage::isComplete() const
{
    return false;
}

DetectionWizardPage::DetectionWizardPage(QWidget *parent)
    : QWizardPage(parent)
    , m_connection(nullptr)
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    , m_userService(nullptr)
    , m_systemService(nullptr)
#endif
    , m_launcher(nullptr)
    , m_timedOut(false)
    , m_configOk(false)
{
    setTitle(tr("Checking current Syncthing setup"));
    setSubTitle(tr("Initializing …"));

    m_timeoutTimer.setInterval(1000);
    m_timeoutTimer.setSingleShot(true);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(0);
    m_progressBar->setVisible(false);
    m_logLabel = new QLabel(this);

    auto *const layout = new QVBoxLayout;
    layout->addWidget(m_progressBar);
    layout->addWidget(m_logLabel);
    setLayout(layout);
}

bool DetectionWizardPage::isComplete() const
{
    return m_configOk && !m_config.guiAddress.isEmpty() && !m_config.guiApiKey.isEmpty();
}

void DetectionWizardPage::initializePage()
{
    m_progressBar->setVisible(true);
    m_configOk = false;
    m_connectionErrors.clear();
    m_launcherExitCode.reset();
    m_launcherExitStatus.reset();
    m_launcherError.reset();
    m_launcherOutput.clear();

    emit completeChanged();
    QTimer::singleShot(0, this, &DetectionWizardPage::tryToConnect);
}

void DetectionWizardPage::cleanupPage()
{
    m_progressBar->setVisible(false);
    if (m_connection) {
        m_connection->abortAllRequests();
    }
    if (m_launcher && m_launcher->isRunning()) {
        m_launcher->terminate();
    }
}

void DetectionWizardPage::tryToConnect()
{
    setSubTitle(tr("Checking whether Syncthing is already running …"));

    // cleanup old instances possibly still present from previous check
    for (auto *instance : std::initializer_list<QObject *>{ m_connection, m_userService, m_systemService, m_launcher }) {
        if (instance) {
            instance->deleteLater();
        }
    }

    // read Syncthing's config file
    m_configFilePath = Data::SyncthingConfig::locateConfigFile();
    m_certPath = Data::SyncthingConfig::locateHttpsCertificate();
    if (m_configFilePath.isEmpty()) {
        auto msgbox = QMessageBox(wizard());
        auto yesButton = QPushButton(tr("Yes, continue configuration"));
        auto noButton = QPushButton(tr("No, let me select Syncthing's configuration file manually"));
        msgbox.setIcon(QMessageBox::Question);
        msgbox.setText(
            tr("It looks like Syncthing has not been running on this system before as its configuration cannot be found. Is that correct?"));
        msgbox.addButton(&yesButton, QMessageBox::YesRole);
        msgbox.addButton(&noButton, QMessageBox::NoRole);
        msgbox.exec();
        if (msgbox.clickedButton() == &noButton) {
            m_configFilePath = QFileDialog::getOpenFileName(
                wizard(), tr("Select Syncthing's configuration file"), QString(), QStringLiteral("XML files (*.xml);All files (*.*)"));
        }
    }
    m_configOk = m_config.restore(m_configFilePath);

    // attempt connecting to Syncthing
    m_connection = new Data::SyncthingConnection(
        m_config.syncthingUrl(), m_config.guiApiKey.toUtf8(), Data::SyncthingConnectionLoggingFlags::FromEnvironment, this);
    connect(m_connection, &Data::SyncthingConnection::error, this, &DetectionWizardPage::handleConnectionError);
    connect(m_connection, &Data::SyncthingConnection::statusChanged, this, &DetectionWizardPage::handleConnectionStatusChanged);
    m_connection->connect();

    // checkout availability of systemd services
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    m_userService = new Data::SyncthingService(Data::SystemdScope::User, this);
    m_userService->setUnitName(QStringLiteral("syncthing.service"));
    m_systemService = new Data::SyncthingService(Data::SystemdScope::System, this);
    m_systemService->setUnitName(QStringLiteral("syncthing@") %
#ifdef PLATFORM_HAS_GETLOGIN
        QString::fromLocal8Bit(getlogin()) %
#endif
        QStringLiteral(".service"));
    connect(m_userService, &Data::SyncthingService::unitFileStateChanged, this, &DetectionWizardPage::continueWithSummaryIfDone);
    connect(m_systemService, &Data::SyncthingService::unitFileStateChanged, this, &DetectionWizardPage::continueWithSummaryIfDone);
#endif

    // check whether we could launch Syncthing as external binary
    auto launcherSettings = Settings::Launcher();
    launcherSettings.syncthingArgs = QStringLiteral("--version");
    m_launcher = new Data::SyncthingLauncher(this);
    m_launcher->setEmittingOutput(true);
    connect(m_launcher, &Data::SyncthingLauncher::outputAvailable, this, &DetectionWizardPage::handleLauncherOutput);
    connect(m_launcher, &Data::SyncthingLauncher::exited, this, &DetectionWizardPage::handleLauncherExit);
    connect(m_launcher, &Data::SyncthingLauncher::errorOccurred, this, &DetectionWizardPage::handleLauncherError);
    m_launcher->launch(launcherSettings);

    // setup a timeout
    m_timeoutTimer.stop();
    m_timedOut = false;
    m_timeoutTimer.start();
}

void DetectionWizardPage::handleConnectionStatusChanged()
{
    if (m_connection->isConnecting()) {
        return;
    }
    showSummary();
}

void DetectionWizardPage::handleConnectionError(const QString &error)
{
    m_connectionErrors << QStringLiteral(" - ") + error;
}

void DetectionWizardPage::handleLauncherExit(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_launcherExitCode = exitCode;
    m_launcherExitStatus = exitStatus;
    continueWithSummaryIfDone();
}

void DetectionWizardPage::handleLauncherError(QProcess::ProcessError error)
{
    m_launcherError = error;
    continueWithSummaryIfDone();
}

void DetectionWizardPage::handleLauncherOutput(const QByteArray &output)
{
    m_launcherOutput.append(output);
}

void DetectionWizardPage::handleTimeout()
{
    m_timedOut = true;
    continueWithSummaryIfDone();
}

void DetectionWizardPage::continueWithSummaryIfDone()
{
    if (m_timedOut
        || (!m_connection->isConnecting() && (m_launcherExitCode.has_value() || m_launcherError.has_value())
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
            && !m_userService->unitFileState().isEmpty() && !m_systemService->unitFileState().isEmpty()
#endif
                )) {
        showSummary();
    }
}

void DetectionWizardPage::showSummary()
{
    auto info = QStringList();

    // add config info
    if (m_configFilePath.isEmpty()) {
        info << tr("Unable to locate Syncthing config file.");
    } else {
        info << tr("Located Syncthing config file: ") + m_configFilePath;
        if (isComplete()) {
            info << tr("Syncthing config file looks ok.");
        } else {
            info << tr("Syncthing config file looks invalid/incomplete.");
        }
    }

    // add connection info
    if (m_connection->isConnected()) {
        auto statusInfo = StatusInfo();
        statusInfo.updateConnectionStatus(*m_connection);
        statusInfo.updateConnectionStatus(*m_connection);
        info << tr("Could connect to Syncthing under: ") + m_connection->syncthingUrl();
        info << tr("Syncthing's version: ") + m_connection->syncthingVersion();
        info << tr("Syncthing's device ID: ") + m_connection->myId();
        info << tr("Syncthing's status: ") + statusInfo.statusText();
        if (!statusInfo.additionalStatusText().isEmpty()) {
            info << tr("Additional Syncthing status info: ") + statusInfo.additionalStatusText();
        }
    }
    if (!m_connectionErrors.isEmpty()) {
        info << tr("Connection errors:");
        info << m_connectionErrors;
    }

    // add systemd service info
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    info << tr("State of systemd user service \"%1\": ").arg(m_userService->unitName()) + m_userService->unitFileState();
    info << tr("State of systemd system service \"%1\": ").arg(m_systemService->unitName()) + m_systemService->unitFileState();
#endif

    // add launcher info
    if (m_launcherExitCode.has_value() && m_launcherExitStatus.value() == QProcess::NormalExit) {
        info << tr("Could test-launch Syncthing successfully, exit code: ") + QString::number(m_launcherExitCode.value());
        info << tr("Syncthing version returned from test-launch: ") + QString::fromLocal8Bit(m_launcherOutput.trimmed());
    } else {
        info << tr("Unable to test-launch Syncthing: ") + m_launcher->errorString();
    }
    info << tr("Built-in Syncthing available: ") + (Data::SyncthingLauncher::isLibSyncthingAvailable() ? tr("yes") : tr("no"));

    // update UI
    emit completeChanged();
    setSubTitle(tr("[Some summary should go here]. Select how to proceed."));
    m_logLabel->setText(info.join(QChar('\n')));
    m_progressBar->setVisible(false);
}

} // namespace QtGui
