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
    qputenv("LIB_SYNCTHING_CONNECTOR_SYNCTHING_CONFIG_DIR", homePath.toLocal8Bit());
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

#endif // SYNCTHINGWIDGETS_TESTHELPER_H
