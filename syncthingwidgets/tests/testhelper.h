#ifndef SYNCTHINGWIDGETS_TESTHELPER_H
#define SYNCTHINGWIDGETS_TESTHELPER_H

#include "../settings/settings.h"

// use meta-data of syncthingtray application here
#include "resources/../../tray/resources/config.h"

#include <qtutilities/misc/compat.h>

#include <QtTest/QtTest>

#include <QDebug>
#include <QLocale>
#include <QTemporaryDir>

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlContext>
#include <QQmlEngine>

#include <syncthingmodel/syncthingdevicemodel.h>
#include <syncthingmodel/syncthingdirectorymodel.h>

#include "../../testhelper/helper.h"
#endif

/*!
 * \brief Ensure all text is English so checks can rely on it.
 */
inline void initTestLocale()
{
    QLocale::setDefault(QLocale::English);
}

/*!
 * \brief Initializes \a settings for testing.
 */
inline void initTestSettings(Settings::Settings &settings)
{
    // assume first launch
    settings.fakeFirstLaunch = true;

    // assert there's no connection setting present initially
    settings.connection.primary.label = QStringLiteral("testconfig");
    QCOMPARE(settings.connection.primary.syncthingUrl, QString());
    QCOMPARE(settings.connection.primary.apiKey, QByteArray());
    QCOMPARE(settings.connection.secondary.size(), 0);
}

/*!
 * \brief Uses the temporary dir \a homeDir as Syncthing home directory for testing.
 */
inline void initTestHomeDir(QTemporaryDir &homeDir)
{
    const auto homePath = homeDir.path();
    qDebug() << "HOME dir: " << homePath;
    qputenv("LIB_SYNCTHING_CONNECTOR_SYNCTHING_CONFIG_DIR", homePath.toUtf8());
    QCOMPARE(homeDir.errorString(), QString());
}

/*!
 * \brief Creates a config file for Syncthing Tray in the working dir so it'll be picked up instead of the user's config file.
 */
inline void initTestConfig()
{
    auto testConfigFile = QFile(QStringLiteral(PROJECT_NAME ".ini"));
    QVERIFY(testConfigFile.open(QFile::WriteOnly | QFile::Truncate));
    testConfigFile.close();
}

/*!
 * \brief Reads the Syncthing executable path from env so it must not necassarily be in PATH for for running tests.
 */
inline void initTestSyncthingPath(QString &syncthingPath)
{
    const auto syncthingPathFromEnv = qgetenv("SYNCTHING_PATH");
    syncthingPath = syncthingPathFromEnv.isEmpty() ? QStringLiteral("syncthing") : QString::fromLocal8Bit(syncthingPathFromEnv);
}

#ifdef SYNCTHINGWIDGETS_GUI_QTQUICK
/*!
 * \brief Prepares the test settings directory, export directory, launcher and settings configuration.
 */
inline void prepareTestEnvironment(QTemporaryDir &settingsDir, QTemporaryDir &exportDir, QString &syncthingPath, QString &testConfigDir,
    bool &withSyncthing, const char *withSyncthingEnvVarName = nullptr)
{
    initTestLocale();
    initTestSettings(Settings::values());
    initTestHomeDir(settingsDir);
    initTestConfig();
    initTestSyncthingPath(syncthingPath);
    QCOMPARE(exportDir.errorString(), QString());

    auto hasWithSyncthing = false;
    auto withSyncthingVal = withSyncthingEnvVarName && qEnvironmentVariableIntValue(withSyncthingEnvVarName, &hasWithSyncthing);
    withSyncthing = !hasWithSyncthing || withSyncthingVal > 0;

    auto settings = QJsonObject();
    auto connectionSettings = QJsonObject();
    auto launcherSettings = QJsonObject();
    connectionSettings.insert(QStringLiteral("useLauncher"), true);
    launcherSettings.insert(QStringLiteral("run"), withSyncthing);
    launcherSettings.insert(QStringLiteral("exePath"), syncthingPath);
    settings.insert(QStringLiteral("connection"), connectionSettings);
    settings.insert(QStringLiteral("launcher"), launcherSettings);

    const auto homePath = settingsDir.path();
    const auto settingsFilePath = homePath + QStringLiteral("/appconfig.json");
    const auto settingsDocument = QJsonDocument(settings);
    const auto settingsData = settingsDocument.toJson();
    auto settingsFile = QFile(settingsFilePath);
    QVERIFY(settingsFile.open(QFile::WriteOnly | QFile::Truncate));
    QCOMPARE(settingsFile.write(settingsData), settingsData.size());
    QVERIFY(settingsFile.flush());
    qputenv("SYNCTHINGWIDGETS_SETTINGS_DIR", homePath.toUtf8());

    // use a single window; that's less noisy when running tests non-headless
    qputenv("SYNCTHINGWIDGETS_POPUP_TYPE", "0");

    testConfigDir = QString::fromStdString(testDirPath("testconfig"));
    testConfigDir = QFileInfo(testConfigDir).absoluteFilePath();
    QVERIFY2(!testConfigDir.isEmpty(), "test config dir located");
    qDebug() << "test config dir: " << testConfigDir;
}

/*!
 * \brief Registers context properties on QQmlEngine.
 */
inline void registerCommonContextProperties(QQmlEngine *engine, bool withSyncthing, const QString &settingsPath, const QString &testConfigDir,
    const QString &testExportDir, QObject *setupObj)
{
    auto *const context = engine->rootContext();
    context->setContextProperty(QStringLiteral("withSyncthing"), withSyncthing);
    context->setContextProperty(QStringLiteral("settingsPath"), settingsPath);
    context->setContextProperty(QStringLiteral("directoryIdRole"), Data::SyncthingDirectoryModel::DirectoryId);
    context->setContextProperty(QStringLiteral("directoryPathRole"), Data::SyncthingDirectoryModel::DirectoryPath);
    context->setContextProperty(QStringLiteral("deviceStatusStringRole"), Data::SyncthingDeviceModel::DeviceStatusString);
    context->setContextProperty(QStringLiteral("testConfigDir"), testConfigDir);
    context->setContextProperty(QStringLiteral("testExportDir"), testExportDir);
    context->setContextProperty(QStringLiteral("setup"), setupObj);
}
#endif

#endif // SYNCTHINGWIDGETS_TESTHELPER_H
