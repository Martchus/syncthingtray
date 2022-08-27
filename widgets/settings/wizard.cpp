#include "./wizard.h"
#include "./setupdetection.h"

#include "../misc/statusinfo.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include "ui_mainconfigwizardpage.h"

#include <qtutilities/misc/dialogutils.h>

#include <QCheckBox>
#include <QCommandLinkButton>
#include <QDesktopServices>
#include <QDialog>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

#include <initializer_list>
#include <string_view>

namespace QtGui {

Wizard *Wizard::s_instance = nullptr;

Wizard::Wizard(QWidget *parent, Qt::WindowFlags flags)
    : QWizard(parent, flags)
{
    setWindowTitle(tr("Setup wizard - ") + QStringLiteral(APP_NAME));
    setMinimumSize(770, 550);

    auto *const detectionPage = new DetectionWizardPage(this);
    auto *const mainConfigPage = new MainConfigWizardPage(this);
    connect(mainConfigPage, &MainConfigWizardPage::retry, detectionPage, &DetectionWizardPage::refresh);
    addPage(new WelcomeWizardPage(this));
    addPage(detectionPage);
    addPage(mainConfigPage);

    connect(this, &QWizard::customButtonClicked, this, &Wizard::showDetailsFromSetupDetection);
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
        statusInfo.updateConnectionStatus(detection.connection);
        infoItems << tr("Could connect to Syncthing under: ") + detection.connection.syncthingUrl();
        infoItems << tr("Syncthing version: ") + detection.connection.syncthingVersion();
        infoItems << tr("Syncthing device ID: ") + detection.connection.myId();
        infoItems << tr("Syncthing status: ") + statusInfo.statusText();
        if (!statusInfo.additionalStatusText().isEmpty()) {
            infoItems << tr("Additional Syncthing status info: ") + statusInfo.additionalStatusText();
        }
    } else {
        infoItems << tr("Coult NOT connect to Syncthing under: ") + detection.connection.syncthingUrl();
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

    // show info in dialog
    auto dlg = QDialog(this);
    dlg.setWindowFlags(Qt::Tool);
    dlg.setWindowTitle(tr("Details from setup detection - ") + QStringLiteral(APP_NAME));
    dlg.resize(500, 400);
    QtUtilities::centerWidgetAvoidingOverflow(&dlg);
    auto layout = QBoxLayout(QBoxLayout::Up);
    layout.setContentsMargins(0, 0, 0, 0);
    auto textEdit = QTextEdit(this);
    textEdit.setHtml(info);
    layout.addWidget(&textEdit);
    dlg.setLayout(&layout);
    dlg.exec();
}

WelcomeWizardPage::WelcomeWizardPage(QWidget *parent)
    : QWizardPage(parent)
{
    auto *const infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);
    const auto &settings = Settings::values();
    if (settings.firstLaunch || settings.fakeFirstLaunch) {
        setTitle(tr("Welcome to Syncthing Tray"));
        setSubTitle(tr("It looks like you're launching Syncthing Tray for the first time."));
        infoLabel->setText(tr("You must configure how to connect to Syncthing and how to launch Syncthing (if that's wanted) when using Syncthing "
                              "Tray the first time.  A "
                              "guided/automated setup is still in the works so the manual setup is currently the only option."));
    } else {
        setTitle(tr("Wizard's start page"));
        setSubTitle(tr("This wizard will help you configuring Syncthing Tray."));
    }

    QCommandLinkButton *startWizardCommand = nullptr;
    if (settings.enableWipFeatures) {
        startWizardCommand = new QCommandLinkButton(this);
        startWizardCommand->setText(tr("Start guided setup"));
        startWizardCommand->setDescription(
            tr("Allows to configure Syncthing Tray automatically for the local Syncthing instance and helps you starting Syncthing if wanted."));
        startWizardCommand->setIcon(
            QIcon::fromTheme(QStringLiteral("tools-wizard"), QIcon(QStringLiteral(":/icons/hicolor/scalable/actions/tools-wizard.svg"))));
        connect(startWizardCommand, &QCommandLinkButton::clicked, this, [this] { this->wizard()->next(); });
    }

