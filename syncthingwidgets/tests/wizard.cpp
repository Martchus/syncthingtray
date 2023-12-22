#include "../settings/wizard.h"
#include "../misc/syncthinglauncher.h"
#include "../settings/settings.h"
#include "../settings/setupdetection.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/misc/compat.h>

#include <QtTest/QtTest>

#include <QApplication>
#include <QCheckBox>
#include <QCommandLinkButton>
#include <QDebug>
#include <QEventLoop>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QProgressBar>
#include <QRadioButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextBrowser>

using namespace QtGui;

class WizardTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void testShowingSettings();
    void testConfiguringLauncher();
    void testConfiguringCurrentlyRunningSyncthing();

private:
    bool confirmMessageBox();
    void configureSyncthingArgs(SetupDetection &setupDetection) const;

    QTemporaryDir m_homeDir;
    QEventLoop m_eventLoop;
    QString m_syncthingPath;
    QString m_syncthingPort;
    QString m_syncthingGuiAddress;
    Data::SyncthingLauncher m_launcher;
    Data::SyncthingConnection m_connection;
    QByteArray m_syncthingLog;
};

/*!
 * \brief Ensure prestine environment with English as language.
 * \remarks Relies on setting HOME; likely breaks on Windows.
 */
void WizardTests::initTestCase()
{
    // ensure all text is English as checks rely on it
    QLocale::setDefault(QLocale::English);

    // assume first launch
    auto &settings = Settings::values();
    settings.fakeFirstLaunch = true;

    // use an empty dir as HOME to simulate a prestine setup
    const auto homePath = m_homeDir.path();
    qDebug() << QStringLiteral("HOME dir: ") + homePath;
    qputenv("LIB_SYNCTHING_CONNECTOR_SYNCTHING_CONFIG_DIR", homePath.toLocal8Bit());
    QVERIFY(m_homeDir.isValid());

    // create a config file for Syncthing Tray in the working dir so it'll be picked up instead of the user's config file
    auto testConfigFile = QFile(QStringLiteral(PROJECT_NAME ".ini"));
    QVERIFY(testConfigFile.open(QFile::WriteOnly | QFile::Truncate));
    testConfigFile.close();

    // assert there's no connection setting present initially
    settings.connection.primary.label = QStringLiteral("testconfig");
    QCOMPARE(settings.connection.primary.syncthingUrl, QString());
    QCOMPARE(settings.connection.primary.apiKey, QByteArray());
    QCOMPARE(settings.connection.secondary.size(), 0);

    // read syncthing executable path from env so it must not necassarily in PATH for this test to run
    const auto syncthingPathFromEnv = qgetenv("SYNCTHING_PATH");
    m_syncthingPath = syncthingPathFromEnv.isEmpty() ? QStringLiteral("syncthing") : QString::fromLocal8Bit(syncthingPathFromEnv);

    // read syncthing port from env so it can be customized should the default port already be used otherwise
    // notes: This is passed via "--gui-address=http://127.0.0.1:$port" so the Syncthing test instance will not interfere with the actual Syncthing setup.
    //        The config file will still contain "127.0.0.1:8384" (or a random port if 8384 is already used by another application), though.
    const auto syncthingPortFromEnv = qEnvironmentVariableIntValue("SYNCTHING_PORT");
    m_syncthingPort = !syncthingPortFromEnv ? QStringLiteral("4001") : QString::number(syncthingPortFromEnv);
    m_syncthingGuiAddress = QStringLiteral("http://127.0.0.1:") + m_syncthingPort;

    // gather Syncthing's output
    m_launcher.setEmittingOutput(true);
    connect(&m_launcher, &Data::SyncthingLauncher::outputAvailable, this, [this](const QByteArray &output) { m_syncthingLog += output; });
}

void WizardTests::cleanupTestCase()
{
    auto syncthingConfig = QFile(m_homeDir.path() + QStringLiteral("/config.xml"));
    qDebug() << "Syncthing config: ";
    !syncthingConfig.open(QFile::ReadOnly) ? (qDebug() << "unable to open") : (qDebug().noquote() << syncthingConfig.readAll());

    if (m_launcher.isRunning()) {
        qDebug() << "terminating Syncthing";
        m_launcher.terminate();
    }
    qDebug().noquote() << "Syncthing log during testrun:\n" << m_syncthingLog;
}

/*!
 * \brief Cleanup the global instance (if used by the test).
 */
