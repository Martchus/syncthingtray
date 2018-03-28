#include "./syncthingconnectionsettings.h"

namespace Data {

bool SyncthingConnectionSettings::loadHttpsCert()
{
    expectedSslErrors.clear();
    if (httpsCertPath.isEmpty()) {
        return true;
    }
    const auto certs(QSslCertificate::fromPath(httpsCertPath));
    if (certs.isEmpty()) {
        return false;
    }
    const auto &cert(certs.front());
    // clang-format off
    expectedSslErrors = {
        QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert),
        QSslError(QSslError::UnableToVerifyFirstCertificate, cert),
        QSslError(QSslError::SelfSignedCertificate, cert),
        QSslError(QSslError::HostNameMismatch, cert)
    };
    // clang-format on
    return true;
}
} // namespace Data
