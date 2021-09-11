#ifndef DATA_SYNCTHINGICONS_H
#define DATA_SYNCTHINGICONS_H

#include "./global.h"

#include <qtforkawesome/qtforkawesome.h>

#include <QIcon>
#include <QSize>

#include <vector>

QT_FORWARD_DECLARE_CLASS(QColor)

namespace Data {

enum class StatusEmblem {
    None,
    Scanning,
    Synchronizing,
    Alert,
    Paused,
    Complete,
    Add,
};

struct LIB_SYNCTHING_MODEL_EXPORT StatusIconColorSet {
    StatusIconColorSet(const QColor &backgroundStart, const QColor &backgroundEnd, const QColor &foreground);
    StatusIconColorSet(QColor &&backgroundStart, QColor &&backgroundEnd, QColor &&foreground);
    StatusIconColorSet(const QString &backgroundStart, const QString &backgroundEnd, const QString &foreground);

    QColor backgroundStart;
    QColor backgroundEnd;
    QColor foreground;
};

inline StatusIconColorSet::StatusIconColorSet(const QColor &backgroundStart, const QColor &backgroundEnd, const QColor &foreground)
    : backgroundStart(backgroundStart)
    , backgroundEnd(backgroundEnd)
    , foreground(foreground)
{
}

inline StatusIconColorSet::StatusIconColorSet(QColor &&backgroundStart, QColor &&backgroundEnd, QColor &&foreground)
    : backgroundStart(backgroundStart)
    , backgroundEnd(backgroundEnd)
    , foreground(foreground)
{
}

inline StatusIconColorSet::StatusIconColorSet(const QString &backgroundStart, const QString &backgroundEnd, const QString &foreground)
    : backgroundStart(backgroundStart)
    , backgroundEnd(backgroundEnd)
    , foreground(foreground)
{
}

LIB_SYNCTHING_MODEL_EXPORT QByteArray makeSyncthingIcon(
    const StatusIconColorSet &colors = StatusIconColorSet{ QStringLiteral("#26B6DB"), QStringLiteral("#0882C8"), QStringLiteral("#FFFFFF") },
    StatusEmblem statusEmblem = StatusEmblem::None);
LIB_SYNCTHING_MODEL_EXPORT QPixmap renderSvgImage(const QString &path, const QSize &size = QSize(32, 32), int margin = 0);
LIB_SYNCTHING_MODEL_EXPORT QPixmap renderSvgImage(const QByteArray &contents, const QSize &size = QSize(32, 32), int margin = 0);

struct LIB_SYNCTHING_MODEL_EXPORT StatusIconSettings {
    struct DarkTheme {
    };
    struct BrightTheme {
    };

    explicit StatusIconSettings();
    explicit StatusIconSettings(DarkTheme);
    explicit StatusIconSettings(BrightTheme);
    explicit StatusIconSettings(const QString &str);

    StatusIconColorSet defaultColor;
    StatusIconColorSet errorColor;
    StatusIconColorSet warningColor;
    StatusIconColorSet idleColor;
    StatusIconColorSet scanningColor;
    StatusIconColorSet synchronizingColor;
    StatusIconColorSet pausedColor;
    StatusIconColorSet disconnectedColor;
    QSize renderSize = QSize(32, 32);

    static constexpr auto distinguishableColorCount = 8;

