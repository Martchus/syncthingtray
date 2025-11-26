#define SYNCTHINGTESTHELPER_FOR_CLI

#include "./testhelper.h"

#include "../quick/app.h"
#include "../quick/appservice.h"
#include "../quick/helpers.h"

#include "../../testhelper/helper.h"

#include <QtQuickTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQmlContext>
#include <QGuiApplication>
#include <QTemporaryDir>

#include <optional>

class Setup : public QObject
{
    Q_OBJECT

public:
    Setup() {}

public Q_SLOTS:
    void applicationAvailable()
    {
        initTestLocale();
        initTestSettings(Settings::values());
        initTestHomeDir(m_settingsDir);
        initTestConfig();
        initTestSyncthingPath(m_syncthingPath);

        auto settings = QJsonObject();
        auto connectionSettings = QJsonObject();
        auto launcherSettings = QJsonObject();
        connectionSettings.insert(QStringLiteral("useLauncher"), true);
        launcherSettings.insert(QStringLiteral("run"), true);
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
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        m_service.emplace(true);
        m_app.emplace(true, engine);
        QCOMPARE(m_service->settingsDir().path(), m_settingsDir.path());
        QCOMPARE(m_app->settingsDir().path(), m_settingsDir.path());
        QtGui::connectAppAndService(*m_app, *m_service);

        qDebug() << "Waiting for service and app to connect to Syncthing";
        QVERIFY(waitForConnected(*m_service->connection()));
        QVERIFY(waitForConnected(*m_app->connection()));
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
};

QUICK_TEST_MAIN_WITH_SETUP(apptest, Setup)

#include "apptests.moc"
