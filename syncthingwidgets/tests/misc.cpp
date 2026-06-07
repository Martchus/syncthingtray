#include "../misc/statusinfo.h"

#include <syncthingconnector/syncthingconnection.h>

#include <qtutilities/misc/disablewarningsmoc.h>
#include <qtutilities/resources/resources.h>

#include <QLocale>

#include <QtTest/QtTest>

#include "resources/config.h"
#include "resources/qtconfig.h"

using namespace QtGui;

class MiscTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void testStatusInfoAndLocalization();

private:
};

void MiscTests::initTestCase()
{
}

void MiscTests::cleanupTestCase()
{
}

void MiscTests::cleanup()
{
}

void MiscTests::testStatusInfoAndLocalization()
{
    QLocale::setDefault(QLocale::German);
    LOAD_QT_TRANSLATIONS;

    auto connection = Data::SyncthingConnection();
    auto statusInfo = StatusInfo();
    statusInfo.updateConnectionStatus(connection);
    statusInfo.updateConnectedDevices(connection);
    QCOMPARE(statusInfo.statusText(), QStringLiteral("Nicht mit Syncthing verbunden"));
    QCOMPARE(statusInfo.additionalStatusText(), QStringLiteral("Versuche alle 30000 ms zu verbinden"));

    connection.m_status = Data::SyncthingStatus::Idle;
    statusInfo.updateConnectionStatus(connection);
    statusInfo.updateConnectedDevices(connection);
    QCOMPARE(statusInfo.statusText(), QStringLiteral("Syncthing ist im Leerlauf"));
    QCOMPARE(statusInfo.additionalStatusText(), QStringLiteral("Nicht mit anderen Geräten verbunden"));

    connection.m_devs.reserve(10);
    auto addDev = [&connection](const QString &name, Data::SyncthingDevStatus status = Data::SyncthingDevStatus::Idle) -> Data::SyncthingDev & {
        auto &dev = connection.m_devs.emplace_back(QStringLiteral("id"), name);
        dev.status = status;
        return dev;
    };

    // test with an unnamed, connected device
    auto &devWithNoName = addDev(QString());
    statusInfo.updateConnectedDevices(connection);
    QCOMPARE(statusInfo.additionalStatusText(), QStringLiteral("Mit 1 Gerät verbunden"));
    devWithNoName.status = Data::SyncthingDevStatus::Disconnected;

    // test with a named, connected device
    addDev(QStringLiteral("fake-dev"));
    statusInfo.updateConnectedDevices(connection);
    QCOMPARE(statusInfo.additionalStatusText(), QStringLiteral("Connected to fake-dev"));

    // test combination of both
    devWithNoName.status = Data::SyncthingDevStatus::Idle;
    statusInfo.updateConnectedDevices(connection);
    QCOMPARE(statusInfo.additionalStatusText(), QStringLiteral("Mit fake-dev und 1 weiteren Gerät verbunden"));
    devWithNoName.status = Data::SyncthingDevStatus::Disconnected;

    // test with further named and unnamed devices
    addDev(QStringLiteral("another-fake-dev"));
    statusInfo.updateConnectedDevices(connection);
    QCOMPARE(statusInfo.additionalStatusText(), QStringLiteral("Connected to fake-dev and another-fake-dev"));
    addDev(QStringLiteral("yet-another-fake-dev"));
    statusInfo.updateConnectedDevices(connection);
    QCOMPARE(statusInfo.additionalStatusText(), QStringLiteral("Connected to fake-dev, another-fake-dev, yet-another-fake-dev"));
    addDev(QString());
    statusInfo.updateConnectedDevices(connection);
    QCOMPARE(
        statusInfo.additionalStatusText(), QStringLiteral("Mit fake-dev, another-fake-dev, yet-another-fake-dev und 1 weiteren Gerät verbunden"));
    addDev(QStringLiteral("forth-dev-which-will-not-be-explicitly-mentioned"));
    statusInfo.updateConnectedDevices(connection);
    QCOMPARE(
        statusInfo.additionalStatusText(), QStringLiteral("Mit fake-dev, another-fake-dev, yet-another-fake-dev und 2 weiteren Geräten verbunden"));
}

QT_UTILITIES_DISABLE_WARNINGS_FOR_MOC_INCLUDE
QTEST_MAIN(MiscTests)
#include "misc.moc"