    struct ColorMapping {
        QString colorName;
        StatusEmblem defaultEmblem;
        StatusIconColorSet &setting;
    };
    std::vector<ColorMapping> colorMapping();
    QString toString() const;
};

struct LIB_SYNCTHING_MODEL_EXPORT StatusIcons {
    StatusIcons();
    StatusIcons(const StatusIconSettings &settings);
    StatusIcons(const StatusIcons &other) = default;
    StatusIcons &operator=(const StatusIcons &other) = default;
    QIcon disconnected;
    QIcon idling;
    QIcon scanninig;
    QIcon notify;
    QIcon pause;
    QIcon sync;
    QIcon syncComplete;
    QIcon error;
    QIcon errorSync;
    QIcon newItem;
    bool isValid;
};

inline StatusIcons::StatusIcons()
    : isValid(false)
{
}

struct LIB_SYNCTHING_MODEL_EXPORT ForkAwesomeIcons {
    ForkAwesomeIcons(QtForkAwesome::Renderer &renderer, const QColor &color, const QSize &size);
    QIcon hashtag;
    QIcon folderOpen;
    QIcon globe;
    QIcon home;
    QIcon shareAlt;
    QIcon refresh;
    QIcon clock;
    QIcon exchangeAlt;
    QIcon exclamationTriangle;
    QIcon cogs;
    QIcon link;
    QIcon eye;
    QIcon fileArchive;
    QIcon folder;
    QIcon certificate;
    QIcon networkWired;
    QIcon cloudDownloadAlt;
    QIcon cloudUploadAlt;
    QIcon tag;
};

class LIB_SYNCTHING_MODEL_EXPORT IconManager : public QObject {
    Q_OBJECT
public:
    static IconManager &instance();

    void applySettings(const StatusIconSettings *statusIconSettings = nullptr, const StatusIconSettings *trayIconSettings = nullptr);
    const StatusIcons &statusIcons() const;
    const StatusIcons &trayIcons() const;
    QtForkAwesome::Renderer &forkAwesomeRenderer();
    const ForkAwesomeIcons &forkAwesomeIconsForLightTheme() const;
    const ForkAwesomeIcons &forkAwesomeIconsForDarkTheme() const;

Q_SIGNALS:
    void statusIconsChanged(const StatusIcons &newStatusIcons, const StatusIcons &newTrayIcons);

private:
    IconManager();

    StatusIcons m_statusIcons;
    StatusIcons m_trayIcons;
    QtForkAwesome::Renderer m_forkAwesomeRenderer;
    ForkAwesomeIcons m_forkAwesomeIconsForLightTheme;
    ForkAwesomeIcons m_fontAwesomeIconsForDarkTheme;
};

inline void IconManager::applySettings(const StatusIconSettings *statusIconSettings, const StatusIconSettings *trayIconSettings)
{
    if (statusIconSettings) {
        m_statusIcons = StatusIcons(*statusIconSettings);
    } else {
        m_statusIcons = StatusIcons(StatusIconSettings());
    }
    if (trayIconSettings) {
        m_trayIcons = StatusIcons(*trayIconSettings);
    } else {
        m_trayIcons = m_statusIcons;
    }
    emit statusIconsChanged(m_statusIcons, m_trayIcons);
}

inline const StatusIcons &IconManager::statusIcons() const
{
    return m_statusIcons;
}

inline const StatusIcons &IconManager::trayIcons() const
{
    return m_trayIcons;
}

inline QtForkAwesome::Renderer &IconManager::forkAwesomeRenderer()
{
    return m_forkAwesomeRenderer;
}

inline const ForkAwesomeIcons &IconManager::forkAwesomeIconsForLightTheme() const
{
    return m_forkAwesomeIconsForLightTheme;
}

inline const ForkAwesomeIcons &IconManager::forkAwesomeIconsForDarkTheme() const
{
    return m_fontAwesomeIconsForDarkTheme;
}

inline const StatusIcons &statusIcons()
{
    return IconManager::instance().statusIcons();
}

inline const StatusIcons &trayIcons()
{
    return IconManager::instance().trayIcons();
}

inline const ForkAwesomeIcons &forkAwesomeIconsForLightTheme()
{
    return IconManager::instance().forkAwesomeIconsForLightTheme();
}

inline const ForkAwesomeIcons &forkAwesomeIconsForDarkTheme()
{
    return IconManager::instance().forkAwesomeIconsForDarkTheme();
}

} // namespace Data

#endif // DATA_SYNCTHINGICONS_H
