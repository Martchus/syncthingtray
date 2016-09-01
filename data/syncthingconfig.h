#ifndef DATA_SYNCTHINGCONFIG_H
#define DATA_SYNCTHINGCONFIG_H

#include <QString>

namespace Data {

struct SyncthingConfig
{
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
};


} // namespace Data

#endif // DATA_SYNCTHINGCONFIG_H
