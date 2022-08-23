#include "./wizard.h"
#include "./setupdetection.h"

#include "../misc/statusinfo.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <QCheckBox>
#include <QCommandLinkButton>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>

#include <string_view>

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
    auto *const detectionPage = new DetectionWizardPage(this);
    auto *const mainConfigPage = new MainConfigWizardPage(this);
    connect(mainConfigPage, &MainConfigWizardPage::retry, detectionPage, &DetectionWizardPage::refresh);
    addPage(detectionPage);
    addPage(mainConfigPage);
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

SetupDetection &Wizard::setupDetection()
{
    if (!m_setupDetection) {
        m_setupDetection = std::make_unique<SetupDetection>();
    }
    return *m_setupDetection;
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
            tr("Allows to configure Syncthing Tray automatically for the local Syncthing instance and helps you starting Syncthing if wanted."));
        startWizardCommand->setIcon(QIcon::fromTheme(QStringLiteral("quickwizard")));
        connect(startWizardCommand, &QCommandLinkButton::clicked, this, [this] { this->wizard()->next(); });
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
{
    setTitle(tr("Checking current Syncthing setup"));
    setSubTitle(tr("Checking Syncthing configuration and whether Syncthing is already running or can be started …"));

    auto *const progressBar = new QProgressBar(this);
    progressBar->setMinimum(0);
    progressBar->setMaximum(0);

    auto *const layout = new QVBoxLayout;
    layout->addWidget(progressBar);
    setLayout(layout);
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
    m_setupDetection = &wizard->setupDetection();
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
    initializePage();
}

void DetectionWizardPage::tryToConnect()
{
    // determine path of Syncthing's config file, possibly ask user to select it
    m_setupDetection->determinePaths();
    if (m_setupDetection->configFilePath.isEmpty()) {
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
            m_setupDetection->configFilePath = QFileDialog::getOpenFileName(
                wizard(), tr("Select Syncthing's configuration file"), QString(), QStringLiteral("XML files (*.xml);All files (*.*)"));
        }
    }

    // start setup detection tests
    connect(m_setupDetection, &SetupDetection::done, this, &DetectionWizardPage::continueIfDone);
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
    , m_detailsLabel(new QLabel(this))
{
    setTitle(tr("Select what configuration to apply"));
    setSubTitle(tr("Something when wrong when checking the Syncthing setup."));

    auto *const showDetailsCheckBox = new QCheckBox(this);
    showDetailsCheckBox->setText(tr("Show report from detecting the local Syncthing setup"));
    m_detailsLabel->setVisible(false);
    m_detailsLabel->setWordWrap(true);
    connect(showDetailsCheckBox, &QCheckBox::toggled, m_detailsLabel, &QLabel::setVisible);

    auto *const layout = new QVBoxLayout;
    layout->addWidget(showDetailsCheckBox);
    layout->addWidget(m_detailsLabel);
    setLayout(layout);
}

bool MainConfigWizardPage::isComplete() const
{
    return false;
}

void MainConfigWizardPage::initializePage()
{
    auto *const wizard = qobject_cast<Wizard *>(this->wizard());
    if (!wizard) {
        return;
    }

    // add config info
    auto const &detection = wizard->setupDetection();
    auto info = QStringList();
    if (detection.configFilePath.isEmpty()) {
        info << tr("Unable to locate Syncthing config file.");
    } else {
        info << tr("Located Syncthing config file: ") + detection.configFilePath;
        if (isComplete()) {
            info << tr("Syncthing config file looks ok.");
        } else {
            info << tr("Syncthing config file looks invalid/incomplete.");
        }
    }

    // add connection info
    if (detection.connection.isConnected()) {
        auto statusInfo = StatusInfo();
        statusInfo.updateConnectionStatus(detection.connection);
        statusInfo.updateConnectionStatus(detection.connection);
        info << tr("Could connect to Syncthing under: ") + detection.connection.syncthingUrl();
        info << tr("Syncthing version: ") + detection.connection.syncthingVersion();
        info << tr("Syncthing device ID: ") + detection.connection.myId();
        info << tr("Syncthing status: ") + statusInfo.statusText();
        if (!statusInfo.additionalStatusText().isEmpty()) {
            info << tr("Additional Syncthing status info: ") + statusInfo.additionalStatusText();
        }
    }
    if (!detection.connectionErrors.isEmpty()) {
        info << tr("Connection errors:");
        info << detection.connectionErrors;
    }

    // add systemd service info
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    info << tr("State of systemd user unit file \"%1\": ").arg(detection.userService.unitName()) + detection.userService.unitFileState();
    info << tr("State of systemd system unit file \"%1\": ").arg(detection.systemService.unitName()) + detection.systemService.unitFileState();
#endif

    // add launcher info
    const auto successfulTestLaunch = detection.launcherExitCode.has_value() && detection.launcherExitStatus.value() == QProcess::NormalExit;
    if (successfulTestLaunch) {
        info << tr("Could test-launch Syncthing successfully, exit code: ") + QString::number(detection.launcherExitCode.value());
        info << tr("Syncthing version returned from test-launch: ") + QString::fromLocal8Bit(detection.launcherOutput.trimmed());
    } else {
        info << tr("Unable to test-launch Syncthing: ") + detection.launcher.errorString();
    }
    info << tr("Built-in Syncthing available: ") + (Data::SyncthingLauncher::isLibSyncthingAvailable() ? tr("yes") : tr("no"));

    // add details info
    m_detailsLabel->setText(QStringLiteral("<ul><li>") % info.join(QStringLiteral("</li><li>")) % QStringLiteral("</ul>"));

    // add short summary
    if (detection.connection.isConnected()) {
        setSubTitle(tr("Looks like Syncthing is already running and Syncthing Tray can be configured accordingly automatically."));
    } else if (successfulTestLaunch || Data::SyncthingLauncher::isLibSyncthingAvailable()) {
        setSubTitle(tr("Looks like Syncthing is not running yet. You can launch it via Syncthing Tray."));
    } else {
        setSubTitle(tr("Looks like Syncthing is not running yet and needs to be installed before Syncthing Tray can be configured."));
    }
}

void MainConfigWizardPage::cleanupPage()
{
    m_detailsLabel->clear();
    emit retry();
}

} // namespace QtGui
