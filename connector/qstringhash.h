#ifndef SYNCTHING_CONNECTOR_QSTRING_HASH_H
#define SYNCTHING_CONNECTOR_QSTRING_HASH_H

#include <QHash>
#include <QString>

#include <functional>

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
namespace std {

template <> struct hash<QString> {
    inline std::size_t operator()(const QString &str) const
    {
        return qHash(str);
    }
};

} // namespace std
#endif

#endif // SYNCTHING_CONNECTOR_QSTRING_HASH_H
