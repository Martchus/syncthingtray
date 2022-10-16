#include "../settings/wizard.h"
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
    void cleanup();
    void testShowingSettings();
    void testConfiguringLauncher();

private:
    void confirmMessageBox();

    QTemporaryDir m_homeDir;
    QEventLoop m_eventLoop;
};

void WizardTests::initTestCase()
{
    // ensure all text is English as checks rely on it
    QLocale::setDefault(QLocale::English);

    // assume first launch and enable WIP guided setup
    auto &settings = Settings::values();
    settings.enableWipFeatures = true;
    settings.fakeFirstLaunch = true;

    // use an empty dir as HOME to simulate a prestine setup
    qDebug() << QStringLiteral("HOME dir: ") + m_homeDir.path();
    qputenv("HOME", m_homeDir.path().toLocal8Bit());
    QVERIFY(m_homeDir.isValid());
}

void WizardTests::cleanup()
{
    // let each test start with a new wizard
    if (Wizard::hasInstance()) {
        delete Wizard::instance();
    }
}

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

void WizardTests::testConfiguringLauncher()
{
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
    auto *const finalPage = qobject_cast<FinalWizardPage *>(wizardDlg.currentPage());
    QVERIFY(finalPage != nullptr);
    auto *const progressBar = finalPage->findChild<QProgressBar *>(QStringLiteral("progressBar"));
    auto *const label = finalPage->findChild<QLabel *>(QStringLiteral("label"));
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
    wizardDlg.next();
}

void WizardTests::confirmMessageBox()
{
    const auto allToplevelWidgets = QApplication::topLevelWidgets();
    for (auto *const w : allToplevelWidgets) {
        if (auto *const mb = qobject_cast<QMessageBox *>(w)) {
            QTest::keyClick(mb, Qt::Key_Enter);
            break;
        }
    }
}

QTEST_MAIN(WizardTests)
#include "wizard.moc"
