#ifndef DATA_SYNCTHINGICONS_H
#define DATA_SYNCTHINGICONS_H

#include "./global.h"

#include <QIcon>
#include <QSize>

QT_FORWARD_DECLARE_CLASS(QColor)

namespace Data {

struct StatusIcons {
    StatusIcons();
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

QPixmap LIB_SYNCTHING_MODEL_EXPORT renderSvgImage(const QString &path, const QSize &size = QSize(128, 128), int margin = 0);
QPixmap LIB_SYNCTHING_MODEL_EXPORT renderSvgImage(const QByteArray &contents, const QSize &size = QSize(128, 128), int margin = 0);
QByteArray LIB_SYNCTHING_MODEL_EXPORT loadFontAwesomeIcon(const QString &iconName, const QColor &color, bool solid = true);
const StatusIcons LIB_SYNCTHING_MODEL_EXPORT &statusIcons();
const FontAwesomeIcons LIB_SYNCTHING_MODEL_EXPORT &fontAwesomeIconsForLightTheme();
const FontAwesomeIcons LIB_SYNCTHING_MODEL_EXPORT &fontAwesomeIconsForDarkTheme();

} // namespace Data

#endif // DATA_SYNCTHINGICONS_H
