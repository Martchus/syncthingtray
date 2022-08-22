#include "./wizard.h"
#include "./settings.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <syncthingconnector/syncthingconfig.h>
#include <syncthingconnector/syncthingconnection.h>
#include <syncthingconnector/syncthingconnectionsettings.h>

#include <QCommandLinkButton>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QTimer>
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
    , m_service(nullptr)
#endif
    , m_configOk(false)
{
    setTitle(tr("Checking current Syncthing setup"));
    setSubTitle(tr("Initializing …"));

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
    emit completeChanged();
    QTimer::singleShot(0, this, &DetectionWizardPage::tryToConnect);
}

void DetectionWizardPage::cleanupPage()
{
    m_progressBar->setVisible(false);
    if (m_connection) {
        m_connection->abortAllRequests();
    }
}

void DetectionWizardPage::tryToConnect()
{
    setSubTitle(tr("Checking whether Syncthing is already running …"));
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
    m_connection = new Data::SyncthingConnection(
        m_config.syncthingUrl(), m_config.guiApiKey.toUtf8(), Data::SyncthingConnectionLoggingFlags::FromEnvironment, this);
    connect(m_connection, &Data::SyncthingConnection::error, this, &DetectionWizardPage::handleConnectionError);
    connect(m_connection, &Data::SyncthingConnection::statusChanged, this, &DetectionWizardPage::handleConnectionStatusChanged);
    m_connection->connect();
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

void DetectionWizardPage::showSummary()
{
    auto info = QStringList();
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
    if (m_connection->isConnected()) {
        info << tr("Could connect to Syncthing under: ") + m_connection->syncthingUrl();
    }
    if (!m_connectionErrors.isEmpty()) {
        info << tr("Connection errors:");
        info << m_connectionErrors;
    }
    emit completeChanged();
    setSubTitle(tr("[Some summary should go here]. Select how to proceed."));
    m_logLabel->setText(info.join(QChar('\n')));
    m_progressBar->setVisible(false);
}

} // namespace QtGui
