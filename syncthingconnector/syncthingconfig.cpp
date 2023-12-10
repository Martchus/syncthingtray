#include "./syncthingconfig.h"
#include "./utils.h"

#include "resources/config.h"

#include <qtutilities/misc/compat.h>

#include <QFile>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QXmlStreamReader>

namespace Data {

/*!
 * \struct SyncthingConfig
 * \brief The SyncthingConfig struct holds the configuration of the local Syncthing instance read from config.xml in the Syncthing home directory.
 * \remarks Only a few fields are required since most of the Syncthing config can be accessed via SyncthingConnection class.
 */

/*!
 * \brief Locates the file with the specified \a fileName in Syncthing's config directory.
 * \remarks The lookup within QStandardPaths::RuntimeLocation is mainly for macOS where the full path
 *          is "$HOME/Library/Application Support/Syncthing/config.xml".
 */
QString SyncthingConfig::locateConfigFile(const QString &fileName)
{
    // check override via environment variable
    auto path = qEnvironmentVariable(PROJECT_VARNAME_UPPER "_SYNCTHING_CONFIG_DIR");
    if (!path.isEmpty()) {
        if (!QFile::exists(path = path % QChar('/') % fileName)) {
            path.clear();
        }
        return path;
    }

    // check usual standard locations
    static const QString casings[] = { QStringLiteral("syncthing/"), QStringLiteral("Syncthing/") };
    static const QStandardPaths::StandardLocation locations[] = { QStandardPaths::GenericConfigLocation, QStandardPaths::RuntimeLocation };
    for (const auto location : locations) {
        for (const auto &casing : casings) {
            if (!(path = QStandardPaths::locate(location, casing + fileName)).isEmpty()) {
                return path;
            }
        }
    }

    // check state dir used by Syncthing on Unix systems as of v1.27.0 (commit b5082f6af8b0a70afd3bc42977dad26920e72b68)
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    if (!(path = qEnvironmentVariable("XDG_STATE_HOME")).isEmpty()) {
        if (QFile::exists(path = path % QStringLiteral("/syncthing/") % fileName)) {
            return path;
        }
    }
    if (!(path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).isEmpty()) {
        if (QFile::exists(path = path % QStringLiteral("/.local/state/syncthing/") % fileName)) {
            return path;
        }
    }
#endif

    path.clear();
    return path;
}

/*!
 * \brief Locates Syncthing's main config file ("config.xml").
 */
QString SyncthingConfig::locateConfigFile()
{
    return locateConfigFile(QStringLiteral("config.xml"));
}

/*!
 * \brief Locates Syncthing's GUI HTTPS certificate.
 */
QString SyncthingConfig::locateHttpsCertificate()
{
    return locateConfigFile(QStringLiteral("https-cert.pem"));
}

bool SyncthingConfig::restore(const QString &configFilePath)
{
    QFile configFile(configFilePath);
    if (!configFile.open(QFile::ReadOnly)) {
        return false;
    }

    QXmlStreamReader xmlReader(&configFile);
    bool ok = false;
#include <qtutilities/misc/xmlparsermacros.h>
    children
    {
        // only version 16 supported, try to parse other versions anyways since the changes might not affect
        // the few parts read here
        version = attribute("version").toString();
        children
        {
            iftag("gui")
            {
                ok = true;
                guiEnabled = attributeFlag("enabled");
                guiEnforcesSecureConnection = attributeFlag("tls");
                children
                {
                    iftag("address")
                    {
                        guiAddress = text;
                    }
                    eliftag("user")
                    {
                        guiUser = text;
                    }
                    eliftag("password")
                    {
                        guiPasswordHash = text;
                    }
                    eliftag("apikey")
                    {
                        guiApiKey = text;
                    }
                    else_skip
                }
            }
            else_skip
        }
    }
#include <qtutilities/misc/undefxmlparsermacros.h>
    return ok;
}

QString SyncthingConfig::syncthingUrl() const
{
    return (guiEnforcesSecureConnection || !isLocal(stripPort(guiAddress)) ? QStringLiteral("https://") : QStringLiteral("http://")) + guiAddress;
}

} // namespace Data
