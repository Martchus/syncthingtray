#include "./syncthingicons.h"

#include <QPainter>
#include <QSvgRenderer>

namespace Data {

/*!
 * \brief Renders an SVG image to a QPixmap.
 * \remarks If instantiating QIcon directly from SVG image the icon is not displayed in the tray under Plasma 5. It works
 *          with Tint2, however.
 */
QPixmap renderSvgImage(const QString &path, const QSize &size)
{
    QSvgRenderer renderer(path);
    QPixmap pm(size);
    pm.fill(QColor(Qt::transparent));
    QPainter painter(&pm);
    renderer.render(&painter, pm.rect());
    return pm;
}

StatusIcons::StatusIcons()
    : disconnected(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-disconnected.svg"))))
    , idling(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-ok.svg"))))
    , scanninig(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-default.svg"))))
    , notify(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-notify.svg"))))
    , pause(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-pause.svg"))))
    , sync(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-sync.svg"))))
    , syncComplete(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-sync-complete.svg"))))
    , error(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-error.svg"))))
    , errorSync(QIcon(renderSvgImage(QStringLiteral(":/icons/hicolor/scalable/status/syncthing-error-sync.svg"))))
{
}

const StatusIcons LIB_SYNCTHING_MODEL_EXPORT &statusIcons()
{
    static const StatusIcons icons;
    return icons;
}

} // namespace Data