void WizardTests::cleanup()
{
    if (Wizard::hasInstance()) {
        delete Wizard::instance();
    }
}

/*!
 * \brief Tests showing settings dialog from wizard's welcome page.
 */
void WizardTests::testShowingSettings()
{
    // obtain global wizard instance and do some basic checks
    auto settingsDlgRequested = 0;
    auto *wizardDlg = Wizard::instance();
    auto c = connect(wizardDlg, &Wizard::settingsDialogRequested, [&] { ++settingsDlgRequested; });
    QVERIFY(!wizardDlg->setupDetection().hasConfig());

    // show wizard and request settings though welcome page
    wizardDlg->show();
    auto *welcomePage = qobject_cast<WelcomeWizardPage *>(wizardDlg->currentPage());
    QVERIFY(welcomePage != nullptr);
    auto *settingsButton = welcomePage->findChild<QCommandLinkButton *>(QStringLiteral("showSettingsCommand"));
    QVERIFY(settingsButton != nullptr);
    QCOMPARE(settingsDlgRequested, 0);
    settingsButton->click();
    m_eventLoop.processEvents();
    QCOMPARE(settingsDlgRequested, 1);

    // other tests might trigger settingsDialogRequested() so better disconnect lambda
    disconnect(c);
}

/*!
 * \brief Tests configuring the built-in launcher to launch an external Syncthing binary.
 */
