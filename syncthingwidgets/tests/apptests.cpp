#define SYNCTHINGTESTHELPER_FOR_CLI

#include "./testhelper.h"

#include "../quick/app.h"
#include "../quick/appservice.h"
#include "../quick/helpers.h"

#include "../../testhelper/helper.h"

#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QtQuickTest>

#include <optional>

class Setup : public QObject {
    Q_OBJECT

public:
    Setup()
    {
    }

public Q_SLOTS:
    void applicationAvailable()
    {
        initTestLocale();
        initTestSettings(Settings::values());
        initTestHomeDir(m_settingsDir);
        initTestConfig();
        initTestSyncthingPath(m_syncthingPath);

        auto hasWithSyncthing = false;
        auto withSyncthing = qEnvironmentVariableIntValue("SYNCTHINGWIDGETS_APP_TESTS_WITH_SYNCTHING", &hasWithSyncthing);
        auto settings = QJsonObject();
        auto connectionSettings = QJsonObject();
        auto launcherSettings = QJsonObject();
        connectionSettings.insert(QStringLiteral("useLauncher"), true);
        launcherSettings.insert(QStringLiteral("run"), (m_withSyncthing = !hasWithSyncthing || withSyncthing > 0));
        launcherSettings.insert(QStringLiteral("exePath"), m_syncthingPath);
        settings.insert(QStringLiteral("connection"), connectionSettings);
        settings.insert(QStringLiteral("launcher"), launcherSettings);

        const auto homePath = m_settingsDir.path();
        const auto settingsFilePath = homePath + QStringLiteral("/appconfig.json");
        const auto settingsDocument = QJsonDocument(settings);
        const auto settingsData = settingsDocument.toJson();
        auto settingsFile = QFile(settingsFilePath);
        QVERIFY(settingsFile.open(QFile::WriteOnly | QFile::Truncate));
        QCOMPARE(settingsFile.write(settingsData), settingsData.size());
        QVERIFY(settingsFile.flush());
        qputenv("SYNCTHINGWIDGETS_SETTINGS_DIR", homePath.toLocal8Bit());

        // use a single window; that's less noisy when running tests non-headless
        qputenv("SYNCTHINGWIDGETS_NATIVE_POPUPS", "0");
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        auto *const context = engine->rootContext();
        context->setContextProperty(QStringLiteral("withSyncthing"), m_withSyncthing);
        context->setContextProperty(QStringLiteral("settingsPath"), m_settingsDir.path());
        context->setContextProperty(QStringLiteral("directoryIdRole"), Data::SyncthingDirectoryModel::DirectoryId);
        context->setContextProperty(QStringLiteral("directoryPathRole"), Data::SyncthingDirectoryModel::DirectoryPath);
        context->setContextProperty(QStringLiteral("deviceStatusStringRole"), Data::SyncthingDeviceModel::DeviceStatusString);

        m_service.emplace(true);
        m_app.emplace(true, engine);
        QCOMPARE(m_service->settingsDir().path(), m_settingsDir.path());
        QCOMPARE(m_app->settingsDir().path(), m_settingsDir.path());
        QtGui::connectAppAndService(*m_app, *m_service);

        if (m_withSyncthing) {
            qDebug() << "Waiting for service and app to connect to Syncthing";
            QVERIFY(waitForConnected(*m_service->connection(), 15000));
            QVERIFY(waitForConnected(*m_app->connection(), 15000));
        } else {
            qDebug() << "Starting test without Syncthing";
        }
    }

    void cleanupTestCase()
    {
        m_service.reset();
        m_app.reset();
    }

private:
    std::optional<QtGui::App> m_app;
    std::optional<QtGui::AppService> m_service;
    QTemporaryDir m_settingsDir;
    QString m_syncthingPath;
    bool m_withSyncthing = false;
};

QUICK_TEST_MAIN_WITH_SETUP(apptest, Setup)

#include "apptests.moc"
