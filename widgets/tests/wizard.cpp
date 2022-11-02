#include "../settings/wizard.h"
#include "../misc/syncthinglauncher.h"
#include "../settings/settings.h"
#include "../settings/setupdetection.h"

#include <QtTest/QtTest>

#include <QApplication>
#include <QCommandLinkButton>
#include <QDebug>
#include <QEventLoop>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QProgressBar>
#include <QRadioButton>
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

    QTemporaryDir m_homeDir;
    QEventLoop m_eventLoop;
    QString m_syncthingPath;
    QString m_syncthingPort;
    QString m_syncthingGuiAddress;
    Data::SyncthingLauncher m_launcher;
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
    qDebug() << QStringLiteral("HOME dir: ") + m_homeDir.path();
    qputenv("HOME", m_homeDir.path().toLocal8Bit());
    QVERIFY(m_homeDir.isValid());

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
}

void WizardTests::cleanupTestCase()
{
    if (m_launcher.isRunning()) {
        qDebug() << "terminating Syncthing";
        m_launcher.terminate();
    }
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
    connect(wizardDlg, &Wizard::settingsDialogRequested, [&] { ++settingsDlgRequested; });
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
}

/*!
 * \brief Tests configuring the built-in launcher to launch an external Syncthing binary.
 */
void WizardTests::testConfiguringLauncher()
{
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
    QVERIFY(!wizardDlg.setupDetection().hasConfig());
    QVERIFY(wizardDlg.setupDetection().launcherExitCode.has_value());
    QCOMPARE(wizardDlg.setupDetection().launcherExitCode.value(), 0);
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
    wizardDlg.next();

    // apply settings
    auto *const applyPage = qobject_cast<ApplyWizardPage *>(wizardDlg.currentPage());
    QVERIFY(applyPage != nullptr);
    auto *const summaryTextBrowser = applyPage->findChild<QTextBrowser *>(QStringLiteral("summaryTextBrowser"));
    QVERIFY(summaryTextBrowser != nullptr);
    const auto summary = summaryTextBrowser->toPlainText();
    QVERIFY(summary.contains(QStringLiteral("Start Syncthing via Syncthing Tray's launcher")));
    QVERIFY(summary.contains(QStringLiteral("executable from PATH as separate process")));
    QVERIFY(summary.contains(QStringLiteral("Keep autostart")));
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

    // override launcher settings so tests can use a custom Syncthing binary and a custom port
    auto &setupDetection = wizardDlg.setupDetection();
    setupDetection.launcherSettings.syncthingPath = m_syncthingPath;
    setupDetection.defaultSyncthingArgs.append(QStringLiteral(" --gui-address="));
    setupDetection.defaultSyncthingArgs.append(m_syncthingGuiAddress);

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
#ifdef LIB_SYNCTHING_CONNECTOR_SUPPORT_SYSTEMD
    QVERIFY(!settings.systemd.considerForReconnect);
    QVERIFY(!settings.systemd.showButton);
#endif
    // cannot verify the port from Syncthing's config, see comment in setup function
    //QCOMPARE(settings.connection.primary.syncthingUrl, QStringLiteral("http://127.0.0.1:8384"));
    QVERIFY(!settings.connection.primary.apiKey.isEmpty());
    QCOMPARE(settings.connection.secondary.size(), 0);
}

/*!
 * \brief Test configuring for the currently running Syncthing instance.
 * \remarks The running instance should have been started by WizardTests::testConfiguringLauncher().
 */
void WizardTests::testConfiguringCurrentlyRunningSyncthing()
{
    // change port in config file
    auto wizardDlg = Wizard();
    auto &setupDetection = wizardDlg.setupDetection();
    setupDetection.determinePaths();
    auto configFile = QFile(setupDetection.configFilePath);
    QVERIFY(configFile.open(QFile::ReadOnly));
    auto config = configFile.readAll();
    configFile.close();
    config.replace(QByteArray(":8384"), (QChar(':') + m_syncthingPort).toUtf8());
    QVERIFY(configFile.open(QFile::WriteOnly | QFile::Truncate));
    QCOMPARE(configFile.write(config), config.size());
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
    auto *const mainConfigPage = qobject_cast<MainConfigWizardPage *>(wizardDlg.currentPage());
    QVERIFY(mainConfigPage != nullptr);
    QVERIFY(setupDetection.hasConfig());
    auto *const cfgCurrentlyRunningRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgCurrentlyRunningRadioButton"));
    QVERIFY(cfgCurrentlyRunningRadioButton != nullptr);
    QVERIFY(!cfgCurrentlyRunningRadioButton->isHidden());
    QVERIFY(cfgCurrentlyRunningRadioButton->isChecked());
    auto *const cfgLauncherExternalRadioButton = mainConfigPage->findChild<QRadioButton *>(QStringLiteral("cfgLauncherExternalRadioButton"));
    QVERIFY(cfgLauncherExternalRadioButton != nullptr);
    QVERIFY(cfgLauncherExternalRadioButton->isHidden());
    wizardDlg.next();

    // keep autostart setting as-is
    auto *const autostartPage = qobject_cast<AutostartWizardPage *>(wizardDlg.currentPage());
    QVERIFY(autostartPage != nullptr);
    wizardDlg.next();

    // override launcher settings so tests can use a custom Syncthing binary and a custom port
    setupDetection.launcherSettings.syncthingPath = m_syncthingPath;
    setupDetection.defaultSyncthingArgs.append(QStringLiteral(" --gui-address="));
    setupDetection.defaultSyncthingArgs.append(m_syncthingGuiAddress);

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

QTEST_MAIN(WizardTests)
#include "wizard.moc"
