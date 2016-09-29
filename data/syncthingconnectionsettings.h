#ifndef SYNCTHINGCONNECTIONSETTINGS_H
#define SYNCTHINGCONNECTIONSETTINGS_H

#include <QString>
#include <QByteArray>
#include <QSslError>

namespace Data {

struct SyncthingConnectionSettings {
    QString label;
    QString syncthingUrl;
    bool authEnabled = false;
    QString userName;
    QString password;
    QByteArray apiKey;
    int trafficPollInterval = 2000;
    int devStatsPollInterval = 60000;
    QString httpsCertPath;
    QList<QSslError> expectedSslErrors;
    bool loadHttpsCert();
};

}

#endif // SYNCTHINGCONNECTIONSETTINGS_H
