#ifndef DATA_SYNCTHINGICONS_H
#define DATA_SYNCTHINGICONS_H

#include "./global.h"

#include <QIcon>
#include <QSize>

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

struct GradientColor {
    QString start;
    QString end;
};

QByteArray LIB_SYNCTHING_MODEL_EXPORT makeSyncthingIcon(const GradientColor &gradientColor, StatusEmblem statusEmblem);
QPixmap LIB_SYNCTHING_MODEL_EXPORT renderSvgImage(const QString &path, const QSize &size = QSize(128, 128), int margin = 0);
QPixmap LIB_SYNCTHING_MODEL_EXPORT renderSvgImage(const QByteArray &contents, const QSize &size = QSize(128, 128), int margin = 0);
QByteArray LIB_SYNCTHING_MODEL_EXPORT loadFontAwesomeIcon(const QString &iconName, const QColor &color, bool solid = true);

struct StatusIconSettings {
    StatusIconSettings();

    GradientColor defaultColor;
    GradientColor errorColor;
    GradientColor warningColor;
    GradientColor idleColor;
    GradientColor disconnectedColor;
};

struct StatusIcons {
    StatusIcons(const StatusIconSettings &settings = StatusIconSettings());
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
};

struct FontAwesomeIcons {
    FontAwesomeIcons(const QColor &color, const QSize &size, int margin);
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

class LIB_SYNCTHING_MODEL_EXPORT IconManager {
public:
    static IconManager &instance(const StatusIconSettings *settingsForFirstTimeSetup = nullptr);

    void applySettings(const StatusIconSettings &settings);
    const StatusIcons &statusIcons() const;
    const FontAwesomeIcons &fontAwesomeIconsForLightTheme() const;
    const FontAwesomeIcons &fontAwesomeIconsForDarkTheme() const;

private:
    IconManager(const StatusIconSettings *settings = nullptr);

    StatusIcons m_statusIcons;
    FontAwesomeIcons m_fontAwesomeIconsForLightTheme;
    FontAwesomeIcons m_fontAwesomeIconsForDarkTheme;
};

inline void IconManager::applySettings(const StatusIconSettings &settings)
{
    m_statusIcons = StatusIcons(settings);
}

inline const StatusIcons &IconManager::statusIcons() const
{
    return m_statusIcons;
}

inline const FontAwesomeIcons &IconManager::fontAwesomeIconsForLightTheme() const
{
    return m_fontAwesomeIconsForLightTheme;
}

inline const FontAwesomeIcons &IconManager::fontAwesomeIconsForDarkTheme() const
{
    return m_fontAwesomeIconsForDarkTheme;
}

inline const StatusIcons &statusIcons()
{
    return IconManager::instance().statusIcons();
}

inline const FontAwesomeIcons &fontAwesomeIconsForLightTheme()
{
    return IconManager::instance().fontAwesomeIconsForLightTheme();
}

inline const FontAwesomeIcons &fontAwesomeIconsForDarkTheme()
{
    return IconManager::instance().fontAwesomeIconsForDarkTheme();
}

} // namespace Data

#endif // DATA_SYNCTHINGICONS_H
