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
 * \brief Parses the specified \a pattern populating the struct's fields.
 */
SyncthingIgnorePattern::SyncthingIgnorePattern(QString &&pattern)
    : pattern(std::move(pattern))
{
    if (glob.startsWith(QLatin1String("//"))) {
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
 * - The specified \a path is *not* supposed to start with a "/". It must always be a path relative to the root of the
 *   Syncthing folder it is contained by. A pattern that is only supposed to match from the root of the Syncthing folder
 *   is supposed to start with a "/", though.
 * - This function probably doesn't work if the pattern or \a path contain a surrogate pair.
 */
bool SyncthingIgnorePattern::matches(const QString &path) const
{
    if (comment || glob.isEmpty()) {
        return false;
    }

    // get iterators
    auto globIter = glob.begin(), globEnd = glob.end();
    auto pathIter = path.begin(), pathEnd = path.end();

    // handle pattners starting with "/" indicating the pattern must match from the root (see last remark in docstring)
    static constexpr auto pathSep = QChar('/');
    const auto matchFromRoot = *globIter == pathSep;
    if (matchFromRoot) {
        ++globIter;
    }

    // define variables to track the state when processing the glob pattern
    using namespace SyncthingIgnorePatternState;
    auto state = MatchVerbatimly;
    auto escapedState = MatchVerbatimly;
    auto inAsterisk = false;
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
            if (*pathIter != pathSep) {
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
        inAsterisk = false;
        return true;
    };

    // define function to handle single match
    const auto handleSingleMatch = [&, this] {
        // proceed with the next characters on a match
        ++globIter;
        inAsterisk = false;
        if (!asterisks.empty()) {
            asterisks.back().visited = false;
        }
    };

    // define function to match single character against the current character in the path
    const auto matchSingleChar
        = [&, this](QChar singleChar) { return caseInsensitive ? pathIter->toCaseFolded() == singleChar.toCaseFolded() : *pathIter == singleChar; };

    // try to match each character of the glob against a character in the path
match:
    while (globIter != globEnd) {
        // decide what to do next depending on the current glob pattern character and state transitioning the state accordingly
        switch (state) {
        case Escaping:
            // treat every character as-is in "escaping" state
            state = escapedState;
            break;
        default:
            // transition state according to special meaning of the current glob pattern character
            switch (globIter->unicode()) {
            case '\\':
                // transition into "escaping" state
                escapedState = state;
                state = Escaping;
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
                    state = MatchVerbatimly;
                }
                break;
            case '-':
                switch (state) {
                case AppendRangeLower:
                    state = AppendRangeUpper;
                    ++globIter;
                    continue;
                default:
                    state = MatchVerbatimly;
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
                    state = MatchVerbatimly;
                }
                break;
            case ',':
                switch (state) {
                case AppendAlternative:
                    alternatives.back().end = globIter;
                    alternatives.emplace_back(++globIter);
                    continue;
                default:
                    state = MatchVerbatimly;
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
                switch (state) {
                case AppendRangeLower:
                case AppendRangeUpper:
                case AppendAlternative:
                    break;
                default:
                    state = MatchVerbatimly;
                }
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
            } else if (inAsterisk && (asterisks.back().state == MatchManyAnyIncludingDirSep || (pathIter == pathEnd || *pathIter != pathSep))) {
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
                        if (*alternative.beg == QChar('\\')) {
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
            if (pathIter == pathEnd || *pathIter != pathSep) {
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
        if (pathIter == pathEnd) {
            return true;
        }
        // try again as of the next path segment if the glob fully matched but there are still characters in the path to be matched
        // note: This allows "foo" to match against "foo/foo" even tough the glob characters have already consumed after matching the first path segment.
        if (!matchFromRoot && *pathIter == pathSep) {
            state = MatchVerbatimly;
            ++pathIter;
            globIter = glob.begin();
            asterisks.clear();
            inAsterisk = false;
            goto match;
        }
    }

    // consider the match a success if there are still characters in the path to be matched but the glob ended with a "*"
    return state == MatchManyAny || state == MatchManyAnyIncludingDirSep;
}

} // namespace Data
