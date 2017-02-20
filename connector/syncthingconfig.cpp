#include "syncthingconfig.h"

#include <QStandardPaths>
#include <QXmlStreamReader>
#include <QFile>
#include <QHostAddress>

namespace Data {

/*!
 * \struct SyncthingConfig
 * \brief The SyncthingConfig struct holds the configuration of the local Syncthing instance read from config.xml in the Syncthing home directory.
 * \remarks Only a few fields are required since most of the Syncthing config can be accessed via SyncthingConnection class.
 */

QString SyncthingConfig::locateConfigFile()
{
    QString path = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("syncthing/config.xml"));
    if(path.isEmpty()) {
        path = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("Syncthing/config.xml"));
    }
    return path;
}

QString SyncthingConfig::locateHttpsCertificate()
{
    QString path = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("syncthing/https-cert.pem"));
    if(path.isEmpty()) {
        path = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("Syncthing/https-cert.pem"));
    }
    return path;
}

bool SyncthingConfig::restore(const QString &configFilePath)
{
    QFile configFile(configFilePath);
    if(!configFile.open(QFile::ReadOnly)) {
        return false;
    }

    QXmlStreamReader xmlReader(&configFile);
    bool ok = false;
#include <qtutilities/misc/xmlparsermacros.h>
    children {
        // only version 16 supported, try to parse other versions anyways since the changes might not affect
        // the few parts read here
        version = attribute("version").toString();
        children {
            iftag("gui") {
                ok = true;
                guiEnabled = attributeFlag("enabled");
                guiEnforcesSecureConnection = attributeFlag("tls");
                children {
                    iftag("address") {
                        guiAddress = text;
                    } eliftag("user") {
                        guiUser = text;
                    } eliftag("password") {
                        guiPasswordHash = text;
                    } eliftag("apikey") {
                        guiApiKey = text;
                    } else_skip
                }
            } else_skip
        }
    }
#include <qtutilities/misc/undefxmlparsermacros.h>
    return ok;
}

QString SyncthingConfig::syncthingUrl() const
{
    return (guiEnforcesSecureConnection || !QHostAddress(guiAddress.mid(0, guiAddress.indexOf(QChar(':')))).isLoopback() ? QStringLiteral("https://") : QStringLiteral("http://")) + guiAddress;
}

} // namespace Data
