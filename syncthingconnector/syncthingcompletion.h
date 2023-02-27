#ifndef DATA_SYNCTHING_COMPLETION_H
#define DATA_SYNCTHING_COMPLETION_H

#include "./global.h"

#include <c++utilities/chrono/datetime.h>

#include <QtGlobal>

namespace Data {

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingCompletion {
    CppUtilities::DateTime lastUpdate;
    double percentage = 0;
    quint64 globalBytes = 0;
    struct Needed {
        quint64 bytes = 0;
        quint64 items = 0;
        quint64 deletes = 0;
        constexpr bool isNull() const;
        constexpr bool operator==(const Needed &other) const;
        constexpr bool operator!=(const Needed &other) const;
        constexpr Needed &operator+=(const Needed &other);
        constexpr Needed &operator-=(const Needed &other);
    } needed;
    bool requested = false;
    constexpr SyncthingCompletion &operator+=(const SyncthingCompletion &other);
    constexpr SyncthingCompletion &operator-=(const SyncthingCompletion &other);
    void recomputePercentage();
};

constexpr bool SyncthingCompletion::Needed::isNull() const
{
    return bytes == 0 && items == 0 && deletes == 0;
}

constexpr bool SyncthingCompletion::Needed::operator==(const SyncthingCompletion::Needed &other) const
{
    return bytes == other.bytes && items == other.items && deletes == other.deletes;
}

constexpr bool SyncthingCompletion::Needed::operator!=(const SyncthingCompletion::Needed &other) const
{
    return !(*this == other);
}

constexpr SyncthingCompletion::Needed &SyncthingCompletion::Needed::operator+=(const SyncthingCompletion::Needed &other)
{
    bytes += other.bytes;
    items += other.items;
    deletes += other.deletes;
    return *this;
}

constexpr SyncthingCompletion::Needed &SyncthingCompletion::Needed::operator-=(const SyncthingCompletion::Needed &other)
{
    bytes -= other.bytes;
    items -= other.items;
    deletes -= other.deletes;
    return *this;
}

constexpr SyncthingCompletion &SyncthingCompletion::operator+=(const SyncthingCompletion &other)
{
    lastUpdate = std::max(lastUpdate, other.lastUpdate);
    globalBytes += other.globalBytes;
    needed += other.needed;
    return *this;
}

constexpr SyncthingCompletion &SyncthingCompletion::operator-=(const SyncthingCompletion &other)
{
    globalBytes -= other.globalBytes;
    needed -= other.needed;
    return *this;
}

inline void SyncthingCompletion::recomputePercentage()
{
    percentage = (static_cast<double>(globalBytes - needed.bytes) / static_cast<double>(globalBytes)) * 100.0;
}

} // namespace Data

#endif // DATA_SYNCTHING_COMPLETION_H
