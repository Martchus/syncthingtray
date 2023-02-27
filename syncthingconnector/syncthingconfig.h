#ifndef DATA_SYNCTHINGCONFIG_H
#define DATA_SYNCTHINGCONFIG_H

#include "./global.h"

#include <QString>

namespace Data {

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingConfig {
    QString version;
    bool guiEnabled = false;
    bool guiEnforcesSecureConnection = false;
    QString guiAddress;
    QString guiUser;
    QString guiPasswordHash;
    QString guiApiKey;

    static QString locateConfigFile();
    static QString locateHttpsCertificate();
    bool restore(const QString &configFilePath);
    QString syncthingUrl() const;
};

} // namespace Data

#endif // DATA_SYNCTHINGCONFIG_H
