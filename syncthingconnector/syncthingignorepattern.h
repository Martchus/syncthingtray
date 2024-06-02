#ifndef DATA_SYNCTHINGIGNOREPATTERN_H
#define DATA_SYNCTHINGIGNOREPATTERN_H

#include "./global.h"

#include <QString>

#include <vector>

namespace Data {

namespace SyncthingIgnorePatternState {
struct Asterisk;
struct CharacterRange;
struct AlternativeRange;
} // namespace SyncthingIgnorePatternState

struct LIB_SYNCTHING_CONNECTOR_EXPORT SyncthingIgnorePattern {
    explicit SyncthingIgnorePattern(QString &&pattern);
    SyncthingIgnorePattern(const SyncthingIgnorePattern &) = delete;
    SyncthingIgnorePattern(SyncthingIgnorePattern &&);
    ~SyncthingIgnorePattern();
    bool matches(const QString &path, QChar pathSeparator = QChar('/')) const;
    static QString forPath(const QString &path, bool ignore = true, bool caseInsensitive = false, bool allowRemovalOnParentDirRemoval  = false);

    /// \brief The full ignore pattern as passed to the c'tor (unless modified).
    QString pattern;
    /// \brief The part of the pattern that will actually be used for globbing by the matches() function.
    QString glob;
    /// \brief Whether the pattern is a comment.
    bool comment = false;
    /// \brief Whether the pattern will lead to ignoring matching items (the default).
    bool ignore = true;
    /// \brief Whether the pattern will match case-insensetively.
    bool caseInsensitive = false;
    /// \brief Whether matching items may be removed when the otherwise empty parent directory would be removed.
    bool allowRemovalOnParentDirRemoval = false;

private:
    mutable std::vector<SyncthingIgnorePatternState::Asterisk> asterisks;
    mutable std::vector<SyncthingIgnorePatternState::CharacterRange> characterRange;
    mutable std::vector<SyncthingIgnorePatternState::AlternativeRange> alternatives;
};

} // namespace Data

#endif // DATA_SYNCTHINGIGNOREPATTERN_H
