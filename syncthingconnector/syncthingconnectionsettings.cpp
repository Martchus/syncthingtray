#include "./syncthingconnectionsettings.h"

#include <QFileInfo>
#include <QJsonObject>
#include <QJsonValue>

namespace Data {

#ifndef QT_NO_SSL
QList<QSslError> SyncthingConnectionSettings::compileSslErrors(const QSslCertificate &trustedCert)
{
    // clang-format off
    return QList<QSslError>{
        QSslError(QSslError::UnableToGetLocalIssuerCertificate, trustedCert),
        QSslError(QSslError::UnableToVerifyFirstCertificate, trustedCert),
        QSslError(QSslError::SelfSignedCertificate, trustedCert),
        QSslError(QSslError::HostNameMismatch, trustedCert),
        QSslError(QSslError::CertificateUntrusted, trustedCert),
        QSslError(QSslError::CertificateRejected, trustedCert)
    };
    // clang-format on
}

bool SyncthingConnectionSettings::loadHttpsCert()
{
    expectedSslErrors.clear();
    if (httpsCertPath.isEmpty()) {
        return true;
    }
    const auto certs = QSslCertificate::fromPath(httpsCertPath);
    if (certs.isEmpty() || certs.at(0).isNull()) {
        return false;
    }

    httpCertLastModified = QFileInfo(httpsCertPath).lastModified();
    expectedSslErrors = compileSslErrors(certs.at(0));
    return true;
}
#endif

void SyncthingConnectionSettings::storeToJson(QJsonObject &object)
{
    auto httpAuth = QJsonObject(), advanced = QJsonObject();
    object.insert(QLatin1String("syncthingUrl"), syncthingUrl);
    object.insert(QLatin1String("apiKey"), QString::fromUtf8(apiKey));
    httpAuth.insert(QLatin1String("enabled"), authEnabled);
    httpAuth.insert(QLatin1String("userName"), userName);
    httpAuth.insert(QLatin1String("password"), password);
    object.insert(QLatin1String("httpAuth"), httpAuth);
    advanced.insert(QLatin1String("localPath"), localPath);
    advanced.insert(QLatin1String("trafficPollInterval"), trafficPollInterval);
    advanced.insert(QLatin1String("devStatsPollInterval"), devStatsPollInterval);
    advanced.insert(QLatin1String("errorsPollInterval"), errorsPollInterval);
    advanced.insert(QLatin1String("reconnectInterval"), reconnectInterval);
    advanced.insert(QLatin1String("longPollingTimeout"), longPollingTimeout);
    advanced.insert(QLatin1String("diskEventLimit"), diskEventLimit);
    advanced.insert(QLatin1String("autoConnect"), autoConnect);
    advanced.insert(QLatin1String("pauseOnMeteredConnection"), pauseOnMeteredConnection);
    object.insert(QLatin1String("advanced"), advanced);
#ifndef QT_NO_SSL
    object.insert(QLatin1String("httpsCertPath"), httpsCertPath);
#endif
}

bool SyncthingConnectionSettings::loadFromJson(const QJsonObject &object)
{
    const auto httpAuth = object.value(QLatin1String("httpAuth")).toObject();
    const auto advanced = object.value(QLatin1String("advanced")).toObject();
    label.clear();
    syncthingUrl = object.value(QLatin1String("syncthingUrl")).toString();
    apiKey = object.value(QLatin1String("apiKey")).toString().toUtf8();
    authEnabled = httpAuth.value(QLatin1String("enabled")).toBool();
    userName = httpAuth.value(QLatin1String("userName")).toString();
    password = httpAuth.value(QLatin1String("password")).toString();
    localPath = advanced.value(QLatin1String("localPath")).toString();
    trafficPollInterval = advanced.value(QLatin1String("trafficPollInterval")).toInt(defaultTrafficPollInterval);
    devStatsPollInterval = advanced.value(QLatin1String("devStatsPollInterval")).toInt(defaultDevStatusPollInterval);
    errorsPollInterval = advanced.value(QLatin1String("errorsPollInterval")).toInt(defaultErrorsPollInterval);
    reconnectInterval = advanced.value(QLatin1String("reconnectInterval")).toInt(defaultReconnectInterval);
    requestTimeout = advanced.value(QLatin1String("longPollingTimeout")).toInt(defaultRequestTimeout);
    longPollingTimeout = advanced.value(QLatin1String("longPollingTimeout")).toInt(defaultLongPollingTimeout);
    diskEventLimit = advanced.value(QLatin1String("diskEventLimit")).toInt(defaultDiskEventLimit);
    statusComputionFlags = SyncthingStatusComputionFlags::Default | SyncthingStatusComputionFlags::RemoteSynchronizing;
    autoConnect = advanced.value(QLatin1String("autoConnect")).toBool(true);
    pauseOnMeteredConnection = advanced.value(QLatin1String("pauseOnMeteredConnection")).toBool();

#ifndef QT_NO_SSL
    httpsCertPath = object.value(QLatin1String("httpsCertPath")).toString();
    httpCertLastModified = QDateTime();
    return loadHttpsCert();
#endif
}

} // namespace Data