void WizardTests::testConfiguringLauncher()
{
    // mock the autostart path; it is supposed to be preserved
    QVERIFY(qputenv(PROJECT_VARNAME_UPPER "_AUTOSTART_PATH_MOCK", "fake-autostart-path"));

    // pretend libsyncthing / systemd is already enabled
    // note: Should be unset as we're selecting to use an external binary.
    auto &settings = Settings::values();
    settings.launcher.useLibSyncthing = true;
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    settings.systemd.considerForReconnect = true;
    settings.systemd.showButton = true;
#endif

    // init new wizard and do some basic checks
    auto wizardDlg = Wizard();
    QVERIFY(!wizardDlg.setupDetection().hasConfig());

    // show wizard and proceed with guided setup
    wizardDlg.show();
    auto *const welcomePage = qobject_cast<WelcomeWizardPage *>(wizardDlg.currentPage());
    QVERIFY(welcomePage != nullptr);
    auto *const startWizardButton = welcomePage->findChild<QCommandLinkButton *>(QStringLiteral("startWizardCommand"));
    QVERIFY(startWizardButton != nullptr);
    startWizardButton->click();
    auto *const detectionPage = qobject_cast<DetectionWizardPage *>(wizardDlg.currentPage());
    QVERIFY(detectionPage != nullptr);

    // confirm there's no Syncthing config yet and wait for main config page to show
    auto confirmMsgBox = std::function<void(void)>();
    confirmMsgBox = [&] {
        if (qobject_cast<MainConfigWizardPage *>(wizardDlg.currentPage())) {
            m_eventLoop.quit();
        } else {
            confirmMessageBox();
            QTimer::singleShot(0, this, confirmMsgBox);
        }
    };
    QTimer::singleShot(0, this, confirmMsgBox);
    m_eventLoop.exec();

    // configure external launcher
    auto *const mainConfigPage = qobject_cast<MainConfigWizardPage *>(wizardDlg.currentPage());
    QVERIFY(mainConfigPage != nullptr);
    auto &setupDetection = wizardDlg.setupDetection();
    const auto &configuredAutostartPath = setupDetection.autostartConfiguredPath;
    QVERIFY(configuredAutostartPath.has_value());
    QCOMPARE(configuredAutostartPath.value(), QStringLiteral("fake-autostart-path"));
    QVERIFY(!setupDetection.hasConfig());
    // -> print debug output in certain launcher error cases to get the full picture if any of the subsequent checks fail
    if (setupDetection.launcherError.has_value()) {
        qDebug() << "a launcher error occurred: " << setupDetection.launcherError.value();
    }
    if (!setupDetection.launcherError.has_value() || setupDetection.launcherExitCode.value_or(-1) != 0) {
        qDebug() << "launcher output: " << setupDetection.launcherOutput;
    }
    if (setupDetection.launcherExitCode.has_value()) {
        qDebug() << "launcher exit code: " << setupDetection.launcherExitCode.value();
    }
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    qDebug() << "user service state: " << setupDetection.userService.unitFileState();
    qDebug() << "system service state: " << setupDetection.systemService.unitFileState();
#endif
    if (setupDetection.timedOut) {
        qDebug() << "timeout of " << setupDetection.timeout.interval() << " ms has been exceeded (normal if systemd units not available)";
    }
    // -> verify whether the launcher setup detection is in the expected state before checking UI itself
    QVERIFY(setupDetection.launcherExitCode.has_value());
    QCOMPARE(setupDetection.launcherExitCode.value(), 0);
    QVERIFY(!setupDetection.launcherError.has_value());
    // -> check UI, select the external launcher and continue
    auto *const cfgCurrentlyRunningRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgCurrentlyRunningRadioButton"));
    QVERIFY(cfgCurrentlyRunningRadioButton != nullptr);
    QVERIFY(cfgCurrentlyRunningRadioButton->isHidden());
    auto *const cfgLauncherExternalRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgLauncherExternalRadioButton"));
    QVERIFY(cfgLauncherExternalRadioButton != nullptr);
    QVERIFY(!cfgLauncherExternalRadioButton->isHidden());
    cfgLauncherExternalRadioButton->setChecked(true);
    wizardDlg.next();

    // keep autostart setting as-is
    auto *const autostartPage = qobject_cast<AutostartWizardPage *>(wizardDlg.currentPage());
    QVERIFY(autostartPage != nullptr);
    auto *const keepExistingCheckBox = autostartPage->findChild<QCheckBox *>(QStringLiteral("keepExistingCheckBox"));
    QVERIFY(keepExistingCheckBox != nullptr);
    QVERIFY(keepExistingCheckBox->isVisible());
    keepExistingCheckBox->setChecked(true);
    wizardDlg.next();

    // apply settings
    auto *const applyPage = qobject_cast<ApplyWizardPage *>(wizardDlg.currentPage());
    QVERIFY(applyPage != nullptr);
    auto *const summaryTextBrowser = applyPage->findChild<QTextBrowser *>(QStringLiteral("summaryTextBrowser"));
    QVERIFY(summaryTextBrowser != nullptr);
    const auto summary = summaryTextBrowser->toPlainText();
    QVERIFY(summary.contains(QStringLiteral("Start Syncthing via Syncthing Tray's launcher")));
    QVERIFY(summary.contains(QStringLiteral("executable from PATH as separate process")));
    QVERIFY(summary.contains(QStringLiteral("Keep autostart disabled")) || summary.contains(QStringLiteral("Preserve existing autostart entry")));
    wizardDlg.next();

    // check results
    auto *finalPage = qobject_cast<FinalWizardPage *>(wizardDlg.currentPage());
    QVERIFY(finalPage != nullptr);
    auto *progressBar = finalPage->findChild<QProgressBar *>(QStringLiteral("progressBar"));
    auto *label = finalPage->findChild<QLabel *>(QStringLiteral("label"));
    QVERIFY(progressBar != nullptr);
    QVERIFY(label != nullptr);

    // wait until configuration has been applied
    auto waitForCfg = std::function<void(void)>();
    waitForCfg = [&] {
        if (!label->isHidden() && progressBar->isHidden()) {
            m_eventLoop.quit();
        } else {
            QTimer::singleShot(0, this, waitForCfg);
        }
    };
    QTimer::singleShot(0, this, waitForCfg);
    m_eventLoop.exec();
    QVERIFY(label->text().contains(QStringLiteral("The internal launcher has not been initialized.")));
    QVERIFY(label->text().contains(QStringLiteral("You may try to head back")));
    configureSyncthingArgs(wizardDlg.setupDetection());

    // try again with launcher being initialized
    wizardDlg.back();
    m_eventLoop.processEvents();
    Data::SyncthingLauncher::setMainInstance(&m_launcher);
    wizardDlg.next();

    // apply changes and wait for results on final wizard page
    qDebug() << "waiting for Syncthing to write config file";
    finalPage = nullptr;
    progressBar = nullptr;
    label = nullptr;
    waitForCfg = [&] {
        if (!finalPage && (finalPage = qobject_cast<FinalWizardPage *>(wizardDlg.currentPage()))) {
            progressBar = finalPage->findChild<QProgressBar *>(QStringLiteral("progressBar"));
            label = finalPage->findChild<QLabel *>(QStringLiteral("label"));
            QVERIFY(progressBar != nullptr);
            QVERIFY(label != nullptr);
        }
        if (label && progressBar && !label->isHidden() && progressBar->isHidden()) {
            m_eventLoop.quit();
        } else {
            QTimer::singleShot(0, this, waitForCfg);
        }
    };
    // -> assign handler to connect to Syncthing with applied settings
    connect(&wizardDlg, &Wizard::settingsChanged, this, [&] {
        m_connection.applySettings(settings.connection.primary);
        m_connection.setSyncthingUrl(QStringLiteral("http://127.0.0.1:") + m_syncthingPort);
        m_connection.reconnect();
        qDebug() << "configured URL: " << m_connection.syncthingUrl();
        qDebug() << "configured API key: " << m_connection.apiKey();
        QTimer::singleShot(0, this, waitForCfg);
    });
    // -> assign handler to re-try connecting to Syncthing or handle a successful connection
    connect(&m_connection, &Data::SyncthingConnection::statusChanged, &wizardDlg,
        [this, &wizardDlg, connectAttempts = 10](Data::SyncthingStatus status) mutable {
            switch (status) {
            case Data::SyncthingStatus::Disconnected:
                // try to connect up to 10 times while Syncthing is still running; otherwise consider test failed
                if (--connectAttempts > 0 && m_launcher.isRunning()) {
                    qDebug() << "attempting to connect to Syncthing again ( remaining attempts:" << connectAttempts << ')';
                    m_connection.reconnectLater(1000);
                } else {
                    qWarning() << "giving up connecting to Syncthing";
                    qDebug() << (m_launcher.isRunning() ? "Syncthing is still running" : "Syncthing is not running (anymore)");
                    wizardDlg.handleConfigurationApplied(QStringLiteral("Unable to connect to Syncthing"), &m_connection);
                }
                return;
            case Data::SyncthingStatus::Reconnecting:
                return;
            case Data::SyncthingStatus::Idle:
            case Data::SyncthingStatus::Scanning:
            case Data::SyncthingStatus::Synchronizing:
            case Data::SyncthingStatus::RemoteNotInSync:
            case Data::SyncthingStatus::Paused:
                qDebug() << "connected to Syncthing: " << m_connection.statusText();
            default:;
            }
            // consider connection to Syncthing successful
            wizardDlg.handleConfigurationApplied(QString(), &m_connection);
        });
    // -> invoke the whole procedure
    wizardDlg.next();
    m_eventLoop.exec();
    QVERIFY(finalPage != nullptr);
    QVERIFY(label->text().contains(QStringLiteral("changed successfully")));
    QVERIFY(label->text().contains(QStringLiteral("open Syncthing")));

    // verify settings
    QVERIFY(settings.launcher.autostartEnabled);
    QVERIFY(settings.launcher.considerForReconnect);
    QVERIFY(settings.launcher.showButton);
    QVERIFY(!settings.launcher.syncthingPath.isEmpty());
    QVERIFY(!settings.launcher.syncthingArgs.isEmpty());
    QVERIFY(!settings.launcher.useLibSyncthing);
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    QVERIFY(!settings.systemd.considerForReconnect);
    QVERIFY(!settings.systemd.showButton);
#endif
    // cannot verify the port from Syncthing's config, see comment in setup function
    //QCOMPARE(settings.connection.primary.syncthingUrl, QStringLiteral("http://127.0.0.1:8384"));
    QVERIFY(!settings.connection.primary.syncthingUrl.isEmpty());
    QVERIFY(!settings.connection.primary.apiKey.isEmpty());
    QCOMPARE(settings.connection.secondary.size(), 0);
    QCOMPARE(qEnvironmentVariable(PROJECT_VARNAME_UPPER "_AUTOSTART_PATH_MOCK"), QStringLiteral("fake-autostart-path"));
}

