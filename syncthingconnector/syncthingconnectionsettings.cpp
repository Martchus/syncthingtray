#include "./syncthingconnectionsettings.h"

namespace Data {

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
    const auto certs(QSslCertificate::fromPath(httpsCertPath));
    if (certs.isEmpty() || certs.at(0).isNull()) {
        return false;
    }

    expectedSslErrors = compileSslErrors(certs.at(0));
    return true;
}
} // namespace Data
