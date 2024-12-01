#ifndef DATA_SYNCTHINGCONFIG_H
#define DATA_SYNCTHINGCONFIG_H

#include "./global.h"

#include <QJsonArray>
#include <QString>

#include <optional>

namespace Data {

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingConfigDetails {
    QJsonArray folders;
    QJsonArray devices;
};

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingConfig {
    QString version;
    bool guiEnabled = false;
    bool guiEnforcesSecureConnection = false;
    QString guiAddress;
    QString guiUser;
    QString guiPasswordHash;
    QString guiApiKey;
    std::optional<SyncthingConfigDetails> details;

    static QString locateConfigFile(const QString &fileName);
    static QString locateConfigFile();
    static QString locateHttpsCertificate();
    bool restore(const QString &configFilePath, bool detailed = false);
    QString syncthingUrl() const;
};

} // namespace Data

#endif // DATA_SYNCTHINGCONFIG_H