/*!
 * \brief Test configuring for the currently running Syncthing instance.
 * \remarks The running instance should have been started by WizardTests::testConfiguringLauncher().
 */
void WizardTests::testConfiguringCurrentlyRunningSyncthing()
{
    // mock the autostart path; it is supposed to be changed
    QVERIFY(qputenv(PROJECT_VARNAME_UPPER "_AUTOSTART_PATH_MOCK", "fake-autostart-path"));

    // change port in config file
    auto wizardDlg = Wizard();
    auto &setupDetection = wizardDlg.setupDetection();
    setupDetection.determinePaths();
    auto configFile = QFile(setupDetection.configFilePath);
    QVERIFY(configFile.open(QFile::ReadOnly));
    auto config = QString::fromUtf8(configFile.readAll());
    configFile.close();
    config.replace(QRegularExpression("<address>127.0.0.1:.*</address>"),
        QStringLiteral("<address>127.0.0.1:") % m_syncthingPort % QStringLiteral("</address>"));
    const auto configData = config.toUtf8();
    QVERIFY(configFile.open(QFile::WriteOnly | QFile::Truncate));
    QCOMPARE(configFile.write(configData), configData.size());
    configFile.close();

    // show wizard and proceed with guided setup
    wizardDlg.show();
    auto *const welcomePage = qobject_cast<WelcomeWizardPage *>(wizardDlg.currentPage());
    QVERIFY(welcomePage != nullptr);
    auto *const startWizardButton = welcomePage->findChild<QCommandLinkButton *>(QStringLiteral("startWizardCommand"));
    QVERIFY(startWizardButton != nullptr);
    startWizardButton->click();
    auto *const detectionPage = qobject_cast<DetectionWizardPage *>(wizardDlg.currentPage());
    QVERIFY(detectionPage != nullptr);

    // wait for main config page to show
    auto awaitDetection = std::function<void(void)>();
    awaitDetection = [&] {
        if (qobject_cast<MainConfigWizardPage *>(wizardDlg.currentPage())) {
            m_eventLoop.quit();
        } else {
            QVERIFY(!confirmMessageBox()); // message box should NOT show up
            QTimer::singleShot(0, this, awaitDetection);
        }
    };
    QTimer::singleShot(0, this, awaitDetection);
    m_eventLoop.exec();

    // configure external launcher
    const auto &url = setupDetection.connection.syncthingUrl();
    const auto &apiKey = setupDetection.connection.apiKey();
    qDebug() << "configured URL: " << url;
    qDebug() << "configured API key: " << apiKey;
    QVERIFY(!url.isEmpty());
    QVERIFY(!apiKey.isEmpty());
    auto *const mainConfigPage = qobject_cast<MainConfigWizardPage *>(wizardDlg.currentPage());
    QVERIFY(mainConfigPage != nullptr);
    QVERIFY(setupDetection.hasConfig());
    auto *const invalidConfigLabel = mainConfigPage->findChild<QLabel *>(QStringLiteral("invalidConfigLabel"));
    QVERIFY(invalidConfigLabel != nullptr);
    if (!invalidConfigLabel->isHidden()) {
        qDebug() << "invalid config label shown: " << invalidConfigLabel->text();
    }
    QVERIFY(invalidConfigLabel->isHidden());
    auto *const cfgLauncherExternalRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgLauncherExternalRadioButton"));
    QVERIFY(cfgLauncherExternalRadioButton != nullptr);
    QVERIFY(cfgLauncherExternalRadioButton->isHidden());
    auto *const cfgLauncherBuiltInRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgLauncherBuiltInRadioButton"));
    QVERIFY(cfgLauncherBuiltInRadioButton != nullptr);
    QVERIFY(cfgLauncherBuiltInRadioButton->isHidden());
    auto *const cfgSystemdUserUnitRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgSystemdUserUnitRadioButton"));
    QVERIFY(cfgSystemdUserUnitRadioButton != nullptr);
    QVERIFY(cfgSystemdUserUnitRadioButton->isHidden());
    auto *const cfgSystemdSystemUnitRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgSystemdSystemUnitRadioButton"));
    QVERIFY(cfgSystemdSystemUnitRadioButton != nullptr);
    QVERIFY(cfgSystemdSystemUnitRadioButton->isHidden());
    auto *const cfgCurrentlyRunningRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgCurrentlyRunningRadioButton"));
    QVERIFY(cfgCurrentlyRunningRadioButton != nullptr);
    QVERIFY(!cfgCurrentlyRunningRadioButton->isHidden());
    QVERIFY(cfgCurrentlyRunningRadioButton->isChecked());
    auto *const cfgNoneRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgNoneRadioButton"));
    QVERIFY(cfgNoneRadioButton != nullptr);
    QVERIFY(!cfgNoneRadioButton->isHidden());
    QVERIFY(!cfgNoneRadioButton->isChecked());
    wizardDlg.next();

    // override existing autostart setting
    auto *const autostartPage = qobject_cast<AutostartWizardPage *>(wizardDlg.currentPage());
    QVERIFY(autostartPage != nullptr);
    auto *const keepExistingCheckBox = autostartPage->findChild<QCheckBox *>(QStringLiteral("keepExistingCheckBox"));
    QVERIFY(keepExistingCheckBox != nullptr);
    QVERIFY(keepExistingCheckBox->isVisible());
    keepExistingCheckBox->setChecked(false);
    wizardDlg.next();
    configureSyncthingArgs(setupDetection);

    // apply settings
    auto *const applyPage = qobject_cast<ApplyWizardPage *>(wizardDlg.currentPage());
    QVERIFY(applyPage != nullptr);
    auto *const summaryTextBrowser = applyPage->findChild<QTextBrowser *>(QStringLiteral("summaryTextBrowser"));
    QVERIFY(summaryTextBrowser != nullptr);
    const auto summary = summaryTextBrowser->toPlainText();
    QVERIFY(summary.contains(QStringLiteral("Configure Syncthing Tray to use the currently running Syncthing instance")));
    QVERIFY(summary.contains(QStringLiteral("Do not change how Syncthing is launched")));
    QVERIFY(summary.contains(QStringLiteral("Keep autostart")));

    // apply changes and wait for results on final wizard page
    auto &settings = Settings::values();
    auto waitForCfg = std::function<void(void)>();
    FinalWizardPage *finalPage = nullptr;
    QProgressBar *progressBar = nullptr;
    QLabel *label = nullptr;
    waitForCfg = [&] {
        if (!finalPage && (finalPage = qobject_cast<FinalWizardPage *>(wizardDlg.currentPage()))) {
            progressBar = finalPage->findChild<QProgressBar *>(QStringLiteral("progressBar"));
            label = finalPage->findChild<QLabel *>(QStringLiteral("label"));
            QVERIFY(progressBar != nullptr);
            QVERIFY(label != nullptr);
        }
        if (label && progressBar && !label->isHidden() && progressBar->isHidden()) {
            m_eventLoop.quit();
        } else {
            QTimer::singleShot(0, this, waitForCfg);
        }
    };
    connect(&wizardDlg, &Wizard::settingsChanged, this, [&] {
        wizardDlg.handleConfigurationApplied();
        QTimer::singleShot(0, this, waitForCfg);
    });
    wizardDlg.next();
    m_eventLoop.exec();
    QVERIFY(finalPage != nullptr);
    QVERIFY(label->text().contains(QStringLiteral("changed successfully")));
    QVERIFY(label->text().contains(QStringLiteral("open Syncthing")));

    // verify settings
    QVERIFY(settings.launcher.autostartEnabled);
    QVERIFY(settings.launcher.considerForReconnect);
    QVERIFY(settings.launcher.showButton);
    QVERIFY(!settings.launcher.syncthingPath.isEmpty());
    QVERIFY(!settings.launcher.syncthingArgs.isEmpty());
    QVERIFY(!settings.launcher.useLibSyncthing);
    QCOMPARE(settings.connection.primary.syncthingUrl, m_syncthingGuiAddress);
    QVERIFY(!settings.connection.primary.apiKey.isEmpty());
    QCOMPARE(settings.connection.secondary.size(), 1);
    QCOMPARE(settings.connection.secondary[0].label, QStringLiteral("Backup of testconfig (created by wizard)"));
    QCOMPARE(qEnvironmentVariable(PROJECT_VARNAME_UPPER "_AUTOSTART_PATH_MOCK"), setupDetection.autostartSupposedPath);
}

bool WizardTests::confirmMessageBox()
{
    const auto allToplevelWidgets = QApplication::topLevelWidgets();
    for (auto *const w : allToplevelWidgets) {
        if (auto *const mb = qobject_cast<QMessageBox *>(w)) {
            QTest::keyClick(mb, Qt::Key_Enter);
            return true;
        }
    }
    return false;
}

/*!
 * \brief Overrides launcher settings so tests can use a custom Syncthing binary and a custom port.
 */
void WizardTests::configureSyncthingArgs(SetupDetection &setupDetection) const
{
    setupDetection.launcherSettings.syncthingPath = m_syncthingPath;
    setupDetection.defaultSyncthingArgs.append(QStringLiteral(" --gui-address="));
    setupDetection.defaultSyncthingArgs.append(m_syncthingGuiAddress);
    setupDetection.defaultSyncthingArgs.append(QStringLiteral(" --home='"));
    setupDetection.defaultSyncthingArgs.append(m_homeDir.path());
    setupDetection.defaultSyncthingArgs.append(QChar('\''));
}

QTEST_MAIN(WizardTests)
#include "wizard.moc"
