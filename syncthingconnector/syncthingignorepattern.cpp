#include "./syncthingignorepattern.h"

namespace Data {

/// \cond
namespace SyncthingIgnorePatternState {
enum State {
    Escaping, // passed the esacping character "\"
    AppendRangeLower, // passed a "[" marking the start of a character range
    AppendRangeUpper, // passed a "-" within a range marking the start of an upper-bound range character
    AppendAlternative, // passed a "{" marking the start of an alternative set
    MatchVerbatimly, // initial/default state
    MatchRange, // passed a "]" marking the end of a character range
    MatchAlternatives, // passed a "}" marking the end of an alternative set
    MatchAny, // passed the "?" character that allows matching any character but the path separator
    MatchManyAny, // passed the "*" character that allows matching many arbitrary characters except the path separator
    MatchManyAnyIncludingDirSep, // passed a sequence of two "*" characters allowing to match also the path separator
};

struct Asterisk {
    QString::const_iterator pos;
    SyncthingIgnorePatternState::State state;
    bool visited = false;
};

struct CharacterRange {
    QChar lowerBound, upperBound;
};

struct AlternativeRange {
    explicit AlternativeRange(QString::const_iterator beg)
        : beg(beg)
        , end(beg)
    {
    }
    QString::const_iterator beg, end;
};
} // namespace SyncthingIgnorePatternState
/// \endcond

/*!
 * \struct SyncthingIgnorePattern
 * \brief The SyncthingIgnorePattern struct allows matching a Syncthing ignore pattern against a path.
 * \remarks
 * - The `#include`-syntax is not supported.
 * - A "/" is always treated as path separator within the pattern. Additionally, the character specified
 *   when calling matches() is treated as path separator as well. This means that under Windows where
 *   patterns can contain a "\" one *must* specify paths with a "\" as separator when invoking the matches()
 *   function to allow patterns using "/" and "\" to match correctly; otherwiise ignore patterns containing
 *   "\" do not work.
 * \sa
 * - https://docs.syncthing.net/users/ignoring.html
 * - https://docs.syncthing.net/rest/db-ignores-get.html
 */

/*!
 * \brief Parses the specified \a pattern populating the struct's fields.
 */
SyncthingIgnorePattern::SyncthingIgnorePattern(QString &&pattern)
    : pattern(std::move(pattern))
{
    if (this->pattern.startsWith(QLatin1String("//"))) {
        comment = true;
        ignore = false;
        return;
    }
    glob = this->pattern;
    for (;;) {
        if (glob.startsWith(QLatin1String("!"))) {
            ignore = !ignore;
            glob.remove(0, 1);
        } else if (glob.startsWith(QLatin1String("(?i)"))) {
            caseInsensitive = true;
            glob.remove(0, 4);
        } else if (glob.startsWith(QLatin1String("(?d)"))) {
            allowRemovalOnParentDirRemoval = true;
            glob.remove(0, 4);
        } else {
            break;
        }
    }
}

/*!
 * \brief Moves the ignore pattern.
 */
SyncthingIgnorePattern::SyncthingIgnorePattern(SyncthingIgnorePattern &&) = default;

/*!
 * \brief Destroys the ignore pattern.
 */
SyncthingIgnorePattern::~SyncthingIgnorePattern()
{
}

/*!
 * \brief Matches the assigned glob against the specified \a path.
 * \remarks
 * - Returns always false if the pattern is flagged as comment or the glob is empty.
 * - This function tries to follow rules outlined on https://docs.syncthing.net/users/ignoring.html.
 * - The specified \a path is *not* supposed to start with the \a pathSeparator (or a "/"). It must always be a path
 *   relative to the root of the Syncthing folder it is contained by. A pattern that is only supposed to match from the
 *   root of the Syncthing folder is supposed to start with \a pathSeparator (or a "/"), though.
 * - This function probably doesn't work if the pattern or \a path contain a surrogate pair.
 * - By default, the path separator is "/". If \a path uses a different separator you must specify it as second argument.
 *   Note that "/" is still be treated as path separator in this case and the specified \a pathSeparator is only used in
 *   addition. This is intended because this way one can specify "\" as \a pathSeparator under Windows but patterns using
 *   "/" still work (as they should because "/" and "\" can both be used as path separator under Windows in ignore
 *   patterns).
 */
bool SyncthingIgnorePattern::matches(const QString &path, QChar pathSeparator) const
{
    if (comment || glob.isEmpty()) {
        return false;
    }

    // get iterators
    auto globIter = glob.begin(), globEnd = glob.end();
    auto pathIter = path.begin(), pathEnd = path.end();

    // handle pattners starting with "/" indicating the pattern must match from the root (see last remark in docstring)
    static
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        constexpr
#else
        const
#endif
        auto genericPathSeparator
        = QChar('/');
    const auto matchFromRoot = *globIter == pathSeparator || *globIter == genericPathSeparator;
    if (matchFromRoot) {
        ++globIter;
    }

    // define variables to track the state when processing the glob pattern
    using namespace SyncthingIgnorePatternState;
    auto state = MatchVerbatimly;
    auto escapedState = MatchVerbatimly;
    auto inAsterisk = false;
    auto wasInAsterisk = false;
    asterisks.clear();

    // define behavior to handle the current character in the glob pattern not matching the current pattern in the path
    const auto handleMismatch = [&, this] {
        // fail the match immediately if the glob pattern started with a "/" indicating it is supposed to match only from the root
        if (matchFromRoot) {
            return false;
        }
        // deal with the mismatch by trying to match previous asterisks more greedily
        while (!asterisks.empty() && asterisks.back().visited) {
            // do not consider asterisks we have already visited, though (as it would lead to an endless loop)
            asterisks.pop_back();
        }
        if (!asterisks.empty()) {
            // rewind back to when we have passed the last non-visited asterisk
            auto &asterisk = asterisks.back();
            globIter = asterisk.pos;
            inAsterisk = asterisk.visited = true;
            state = asterisk.state;
            return true;
        }
        // deal with the mismatch by checking the path as of the next path element
        for (; pathIter != pathEnd; ++pathIter) {
            // forward to the next path separator
            if (*pathIter != pathSeparator && *pathIter != genericPathSeparator) {
                continue;
            }
            // skip the path separator itself and give up when the end of the path is reached
            if (++pathIter == pathEnd) {
                return false;
            }
            break;
        }
        // give up when the end of the path is reached
        if (pathIter == pathEnd) {
            return false;
        }
        // start matching the glob pattern from the beginning
        globIter = glob.begin();
        asterisks.clear();
        inAsterisk = wasInAsterisk = false;
        return true;
    };

    // define function to handle single match
    const auto handleSingleMatch = [&, this] {
        // proceed with the next characters on a match
        const auto matchedChar = *(globIter++);
        // consider the asterisk dealt with after a single match
        if (inAsterisk || !(matchedChar == pathSeparator || matchedChar == genericPathSeparator)) {
            // take note that we were at an asterisk when it was a double asterisk (that allows matching the dir sep)
            wasInAsterisk = !asterisks.empty() && asterisks.back().state == MatchManyAnyIncludingDirSep && inAsterisk;
            inAsterisk = false;
            if (!asterisks.empty()) {
                asterisks.back().visited = false;
            }
        }
    };

    // define function to match single character against the current character in the path
    static constexpr auto compareSingleCharBasic = [](QChar expectedChar, QChar presentChar, QChar pathSep, bool caseInsensitive) {
        Q_UNUSED(pathSep)
        return caseInsensitive ? presentChar.toCaseFolded() == expectedChar.toCaseFolded() : presentChar == expectedChar;
    };
    const auto compareSingleChar = pathSeparator == genericPathSeparator
        ? compareSingleCharBasic
        : [](QChar expectedChar, QChar presentChar, QChar pathSep, bool caseInsensitive) {
              return compareSingleCharBasic(expectedChar, presentChar, pathSep, caseInsensitive)
                  || (expectedChar == genericPathSeparator && presentChar == pathSep);
          };
    const auto matchSingleChar = [&, this](QChar expectedChar) { return compareSingleChar(expectedChar, *pathIter, pathSeparator, caseInsensitive); };

    // define function to transition to verbatim matching (which makes only sense when not in any of the "Append…"-states)
    const auto transitionToVerbatimMatching = [&] {
        switch (state) {
        case AppendRangeLower:
        case AppendRangeUpper:
        case AppendAlternative:
            break;
        default:
            state = MatchVerbatimly;
        }
    };

    // try to match each character of the glob against a character in the path
match:
    while (globIter != globEnd) {
        // decide what to do next depending on the current glob pattern character and state transitioning the state accordingly
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        static constexpr auto escapeCharacter = QChar('\\');
        static constexpr auto escapeCharacterUnicode = escapeCharacter.unicode();
#else
#define escapeCharacterUnicode '\\'
#define escapeCharacter QChar(escapeCharacterUnicode)
#endif
        switch (state) {
        case Escaping:
            // treat every character as-is in "escaping" state
            state = escapedState;
            break;
        default:
            // transition state according to special meaning of the current glob pattern character
            switch (globIter->unicode()) {
            case escapeCharacterUnicode:
                if (pathSeparator != escapeCharacter) {
                    // transition into "escaping" state
                    escapedState = state;
                    state = Escaping;
                } else {
                    // treat the escape character as normal character if it is the path separator
                    // quote from Syncthing documentation: Escaped characters are not supported on Windows, where "\" is the path
                    //                                     separator.
                    transitionToVerbatimMatching();
                }
                break;
            case '[':
                state = AppendRangeLower;
                characterRange.clear();
                ++globIter;
                continue;
            case ']':
                switch (state) {
                case AppendRangeLower:
                case AppendRangeUpper:
                    state = MatchRange;
                    break;
                default:
                    transitionToVerbatimMatching();
                }
                break;
            case '-':
                switch (state) {
                case AppendRangeLower:
                    state = AppendRangeUpper;
                    ++globIter;
                    continue;
                default:
                    transitionToVerbatimMatching();
                }
                break;
            case '{':
                switch (state) {
                case AppendAlternative:
                    continue;
                default:
                    state = AppendAlternative;
                    alternatives.clear();
                    alternatives.emplace_back(++globIter);
                }
                continue;
            case '}':
                switch (state) {
                case AppendAlternative:
                    alternatives.back().end = globIter;
                    state = MatchAlternatives;
                    break;
                default:
                    transitionToVerbatimMatching();
                }
                break;
            case ',':
                switch (state) {
                case AppendAlternative:
                    alternatives.back().end = globIter;
                    alternatives.emplace_back(++globIter);
                    continue;
                default:
                    transitionToVerbatimMatching();
                }
                break;
            case '?':
                // transition into "match any" state
                state = MatchAny;
                break;
            case '*':
                // transition into one of the "match many any" state (depending on current state)
                switch (state) {
                case MatchManyAny:
                    state = MatchManyAnyIncludingDirSep;
                    break;
                default:
                    state = MatchManyAny;
                }
                break;
            default:
                // try to match/append all other non-special characters as-is
                transitionToVerbatimMatching();
            }
        }

        // proceed according to state
        switch (state) {
        case Escaping:
            // proceed with the next character in the glob pattern which will be matched as-is (even if it is special)
            [[fallthrough]];
        case AppendAlternative:
            // just move on to the next character (alternatives are populated completely in the previous switch-case)
            ++globIter;
            break;
        case AppendRangeLower:
            // add the current character in the glob pattern as start of a new range
            characterRange.emplace_back().lowerBound = *globIter++;
            break;
        case AppendRangeUpper:
            // add the current character in the glob pattern as end of a new or the current range
            (characterRange.empty() ? characterRange.emplace_back() : characterRange.back()).upperBound = *globIter++;
            state = AppendRangeLower;
            break;
        case MatchVerbatimly:
            // match the current character in the glob pattern verbatimly against the current character in the path
            if (pathIter != pathEnd && matchSingleChar(*globIter)) {
                ++pathIter;
                handleSingleMatch();
            } else if ((wasInAsterisk || inAsterisk)
                && (asterisks.back().state == MatchManyAnyIncludingDirSep
                    || (pathIter == pathEnd || (*pathIter != pathSeparator && *pathIter != genericPathSeparator)))) {
                if (wasInAsterisk) {
                    // match further characters via the asterisk we thought we have dealt with after all
                    --globIter; // get back to where we were in the glob before considering the asterisk dealt with
                    inAsterisk = true;
                    wasInAsterisk = false;
                }
                // consider the path character dealt with despite no match if we have just passed an asterisk in the glob pattern
                if (pathIter != pathEnd) {
                    ++pathIter;
                } else {
                    inAsterisk = false;
                }
            } else if (!handleMismatch()) {
                return false;
            }
            break;
        case MatchRange:
            // match the concluded character range in the glob pattern against the current character in the path
            if (pathIter != pathEnd) {
                auto inRange = false;
                for (const auto &bounds : characterRange) {
                    if ((!bounds.upperBound.isNull() && *pathIter >= bounds.lowerBound && *pathIter <= bounds.upperBound)
                        || (bounds.upperBound.isNull() && matchSingleChar(bounds.lowerBound))) {
                        inRange = true;
                        break;
                    }
                }
                if (inRange) {
                    characterRange.clear();
                    state = MatchVerbatimly;
                    ++pathIter;
                    handleSingleMatch();
                    break;
                }
            }
            if (!handleMismatch()) {
                return false;
            }
            break;
        case MatchAlternatives:
            // match the current alternatives as of the current character in the path
            if (pathIter != pathEnd) {
                const auto pathStart = pathIter;
                for (auto &alternative : alternatives) {
                    // match characters in the alternative against the path
                    // note: Special characters like "*" are matched verbatimly. Is that the correct behavior?
                    pathIter = pathStart;
                    for (; alternative.beg != alternative.end && pathIter != pathEnd; ++alternative.beg) {
                        if (*alternative.beg == escapeCharacter) {
                            continue;
                        }
                        if (!matchSingleChar(*alternative.beg)) {
                            break;
                        }
                        ++pathIter;
                    }
                    // go with the first alternative that fully matched
                    // note: What is the correct behavior? Should this be most/least greedy (matching the longest/shortest possible alternative) instead?
                    if (alternative.beg == alternative.end) {
                        alternatives.clear();
                        break;
                    }
                }
                if (alternatives.empty()) {
                    state = MatchVerbatimly;
                    handleSingleMatch();
                    break;
                }
            }
            if (!handleMismatch()) {
                return false;
            }
            break;
        case MatchAny:
            // allow the current character in the path to be anything but a path separator; otherwise consider it as mismatch as in the case for an exact match
            if (pathIter == pathEnd || (*pathIter != pathSeparator && *pathIter != genericPathSeparator)) {
                ++globIter, ++pathIter;
            } else if (!handleMismatch()) {
                return false;
            }
            break;
        case MatchManyAny: {
            // take record of the asterisks
            auto &glob = asterisks.emplace_back();
            glob.pos = ++globIter;
            glob.state = MatchManyAny;
            inAsterisk = true;
            break;
        }
        case MatchManyAnyIncludingDirSep: {
            // take record of the second asterisks
            auto &glob = asterisks.back();
            glob.pos = ++globIter;
            glob.state = MatchManyAnyIncludingDirSep;
            break;
        }
        }
    }

    // check whether all characters of the glob have been matched against all characters of the path
    if (globIter == globEnd) {
        // consider the match a success if all characters of the path were matched or the glob ended with a "**"
        if (pathIter == pathEnd || state == MatchManyAnyIncludingDirSep) {
            return true;
        }
        if (const auto remainingPath = QStringView(pathIter, pathEnd);
            state == MatchManyAny && !(remainingPath.contains(pathSeparator) || remainingPath.contains(genericPathSeparator))) {
            return true;
        }

        // try again as of the next path segment if the glob fully matched but there are still characters in the path to be matched
        // note: This allows "foo" to match against "foo/foo" even tough the glob characters have already consumed after matching the first path segment.
        if (!matchFromRoot && (*pathIter == pathSeparator || *pathIter == genericPathSeparator)) {
            state = MatchVerbatimly;
            ++pathIter;
            globIter = glob.begin();
            asterisks.clear();
            inAsterisk = false;
            goto match;
        }
    }
    return false;
}

/*!
 * \brief Makes an ignore pattern for \a path with the specified settings.
 */
QString SyncthingIgnorePattern::forPath(const QString &path, bool ignore, bool caseInsensitive, bool allowRemovalOnParentDirRemoval)
{
    auto res = QString();
    res.reserve(10 + path.size());
    if (!ignore) {
        res += QChar('!');
    }
    if (caseInsensitive) {
        res += QStringLiteral("(?i)");
    }
    if (allowRemovalOnParentDirRemoval) {
        res += QStringLiteral("(?d)");
    }
    return res += path;
}

} // namespace Data