    auto *const showSettingsCommand = new QCommandLinkButton(this);
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
    showDocsCommand->setIcon(
        QIcon::fromTheme(QStringLiteral("help-contents"), QIcon(QStringLiteral(":/icons/hicolor/scalable/apps/system-help.svg"))));
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
    , m_ui(new Ui::MainConfigWizardPage)
    , m_configSelected(false)
{
    setTitle(tr("Select what configuration to apply"));
    setSubTitle(tr("Something when wrong when checking the Syncthing setup."));
    setButtonText(QWizard::CustomButton1, tr("Show details from setup detection"));
    m_ui->setupUi(this);

    // connect signals & slots
    for (auto *const option : std::initializer_list<QRadioButton *>{ m_ui->cfgCurrentlyRunningRadioButton, m_ui->cfgLauncherExternalRadioButton,
             m_ui->cfgLauncherBuiltInlRadioButton, m_ui->cfgSystemdUserUnitlRadioButton, m_ui->cfgSystemdSystemUnitlRadioButton }) {
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
             m_ui->cfgLauncherBuiltInlRadioButton, m_ui->cfgSystemdUserUnitlRadioButton, m_ui->cfgSystemdSystemUnitlRadioButton,
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
    if (detection.connection.isConnected()) {
        // do not propose any options to launch Syncthing if it is already running
        setSubTitle(tr("Looks like Syncthing is already running and Syncthing Tray can be configured accordingly automatically."));
        m_ui->cfgCurrentlyRunningRadioButton->show();
        m_ui->cfgCurrentlyRunningRadioButton->setChecked(true);
    } else {
        // propose options to launch Syncthing if it is not running
        auto launchOptions = QStringList();
        launchOptions.reserve(2);

        // enable options to launch Syncthing via Systemd if Systemd units have been found
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
        const auto canLaunchViaUserUnit = detection.userService.canEnableOrStart();
        const auto canLaunchViaSystemUnit = detection.systemService.canEnableOrStart();
        if (canLaunchViaUserUnit) {
            m_ui->cfgSystemdUserUnitlRadioButton->show();
        }
        if (canLaunchViaSystemUnit) {
            m_ui->cfgSystemdSystemUnitlRadioButton->show();
        }
        if (canLaunchViaUserUnit) {
            m_ui->cfgSystemdUserUnitlRadioButton->setChecked(true);
        } else if (canLaunchViaSystemUnit) {
            m_ui->cfgSystemdSystemUnitlRadioButton->setChecked(true);
        }
        if (canLaunchViaUserUnit || canLaunchViaSystemUnit) {
            launchOptions << tr("Systemd");
        }
#endif

        // enable options to launch Syncthing via built-in launcher if Syncthing executable found or libsyncthing available
        const auto successfulTestLaunch = detection.launcherExitCode.has_value() && detection.launcherExitStatus.value() == QProcess::NormalExit;
        if (successfulTestLaunch || Data::SyncthingLauncher::isLibSyncthingAvailable()) {
            launchOptions << tr("Syncthing Tray");
            if (successfulTestLaunch) {
                m_ui->cfgLauncherExternalRadioButton->show();
            }
            if (Data::SyncthingLauncher::isLibSyncthingAvailable()) {
                m_ui->cfgLauncherBuiltInlRadioButton->show();
            }
            if (successfulTestLaunch) {
                m_ui->cfgLauncherExternalRadioButton->setChecked(true);
            } else {
                m_ui->cfgLauncherBuiltInlRadioButton->setChecked(true);
            }
        }

        if (!launchOptions.isEmpty()) {
            setSubTitle(tr("Looks like Syncthing is not running yet. You can launch it via %1.").arg(launchOptions.join(tr(" and "))));
        } else {
            setSubTitle(tr("Looks like Syncthing is not running yet and needs to be installed before Syncthing Tray can be configured."));
        }
    }

    handleSelectionChanged();
}

void MainConfigWizardPage::cleanupPage()
{
    wizard()->setOption(QWizard::HaveCustomButton1, false);
    emit retry();
}

void MainConfigWizardPage::handleSelectionChanged()
{
    // enable/disable option for Systemd integration
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    if (m_ui->cfgCurrentlyRunningRadioButton->isVisible()) {
        const auto &systemdSettings = Settings::values().systemd;
        m_ui->enableSystemdIntegrationCheckBox->setEnabled(systemdSettings.showButton || systemdSettings.considerForReconnect);
    } else {
        if ((m_ui->cfgSystemdUserUnitlRadioButton->isVisible() && m_ui->cfgSystemdUserUnitlRadioButton->isChecked())
            || ((m_ui->cfgSystemdSystemUnitlRadioButton->isVisible() && m_ui->cfgSystemdSystemUnitlRadioButton->isChecked()))) {
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
             m_ui->cfgLauncherBuiltInlRadioButton, m_ui->cfgSystemdUserUnitlRadioButton, m_ui->cfgSystemdSystemUnitlRadioButton }) {
        if ((configSelected = option->isChecked())) {
            break;
        }
    }
    configSelected = configSelected || (m_ui->enableSystemdIntegrationCheckBox->isEnabled() && m_ui->enableSystemdIntegrationCheckBox->isChecked());
    if (configSelected != m_configSelected) {
        m_configSelected = configSelected;
        emit completeChanged();
    }
}

} // namespace QtGui
