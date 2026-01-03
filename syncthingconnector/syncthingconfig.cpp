#include "./syncthingconfig.h"
#include "./utils.h"

#include "resources/config.h"

#include <qtutilities/misc/compat.h>

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QHostAddress>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QXmlStreamReader>

#include <QJsonObject>

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

/*!
 * \brief Converts a single text value to a JSON value guessing the type.
 */
static QJsonValue xmlValueToJsonValue(QStringView value)
{
    if (value == QLatin1String("true")) {
        return QJsonValue(true);
    } else if (value == QLatin1String("false")) {
        return QJsonValue(false);
    }
    auto isNumber = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) // could be QT_VERSION >= QT_VERSION_CHECK(5, 15, 2)
    auto number = value.toDouble(&isNumber);
#else
    auto number = value.toString().toDouble(&isNumber);
#endif
    return isNumber ? QJsonValue(number) : QJsonValue(value.toString());
}

/*!
 * \brief Adds current attributes form \a xmlReader to \a jsonObject.
 */
static void xmlAttributesToJsonObject(QXmlStreamReader &xmlReader, QJsonObject &jsonObject)
{
    for (const auto &attribute : xmlReader.attributes()) {
        jsonObject.insert(attribute.name(), xmlValueToJsonValue(attribute.value()));
    }
}

/*!
 * \brief Adds the current element of \a xmlReader to \a object.
 * \remarks The tokenType() of \a xmlReader is supposed to be QXmlStreamReader::StartElement.
 */
static void xmlElementToJsonValue(QXmlStreamReader &xmlReader, QJsonObject &object)
{
    // define list of elements that need to be put into an array
    static const auto arrayElements = QHash<QString, QString>{
        { QStringLiteral("device"), QStringLiteral("devices") },
        { QStringLiteral("address"), QStringLiteral("addresses") },
    };

    // define list of elements that are known to be objects (so an empty element is mapped to an empty object
    // and not an empty string)
    static const auto objectElements = QSet<QString>{
        QStringLiteral("versioning"),
    };

    // read element name, text and possibly nested objects
    auto name = xmlReader.name().toString();
    auto arrayName = arrayElements.find(name);
    auto text = QString();
    auto nestedObject = QJsonObject();
    auto valid = true;
    xmlAttributesToJsonObject(xmlReader, nestedObject);
    while (valid) {
        switch (xmlReader.readNext()) {
        case QXmlStreamReader::StartElement:
            xmlAttributesToJsonObject(xmlReader, nestedObject);
            xmlElementToJsonValue(xmlReader, nestedObject);
            break;
        case QXmlStreamReader::Characters:
            text.append(xmlReader.text());
            break;
        case QXmlStreamReader::Invalid:
        case QXmlStreamReader::EndDocument:
        case QXmlStreamReader::EndElement:
            valid = false;
            break;
        default:;
        }
    }

    // handle special mapping of ID for device elements
    if (arrayName != arrayElements.cend() && *arrayName == QLatin1String("devices")) {
        nestedObject.insert(QStringLiteral("deviceID"), nestedObject.take(QLatin1String("id")));
    }

    // insert/append the value into the destination object/array
    auto value = QJsonValue(nestedObject.isEmpty() && !objectElements.contains(name) ? xmlValueToJsonValue(text) : std::move(nestedObject));
    if (arrayName == arrayElements.cend()) {
        object.insert(std::move(name), std::move(value));
        return;
    }
    auto valueRef = object[*arrayName];
    auto array = valueRef.isArray() ? valueRef.toArray() : QJsonArray();
    array.append(std::move(value));
    valueRef = array;
}

/*!
 * \brief Converts the current sub-tree of \a xmlReader to a QJsonObject.
 * \remarks The tokenType() of \a xmlReader is supposed to be QXmlStreamReader::StartElement.
 */
static QJsonObject xmlToJson(QXmlStreamReader &xmlReader, bool convertDeviceId)
{
    auto json = QJsonObject();
    xmlAttributesToJsonObject(xmlReader, json);
    while (xmlReader.readNextStartElement()) {
        xmlElementToJsonValue(xmlReader, json);
    }
    if (convertDeviceId) {
        json.insert(QStringLiteral("deviceID"), json.take(QLatin1String("id")));
    }
    return json;
}

/*!
 * \brief Reads the configuration at the specified \a configFilePath.
 * \param details Whether details should be populates as well.
 */
bool SyncthingConfig::restore(const QString &configFilePath, bool detailed)
{
    auto configFile = QFile(configFilePath);
    if (!configFile.open(QFile::ReadOnly)) {
        return false;
    }

    auto xmlReader = QXmlStreamReader(&configFile);
    auto ok = false;
    auto *const details = detailed ? &this->details.emplace() : nullptr;
#include <qtutilities/misc/xmlparsermacros.h>
    children
    {
        // only version 16 supported, try to parse other versions anyway since the changes might not affect
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
            eliftag("folder")
            {
                if (details) {
                    details->folders.append(xmlToJson(xmlReader, false));
                }
                else_skip
            }
            eliftag("device")
            {
                if (details) {
                    details->devices.append(xmlToJson(xmlReader, true));
                }
                else_skip
            }
            else_skip
        }
    }
#include <qtutilities/misc/undefxmlparsermacros.h>
    return ok;
}

QString SyncthingConfig::syncthingUrl() const
{
    // split the GUI address into IP address/hostname and port
    const auto portStart = guiAddress.indexOf(QChar(':'));
    auto guiHost = guiAddress.mid(0, portStart);
    const auto guiPort = portStart > 0 ? QtUtilities::midRef(guiAddress, portStart) : QtUtilities::StringView();
    const auto guiAddress = QHostAddress(guiHost);

    // check whether address is local and handle "any" addresses
    auto localConnection = true;
    if (guiAddress == QHostAddress::AnyIPv4) {
        // assume local connection if address is "0.0.0.0"
        guiHost = QStringLiteral("127.0.0.1");
    } else if (guiAddress == QHostAddress::AnyIPv6) {
        // assume local connection if address is "::"
        guiHost = QStringLiteral("[::1]");
    } else if (!isLocal(guiHost, guiAddress)) {
        localConnection = false;
    }

    return ((guiEnforcesSecureConnection || !localConnection) ? QStringLiteral("https://") : QStringLiteral("http://")) % guiHost % guiPort;
}

SyncthingConnectInfo SyncthingConfig::syncthingConnectInfo() const
{
    auto res = SyncthingConnectInfo();
    if (QFileInfo::exists(guiAddress)) {
        res.url = guiEnforcesSecureConnection ? QStringLiteral("unix+https://localhost") : QStringLiteral("unix+http://localhost");
        res.path = guiAddress;
    } else {
        res.url = syncthingUrl();
    }
    return res;
}

} // namespace Data
